#pragma once

#include <queue>
#include <pqxx/pqxx>
#include <thread>
#include <mutex>
#include <string>
#include <chrono>
#include <memory>

#include "EmailsInstance.h"

namespace ISXMailDB
{
/**
 * @brief A class for managing and inserting email data into a PostgreSQL database.
 *
 * This class handles email data by queuing emails for insertion into a PostgreSQL database. 
 * It processes the queue in a separate thread and ensures that the email data is correctly
 * inserted into the database.
 */
class PgEmailsWriter
{
public:
/**
     * @brief Constructs a `PgEmailsWriter` instance.
     *
     * Initializes the `PgEmailsWriter` with the connection string, host ID, maximum queue size,
     * and thread sleep interval. It also starts the worker thread for processing the email queue.
     *
     * @param connection_string The connection string for PostgreSQL.
     * @param host_id The ID of the host in the database.
     * @param max_queue_size Maximum number of emails that can be held in the queue.
     * @param thread_sleep_interval Interval at which the worker thread sleeps between processing.
     */
    PgEmailsWriter(const std::string_view connection_string, uint32_t host_id,
                   const uint16_t max_queue_size, const std::chrono::milliseconds& thread_sleep_interval);

    /**
     * @brief Destructor that stops the worker thread and cleans up resources.
     */
    ~PgEmailsWriter();

    PgEmailsWriter(const PgEmailsWriter&) = delete;
    PgEmailsWriter& operator=(const PgEmailsWriter&) = delete;

    PgEmailsWriter(PgEmailsWriter&&) = delete;
    PgEmailsWriter& operator=(PgEmailsWriter&&) = delete;

    /**
     * @brief Adds emails to the queue for processing.
     *
     * Moves the provided `EmailsInstance` object into the queue.
     *
     * @param emails The `EmailsInstance` object containing the emails to be added.
     */
    void AddEmails(EmailsInstance&& emails);


private:
    /**
     * @brief Processes the email queue in a separate thread.
     *
     * This method continuously processes emails from the queue and inserts them into the database.
     */
    void ProcessQueue();

    /**
     * @brief Inserts email data into the database.
     *
     * Handles the insertion of email data within a transaction.
     *
     * @param emails The `EmailsInstance` object containing the emails to be inserted.
     * @param work The `pqxx::work` transaction object used for database operations.
     * @return `true` if the insertion was successful, `false` otherwise.
     */
    bool InsertEmail(const EmailsInstance& emails, pqxx::work& work);

    /**
     * @brief Retrieves the user ID for a given user name.
     *
     * Queries the database to find the user ID for the specified user name.
     *
     * @param user_name The user name for which to retrieve the ID.
     * @param ntx The `pqxx::transaction_base` object used for database operations.
     * @return The user ID associated with the specified user name.
     */
    uint32_t RetriveUserId(const std::string_view user_name, pqxx::transaction_base& ntx);

    /**
     * @brief Inserts an email body into the database.
     *
     * Handles the insertion of email body content and returns the ID of the inserted body.
     *
     * @param content The content of the email body.
     * @param transaction The `pqxx::transaction_base` object used for database operations.
     * @return The ID of the inserted email body.
     */
    uint32_t InsertEmailBody(const std::string_view content, pqxx::transaction_base& transaction);

    /**
     * @brief Inserts an email body into the database.
     *
     * Handles the insertion of email body content and returns the ID of the inserted body.
     *
     * @param attachment The content of the email attachment.
     * @param transaction The `pqxx::transaction_base` object used for database operations.
     * @return The ID of the inserted email attachment.
     */
    void InsertAttachment(const std::string_view attachment_data, const std::vector<uint32_t>& email_ids, pqxx::transaction_base &transaction) const;
    uint32_t InsertAttachmentData(const std::string_view attachment_data, pqxx::transaction_base &transaction) const;

    void InsertFolderForUser(const std::string_view folder_name, uint32_t user_id, pqxx::transaction_base& transaction) const;
    void InsertMailToFolder(const std::string_view folder_name, uint32_t mail_id, uint32_t user_id, pqxx::transaction_base& transaction) const;
    void InsertFlagForMessage(const std::string_view flag_name, uint32_t mail_id, pqxx::transaction_base& transaction) const;

    uint32_t RetrieveFolderID(const std::string_view folder_name, uint32_t user_id, pqxx::transaction_base& transaction) const;
    uint32_t RetrieveFlagID(const std::string_view flag_name, pqxx::transaction_base& transaction) const;
    /**
     * @brief Performs the insertion of an email into the database.
     *
     * Completes the email insertion process by adding the sender, receiver, subject, and body ID.
     *
     * @param sender_id The ID of the sender.
     * @param receiver_id The ID of the receiver.
     * @param subject The subject of the email.
     * @param body_id The ID of the email body.
     * @param attachments_id A vector of attachments's id
     * @param transaction The `pqxx::transaction_base` object used for database operations.
     */
    uint32_t PerformEmailInsertion(const uint32_t sender_id, const uint32_t receiver_id,
                               const std::string_view subject, const uint32_t body_id,
                               pqxx::transaction_base& transaction);

    std::queue<EmailsInstance> m_queue;                ///< Queue of email instances to be processed
    std::mutex m_mtx;                                  ///< Mutex for thread safety
    std::thread m_worker_thread;                       ///< Worker thread for processing the email queue
    bool m_stop_thread;                                ///< Flag to stop the worker thread
    std::unique_ptr<pqxx::connection> m_conn;          ///< Database connection
    uint16_t m_max_queue_size;                         ///< Maximum size of the email queue
    std::chrono::milliseconds m_tread_sleep_interval;  ///< Interval at which the worker thread sleeps
    const uint32_t HOST_ID;                            ///< ID of the host in the database
};

}