#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <cstdint>
#include <functional>

#include "MailException.h"

namespace ISXMailDB
{
/**
 * @brief A thread-safe connection pool for managing database connections.
 *
 * This class provides a pool of reusable connections. It allows acquiring and releasing connections
 * in a thread-safe manner. It uses a condition variable to manage connection availability and 
 * a timeout for connection acquisition.
 *
 * @tparam ConnectionType The type of the connection (e.g., `pqxx::connection`).
 */
template <typename ConnectionType>
class ConnectionPool {
public:
    /**
     * @brief Type alias for a function that creates a new connection.
     *
     * This function takes a connection string and returns a shared pointer to a `ConnectionType`.
     */
    using ConnCreationFunction = std::function<std::shared_ptr<ConnectionType>(const std::string&)>;

    /**
     * @brief Constructs a `ConnectionPool` with a specified number of connections.
     *
     * Initializes the pool with a specified number of connections. If the number of requested connections
     * exceeds the maximum allowed, the pool will be initialized with the maximum number.
     *
     * @param pool_size Number of connections to create initially.
     * @param connection_str Connection string to be used by the connection creation function.
     * @param create_connection Function used to create new connections.
     *
     * @throws MailException if the connection creation fails.
     */
    ConnectionPool(uint16_t pool_size, const std::string& connection_str,
                   ConnCreationFunction create_connection)
        : m_connection_string(connection_str), m_timeout(std::chrono::seconds(20))
    {
        try
        {
            for (uint16_t i;i < std::min(pool_size, MAX_DATABASE_CONNECTIONS); ++i) 
            {
                m_pool.push(create_connection(m_connection_string));
            }
        }
        catch(const std::exception& e)
        {
            throw MailException(e.what());
        }
    }
    /**
     * @brief Deleted copy constructor.
     */
    ConnectionPool(const ConnectionPool&) = delete;
    /**
     * @brief Deleted copy assignment operator.
     */
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    /**
     * @brief Acquires a connection from the pool.
     *
     * Blocks until a connection becomes available or the timeout expires.
     *
     * @return A shared pointer to a `ConnectionType` representing the acquired connection.
     *
     * @throws MailException if the timeout expires without acquiring a connection.
     */
    std::shared_ptr<ConnectionType> Acquire() 
    {
        std::unique_lock<std::mutex> lock(m_pool_mutex);

        // Wait until a connection becomes available, or until the timeout expires
        if (!m_cv.wait_for(lock, m_timeout, [this]() { return !m_pool.empty(); })) 
        {
            throw MailException("Timeout: No available connections after waiting.");
        }

        auto connection = m_pool.front();
        m_pool.pop();
        return connection;
    }

    /**
     * @brief Releases a connection back to the pool.
     *
     * Returns the connection to the pool and notifies any waiting threads that a connection is available.
     *
     * @param connection A shared pointer to the `ConnectionType` connection to be released.
     */
    void Release(std::shared_ptr<ConnectionType> connection)
    {
        std::unique_lock<std::mutex> lock(m_pool_mutex);
      
        m_pool.push(connection);
        lock.unlock();
        m_cv.notify_one();
    }

    /**
     * @brief Sets the timeout for acquiring a connection.
     *
     * @param timeout The timeout duration to set for acquiring a connection.
     */
    void set_timeout(std::chrono::seconds timeout) { m_timeout = timeout;}

private:
    std::queue<std::shared_ptr<pqxx::connection>> m_pool;  ///< The connection pool
    std::mutex m_pool_mutex;                               ///< Mutex for thread safety
    std::condition_variable m_cv;                          ///< Condition variable to manage waiting threads
    std::string m_connection_string;                       ///< Connection string for PostgreSQL
    const uint16_t MAX_DATABASE_CONNECTIONS = 10;          ///< Max number of database connections
    std::chrono::seconds m_timeout; ///< Timeout to acquire connection
};

}
