#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include "ImapRequest.h"
#include "MailDB/MailException.h"
#include "MailDB/PgMailDB.h"
#include "MailDB/PgManager.h"
#include "Server.h"

using ISXImapRequest::ImapRequest;
using ISXMailDB::MailException;
using ISXSockets::ISocketWrapper;

enum class ClientState
{
    CONNECTED,
    AUTHENTICATED,
    SELECTED,
    FETCHING,
    LOGOUT,
    DISCONNECTED,
};

namespace ISXCS
{
class ClientSession
{
public:
    ClientSession(std::shared_ptr<ISocketWrapper> socket, boost::asio::ssl::context& ssl_context,
                  std::chrono::seconds timeout_duraion, boost::asio::io_context& io_context,
                  ISXMailDB::PgManager& pg_manager);
    ~ClientSession() = default;

    void PollForRequest();

private:
    void HandleNewRequest();
    void ProcessRequest(const ImapRequest& request);

    std::future<void> AsyncPerformHandshake();

    void HandleCapability(const ImapRequest& request);
    void HandleLogin(const ImapRequest& request);
    void HandleBye(const ImapRequest& request);
    void HandleFetch(const ImapRequest& request);

private:
    ClientState m_current_state;

private:
    std::shared_ptr<ISocketWrapper> m_socket_wrapper;
    boost::asio::io_context& m_io_context;
    boost::asio::ssl::context& m_ssl_context;

    std::chrono::seconds m_timeout_duration;
    std::unique_ptr<ISXMailDB::PgMailDB> m_database;
};
}  // namespace ISXCS
#endif  // CLIENT_SESSION_H
