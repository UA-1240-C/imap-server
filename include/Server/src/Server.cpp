#include "Server.h"

#include "ClientSession.h"
#include "ISocketWrapper.h"
#include "TcpSocketWrapper.h"

using namespace boost::asio::ip;
using ISXCS::ClientSession;

using ISXSockets::ISocketWrapper;
using ISXSockets::TcpSocketWrapper;

namespace ISXSS
{
Server::Server(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context)
    : m_config("../config.txt"), m_timeout_timer(io_context), m_io_context(io_context), m_ssl_context(ssl_context)
{
    Logger::LogTrace("Constructor params: io_context, ssl_context");

    InitializeAcceptor();
    InitializeLogger();
    InitializeThreadPool();
    InitializeTimeout();
    ConfigureSslContext();
}

void Server::Start() { Accept(); }

void Server::Accept()
{
    Logger::LogDebug("Entering Server::Accept");

    auto socket_wrapper = std::make_shared<TcpSocketWrapper>(m_io_context);
    auto tcp_socket = std::static_pointer_cast<TcpSocket>(socket_wrapper->get_socket());

    if (!tcp_socket)
    {
        Logger::LogError("Failed to retrieve a valid TCP socket");
        return;
    }

    Logger::LogProd("Ready to accept new connections.");

    m_acceptor->async_accept(*tcp_socket,
                             [this, socket_wrapper](const boost::system::error_code& error)
                             {
                                 if (error)
                                 {
                                     Logger::LogError("Boost error in Server::Accept: " + error.message());
                                     return;
                                 }

                                 Logger::LogProd("Accepted new connection.");
                                 m_thread_pool->EnqueueDetach(
                                     [this, socket_wrapper]
                                     {
                                         try
                                         {
                                             std::unique_ptr<ClientSession> client_session =
                                                 std::make_unique<ClientSession>(socket_wrapper, m_ssl_context,
                                                                                 m_timeout_duration, m_io_context);

                                             client_session->PollForRequest();
                                         }
                                         catch (const std::exception& e)
                                         {
                                             Logger::LogError("Error in Server::Accept: " + std::string(e.what()));
                                         }
                                     });

                                 Accept();
                             });

    Logger::LogDebug("Exiting Accept");
}

void Server::InitializeAcceptor()
{
    std::tie(m_server_name, m_server_display_name, m_port, m_server_ip) = m_config.get_server_tuple();

    std::cout << "port: " << m_port << std::endl;
    try
    {
        tcp::resolver resolver(m_io_context);
        tcp::resolver::query query(m_server_ip, std::to_string(m_port));
        auto endpoints = resolver.resolve(query);

        if (endpoints.empty())
        {
            throw std::runtime_error("No endpoints resolved");
        }

        tcp::endpoint endpoint = *endpoints.begin();

        m_acceptor = std::make_unique<tcp::acceptor>(m_io_context);
        m_acceptor->open(endpoint.protocol());
        m_acceptor->set_option(tcp::acceptor::reuse_address(true));
        m_acceptor->bind(endpoint);
        m_acceptor->listen();
    }
    catch (std::exception& e)
    {
        Logger::LogError("Exception in InitializeAcceptor: " + std::string(e.what()));
        throw;
    }
}

void Server::InitializeLogger()
{
    Logger::Setup(m_config.get_logging());

    Logger::LogTrace("Logging initialized with log_level: " + std::to_string(m_log_level));
}

void Server::InitializeThreadPool()
{
    auto [max_working_threads] = m_config.get_thread_pool();
    m_max_threads = max_working_threads > std::thread::hardware_concurrency() ? std::thread::hardware_concurrency()
                                                                              : max_working_threads;

    auto thread_pool = std::make_unique<ISXThreadPool::ThreadPool<>>(m_max_threads);
    m_thread_pool = std::move(thread_pool);

    Logger::LogTrace("Thread pool initialized with " + std::to_string(m_max_threads) + " threads");
}

void Server::InitializeTimeout()
{
    const auto& [blocking, socket_timeout] = m_config.get_communication_settings();

    m_timeout_duration = std::chrono::seconds(socket_timeout);
    Logger::LogDebug("Timeout initialized to " + std::to_string(m_timeout_duration.count()) + " seconds");
}

void Server::ConfigureSslContext()
{
    m_ssl_context.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                              boost::asio::ssl::context::no_sslv3 | boost::asio::ssl::context::no_tlsv1 |
                              boost::asio::ssl::context::no_tlsv1_1);

    m_ssl_context.set_default_verify_paths();

    m_ssl_context.set_verify_mode(boost::asio::ssl::verify_peer);
}

}  // namespace ISXSS