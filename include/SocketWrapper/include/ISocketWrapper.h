#ifndef I_SOCKET_WRAPPER_H
#define I_SOCKET_WRAPPER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "Logger.h"

using TcpSocket = boost::asio::ip::tcp::socket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;

using SslSocket = boost::asio::ssl::stream<TcpSocket>;
using SslSocketPtr = std::shared_ptr<SslSocket>;

namespace ISXSockets
{
class ISocketWrapper
{
public:
    ISocketWrapper(boost::asio::io_context& io_context) : m_io_context(io_context), m_timeout_timer(nullptr) {}
    virtual std::future<void> SendResponseAsync(const std::string& message) = 0;
    virtual std::future<std::string> ReadFromSocketAsync() = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;

    template <typename SocketType>
    std::shared_ptr<SocketType> get_socket();

    static constexpr const char* CRLF = "\r\n";

    std::shared_ptr<boost::asio::steady_timer> m_timeout_timer;
    boost::asio::io_context& m_io_context;

public:
    inline void ISocketWrapper::StartTimeoutTimer(std::chrono::seconds timeout_duration)
    {
        if (!m_timeout_timer)
        {
            m_timeout_timer = std::make_shared<boost::asio::steady_timer>(m_io_context);
        }

        m_timeout_timer->expires_after(timeout_duration);

        m_timeout_timer->async_wait(
            [this](const boost::system::error_code& error)
            {
                if (error)
                {
                    std::cerr << "Timeout timer handler was cancelled or an error occurred: " << error.message()
                              << std::endl;
                    return;
                }
                this->Close();
            });
    }

    inline void ISocketWrapper::CancelTimeoutTimer()
    {
        if (!m_timeout_timer)
        {
            std::cerr << "Timeout timer is not valid" << std::endl;
            return;
        }

        boost::system::error_code ec;
        m_timeout_timer->cancel(ec);
        if (!ec)
        {
            std::cerr << "Timeout timer successfully cancelled." << std::endl;
        }
        else
        {
            std::cerr << "Failed to cancel timeout timer: " << ec.message() << std::endl;
        }
    }

    inline void ISocketWrapper::RestartTimeoutTimer(std::chrono::seconds timeout_duration)
    {
        CancelTimeoutTimer();
        StartTimeoutTimer(timeout_duration);
    }
};
}  // namespace ISXISocketWrapper

#endif  // I_SOCKET_WRAPPER_H