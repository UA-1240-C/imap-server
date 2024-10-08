#ifndef SSL_SOCKET_H
#define SSL_SOCKET_H

#include "ISocketWrapper.h"

using ISXSockets::ISocketWrapper;

namespace ISXSockets
{
class SslSocketWrapper : public ISocketWrapper
{
public:
    SslSocketWrapper(boost::asio::io_context& io_context, boost::asio::ssl::context& ssl_context,
                     std::shared_ptr<TcpSocket> tcp_socket);
    ~SslSocketWrapper();

    std::future<void> SendResponseAsync(const std::string& message) override;
    std::future<std::string> ReadFromSocketAsync() override;
    void Close() override;
    bool IsOpen() const override;

    template <typename SocketType>
    std::shared_ptr<SocketType> get_socket()
    {
        return m_socket;
    }

private:
    SslSocketPtr m_socket;
};

}  // namespace ISXSslSocketWrapper

#endif  // SSL_SOCKET_H