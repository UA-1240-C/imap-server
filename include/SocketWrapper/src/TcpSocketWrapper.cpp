#include "TcpSocketWrapper.h"

namespace ISXSockets
{
TcpSocketWrapper::TcpSocketWrapper(boost::asio::io_context& io_context)
    : m_socket(std::make_shared<TcpSocket>(io_context)), ISocketWrapper(io_context)
{
}

TcpSocketWrapper::~TcpSocketWrapper() {}

std::future<void> TcpSocketWrapper::SendResponseAsync(const std::string& message)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    if (!m_socket)
    {
        const std::string error_message = "TCP Socket doesn't exist";
        Logger::LogError(error_message);
        promise->set_exception(std::make_exception_ptr(std::runtime_error(error_message)));
        return future;
    }

    async_write(*m_socket, boost::asio::buffer(message),
                [promise](const boost::system::error_code& error, std::size_t)
                {
                    if (error)
                    {
                        Logger::LogError("Error during async_write: " + error.message());
                        promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                        return;
                    }
                    promise->set_value();
                });

    return future;
}

std::future<std::string> TcpSocketWrapper::ReadFromSocketAsync()
{
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    auto buffer = std::make_shared<boost::asio::streambuf>();  // створення буфера

    if (!m_socket)
    {
        const std::string error_message = "TCP Socket doesn't exist";
        Logger::LogError(error_message);
        promise->set_exception(std::make_exception_ptr(std::runtime_error(error_message)));
        return future;
    }

    boost::asio::async_read_until(
        *m_socket, *buffer, ISocketWrapper::CRLF,
        [promise, buffer](const boost::system::error_code& error, std::size_t bytes_transferred)
        {
            if (error)
            {
                Logger::LogError("Error in async_read_until: " + error.message());
                promise->set_exception(std::make_exception_ptr(std::runtime_error(error.message())));
                return;
            }

            std::istream is(buffer.get());
            std::string message;
            std::getline(is, message);
            promise->set_value(message);
        });

    return future;
}

void TcpSocketWrapper::Close()
{
    if (m_socket && m_socket->is_open())
    {
        boost::system::error_code ec;

        m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (ec)
        {
            Logger::LogError("Error during shutdown: " + ec.message());
        }

        m_socket->cancel(ec);
        if (ec)
        {
            Logger::LogError("Error during cancel: " + ec.message());
        }

        m_socket->close(ec);
        if (ec)
        {
            Logger::LogError("Error during close: " + ec.message());
        }
    }
    else
    {
        Logger::LogError("TCP socket already closed or doesn't exist.");
    }
}

bool TcpSocketWrapper::IsOpen() const { return m_socket->is_open() ? true : false; }

}  // namespace ISXTcpSocketWrapper