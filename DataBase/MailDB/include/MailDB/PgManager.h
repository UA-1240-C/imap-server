#pragma once

#include <pqxx/pqxx>

#include "ConnectionPool.h"
#include "PgEmailsWriter.h"

namespace ISXMailDB
{

/**
 * @brief Manages database connections and email writer functionality.
 *
 * The `PgManager` class is responsible for initializing and managing the connection pool
 * and the email writer. It handles the setup of connections, manages email caching,
 * and provides access to key components of the database management system.
 */
class PgManager
{
public:
    /**
     * @brief Constructs a `PgManager` instance.
     *
     * Initializes the manager with the given connection string, host name, and a flag
     * indicating whether to cache emails.
     *
     * @param connection_string The connection string for PostgreSQL.
     * @param host_name The name of the host.
     * @param should_cache_emails Flag indicating whether to cache emails.
     */
    PgManager(const std::string_view connection_string, const std::string_view host_name, bool should_cache_emails);

    /**
     * @brief Retrieves the connection pool.
     *
     * Provides access to the connection pool used for managing database connections.
     *
     * @return A shared pointer to the `ConnectionPool` for PostgreSQL connections.
     */
    std::shared_ptr<ConnectionPool<pqxx::connection>> get_connection_pool() { return m_connection_pool; }

    /**
     * @brief Retrieves the email writer.
     *
     * Provides access to the `PgEmailsWriter` instance used for handling email data.
     *
     * @return A shared pointer to the `PgEmailsWriter`.
     */
    std::shared_ptr<PgEmailsWriter> get_emails_writer() { return m_emails_writer; }

    /**
     * @brief Retrieves the host name.
     *
     * Provides the name of the host as set during initialization.
     *
     * @return The host name.
     */
    std::string get_host_name() const { return HOST_NAME; }

    /**
     * @brief Retrieves the host ID.
     *
     * Provides the ID of the host as set during initialization.
     *
     * @return The host ID.
     */
    uint32_t get_host_id() const { return m_host_id; }

    /**
     * @brief Retrieves the maximum queue size for the email writer.
     *
     * Provides the maximum number of emails that can be held in the writer's queue if caching is enabled.
     *
     * @return The maximum writer queue size. Returns 0 if email caching is not enabled.
     */
    uint32_t get_max_writer_queue_size() const { return m_should_cache_emails ? MAX_WRITER_QUEUE_SIZE : 0; }

    /**
     * @brief Retrieves the writer timeout duration.
     *
     * Provides the duration for which the email writer will wait before performing its operations.
     * Returns a zero duration if email caching is not enabled.
     *
     * @return The writer timeout duration.
     */
    std::chrono::milliseconds get_writer_timeout() const 
    { 
        return m_should_cache_emails ? WRITER_TIMEOUT 
            : std::chrono::milliseconds(0);
    }

private:
    /**
     * @brief Initializes the manager.
     *
     * Calls the initialization methods for setting up the connection pool and email writer.
     */
    void Init();

    /**
     * @brief Initializes the email writer.
     *
     * Sets up the `PgEmailsWriter` instance based on the caching configuration.
     */
    void InitEmaisWriter();

    /**
     * @brief Initializes the connection pool.
     *
     * Sets up the `ConnectionPool` for PostgreSQL connections.
     */
    void InitConnectionPool();

    /**
     * @brief Inserts the host information into the database.
     *
     * Adds the host details to the database and retrieves its ID.
     */
    void InsertHost();

    bool m_should_cache_emails;                              ///< Flag indicating whether to cache emails
    std::shared_ptr<PgEmailsWriter> m_emails_writer;         ///< Shared pointer to the email writer
    const uint32_t MAX_WRITER_QUEUE_SIZE = 100;              ///< Maximum size of the email writer queue
    const std::chrono::milliseconds WRITER_TIMEOUT{1000};    ///< Timeout for the email writer operations

    const std::string CONNECTION_STRING;                     ///< PostgreSQL connection string
    std::shared_ptr<ConnectionPool<pqxx::connection>> m_connection_pool; ///< Shared pointer to the connection pool
    const uint16_t POOL_INITIAL_SIZE = 10;                   ///< Initial size of the connection pool
    
    const std::string HOST_NAME;                             ///< Name of the host
    uint32_t m_host_id;                                      ///< ID of the host in the database
};

}
