#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "MailException.h"

namespace ISXMailDB
{

enum ReceivedState
{
    TRUE,
    FALSE,
    BOTH,
};

/**
 * @brief Represents a user with a username, password, and host name.
 */
struct User
{
    /**
     * @brief Constructs a User object.
     * @param user_name The user's name.
     * @param user_password The user's password.
     * @param host_name The host name associated with the user.
     */
    User(std::string user_name, std::string user_password, std::string host_name)
        : user_name{std::move(user_name)}, user_password{std::move(user_password)}, host_name{std::move(host_name)}
    {
    }

    std::string user_name;
    std::string user_password;
    std::string host_name;
};
/**
 * @brief Represents an email with a recipient, sender, subject, and body.
 */
struct Mail
{
    /**
     * @brief Constructs a Mail object.
     * @param recipient The recipient of the email.
     * @param sender The sender of the email.
     * @param subject The subject of the email.
     * @param body The body of the email.
     */
    Mail(const std::string_view recipient, const std::string_view sender,
         const std::string_view subject, const std::string_view body, const std::string_view sent_at, const std::vector<std::string>& attachments)
        : recipient(recipient), sender(sender), 
          subject(subject), body(body),
          sent_at(sent_at),
          attachments(attachments)
    {
    }
    
    std::string recipient;
    std::string sender;
    std::string subject;
    std::string body;
    std::string sent_at;
    std::vector<std::string> attachments;
};

class FlagsSearchBy
{

public:
    FlagsSearchBy& AddFlagToInclude(const std::string_view flag)
    {
        include.emplace_back(flag);
        return *this;
    }

    FlagsSearchBy& AddFlagToExclude(const std::string_view flag)
    {
        exclude.emplace_back(flag);
        return *this;
    }

    FlagsSearchBy& AddFlagToEither(const std::string_view flag)
    {
        either.emplace_back(flag);
        return *this;
    }

    std::string BuildQuery(uint32_t folder_id)
    {
        std::string query = "SELECT DISTINCT mf.email_message_id FROM \"messageFlags\" AS mf LEFT JOIN \"folderMessages\" AS fm ON mf.email_message_id = fm.email_message_id "
                            "LEFT JOIN \"folders\" AS f ON fm.folder_id = f.folder_id WHERE f.folder_id = " + std::to_string(folder_id) + " AND (";

        if(!include.empty())
        {

            AppendQuery(query, include, " mf.email_message_id IN(SELECT email_message_id FROM \"messageFlags\" AS mf LEFT JOIN flags AS f ON f.flag_id = mf.flag_id WHERE f.flag_name = '", "') AND");        
        }
        else if(!either.empty())
        {
            query.append("(");

            AppendQuery(query, either, " mf.email_message_id IN(SELECT email_message_id FROM \"messageFlags\" AS mf LEFT JOIN flags AS f ON f.flag_id = mf.flag_id WHERE f.flag_name = '", "') OR");
            
            query.append(")");
        }

        if(!exclude.empty())
        {
            if(query.find_last_of('(') != query.length() - 1)
            {
                query.append(" AND");
            }

            AppendQuery(query, exclude, " mf.email_message_id NOT IN(SELECT email_message_id FROM \"messageFlags\" AS mf LEFT JOIN flags AS f ON f.flag_id = mf.flag_id WHERE f.flag_name = '", "') AND");
        }

        query.append(")");

        return query;
    }

private:

    void AppendQuery(std::string& query, std::vector<std::string> flags, const std::string& before_flag, const std::string& after_flag)
    {
        for(size_t i{}; i < flags.size(); i++)
        {
            if(i == flags.size()-1)
            {
                query.append(before_flag + flags[i] + after_flag.substr(0, 2));
                break;
            }
            query.append(before_flag + flags[i] + after_flag);
        }        
    }   

    std::vector<std::string> include;
    std::vector<std::string> exclude;
    std::vector<std::string> either;
};

/**
 * @brief Overloads the output stream operator for the Mail structure.
 * @param os The output stream.
 * @param mail The Mail object to be output.
 * @return A reference to the modified output stream.
 */
std::ostream& operator<<(std::ostream& os, const Mail& mail);

/**
 * @class IMailDB
 * @brief Interface for a mail database.
 *
 * The IMailDB class provides methods for connecting, user management,
 * and email management within a mail database. This interface must be 
 * implemented by any class that manages email and user data storage.
 */
class IMailDB
{
public:
    /**
     * @brief Constructs an IMailDB object.
     */
    IMailDB() : m_user_id(0)
    {
    }

    /**
     * @brief Copy constructor is deletd.
     */
    IMailDB(const IMailDB&) = delete;

    /**
     * @brief Virtual destructor.
     */
    virtual ~IMailDB() = default;

    /**
     * @brief Deleted copy assignment operator.
     */
    IMailDB& operator=(const IMailDB&) = delete;

    /**
     * @brief Deleted move constructor.
     */
    IMailDB(IMailDB&&) = delete;
    /**
     * @brief Deleted move assignment operator.
     */    
    IMailDB& operator=(IMailDB&&) = delete;

    /**
     * @brief Registers a new user in the mail database.
     * 
     * This method inserts a new user in the mail databse if does not exist.
     * It also hashes the password using 'HashPassword' method. 
     * Hashed version of password is stored in the database.
     * 
     * @param user_name The username of the new user.
     * @param password The password of the new user.
     * @throw MailException if the user already exists or hashing of password fails.
     */
    virtual void SignUp(const std::string_view user_name, const std::string_view password) = 0;
    /**
     * @brief Authenticates a user in the mail database.
     * 
     * This method checks equality of 'password' parameter and hashed password stored in the mail database
     * using 'VerifyPassword' method. 
     * If user is successfully logged in, saves user_name in m_user_name and user_id in m_user_id
     * 
     * @param user_name The username of the user.
     * @param password The password of the user.
     * @throw MailException if authentification fails.
     */
    virtual void Login(const std::string_view user_name, const std::string_view password) = 0;


    /**
     * @brief Logs out user. Set m_user_name and m_user_id to default value.
     * 
     */
    virtual void Logout() = 0;
    /**
     * @brief Retrieves a list of users from the database.
     * 
     * This method returns a list of users based on the specified `user_name`.
     * If a `user_name` is provided, it returns users matching that name.
     * If an empty string literal (`""`) is passed, it returns a list of all users.
     * 
     * @param user_name The name of the user to retrieve. If an empty string is provided, all users are retrieved.
     * @return std::vector<User> A vector containing the list of users.
     * @throw MailException if the process fails.
     */
    virtual std::vector<User> RetrieveUserInfo(const std::string_view user_name) = 0;
    /**
     * @brief Retrieves the body content of emails from the database.
     * 
     * This method returns the body content of emails based on the specified `content`.
     * If a `content` string is provided, it returns email bodies matching that content.
     * If an empty string literal (`""`) is passed, it returns a list of all email bodies.
     * 
     * @param content The content to search for in email bodies. If an empty string is provided, all email bodies are retrieved.
     * @return std::vector<std::string> A vector containing the body content of the emails.
     * @throw MailException if the process fails.
     */
    virtual std::vector<std::string> RetrieveEmailContentInfo(const std::string_view content) = 0;
    /**
     * @brief Inserts an email with a single receiver into the database. The sender is current logged in user.
     * 
     * This method inserts an email with the specified sender, receiver, subject, 
     * and body content into the database. It also implicitly inserts the mail 
     * content into the database.
     * 
     * @param receiver The email address of the receiver.
     * @param subject The subject of the email.
     * @param body The body content of the email.
     * @param attachments A vector of attachments.
     * 
     * @return bool Returns `true` if the email was successfully inserted, otherwise `false`.
     * 
     * @throw MailException if the process fails or if current user is not logged in.
     */
    virtual void InsertEmail(const std::string_view receiver,
                             const std::string_view subject, const std::string_view body, const std::vector<std::string> attachments) = 0;
    
    /**
     * @brief Inserts an email with multiple receivers into the database. The sender is current logged in user.
     * 
     * This method inserts an email with the specified sender, a list of receivers, 
     * subject, and body content into the database. It also implicitly inserts the 
     * mail content into the database.
     * 
     * @param receivers A vector of email addresses representing the receivers.
     * @param subject The subject of the email.
     * @param body The body content of the email.
     * @param attachments A vector of attachments.
     * @return bool Returns `true` if the email was successfully inserted, otherwise `false`.
     * 
     * @throw MailException if the process fails if current user is not logged in.
     */
    virtual void InsertEmail(const std::vector<std::string_view> receivers,
                                const std::string_view subject, const std::string_view body, const std::vector<std::string> attachments) = 0;

    /**
     * @brief Retrieves emails for a current logged in user.
     * 
     * If `should_retrieve_all` is `false`, retrieves emails that have not yet been marked as received.
     * If `should_retrieve_all` is `true`, retrieves all emails for the specified user.
     * If there are no emails for the specified user returns empty vector
     * 
     * The emails are returned in the order from newest to oldest.
     * 
     * @param should_retrieve_all If true, retrieves all emails; otherwise, retrieves only emails that haven't been received.
     * @return A vector of Mail objects for the specified user.
     * @throw MailException if the user is not logged in.
     */
    virtual std::vector<Mail> RetrieveEmails( bool should_retrieve_all = false) = 0;

    /**
     * @brief Marks all emails as received for a current logged in user.
     * 
     * This method updates the status of all emails for the the current logged in user to indicate that they have been received.
     * 
     * @throw MailException if the user is not logged in.
     */
    virtual void MarkEmailsAsReceived() = 0;
    
    /**
     * @brief Checks if a user exists.
     * 
     * This method verifies whether a user with the specified `user_name` and on current host exists in the database.
     * 
     * @param user_name The username to check for existence.
     * @return True if the user exists, false otherwise.
     */
    virtual bool UserExists(const std::string_view user_name) = 0;

    /**
     * @brief Removes all emails associated with a specific user.
     * 
     * This method deletes all emails from the email table where the user with the specified `user_name` is either the sender or the receiver.
     * 
     * @param user_name The username whose associated emails are to be deleted.
     * @throw MailException if the process fails.
     */
    virtual void DeleteEmail(const std::string_view user_name) = 0;
    /**
     * @brief Removes a user from the database along with all associated emails.
     * 
     * This method deletes the user with the specified `user_name` from the database. It also removes all emails from the email table where the user was either the sender or the receiver.
     * 
     * @param user_name The username of the user to be deleted.
     * @param password The password of the user for authentication purposes.
     * @return True if the user was successfully deleted along with their associated emails, false otherwise.
     * @throw MailException if the process fails.
     */
    virtual void DeleteUser(const std::string_view user_name, const std::string_view password) = 0;

    
    virtual void AddFolder(const std::string_view folder_name) = 0;
    virtual void AddMessageToFolder(const std::string_view folder_name, const Mail& message) = 0;
    virtual void MoveMessageToFolder(const std::string_view from, const std::string_view to, const Mail& message) = 0;
    virtual void FlagMessage(const std::string_view flag_name, const Mail& message) = 0;

    virtual void DeleteFolder(const std::string_view folder_name) = 0;
    virtual void RemoveMessageFromFolder(const std::string_view folder_name, const Mail& message) = 0;
    virtual void RemoveFlagFromMessage(const std::string_view flag_name, const Mail& message) = 0;
    
    
    virtual std::vector<Mail> RetrieveMessagesFromFolder(const std::string_view folder_name, const ReceivedState& is_received) = 0;
    virtual std::vector<Mail> RetrieveMessagesFromFolderWithFlags(const std::string_view folder_name, FlagsSearchBy& flags, const ReceivedState& is_received) = 0;
    
    /**
     * @brief Returns name of current logged in user. If nobody is logged in, returns empty string.
     */
    std::string get_user_name() const {return m_user_name;}

    /**
     * @brief Returns id of current logged in user. If nobody is logged in, returns zero.
     */
    uint32_t get_user_id() const {return m_user_id;}

protected:
    /**
     * @brief Hashes a password for secure storage.
     * @param password The password to hash.
     * @return The hashed password.
     * @throw MailException if password hashing fails.
     */
    virtual std::string HashPassword(const std::string& password) = 0;
    /**
     * @brief Verifies a password against a stored hashed password.
     * @param password The password to verify.
     * @param hashed_password The hashed password to compare against.
     * @return True if the password matches the hashed password, otherwise false.
     */
    virtual bool VerifyPassword(const std::string& password, const std::string& hashed_password) = 0;

    std::string m_user_name; ///< The name of current logged in user.
    uint32_t m_user_id; ///< The ID of current logged in user.
};

}