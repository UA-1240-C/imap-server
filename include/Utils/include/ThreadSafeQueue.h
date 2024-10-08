#pragma once

#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <algorithm>
#include <mutex>
#include <deque>
#include <optional>

namespace ISXThreadPool
{
/**
 * @brief Concept to check if a type provides basic lock functionality.
 *
 * This concept checks if a type can be locked and unlocked, and if it supports
 * try-lock functionality.
 *
 * @tparam Lock The type to be checked for lockable functionality.
*/
template <typename Lock>
concept IsLocable = requires(Lock&& lock)
{
  lock.lock();
  lock.unlock();
  { lock.try_lock() } -> std::convertible_to<bool>;
};

/**
 * @class ThreadSafeQueue
 * @brief A thread-safe queue implementation.
 *
 * This class provides a thread-safe implementation of a double-ended queue (deque)
 * that supports adding and removing elements from both the front and the back in
 * synchronized manner.It also includes methods for rotating elements and copying
 * the front element to the back of the queue.
 *
 * @tparam T The type of elements stored in the queue.
 * @tparam Lock The type of lock used for synchronization (default is std::mutex).
 */
template <typename T, IsLocable Lock = std::mutex>
class ThreadSafeQueue
{
public:
  /**
   * @brief Constructs an empty ThreadSafeQueue.
   */
  ThreadSafeQueue() : m_mutex(), m_data() {}

  /**
	* @brief Adds an element by the actual
	 * value type to the back of the queue.
   * @param value The element to be added.
   */
  void PushBack(T&& value)
  {
    std::scoped_lock<std::mutex> lock(m_mutex);
    m_data.push_back(std::forward<T>(value));
  }

  /**
   * @brief Adds an element to the front of the queue.
   * @param value The element to be added.
   */
  void PushFront(T&& value)
  {
    std::scoped_lock<std::mutex> lock(m_mutex);
    m_data.push_front(std::forward<T>(value));
  }

  /**
   * @brief Removes and returns the front element of the queue.
   * @return An std::optional containing the front element if the queue is not empty,
   *         or std::nullopt otherwise.
   */
  std::optional<T> PopFront()
  {
    std::scoped_lock<std::mutex> lock(m_mutex);
    if (m_data.empty()) return std::nullopt;

    auto front = std::move(m_data.front());
    m_data.pop_front();
    return front;
  }

  /**
   * @brief Removes and returns the back element of the queue.
   * @return An std::optional containing the back element if the queue is not empty,
   *         or std::nullopt if the queue is empty.
   */
  std::optional<T> PopBack()
  {
    std::scoped_lock<std::mutex> lock(m_mutex);
    if (m_data.empty()) return std::nullopt;

    auto back = std::move(m_data.back());
    m_data.pop_back();
    return back;
  }

	/**
	 * @brief Moves an element to the front of the queue.
	 *
	 * If the item is found in the queue, it will be removed from its current position
	 * and added to the front of the queue.
	 *
	 * @param item The item to be moved to the front.
	 */
  void RotateToFront(const T& item)
  {
    std::scoped_lock lock(m_mutex);
    auto iter = std::find(m_data.begin(), m_data.end(), item);

    if (iter != m_data.end())
    {
      m_data.erase(iter);
    	m_data.push_front(item);
    }
  }

	/**
	 * @brief Copies the front element and moves it to the back of the queue and returns it.
	 *
	 * @return An std::optional containing the copied front element if the queue is not empty,
	 *         or std::nullopt if the queue is empty.
	 */
  std::optional<T> CopyFrontAndRotateToBack()
  {
    std::scoped_lock lock(m_mutex);

    if (m_data.empty()) return std::nullopt;

    auto front = std::move(m_data.front());
    m_data.pop_front();

    m_data.push_back(front);

    return front;
  }
private:
	/**
	 * @brief The mutex used for multithreding synchronization.
	 * This mutex has an ability to change it's state in
	 * methods marked as const.
	 */
  mutable Lock m_mutex;
  std::deque<T> m_data; ///< The underlying deque storing the elements, guarded by m_mutex.
};
}
#endif //THREADSAFEQUEUE_H
