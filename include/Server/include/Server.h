#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <chrono>

#include "ISocketWrapper.h"
#include "TcpSocketWrapper.h"
#include "ServerConfig.h"
#include "ThreadPool.h"

using boost::asio::ip::tcp;

namespace ISXSS
{
class Server
{
public:
    Server(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context);
    ~Server();

    void Start();

protected:
    std::chrono::seconds m_timeout_duration;
    boost::asio::io_context& m_io_context;
    boost::asio::ssl::context& m_ssl_context;

private:
    std::string m_server_name;
    std::string m_server_display_name;
    uint8_t m_port;
    std::string m_server_ip;

    uint8_t m_max_threads;
    std::unique_ptr<ISXThreadPool::ThreadPool<>> m_thread_pool;
    uint8_t m_log_level;

    std::unique_ptr<tcp::acceptor> m_acceptor;

private:
    void Accept();
    void InitializeAcceptor();
    void InitializeLogger();
    void InitializeThreadPool();
    void InitializeTimeout();

    void ConfigureSslContext();

    boost::asio::steady_timer m_timeout_timer;

private:
    Config m_config;
};
}  // namespace ISXSS
#endif  // SERVER_H