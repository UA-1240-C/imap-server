#include "SocketWrapper.h"

#include <iostream>

SocketWrapper::SocketWrapper(boost::asio::io_context io_context) : m_socket(std::make_shared<TcpSocket>(io_context)) {}

SocketWrapper::~SocketWrapper() { Close(); }

std::future<void> SocketWrapper::PerformTlsHandshake(boost::asio::ssl::context& ssl_context,
                                                     boost::asio::ssl::stream_base::handshake_type type)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    if (!std::get_if<TcpSocketPtr>(&m_socket))
    {
        const std::string error_message = "No valid TCP socket available for TLS handshake.";
        std::cerr << error_message << std::endl;
        promise->set_exception(std::make_exception_ptr(std::runtime_error(error_message)));
        return future;
    }

    auto* tcp_socket = std::get_if<TcpSocketPtr>(&m_socket);
    auto ssl_socket = std::make_shared<SslSocket>(std::move(**tcp_socket), ssl_context);

    ssl_socket->async_handshake(
        type,
        [this, ssl_socket, promise](const boost::system::error_code& error)
        {
            if (error)
            {
                std::cerr << "TLS handshake error: " << error.message() << std::endl;
                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                return;
            }

            set_socket(ssl_socket);
            promise->set_value();
        });

    return future;
}

std::future<void> SocketWrapper::SendResponseAsync(const std::string& message)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    if (!std::get_if<TcpSocketPtr>(&m_socket) && !std::get_if<SslSocketPtr>(&m_socket))
    {
        const std::string error_message = "No valid socket available for sending data.";
        std::cerr << error_message << std::endl;
        promise->set_exception(std::make_exception_ptr(std::runtime_error(error_message)));
        return future;
    }

    std::visit(
        [this, &message, promise](auto& socket)
        {
            async_write(*socket, boost::asio::buffer(message),
                        [promise](const boost::system::error_code& error, std::size_t)
                        {
                            if (error)
                            {
                                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                                return;
                            }
                            promise->set_value();
                        });
        },
        m_socket);

    return future;
}

std::future<std::string> SocketWrapper::ReadFromSocketAsync()
{
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    auto buffer = std::make_shared<boost::asio::streambuf>();

    if (!std::get_if<TcpSocketPtr>(&m_socket) && !std::get_if<SslSocketPtr>(&m_socket))
    {
        const std::string error_message = "No valid socket available for sending data.";
        std::cerr << error_message << std::endl;
        promise->set_exception(std::make_exception_ptr(std::runtime_error(error_message)));
        return future;
    }

    std::visit(
        [this, promise, buffer](auto& socket)
        {
            boost::asio::async_read_until(
                *socket, *buffer, SocketWrapper::CRLF,
                [promise, buffer](const boost::system::error_code& error, std::size_t bytes_transferred)
                {
                    if (error)
                    {
                        std::cerr << "Error in async_read_until: " << error.message() << std::endl;
                        promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                        return;
                    }

                    std::istream is(buffer.get());
                    std::string message;
                    std::getline(is, message);
                    promise->set_value(message);
                });
        },
        m_socket);

    return future;
}

bool SocketWrapper::IsOpen()
{
    bool is_open = false;

    std::visit(
        [this, &is_open](auto& socket)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(socket)>, SslSocketPtr>)
            {
                is_open = socket && socket->lowest_layer().is_open();
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(socket)>, TcpSocketPtr>)
            {
                is_open = socket && socket->is_open();
            }
        },
        m_socket);

    return is_open;
}

void SocketWrapper::Close()
{
    std::visit(
        [this](auto& socket)
        {
            if (socket)
            {
                using SocketType = std::decay_t<decltype(socket)>;

                if constexpr (std::is_same_v<SocketType, SslSocketPtr>)
                {
                    TerminateSslConnection(*socket);
                }
                else if constexpr (std::is_same_v<SocketType, TcpSocketPtr>)
                {
                    TerminateTcpConnection(*socket);
                }
            }
        },
        m_socket);
}

void SocketWrapper::TerminateSslConnection(SslSocket& ssl_socket)
{
    boost::system::error_code ec;

    if (!(ssl_socket.lowest_layer().is_open()))
    {
        std::cerr << "SSL socket closed" << std::endl;
        return;
    }

    ssl_socket.lowest_layer().shutdown(TcpSocket::shutdown_both, ec);
    ssl_socket.lowest_layer().cancel(ec);
    ssl_socket.lowest_layer().close(ec);
}

void SocketWrapper::TerminateTcpConnection(TcpSocket& tcp_socket)
{
    boost::system::error_code ec;

    if (!(tcp_socket.is_open()))
    {
        std::cerr << "TCP socket closed" << std::endl;
        return;
    }

    tcp_socket.shutdown(TcpSocket::shutdown_both, ec);
    tcp_socket.cancel(ec);
    tcp_socket.close(ec);
}

void SocketWrapper::StartTimeoutTimer(std::chrono::seconds timeout_duration)
{
    if (!m_timeout_timer)
    {
        std::cerr << "Timeout timer is not valid" << std::endl;
        return;
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

void SocketWrapper::CancelTimeoutTimer()
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
        return;
    }

    std::cerr << "Failed to cancel timeout timer: " << ec.message() << std::endl;
}

void SocketWrapper::RestartTimeoutTimer(std::chrono::seconds timeout_duration)
{
    CancelTimeoutTimer();
    if (!m_timeout_timer)
    {
        std::cerr << "Timeout timer is not valid" << std::endl;
        return;
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
