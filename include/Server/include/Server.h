#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <chrono>

#include "ISocketWrapper.h"
#include "JSONParser.h"
#include "ServerConfig.h"
#include "TcpSocketWrapper.h"
#include "ThreadPool.h"

#include "MailDB/PgMailDB.h"
#include "MailDB/ConnectionPool.h"
#include "MailDB/PgManager.h"

using boost::asio::ip::tcp;

namespace ISXSS
{
class Server
{
public:
    Server(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context);
    ~Server() = default;

    void Start();

protected:
    std::chrono::seconds m_timeout_duration;
    boost::asio::io_context& m_io_context;
    boost::asio::ssl::context& m_ssl_context;

private:
    std::string m_server_name;
    std::string m_server_display_name;
    uint16_t m_port;
    std::string m_server_ip;

    uint8_t m_max_threads;
    std::unique_ptr<ISXThreadPool::ThreadPool<>> m_thread_pool;
    uint8_t m_log_level;

    std::unique_ptr<tcp::acceptor> m_acceptor;

    std::unique_ptr<ISXMailDB::PgManager> m_pg_manager;
    std::unique_ptr<ISXMailDB::ConnectionPool<pqxx::connection>> m_connection_pool;
    const uint16_t MAX_DATABASE_CONNECTIONS = 10;

    static constexpr const char* S_CONNECTION_STRING =
        "postgresql://postgres.qotrdwfvknwbfrompcji:"
        "yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler."
        "supabase.com:6543/postgres?sslmode=require";

private:
    void Accept();
    void InitializeAcceptor();
    void InitializeLogger();
    void InitializeThreadPool();
    void InitializeTimeout();

    void ConfigureSslContext();

    void InitializeDatabaseManager();

    boost::asio::steady_timer m_timeout_timer;

private:
    Config m_config;
};
}  // namespace ISXSS
#endif  // SERVER_H
