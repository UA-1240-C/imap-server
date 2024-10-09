#include "ClientSession.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "MailDB/PgMailDB.h"
#include "ImapRequest.h"
#include "SslSocketWrapper.h"

using ISXSockets::SslSocketWrapper;

namespace ISXCS
{
ClientSession::ClientSession(std::shared_ptr<ISocketWrapper> socket, boost::asio::ssl::context& ssl_context,
                             std::chrono::seconds timeout_duration, boost::asio::io_context& io_context,
                             ISXMailDB::PgManager& pg_manager)
        : m_io_context(io_context)
        , m_ssl_context(ssl_context)
        , m_timeout_duration(timeout_duration)
        , m_current_state(IMAPState::CONNECTED)
        , m_database(std::make_unique<ISXMailDB::PgMailDB>(pg_manager))

{
    m_socket_wrapper = socket;
    m_socket_wrapper->StartTimeoutTimer(timeout_duration);
    m_socket_wrapper->WhoIs();
    m_socket_wrapper->WhoIs();
}

void ClientSession::PollForRequest()
{
    while (true)
    {
        try
        {
            HandleNewRequest();
        }
        catch (const boost::system::system_error& e)
        {
            if (e.code() == boost::asio::error::operation_aborted)
            {
                Logger::LogDebug("Client disconnected");
                break;
            }
            throw;
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Exception in ClientSession::PollForRequest: " + std::string(e.what()));
            throw;
        }
    }
}

void ClientSession::HandleNewRequest()
{
    if (!m_socket_wrapper->IsOpen())
    {
        Logger::LogWarning("Client disconneced.");
        throw std::runtime_error("Client disconnected.");
    }

    std::string buffer = m_socket_wrapper->ReadFromSocketAsync().get();
    m_socket_wrapper->WhoIs();
    Logger::LogProd("Received data: " + buffer);

    if (buffer.find("STARTTLS") != std::string::npos)
    {
        std::cout << "IN IF" << std::endl;
        m_socket_wrapper->SendResponseAsync("* OK SOSI\r\n").get();
        AsyncPerformHandshake().get();
    }

    std::cout << "after IF" << std::endl;

    m_socket_wrapper->RestartTimeoutTimer(m_timeout_duration);

    std::string current_line{};
    current_line.append(buffer);

    std::size_t pos;
    while ((pos = current_line.find(ISocketWrapper::CRLF)) != std::string::npos)
    {
        std::string line = current_line.substr(0, pos);
        current_line.erase(0, pos + std::string(ISocketWrapper::CRLF).length());

        try
        {
            // ProcessRequest()
        }
        catch (const std::exception& e)
        {
        }
    }
}

std::future<void> ClientSession::AsyncPerformHandshake()
{
    Logger::LogDebug("Entering ClientSession::PerformTlsHandshake");
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    TcpSocketPtr tcp_socket = std::static_pointer_cast<TcpSocket>(m_socket_wrapper->get_socket());
    if (!tcp_socket)
    {
        promise->set_exception(std::make_exception_ptr(std::runtime_error("Failed to retrieve TCP socket")));
        return future;
    }

    auto ssl_socket_wrapper = std::make_shared<SslSocketWrapper>(m_io_context, m_ssl_context, tcp_socket);

    auto ssl_socket = std::static_pointer_cast<SslSocket>(ssl_socket_wrapper->get_socket());

    std::cout << "Is SSL socket open: " << std::boolalpha << ssl_socket->lowest_layer().is_open() << std::endl;

    try
    {
        auto remote_port = ssl_socket->lowest_layer().remote_endpoint().port();
        std::cout << "Remote endpoint port: " << remote_port << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error retrieving remote endpoint: " << e.what() << std::endl;
    }

    ssl_socket->async_handshake(
        boost::asio::ssl::stream_base::server,
        [promise, this, ssl_socket_wrapper](const boost::system::error_code& error)
        {
            if (error)
            {
                Logger::LogError("Error during handshake: " + error.message());
                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                return;
            }

            m_socket_wrapper = ssl_socket_wrapper;
            m_socket_wrapper->WhoIs();
            m_socket_wrapper->SendResponseAsync("* OK 228\r\n");
            promise->set_value();
        });

    Logger::LogDebug("Exiting ClientSession::PerformTlsHandshake");
    return future;
}

void ClientSession::HandleCapability(ISXImapRequest::ImapRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleCapability");
    std::future<void> wrft =  m_socket_wrapper->SendResponseAsync(
        "* CAPABILITY IMAP4rev1 STARTTLS\r\n");

    try
    {
        wrft.get();
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Error sending response: " + std::string(e.what()));
        Logger::LogDebug("Exiting ClientSession::HandleCapability");
        throw;
    }

    Logger::LogDebug("Exiting ClientSession::HandleCapability");
}

void ClientSession::HandleLogin(ISXImapRequest::ImapRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleLogin");

    try
    {
        auto [username, password] = ISXImapRequest::ImapParser::ExtractUserAndPass(request.data);
        try
        {
            m_database->Login(username, password);
        }
        catch (const ISXMailDB::MailException& e)
        {
            m_socket_wrapper->SendResponseAsync(std::string("- NO ") + e.what() + "\r\n").get();
            return;
        }

        std::future<void> wrft = m_socket_wrapper->SendResponseAsync("+ OK Logged in\r\n");

        try
        {
            wrft.get();
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Error sending response: " + std::string(e.what()));
            throw;
        }
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Error extracting username and password: " + std::string(e.what()));
        Logger::LogDebug("Exiting ClientSession::HandleLogin");
        throw;
    }

    Logger::LogDebug("Exiting ClientSession::HandleLogin");
}

}  // namespace ISXCS
