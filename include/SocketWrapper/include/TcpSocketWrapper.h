#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "ISocketWrapper.h"
#include "SslSocketWrapper.h"

using ISXSockets::ISocketWrapper;
using ISXSockets::SslSocketWrapper;

namespace ISXSockets
{
class TcpSocketWrapper : public ISocketWrapper
{
public:
    TcpSocketWrapper(boost::asio::io_context& io_context);
    ~TcpSocketWrapper();

    std::future<void> SendResponseAsync(const std::string& message) override;
    std::future<std::string> ReadFromSocketAsync() override;
    void Close() override;
    bool IsOpen() const override;

    virtual void WhoIs() { std::cout << "SslWrapper" << std::endl; }

    inline std::shared_ptr<void> get_socket() const override { return m_socket; }

private:
    TcpSocketPtr m_socket;
};
}  // namespace ISXSockets

#endif  // TCP_SOCKET_H