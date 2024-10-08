#include "PgManager.h"

namespace ISXMailDB
{

PgManager::PgManager(const std::string_view connection_string, const std::string_view host_name,
                     bool should_cache_emails)
    : CONNECTION_STRING(connection_string), HOST_NAME(host_name),
      m_should_cache_emails(should_cache_emails)
{
    Init();
}

void PgManager::Init()
{
    InsertHost();
    InitConnectionPool();
    InitEmaisWriter();
}

void PgManager::InitEmaisWriter()
{
    try
    {
        if (!m_should_cache_emails)
        {
            m_emails_writer = nullptr;
            return;
        }
        m_emails_writer = std::make_shared<PgEmailsWriter>(
            CONNECTION_STRING,
            m_host_id,
            MAX_WRITER_QUEUE_SIZE, 
            WRITER_TIMEOUT
        );
    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
    
}

void PgManager::InitConnectionPool()
{
    try
    {
        m_connection_pool = std::make_shared<ISXMailDB::ConnectionPool<pqxx::connection>>(
            POOL_INITIAL_SIZE,
            CONNECTION_STRING,
            [] (const std::string& connection_str)
            { 
                return std::make_shared<pqxx::connection>(connection_str);
            }
        );

    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}

void PgManager::InsertHost()
{
    if (HOST_NAME.empty())
        throw MailException("Host name can't be empty.");
    try
    {
        pqxx::connection conn{CONNECTION_STRING};
        pqxx::work tx(conn);
        try
        {
            m_host_id = tx.query_value<uint32_t>("SELECT host_id FROM hosts WHERE host_name = " + tx.quote(HOST_NAME));
        }
        catch (pqxx::unexpected_rows& e)
        {
            pqxx::result host_id_result = tx.exec_params(
                "INSERT INTO hosts (host_name) VALUES ( $1 ) RETURNING host_id", HOST_NAME);

            m_host_id = host_id_result[0][0].as<uint32_t>();
        }
        tx.commit();

    }
    catch(const std::exception& e)
    {
        throw MailException(e.what());
    }
}
}