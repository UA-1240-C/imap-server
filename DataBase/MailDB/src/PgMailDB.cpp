#include <memory>
#include <iostream>

#include "MailException.h"
#include "PgMailDB.h"

namespace ISXMailDB
{

using PgConnection = ConnectionPoolWrapper<pqxx::connection>;

PgMailDB::PgMailDB(PgManager& manager)
    : m_connection_pool(manager.get_connection_pool()),
      m_email_writer(manager.get_emails_writer()),
      HOST_NAME(manager.get_host_name()),
      HOST_ID(manager.get_host_id())
{
}

PgMailDB::~PgMailDB()
{   
}

void PgMailDB::SignUp(const std::string_view user_name, const std::string_view password)
{
    PgConnection conn(*m_connection_pool);
    pqxx::work tx(*conn);
    try
    {
        tx.exec_params0("SELECT 1 FROM users WHERE host_id = $1 AND user_name = $2", HOST_ID, user_name);

        std::string hashed_password = HashPassword(std::string(password));

        tx.exec_params(
            "INSERT INTO users (host_id, user_name, password_hash)"
            "VALUES ($1, $2, $3)",
            HOST_ID, user_name, hashed_password);
    }
    catch (const pqxx::unexpected_rows& e)
    {
        throw MailException("User already exists");
    }
    tx.commit();
}

std::string PgMailDB::HashPassword(const std::string& password)
{
    std::string hashed_password(crypto_pwhash_STRBYTES, '\0');

    if (crypto_pwhash_str(hashed_password.data(),
        password.c_str(),
        password.size(),
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        throw MailException("Password hashing failed");
    }

    return hashed_password;
}

bool PgMailDB::VerifyPassword(const std::string& password, const std::string& hashed_password)
{
    return crypto_pwhash_str_verify(hashed_password.c_str(), password.c_str(),
                                    password.size()) == 0;
}

void PgMailDB::Login(const std::string_view user_name, const std::string_view password)
{
    PgConnection conn(*m_connection_pool);
    pqxx::nontransaction ntx(*conn);
    try
    {
        auto [hashed_password, user_id] = ntx.query1<std::string, uint32_t>
        ("SELECT password_hash, user_id FROM users "
            "WHERE user_name = " + ntx.quote(user_name) 
            +  " AND host_id = " + ntx.quote(HOST_ID)
            );
        
        if (!VerifyPassword(std::string(password), hashed_password))
        {
            throw MailException("Invalid username or password");
        }
        m_user_id = user_id;
        m_user_name = user_name;

    }
    catch (const pqxx::unexpected_rows &e)
    {
        throw MailException("User with mentioned username doesn't exist");
    }
}

void PgMailDB::Logout()
{
    m_user_id = 0;
    m_user_name.clear();
}

std::vector<User> PgMailDB::RetrieveUserInfo(const std::string_view user_name)
    {
        PgConnection conn(*m_connection_pool);

        pqxx::nontransaction nontransaction(*conn);
        pqxx::result user_query_result;
    
        if (user_name.empty())
        {
            user_query_result = nontransaction.exec_params(
                "SELECT u.user_name, u.password_hash, h.host_name FROM users u "
                "LEFT JOIN hosts h ON u.host_id = h.host_id"
            );
        }
        else
        {
            user_query_result = nontransaction.exec_params(
                "SELECT u.user_name, u.password_hash, h.host_name FROM users u "
                "LEFT JOIN hosts h ON u.host_id = h.host_id "
                "WHERE u.user_name = $1"
                , nontransaction.esc(user_name)
            );
        }
        
        std::vector<User> info;
        if (!user_query_result.empty())
        {
            for (auto&& row : user_query_result)
            {
                info.emplace_back(User(row.at("user_name").as<std::string>()
                                  , row.at("password_hash").as<std::string>()
                                  , row.at("host_name").as<std::string>()));
            }

            return info;
        }

        return std::vector<User>();
    }

std::vector<std::string> PgMailDB::RetrieveEmailContentInfo(const std::string_view content)
{
    PgConnection conn(*m_connection_pool);

    pqxx::nontransaction nontransaction(*conn);
    pqxx::result content_query_result;
   
    if (content.empty())
    {
        content_query_result = nontransaction.exec_params(
            "SELECT body_content FROM \"mailBodies\""
        );
    }
    else
    {
        content_query_result = nontransaction.exec_params(
            "SELECT body_content FROM \"mailBodies\" "
            "WHERE body_content = $1"
            , nontransaction.esc(content)
        );
    }

    std::vector<std::string> info{};
    if (!content_query_result.empty())
    {
        for (auto&& row : content_query_result)
        {
            info.emplace_back(row.at("body_content").as<std::string>());
        }

        return info;
    }

    return std::vector<std::string>();
}

void PgMailDB::InsertEmail(const std::string_view receiver, const std::string_view subject, 
                           const std::string_view body, const std::vector<std::string> attachments)
{
    InsertEmail(std::vector<std::string_view>{receiver}, subject, body, attachments);
}

void PgMailDB::InsertEmail(const std::vector<std::string_view> receivers, const std::string_view subject, 
                           const std::string_view body, const std::vector<std::string> attachments)
{
    CheckIfUserLoggedIn();


    if(m_email_writer)
    {
        // Current logged in user is the sender
        EmailsInstance emails{{m_user_id, m_user_name},
            std::vector<std::string>(receivers.begin(), receivers.end()),
            std::string(subject),
            std::string(body),
            std::vector<std::string>(attachments.begin(), attachments.end())
        };
        m_email_writer->AddEmails(std::move(emails));
        return;
    }

    PgConnection conn(*m_connection_pool);
    uint32_t sender_id, body_id;
    std::vector<uint32_t> receivers_id, email_id;
    {
        try
        {
            {
                AddFolder("Sent");
                
                pqxx::nontransaction nontransaction(*conn);
                sender_id = m_user_id;

                for (size_t i = 0; i < receivers.size(); i++)
                {
                    receivers_id.emplace_back(RetriveUserId(receivers[i], nontransaction));
                }
                
                body_id = InsertEmailContent(body, nontransaction);
            }
        }
        catch (const std::exception& e)
        {
            throw MailException("Given value doesn't exist in database.");
        }
    }

    try {
        pqxx::work transaction(*conn);
        for (size_t i = 0; i < receivers_id.size(); i++) 
        {
            email_id.emplace_back(PerformEmailInsertion(sender_id, receivers_id[i], subject, body_id, transaction));

            InsertFolderForUser("Inbox", receivers_id[i], transaction);
            InsertMailToFolder("Sent", email_id.back(), m_user_id, transaction);
            InsertMailToFolder("Inbox", email_id.back(), receivers_id[i], transaction);
            InsertFlagForMessage("\\Recent", email_id.back(), transaction);
        }

        for(auto&& att: attachments)
        {
            InsertAttachment(att, email_id, transaction);
        }

        transaction.commit();
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }
}

void PgMailDB::InsertFolderForUser(const std::string_view folder_name, uint32_t user_id, pqxx::transaction_base& transaction) const
{
    try
    {
        pqxx::result result = transaction.exec_params("SELECT * FROM \"folders\" WHERE user_id = $1 AND folder_name = $2"
                                                     , user_id, folder_name);

        if(result.empty())
        {
            transaction.exec_params("INSERT INTO \"folders\" (user_id, folder_name)VALUES($1, $2)"
                                    , user_id, folder_name);
        }
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

void PgMailDB::InsertMailToFolder(const std::string_view folder_name, uint32_t mail_id, uint32_t user_id, pqxx::transaction_base& transaction) const
{
    try
    {
        uint32_t folder_id = RetrieveFolderID(folder_name, user_id, transaction);

        pqxx::result result = transaction.exec_params("SELECT * FROM \"folderMessages\" WHERE email_message_id = $1 AND folder_id = $2"
                                                     , mail_id, folder_id);

        if(result.empty())
        {
            transaction.exec_params("INSERT INTO \"folderMessages\" (email_message_id, folder_id)VALUES($1, $2)"
                                    , mail_id, folder_id);
        }
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

void PgMailDB::InsertFlagForMessage(const std::string_view flag_name, uint32_t mail_id, pqxx::transaction_base& transaction) const
{
    try
    {
        uint32_t flag_id = RetrieveFlagID(flag_name, transaction);

        pqxx::result result = transaction.exec_params("SELECT * FROM \"messageFlags\" WHERE email_message_id = $1 AND flag_id = $2"
                                                     , mail_id, flag_id);

        if(result.empty())
        {
            transaction.exec_params("INSERT INTO \"messageFlags\" (email_message_id, flag_id)VALUES($1, $2)"
                                    , mail_id, flag_id);
        }
    }
    catch(const std::exception& e)
    {
       throw MailException(e.what()); 
    }

}

std::vector<Mail> PgMailDB::RetrieveEmails(bool should_retrieve_all)
{
    CheckIfUserLoggedIn();

    PgConnection conn(*m_connection_pool);
    pqxx::nontransaction ntx(*conn);

    // Retrieves email details for the current user, optionally filtering for unread emails, and orders them by sent time.
    std::string query =
        "WITH filtered_emails AS ( "
        "    SELECT email_message_id, sender_id, subject, mail_body_id, sent_at "
        "    FROM \"emailMessages\" "
        "    WHERE recipient_id = " +
        ntx.quote(m_user_id) + 
        (should_retrieve_all ? "": " AND is_received = FALSE")+
        ")"
        "SELECT f.email_message_id, u.user_name AS sender_name, f.subject, m.body_content, f.sent_at "
        "FROM filtered_emails AS f "
        "LEFT JOIN users AS u ON u.user_id = f.sender_id "
        "LEFT JOIN \"mailBodies\" AS m ON m.mail_body_id = f.mail_body_id "
        "ORDER BY f.sent_at DESC; ";

    std::vector<Mail> resutl_mails;

    for (auto& [email_id, sender, subject, body, sent_at] : ntx.query<uint32_t, std::string, std::string, std::string, std::string>(query))
    {
        resutl_mails.emplace_back(m_user_name, sender, subject, body, sent_at, RetrieveAttachments(email_id, ntx));
    }

    return resutl_mails;
}

std::vector<std::string> PgMailDB::RetrieveAttachments(const uint32_t email_id, pqxx::transaction_base& transaction) const
{
    pqxx::result result;
    try
    {
        result = transaction.exec_params("SELECT data_text FROM \"attachmentData\" AS ad LEFT JOIN \"mailAttachments\" AS ma ON ma.data_id = ad.data_id "
                                         "WHERE ma.email_message_id = $1", email_id);
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
    
    std::vector<std::string> attachments;
    for(auto&& row: result)
    {
        attachments.emplace_back(row.at("data_text").as<std::string>());
    }

    return attachments;
}

void PgMailDB::MarkEmailsAsReceived()
{
    CheckIfUserLoggedIn();

    PgConnection conn(*m_connection_pool);
    pqxx::work tx(*conn);

    tx.exec_params(
        "UPDATE \"emailMessages\" "
        "SET is_received = TRUE "
        "WHERE recipient_id = $1 "
        "AND is_received = FALSE"
        , m_user_id);

    tx.commit();
}

bool PgMailDB::UserExists(const std::string_view user_name)
{
    PgConnection conn(*m_connection_pool);
    pqxx::nontransaction ntx(*conn);
    try 
    {
        ntx.exec_params1("SELECT 1 FROM users WHERE host_id = $1 AND user_name = $2", HOST_ID, user_name);
        return true;
    }
    catch(pqxx::unexpected_rows &e)
    {
        return false;
    }
}

void PgMailDB::DeleteEmail(const std::string_view user_name)
{
    PgConnection conn(*m_connection_pool);

    uint32_t user_info;
    {
        try {
            pqxx::nontransaction nontransaction(*conn);
            user_info = RetriveUserId(user_name, nontransaction);
        }
        catch (const std::exception& e)
        {
            throw MailException("Given value doesn't exist in database.");
        }
    }

    try
    {
        pqxx::work transaction(*conn);
        transaction.exec_params(
            "DELETE FROM \"emailMessages\" "
            "WHERE sender_id = $1 OR recipient_id = $1"
            , user_info
        );
        transaction.commit();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

void PgMailDB::DeleteUser(const std::string_view user_name, const std::string_view password)
{
    PgConnection conn(*m_connection_pool);
    
    try 
    {
        Login(user_name, password);
        DeleteEmail(user_name);
    }
    catch (const MailException& e) {
        throw MailException(e.what());
    }

    try
    {
        pqxx::work transaction(*conn);

        std::string hashed_password = transaction.query_value<std::string>("SELECT password_hash FROM users "
            "WHERE user_name = " + transaction.quote(user_name) 
            +  " AND host_id = " + transaction.quote(HOST_ID)
            );

        transaction.exec_params(
            "DELETE FROM users "
            "WHERE user_name = $1 AND password_hash = $2 AND host_id = $3"
            , transaction.esc(user_name), transaction.esc(hashed_password), HOST_ID
        );
        transaction.commit();
    }
    catch (const std::exception& e)
    {
        throw MailException(e.what());
    }
} 

uint32_t PgMailDB::InsertEmailContent(const std::string_view content, pqxx::transaction_base& transaction)
{
    pqxx::result body_id;
    try {
        body_id = transaction.exec_params("SELECT * FROM \"mailBodies\" WHERE body_content = $1 LIMIT 1", content);

        if(body_id.empty())
        {
            body_id.clear();
            body_id = transaction.exec_params(
                "INSERT INTO \"mailBodies\" (body_content)VALUES($1) RETURNING mail_body_id"
                , content
            );
        }

        transaction.commit();
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }

    return body_id[0].at("mail_body_id").as<uint32_t>();
}

void PgMailDB::InsertAttachment(const std::string_view attachment_data, const std::vector<uint32_t>& email_ids, pqxx::transaction_base &transaction) const
{
    try {
        uint32_t attachment_id = InsertAttachmentData(attachment_data, transaction);

        for(auto&& id: email_ids)
        {
            transaction.exec_params("INSERT INTO \"mailAttachments\" (email_message_id, data_id)VALUES($1, $2)"
                                    , id, attachment_id);
        }
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }

}

uint32_t PgMailDB::InsertAttachmentData(const std::string_view attachment_data, pqxx::transaction_base &transaction) const
{
    pqxx::result attachment_id;
    try {
        attachment_id = transaction.exec_params("SELECT * FROM \"attachmentData\" WHERE data_text = $1 LIMIT 1", attachment_data);

        if(attachment_id.empty())
        {
            attachment_id.clear();
            attachment_id = transaction.exec_params(
                "INSERT INTO \"attachmentData\" (data_text)VALUES($1) RETURNING data_id"
                , attachment_data
            );
        }
        
        return attachment_id[0].at("data_id").as<uint32_t>();
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }

}

uint32_t PgMailDB::RetriveUserId(const std::string_view user_name, pqxx::transaction_base &ntx) const
{
    try
    {
        return ntx.query_value<uint32_t>("SELECT user_id FROM users WHERE user_name = " + ntx.quote(user_name) 
                                         + "  AND host_id = " + ntx.quote(HOST_ID));
    }
    catch (pqxx::unexpected_rows &e)
    {
        throw MailException("User doesn't exist");
    };
}  

uint32_t PgMailDB::PerformEmailInsertion(const uint32_t sender_id, const uint32_t receiver_id,
                                const std::string_view subject, const uint32_t body_id, pqxx::transaction_base& transaction)
{
    pqxx::result email_result;
    try {
        email_result = transaction.exec_params(
                "INSERT INTO \"emailMessages\" (sender_id, recipient_id, subject, mail_body_id, is_received) "
                "VALUES ($1, $2, $3, $4, false) RETURNING email_message_id"
                , sender_id, receiver_id,
                subject, body_id
        );
        
        return email_result[0].at("email_message_id").as<uint32_t>();
    }
    catch (const std::exception& e) {
        throw MailException(e.what());
    }

}

void PgMailDB::CheckIfUserLoggedIn()
{
    if (m_user_id == 0)
    {
        throw MailException("There is no logged in user.");
    }
}

void PgMailDB::AddFolder(const std::string_view folder_name)
{
    PgConnection conn(*m_connection_pool);
    pqxx::work transaction(*conn);

    try
    {
        InsertFolderForUser(folder_name, m_user_id, transaction);
        transaction.commit();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }    
}

void PgMailDB::AddMessageToFolder(const std::string_view folder_name, const Mail& message)
{
    PgConnection conn(*m_connection_pool);
    pqxx::work transaction(*conn);

    try
    {
        uint32_t email_message_id = RetrieveMessageID(message, transaction);
        InsertMailToFolder(folder_name, email_message_id, m_user_id, transaction);
        transaction.commit();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }

}

void PgMailDB::MoveMessageToFolder(const std::string_view from, const std::string_view to, const Mail& message)
{
    PgConnection conn(*m_connection_pool);
    pqxx::work transaction(*conn);

    try
    {
        uint32_t email_message_id = RetrieveMessageID(message, transaction), 
                 from_folder_id = RetrieveFolderID(from, m_user_id, transaction),
                 to_folder_id = RetrieveFolderID(to, m_user_id, transaction);

        pqxx::result result = transaction.exec_params("SELECT * FROM \"folderMessages\" WHERE email_message_id = $1 AND folder_id = $2"
                                                     , email_message_id, to_folder_id);

        if(result.empty())
        {
            transaction.exec_params("UPDATE \"folderMessages\" SET folder_id = $1 WHERE email_message_id = $2 AND folder_id = $3"
                                   , to_folder_id, email_message_id, from_folder_id);
        }

        transaction.commit();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

uint32_t PgMailDB::RetrieveMessageID(const Mail& message, pqxx::transaction_base& transaction) const
{
    pqxx::result message_id;
    
    try
    {
        message_id = transaction.exec_params("SELECT email_message_id FROM \"emailMessages\" AS em "
                                                         "LEFT JOIN \"users\" AS u ON u.user_id = em.sender_id "
                                                         "LEFT JOIN \"mailBodies\" AS mb ON mb.mail_body_id = em.mail_body_id "
                                                         "WHERE em.recipient_id = $1 AND u.user_name = $2 AND em.subject = $3 AND mb.body_content = $4 AND em.sent_at = $5 "
                                                         , m_user_id, message.sender, message.subject, message.body, message.sent_at
                                                        );

        return message_id[0].at("email_message_id").as<uint32_t>();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

uint32_t PgMailDB::RetrieveFolderID(const std::string_view folder_name, uint32_t user_id, pqxx::transaction_base& transaction) const
{
    pqxx::result folder_id;
    try
    {
        folder_id = transaction.exec_params("SELECT folder_id FROM \"folders\" WHERE user_id = $1 AND folder_name = $2"
                                            , user_id, folder_name);
        
        if(folder_id.empty())
        {
            throw MailException("Folder doesn't exist");
        }

        return folder_id[0].at("folder_id").as<uint32_t>();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

void PgMailDB::FlagMessage(const std::string_view flag_name, const Mail& message)
{
    PgConnection conn(*m_connection_pool);
    pqxx::work transaction(*conn);

    try
    {
        uint32_t email_message_id = RetrieveMessageID(message, transaction);
        InsertFlagForMessage(flag_name, email_message_id, transaction);
        transaction.commit();
    }
    catch(const std::exception& e)
    {
       throw MailException(e.what()); 
    }
}

uint32_t PgMailDB::RetrieveFlagID(const std::string_view flag_name, pqxx::transaction_base& transaction) const
{
    pqxx::result flag_id;
    try
    {
        flag_id = transaction.exec_params("SELECT flag_id FROM \"flags\" WHERE flag_name = $1"
                                            , flag_name);
        
        if(flag_id.empty())
        {
            throw MailException("Flag doesn't exist");
        }

        return flag_id[0].at("flag_id").as<uint32_t>();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }

}

void PgMailDB::DeleteFolder(const std::string_view folder_name)
{
    PgConnection conn(*m_connection_pool);
    pqxx::work transaction(*conn);

    try
    {
        transaction.exec_params("DELETE FROM \"folders\" WHERE folder_name = $1 AND user_id = $2"
                                , folder_name, m_user_id);

        transaction.commit();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

void PgMailDB::RemoveMessageFromFolder(const std::string_view folder_name, const Mail& message)
{
    PgConnection conn(*m_connection_pool);
    pqxx::work transaction(*conn);

    try
    {
        uint32_t email_message_id = RetrieveMessageID(message, transaction), 
                 folder_id = RetrieveFolderID(folder_name, m_user_id, transaction);

        transaction.exec_params("DELETE FROM \"folderMessages\" WHERE folder_id = $1 AND email_message_id = $2"
                                , folder_id, email_message_id);

        transaction.commit();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

void PgMailDB::RemoveFlagFromMessage(const std::string_view flag_name, const Mail& message)
{
    PgConnection conn(*m_connection_pool);
    pqxx::work transaction(*conn);

    try
    {
        uint32_t email_message_id = RetrieveMessageID(message, transaction), 
                 flag_id = RetrieveFlagID(flag_name, transaction);

        transaction.exec_params("DELETE FROM \"messageFlags\" WHERE flag_id = $1 AND email_message_id = $2"
                                , flag_id, email_message_id);

        transaction.commit();
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

std::vector<Mail> PgMailDB::RetrieveMessagesFromFolder(const std::string_view folder_name, const ReceivedState& is_received)
{
    PgConnection conn(*m_connection_pool);
    pqxx::transaction transaction(*conn);

    try
    {
        uint32_t folder_id = RetrieveFolderID(folder_name, m_user_id, transaction);

        std::vector<Mail> resutl_mails;
        std::string query = "SELECT em.email_message_id, u.user_name AS sender_name, em.subject, m.body_content, em.sent_at "
                            "FROM \"emailMessages\" AS em "
                            "LEFT JOIN users AS u ON u.user_id = em.sender_id "
                            "LEFT JOIN \"mailBodies\" AS m ON m.mail_body_id = em.mail_body_id "
                            "LEFT JOIN \"folderMessages\" AS fm ON fm.email_message_id = em.email_message_id "
                            "WHERE fm.folder_id = " +
                            std::to_string(folder_id) +
                            " AND " + get_received_state_string(is_received) + " " 
                            "ORDER BY em.sent_at DESC; ";
                               
        for(auto& [email_id, sender, subject, body, sent_at] : transaction.query<uint32_t, std::string, std::string, std::string, std::string>(query))
        {
            resutl_mails.emplace_back(m_user_name, sender, subject, body, sent_at, RetrieveAttachments(email_id, transaction));
        }

        transaction.commit();

        return resutl_mails;
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
    return {};
}

std::vector<Mail> PgMailDB::RetrieveMessagesFromFolderWithFlags(const std::string_view folder_name, FlagsSearchBy& flags, const ReceivedState& is_received)
{
    PgConnection conn(*m_connection_pool);
    pqxx::transaction transaction(*conn);

    try
    {
        uint32_t folder_id = RetrieveFolderID(folder_name, m_user_id, transaction);

        pqxx::result mails_id = transaction.exec_params(flags.BuildQuery(folder_id));
        
        std::vector<Mail> resutl_mails;
        std::string query;

        for(auto&& row : mails_id)
        {
            query = "SELECT em.email_message_id, u.user_name AS sender_name, em.subject, m.body_content, em.sent_at "
                    "FROM \"emailMessages\" AS em "
                    "LEFT JOIN users AS u ON u.user_id = em.sender_id "
                    "LEFT JOIN \"mailBodies\" AS m ON m.mail_body_id = em.mail_body_id "
                    "WHERE email_message_id = " +
                    row.at("email_message_id").as<std::string>() + 
                    " AND " + get_received_state_string(is_received) + " " 
                    "ORDER BY em.sent_at DESC; ";
                                   
            for(auto& [email_id, sender, subject, body, sent_at] : transaction.query<uint32_t, std::string, std::string, std::string, std::string>(query))
            {
                resutl_mails.emplace_back(m_user_name, sender, subject, body, sent_at, RetrieveAttachments(email_id, transaction));
            }
        }

        transaction.commit();

        return resutl_mails;
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

std::string PgMailDB::get_received_state_string(const ReceivedState& is_received)
{
    switch(is_received)
    {
        case ReceivedState::TRUE :
        {
            return "is_received = TRUE";
        }
        case ReceivedState::FALSE :
        {
            return "is_received = FALSE";
        }
        case ReceivedState::BOTH :
        {
            return "(is_received = TRUE OR is_received = FALSE)";
        }
        default: 
        {
            return "is_received = FALSE";
        }
    }
}

}