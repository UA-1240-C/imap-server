#pragma once

#include <memory>
#include <pqxx/pqxx>
#include <sodium/crypto_pwhash.h>

#include "IMailDB.h"
#include "ConnectionPool.h"
#include "ConnectionPoolWrapper.h"
#include "PgEmailsWriter.h"
#include "PgManager.h"
#include "EmailsInstance.h"

namespace ISXMailDB
{

/**
 * @class PgMailDB
 * @brief Concrete implementation of the IMailDB interface using PostgreSQL as the database backend.
 * 
 * This class provides an implementation of the `IMailDB` interface for interacting with a PostgreSQL database using pqxx library.
 * It handles operations such as connecting to the database, signing up and logging in users, and managing emails.
 */
class PgMailDB : public IMailDB
{

public:
    /**
     * @brief Constructs a PgMailDB.
     * 
     * @param manager The class that stores data required for initialization.
     */
    PgMailDB(PgManager& manager);

    /**
     * @brief Destructor for PgMailDB.
     */
    ~PgMailDB() override;

    /**
     * @copydoc IMailDB::SignUp
     */    
    void SignUp(const std::string_view user_name, const std::string_view password) override;
    /**
     * @copydoc IMailDB::Login
     */  
    void Login(const std::string_view user_name, const std::string_view password) override;

    /**
     * @copydoc IMailDB::Logout
     */  
    void Logout() override;

    /**
     * @copydoc IMailDB::RetrieveUserInfo
     */ 
    std::vector<User> RetrieveUserInfo(const std::string_view user_name) override;
    /**
     * @copydoc IMailDB::RetrieveEmailContentInfo
     */     
    std::vector<std::string> RetrieveEmailContentInfo(const std::string_view content) override;
    /**
     * @copydoc IMailDB::InsertEmail
     */     
    void InsertEmail(const std::string_view receiver, const std::string_view subject,
                     const std::string_view body, const std::vector<std::string> attachments) override;
    /**
     * @copydoc IMailDB::InsertEmail
     */      
    void InsertEmail(const std::vector<std::string_view> receivers, const std::string_view subject, 
                     const std::string_view body, const std::vector<std::string> attachments) override;

    /**
     * @copydoc IMailDB::RetrieveEmails
     */    
    std::vector<Mail> RetrieveEmails(bool should_retrieve_all = false) override;
    /**
     * @copydoc IMailDB::MarkEmailsAsReceived
     */    
    void MarkEmailsAsReceived() override;
    /**
     * @copydoc IMailDB::UserExists
     */    
    bool UserExists(const std::string_view user_name) override;

    /**
     * @copydoc IMailDB::DeleteEmail
     */    
    void DeleteEmail(const std::string_view user_name) override;
    /**
     * @copydoc IMailDB::DeleteUser
     */    
    void DeleteUser(const std::string_view user_name, const std::string_view password) override;

    void AddFolder(const std::string_view folder_name) override;
    void AddMessageToFolder(const std::string_view folder_name, const Mail& message) override;
    void MoveMessageToFolder(const std::string_view from, const std::string_view to, const Mail& message) override;
    void FlagMessage(const std::string_view flag_name, const Mail& message) override;

    void DeleteFolder(const std::string_view folder_name) override;
    void RemoveMessageFromFolder(const std::string_view folder_name, const Mail& message) override;
    void RemoveFlagFromMessage(const std::string_view flag_name, const Mail& message) override;

    std::vector<Mail> RetrieveMessagesFromFolder(const std::string_view folder_name, const ReceivedState& is_received) override;
    std::vector<Mail> RetrieveMessagesFromFolderWithFlags(const std::string_view folder_name, FlagsSearchBy& flags, const ReceivedState& is_received) override;
    
protected:
    /**
     * @copydoc IMailDB::HashPassword
     */ 
    std::string HashPassword(const std::string& password) override;
    /**
     * @copydoc IMailDB::VerifyPassword
     */ 
    bool VerifyPassword(const std::string& password, const std::string& hashed_password) override;

    /**
     * @brief Inserts a new email body content into the "mailBodies" table.
     * 
     * This method inserts the provided email body content into the `mailBodies` table and retrieves the generated 
     * `mail_body_id` for the inserted content. The method ensures that the database connection is valid before attempting 
     * the insertion. If the connection is lost or manually closed, a `MailException` is thrown. Upon successful 
     * insertion, the method commits the transaction and returns the ID of the newly inserted email body content.
     * 
     * @param content A `std::string_view` representing the email body content to be inserted into the database.
     * @param transaction A `pqxx::transaction_base` object representing the ongoing database transaction. The transaction 
     *        must be active when calling this method.
     * 
     * @return uint32_t The ID of the newly inserted email body content, as retrieved from the `mailBodies` table.
     * 
     * @throws MailException If the database connection is lost or manually closed, or if any exception is thrown 
     *         during the execution of the database query.
     */
    uint32_t InsertEmailContent(const std::string_view content, pqxx::transaction_base& transaction);

    /**
     * @brief Retrieves the user ID for a given username from the "users" table.
     * 
     * @param user_name A `std::string_view` representing the username for which to retrieve the user ID.
     * @param ntx A `pqxx::transaction_base` object representing the ongoing database transaction. The transaction 
     *        must be active when calling this method.
     * 
     * @return uint32_t The ID of the user associated with the given username.
     * 
     * @throws MailException If the user does not exist in the database or if any exception is thrown during the 
     *         execution of the database query.
     */
    uint32_t RetriveUserId(const std::string_view user_name, pqxx::transaction_base &ntx) const;

    /**
     * @brief Inserts a new email record into the "emailMessages" table.
     * 
     * @param sender_id The ID of the user sending the email.
     * @param receiver_id The ID of the user receiving the email.
     * @param subject A `std::string_view` representing the subject of the email.
     * @param body_id The ID of the email body content.
     * @param attachment_id A vector of attachments's id.
     * @param transaction A `pqxx::transaction_base` object representing the ongoing database transaction. The 
     *        transaction must be active when calling this method.
     * 
     * @throws MailException If the insertion fails or if any exception is thrown during the execution of the 
     *         database query.
     */
    uint32_t PerformEmailInsertion(const uint32_t sender_id, const uint32_t receiver_id,
                               const std::string_view subject, const uint32_t body_id, pqxx::transaction_base& transaction);

    /**
     * @brief Check if there is a logged in user.
     * 
     * @throws MailException if the user is not logged in.
     */
    void CheckIfUserLoggedIn();

    /**
     * @brief Inserts a new email attachment content into the "mailAttachments" table.
     * 
     * This method inserts the provided email attachment content into the `mailAttachments` table and retrieves the generated 
     * `id` for the inserted content. The method ensures that the database connection is valid before attempting 
     * the insertion. If the connection is lost or manually closed, a `MailException` is thrown. Upon successful 
     * insertion, the method commits the transaction and returns the ID of the newly inserted email attachment content.
     * 
     * @param attachment A `std::string_view` representing the email attachment content to be inserted into the database.
     * @param transaction A `pqxx::transaction_base` object representing the ongoing database transaction. The transaction 
     *        must be active when calling this method.
     * 
     * @return uint32_t The ID of the newly inserted email attachment content, as retrieved from the `mailAttachments` table.
     * 
     * @throws MailException If the database connection is lost or manually closed, or if any exception is thrown 
     *         during the execution of the database query.
     */
    void InsertAttachment(const std::string_view attachment_data, const std::vector<uint32_t>& email_ids, pqxx::transaction_base &transaction) const;
    uint32_t InsertAttachmentData(const std::string_view attachment_data, pqxx::transaction_base &transaction) const;

    std::vector<std::string> RetrieveAttachments(const uint32_t email_id, pqxx::transaction_base& transaction) const;

    void InsertFolderForUser(const std::string_view folder_name, uint32_t user_id, pqxx::transaction_base& transaction) const;
    void InsertMailToFolder(const std::string_view folder_name, uint32_t mail_id, uint32_t user_id, pqxx::transaction_base& transaction) const;
    void InsertFlagForMessage(const std::string_view flag_name, uint32_t mail_id, pqxx::transaction_base& transaction) const;

    uint32_t RetrieveMessageID(const Mail& message, pqxx::transaction_base& transaction) const;
    uint32_t RetrieveFolderID(const std::string_view folder_name, uint32_t user_id, pqxx::transaction_base& transaction) const;
    uint32_t RetrieveFlagID(const std::string_view flag_name, pqxx::transaction_base& transaction) const;

    std::string get_received_state_string(const ReceivedState& is_received);

    const std::string HOST_NAME; ///< The host name associated with the database.
    const uint32_t HOST_ID; ///< The host ID associated with the database.

    std::shared_ptr<ConnectionPool<pqxx::connection>> m_connection_pool; ///< The connection pool of connections to the PostgreSQL database.
    std::shared_ptr<PgEmailsWriter> m_email_writer; ///< The object that is responsible for caching emails
};

}