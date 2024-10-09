#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <concepts>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <semaphore>
#include <thread>
#include <type_traits>

#include "ThreadSafeQueue.h"

namespace ISXThreadPool
{
/**
 * @brief A thread pool for managing and executing tasks concurrently.
 *
 * This class provides a thread pool that can manage a collection of threads, execute tasks
 * concurrently, and synchronize the completion of tasks. The thread pool allows tasks to be enqueued
 * for execution, either with or without return values, and supports thread initialization functions.
 *
 * @tparam FunctionType The type of function to be executed by the threads in the pool. Defaults to std::function<void()>.
 * @tparam ThreadType The type of thread to be used in the pool. Defaults to std::jthread.
 */
template <typename FunctionType = std::function<void()>, typename ThreadType = std::jthread>
    requires std::invocable<FunctionType> && std::is_same_v<void, std::invoke_result_t<FunctionType>>
class ThreadPool
{
public:
    /**
     * @brief Constructs a thread pool with a specified number of threads.
     *
     * The constructor initializes the thread pool with the given number of threads and an optional
     * initialization function that is called for each thread. In the body of the constructor each thread
     * is being initialized with specific task.
     *
     * @tparam InitFunction The type of the initialization function. Defaults to std::function<void(std::size_t)>.
     * @param number_of_threads The number of threads to create in the pool. Defaults to the hardware concurrency.
     * @param init The initialization function to call for each thread. Defaults to an empty function.
     */
    template <typename InitFunction = std::function<void(std::size_t)>>
        requires std::invocable<InitFunction, std::size_t> &&
                 std::is_same_v<void, std::invoke_result_t<InitFunction, std::size_t>>
    explicit ThreadPool(
        const unsigned int &number_of_threads = std::thread::hardware_concurrency(),
        InitFunction init = [](std::size_t) {})
        : m_tasks(number_of_threads)
    {
        InitializeThreads(number_of_threads, init);
    }

    /**
     * @brief Destroys the thread pool.
     *
     * The destructor waits for all tasks to complete and stops all threads in the pool.
     */
    ~ThreadPool()
    {
        WaitForTasks();
        StopAllThreads();
    }

    /// Thread pool is non-copyable
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool<>& operator= (const ThreadPool &) = delete;

    /**
     * @brief Enqueue a task into the thread pool that returns a result.
     * @details Note that task execution begins once the task is enqueued.
     * @tparam Function An invokable type.
     * @tparam Args Argument parameter pack
     * @tparam ReturnType The return type of the Function
     * @param f The callable function
     * @param args The parameters that will be passed (copied) to the function.
     * @return A std::future<ReturnType> that can be used to retrieve the returned value.
     */
    template <typename Function, typename... Args, typename ReturnType = std::invoke_result_t<Function &&, Args &&...>>
        requires std::invocable<Function, Args...>
    [[nodiscard]] std::future<ReturnType> Enqueue(Function f, Args... args)
    {
        auto shared_promise = std::make_shared<std::promise<ReturnType>>();
        auto task = CreateTask(std::move(f), std::move(args)..., shared_promise);
        auto future = shared_promise->get_future();
        EnqueueTask(std::move(task));
        return future;
    }

    /**
     * @brief Enqueue a task to be executed in the thread pool. Any return value of the function
     * will be ignored.
     * @tparam Function An invokable type.
     * @tparam Args Argument parameter pack for Function
     * @param func The callable to be executed
     * @param args Arguments that will be passed to the function.
     */
    template <typename Function, typename... Args>
        requires std::invocable<Function, Args...>
    void EnqueueDetach(Function &&func, Args &&...args)
    {
        EnqueueTask(CreateDetachedTask(std::forward<Function>(func), std::forward<Args>(args)...));
    }

    /**
     * @brief Returns the number of threads in the pool.
     * @return std::size_t The number of threads in the pool.
     */
    [[nodiscard]] auto Size() const { return m_threads.size(); }

    /**
     * @brief Wait for all tasks to finish.
     */
    void WaitForTasks() const
    {
        if (m_in_flight_tasks.load(std::memory_order_acquire) > 0)
        {
            m_threads_complete_signal.wait(false);
        }
    }

private:
    /**
     * @brief Initialize the threads in the pool with tasks.
     * @tparam InitFunction A function to initialize each thread.
     * @param number_of_threads The number of threads to create.
     * @param init The initialization function.
     */
    template <typename InitFunction>
    void InitializeThreads(const unsigned int &number_of_threads, InitFunction init)
    {
        std::size_t current_id = 0;
        for (std::size_t i = 0; i < number_of_threads; ++i)
        {
            m_priority_queue.PushBack(static_cast<size_t>(current_id));
            try
            {
                m_threads.emplace_back([&, id = current_id, init](const std::stop_token &stop_tok)
                                       { ThreadLoop(id, init, stop_tok); });
                ++current_id;
            }
            catch (...)
            {
                m_tasks.pop_back();
                std::ignore = m_priority_queue.PopBack();   ///< Erase thread pushed to the back
            }
        }
    }

    /**
     * @brief Main loop executed by each thread.
     * @param id The thread's identifier.
     * @param init The initialization function for the thread.
     * @param stop_tok Stop token for the thread.
     */
    void ThreadLoop(std::size_t id, const std::function<void(std::size_t)>& init, const std::stop_token &stop_tok)
    {
        try
        {
            std::invoke(init, id);
        }
        catch (...) { /* suppress exception*/ }

        do
        {
            m_tasks[id].m_signal.acquire();
            ProcessTasks(id);
            m_priority_queue.RotateToFront(id);
            SignalCompletion();
        } while (!stop_tok.stop_requested());
    }

    /**
     * @brief Process tasks for a given thread.
     * @param id The thread's identifier.
     */
    void ProcessTasks(std::size_t id)
    {
        do
        {
            while (auto task = m_tasks[id].m_tasks.PopFront())
            {
                ExecuteTask(std::move(task.value()));
            }
            StealTask(id);
        } while (m_unassigned_tasks.load(std::memory_order_acquire) > 0);
    }

    /**
     * @brief Steal a task from another thread's queue.
     * @param id The thread's identifier.
     */
    void StealTask(std::size_t id)
    {
        for (std::size_t j = 1; j < m_tasks.size(); ++j)
        {
            const std::size_t index = (id + j) % m_tasks.size();
            if (auto task = m_tasks[index].m_tasks.PopBack())
            {
                ExecuteTask(std::move(task.value()));
                break;
            }
        }
    }

    /**
     * @brief Execute a task and update counters.
     * @param task The task to be executed.
     */
    void ExecuteTask(FunctionType task)
    {
        m_unassigned_tasks.fetch_sub(1, std::memory_order_release);
        std::invoke(std::move(task));
        m_in_flight_tasks.fetch_sub(1, std::memory_order_release);
    }

    /**
     * @brief Signal the completion of all tasks.
     */
    void SignalCompletion()
    {
        if (m_in_flight_tasks.load(std::memory_order_acquire) == 0)
        {
            m_threads_complete_signal.store(true, std::memory_order_release);
            m_threads_complete_signal.notify_one();
        }
    }

    /**
     * @brief Stop all threads in the pool.
     */
    void StopAllThreads()
    {
        for (std::size_t i = 0; i < m_threads.size(); ++i)
        {
            m_threads[i].request_stop();
            m_tasks[i].m_signal.release();
            m_threads[i].join();
        }
    }

    /**
     * @brief Create a task with a return value.
     * @tparam Function The function type.
     * @tparam Args The argument types.
     * @tparam ReturnType The return type of the function.
     * @param f The function to be executed.
     * @param args The arguments to be passed to the function.
     * @param promise A shared promise for the task's return value.
     * @return A callable task.
     */
    template <typename Function, typename... Args, typename ReturnType>
    auto CreateTask(Function &&f, Args &&...args, std::shared_ptr<std::promise<ReturnType>> promise)
    {
        return [func = std::forward<Function>(f), ... largs = std::move(args), promise]
        {
            try
            {
                if constexpr (std::is_same_v<ReturnType, void>)
                {
                    func(largs...);
                    promise->set_value();
                }
                else
                {
                    promise->set_value(func(largs...));
                }
            }
            catch (...)
            {
                promise->set_exception(std::current_exception());
            }
        };
    }

    /**
     * @brief Create a detached task that ignores its return value.
     * @tparam Function The function type.
     * @tparam Args The argument types.
     * @param func The function to be executed.
     * @param args The arguments to be passed to the function.
     * @return A callable task.
     */
    template <typename Function, typename... Args>
    auto CreateDetachedTask(Function &&func, Args &&...args)
    {
        return [f = std::forward<Function>(func), ... largs = std::forward<Args>(args)]() mutable -> decltype(auto)
        {
            try
            {
                if constexpr (std::is_same_v<void, std::invoke_result_t<Function &&, Args &&...>>)
                {
                    std::invoke(f, largs...);
                }
                else
                {
                    std::ignore = std::invoke(f, largs...);
                }
            }
            catch (...) {}
        };
    }

    /**
     * @brief Enqueue a task to the thread pool.
     * @tparam Function The function type.
     * @param f The task to be enqueued.
     */

    template <typename Function>
    void EnqueueTask(Function &&f)
    {
        ///< Retrive the thread id with the biggest priority
        auto i_opt = m_priority_queue.CopyFrontAndRotateToBack();
        if (!i_opt.has_value()) return;

        // get the index from the std::optional
        auto i = *i_opt;

        // increment the unassigned tasks and in flight tasks
        m_unassigned_tasks.fetch_add(1, std::memory_order_release);

        // reset the in flight signal if the list was previously empty
        if (const auto prev_in_flight = m_in_flight_tasks.fetch_add(1, std::memory_order_release) == 0)
        {
            m_threads_complete_signal.store(false, std::memory_order_release);
        }

        // assign work
        m_tasks[i].m_tasks.PushBack(std::forward<Function>(f));
        m_tasks[i].m_signal.release(); ///< Update m_signal to 1 which gives an ability to free thread to execute this task
    }

    /**
     * @struct TaskItem
     * @brief A structure representing a task item in a thread pool or task processing system.
     *
     * This structure holds a queue of tasks and a semaphore used for signaling. It is designed
     * to manage tasks that need to be processed by threads in a thread pool.
     */
    struct TaskItem
    {
        /**
         * @var m_tasks
         * @brief A thread-safe queue holding tasks to be processed.
         *
         * This queue contains tasks of type `FunctionType`. It is designed to be accessed safely
         * from multiple threads.
         */
        ThreadSafeQueue<FunctionType> m_tasks{};

        /**
         * @var m_signal
         * @brief A semaphore used for signaling task availability.
         *
         * The semaphore is initialized to zero and used to signal to free thread when tasks are available
         * for processing. It helps in synchronizing task processing among threads.
         */
        std::binary_semaphore m_signal{0};
    };

    /**
     * @var m_threads
     * @brief A vector of threads used for task processing.
     *
     * This vector holds the threads that are responsible for processing tasks from the task items.
     */
    std::vector<ThreadType> m_threads;

    /**
     * @var m_tasks
     * @brief A deque of task items.
     *
     * This deque contains task items represented by main client handling loop,
     * each of which includes a queue of tasks asked by the client and a semaphore
     * for signaling. It helps in organizing and managing tasks to be processed.
     */
    std::deque<TaskItem> m_tasks;

    /**
     * @var m_priority_queue
     * @brief A thread-safe queue for managing thread prioritization.
     *
     * This queue stores prioritized thread IDs rather than tasks. It is used to determine the order
     * in which threads are assigned new tasks, ensuring that threads with fewer tasks are prioritized
     * for assignment.
     */
    ThreadSafeQueue<std::size_t> m_priority_queue;

    /**
     * @var m_unassigned_tasks
     * @brief An atomic counter for the number of unassigned tasks.
     *
     * This atomic variable keeps track of the number of tasks that have not yet been assigned
     * to any thread for processing. It is zero-initialized.
     */
    std::atomic_int_fast64_t m_unassigned_tasks{0};

    /**
     * @var m_in_flight_tasks
     * @brief An atomic counter for the number of in-flight tasks.
     *
     * This atomic variable counts the number of tasks that are currently being processed by threads.
     * It is zero-initialized.
     */
    std::atomic_int_fast64_t m_in_flight_tasks{0};

    /**
     * @var m_threads_complete_signal
     * @brief An atomic flag indicating if all threads have completed their work.
     *
     * This atomic boolean flag is used to signal when all threads in the pool have finished
     * processing their tasks. It is zero-initialized.
     */
    std::atomic_bool m_threads_complete_signal{false};
};
};  // namespace ISXThreadPool

#endif  // THREAD_POOL_H