#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include "Server.h"

using ISXSockets::ISocketWrapper;

enum class IMAPState
{
    CONNECTED,
    AUTHENTICATED,
    SELECTED,
    FETCHING,
    APPENDING,
    IDLE,
    LOGOUT,
    DISCONNECTED,
};

namespace ISXCS
{
class ClientSession
{
public:
    ClientSession(TcpSocketPtr socket, boost::asio::ssl::context& ssl_context, std::chrono::seconds timeout_duraion,
                  boost::asio::io_context& io_context);
    ~ClientSession() = default;

    void PollForRequest();

private:
    void HandleNewRequest();
    // void ProcessRequest();

    void StartTLS();

private:
    IMAPState m_current_state;

private:
    std::shared_ptr<ISocketWrapper> m_socket;
    boost::asio::io_context& m_io_context;
    boost::asio::ssl::context& m_ssl_context;

    std::chrono::seconds m_timeout_duration;
};
}  // namespace ISXCS
#endif  // CLIENT_SESSION_H