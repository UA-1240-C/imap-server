#pragma once

#include "ConnectionPool.h"

namespace ISXMailDB
{
/**
 * @brief A wrapper class for managing a single connection from the `ConnectionPool`.
 *
 * This class provides automatic acquisition and release of a connection from the
 * `ConnectionPool`. It ensures that the connection is returned to the pool when the
 * `ConnectionPoolWrapper` object is destroyed.
 *
 * @tparam ConnectionType The type of the connection (e.g., `pqxx::connection`).
 */
template <typename ConnectionType>
class ConnectionPoolWrapper {
public:
    /**
     * @brief Constructs a `ConnectionPoolWrapper` by acquiring a connection from the pool.
     *
     * @param pool The connection pool from which to acquire the connection.
     */
    ConnectionPoolWrapper(ConnectionPool<ConnectionType>& pool)
        : m_connection_pool(pool), m_connection(pool.Acquire()) 
    {}

    /**
     * @brief Destructor that releases the connection back to the pool.
     */
    ~ConnectionPoolWrapper() 
    {
        m_connection_pool.Release(m_connection);
    }

    ConnectionPoolWrapper(const ConnectionPoolWrapper&) = delete;
    ConnectionPoolWrapper& operator=(const ConnectionPoolWrapper&) = delete;

    ConnectionPoolWrapper(ConnectionPoolWrapper&&) = delete;
    ConnectionPoolWrapper& operator=(ConnectionPoolWrapper&&) = delete;

    /**
     * @brief Dereference operator for accessing the underlying connection.
     *
     * @return A reference to the underlying connection.
     */
    ConnectionType& operator*() {
        return *m_connection;
    }

    /**
     * @brief Arrow operator for accessing members of the underlying connection.
     *
     * @return A pointer to the underlying connection.
     */
    ConnectionType* operator->() {
        return m_connection.get();
    }

private:
    ConnectionPool<ConnectionType>& m_connection_pool;        //< Reference to the connection pool
    std::shared_ptr<ConnectionType> m_connection;             //< The acquired connection
};

}