#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using TcpSocket = boost::asio::ip::tcp::socket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;

using SslSocket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
using SslSocketPtr = std::shared_ptr<SslSocket>;

class SocketWrapper
{
public:
    explicit SocketWrapper(boost::asio::io_context io_context);
    ~SocketWrapper();

    template <typename SocketType>
    void set_socket(SocketType socket)
    {
        m_socket = socket;
        m_is_tls = std::is_same_v<SocketType, SslSocket>;
    }

    std::variant<TcpSocketPtr, SslSocketPtr> get_socket() const;

    std::future<void> PerformTlsHandshake(boost::asio::ssl::context& ssl_context,
                                          boost::asio::ssl::stream_base::handshake_type type);

    std::future<void> SendResponseAsync(const std::string& message);
    std::future<std::string> ReadFromSocketAsync();

public:
    void StartTimeoutTimer(std::chrono::seconds timeout_duration);
    void CancelTimeoutTimer();
    void RestartTimeoutTimer(std::chrono::seconds timeout_duration);

    void set_timeout_timer();

    bool IsOpen();
    void Close();

    void TerminateTcpConnection(TcpSocket& tcp_socket);
    void TerminateSslConnection(SslSocket& ssl_socket);

private:
    bool m_is_tls;
    std::variant<TcpSocketPtr, SslSocketPtr> m_socket;
    std::shared_ptr<boost::asio::steady_timer> m_timeout_timer;
    static constexpr const char* CRLF = "\r\n";
};

#endif  // SOCKET_WRAPPER_H