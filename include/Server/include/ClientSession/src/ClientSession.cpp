#include "ClientSession.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "SslSocketWrapper.h"
using ISXSockets::SslSocketWrapper;

namespace ISXCS
{
ClientSession::ClientSession(std::shared_ptr<ISocketWrapper> socket, boost::asio::ssl::context& ssl_context,
                             std::chrono::seconds timeout_duration, boost::asio::io_context& io_context)
    : m_io_context(io_context),
      m_ssl_context(ssl_context),
      m_timeout_duration(timeout_duration),
      m_current_state(IMAPState::CONNECTED)
{
    m_socket = socket;
    m_socket->StartTimeoutTimer(timeout_duration);
    m_socket->WhoIs();
    m_socket->SendResponseAsync("Hello");
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
    if (!m_socket->IsOpen())
    {
        Logger::LogWarning("Client disconneced.");
        throw std::runtime_error("Client disconnected.");
    }

    auto buffer = m_socket->ReadFromSocketAsync().get();
    m_socket->RestartTimeoutTimer(m_timeout_duration);
    Logger::LogProd("Received data: " + buffer);

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

void ClientSession::StartTLS()
{
    /*    auto tcp_socket = m_socket->get_socket<TcpSocket>();

    m_socket = std::make_shared<SslSocketWrapper>(m_io_context, m_ssl_context, tcp_socket);

    std::dynamic_pointer_cast<SslSocket>(m_socket)->async_handshake(
        boost::asio::ssl::stream_base::server,
        [this](const boost::system::error_code& error)
        {
            if (error)
            {
                Logger::LogError("Error during handshake: " + error.message());
                return;
            }
        });
    */
}
}  // namespace ISXCS