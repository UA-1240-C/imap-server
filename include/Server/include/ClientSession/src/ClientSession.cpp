#include "ClientSession.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/concept_check.hpp>
#include <future>
#include <set>

#include "ImapRequest.h"
#include "MailDB/IMailDB.h"
#include "MailDB/PgMailDB.h"
#include "SslSocketWrapper.h"

using ISXImapRequest::ImapParser;
using ISXSockets::SslSocketWrapper;

using ISXMailDB::Mail;

using ISXMailDB::ReceivedState;

namespace ISXCS
{
ClientSession::ClientSession(std::shared_ptr<ISocketWrapper> socket, boost::asio::ssl::context& ssl_context,
                             std::chrono::seconds timeout_duration, boost::asio::io_context& io_context,
                             ISXMailDB::PgManager& pg_manager)
    : m_io_context(io_context),
      m_ssl_context(ssl_context),
      m_timeout_duration(timeout_duration),
      m_current_state(ClientState::CONNECTED),
      m_database(std::make_unique<ISXMailDB::PgMailDB>(pg_manager))

{
    m_socket_wrapper = socket;
    m_socket_wrapper->StartTimeoutTimer(timeout_duration);
    m_socket_wrapper->WhoIs();
    m_socket_wrapper->WhoIs();
}

void ClientSession::PollForRequest()
{
    if (!m_socket_wrapper->IsOpen())
    {
        Logger::LogWarning("Client disconnected.");
        return;
    }

    std::future<void> wrft = m_socket_wrapper->SendResponseAsync("* OK [STARTTLS CAPABILITY] Service Ready\r\n");

    try
    {
        wrft.get();
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Error sending response: " + std::string(e.what()));
        Logger::LogDebug("Exiting ClientSession::PollForRequest");
        throw;
    }

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
            Logger::LogError("Exception in ClientSession::PollForRequest: " + std::string(e.what()));
            Logger::LogDebug("Exiting ClientSession::PollForRequest");
            throw;
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Exception in ClientSession::PollForRequest: " + std::string(e.what()));
            Logger::LogDebug("Exiting ClientSession::PollForRequest");
            throw;
        }
    }

    Logger::LogDebug("Exiting ClientSession::PollForRequest");
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

    m_socket_wrapper->RestartTimeoutTimer(m_timeout_duration);

    try
    {
        ProcessRequest(buffer);
    }
    catch (...)
    {
        Logger::LogError("Error processing request");
    }
}

void ClientSession::ProcessRequest(std::string& buffer)
{
    ISXImapRequest::ImapRequest request = ISXImapRequest::ImapParser::Parse(buffer);

    Logger::LogProd("Command: " + std::to_string(static_cast<int>(request.command)));
    Logger::LogProd("Data: " + request.data);

    switch (m_current_state)
    {
        case IMAPState::CONNECTED:
            if (request.command == ISXImapRequest::IMAPCommand::CAPABILITY)
            {
                std::string commands = "STARTTLS CAPABILITY";
                HandleCapability(request, commands);
                return;
            }
            else if (request.command == ISXImapRequest::IMAPCommand::STARTTLS)
            {
                HandleStartTLS(request);
                m_current_state = IMAPState::ENCRYPTED;
            }
            else
            {
                m_socket_wrapper->SendResponseAsync("* ERR bad sequence\r\n").get();
                return;
            }
            break;
        case IMAPState::ENCRYPTED:
            if (request.command == ISXImapRequest::IMAPCommand::CAPABILITY)
            {break, commands);
                return;
            }
            else if (request.command == ISXImapRequest::IMAPCommand::LOGIN)
            {
                HandleLogin(request);
                m_current_state = IMAPState::AUTHENTICATED;
                return;
            }
            else
            {
                m_socket_wrapper->SendResponseAsync("* ERR bad sequence\r\n").get();
                return;
            }
            break;
        case IMAPState::AUTHENTICATED:
            if (request.command == ISXImapRequest::IMAPCommand::CAPABILITY)
            {
                std::string commands = "SELECT CAPABILITY";
                HandleCapability(request, commands);
                return;
            }
            else
            {
                m_socket_wrapper->SendResponseAsync("* ERR bad sequence\r\n").get();
                return;
            }
            break;
        default:
            /* Unreachable */
            m_socket_wrapper->SendResponseAsync("* ERR bad sequence\r\n").get();
            return;
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
            promise->set_value();
        });

    Logger::LogDebug("Exiting ClientSession::PerformTlsHandshake");
    return future;
}

void ClientSession::HandleStartTLS(ISXImapRequest::ImapRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleStartTLS");

    try
    {
        m_socket_wrapper->SendResponseAsync("* OK Begin TLS negotiation now\r\n").get();
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Error sending response: " + std::string(e.what()));
        Logger::LogDebug("Exiting ClientSession::HandleStartTLS");
        throw;
    }

    AsyncPerformHandshake().get();
    m_socket_wrapper->SendResponseAsync("* OK Handshake successful\r\n").get();

    Logger::LogDebug("Exiting ClientSession::HandleStartTLS");
}

void ClientSession::HandleCapability(ISXImapRequest::ImapRequest& request, std::string& commands)
{
    Logger::LogDebug("Entering ClientSession::HandleCapability");
    std::future<void> wrft =
        m_socket_wrapper->SendResponseAsync("* CAPABILITY [" + commands + "] IMAP server ready\r\n");

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
            m_database->Login("user@gmail.com", "password");
            Logger::LogProd("User " + username + " logged in");
        }
        catch (const ISXMailDB::MailException& e)
        {
            m_socket_wrapper->SendResponseAsync(std::string("* BAD ") + e.what() + "\r\n").get();
            return;
        }

        std::future<void> wrft = m_socket_wrapper->SendResponseAsync("* OK Logged in\r\n");

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

void ClientSession::HandleBye(const ImapRequest& request)
{
    Logger::LogProd("Entering ClientSession::HandleBye");
    try
    {
        m_socket_wrapper->SendResponseAsync("BYE");
        m_socket_wrapper->Close();
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Exception while closing socket: " + std::string(e.what()));
    }
    Logger::LogProd("Exiting ClientSession::HandleBye");
}

void ClientSession::HandleFetch(const ImapRequest& request)
{
    Logger::LogProd("Entering ClientSession::HandleFetch");

    // Парсимо FETCH-запит
    auto [request_id, message_set, fetch_attribute] = ImapParser::ParseFetchRequest(request.data);

    // Визначаємо, з якої папки отримувати повідомлення (наприклад, Sent)
    std::string folder_name = "Sent";
    std::vector<Mail> mails = m_database->RetrieveMessagesFromFolder(folder_name, ReceivedState::FALSE);

    std::string imap_response;

    // Обробка message_set для отримання відповідних індексів
    std::set<int> indices = ImapParser::ParseMessageSet(message_set);  // Функція, що парсить message_set

    for (int index : indices)
    {
        if (index <= 0 || index > mails.size())
        {
            Logger::LogError("Message index out of range: " + std::to_string(index));
            continue;  // Пропустити некоректні індекси
        }

        const Mail& mail = mails[index - 1];  // Індексація з 0

        if (fetch_attribute == "ENVELOPE")
        {
            imap_response += "FROM \"" + mail.sender + "\" ";
            imap_response += "TO \"" + mail.recipient + "\" ";
            imap_response += "SUBJECT \"" + mail.subject + "\" ";
            imap_response += "SENT \"" + mail.sent_at + "\" ";
        }
        else if (fetch_attribute == "RFC822")
        {
            imap_response += "RFC822 \"" + mail.body + "\" ";
        }
        else if (fetch_attribute == "FLAGS")
        {
            imap_response += "FLAGS (\\Seen) ";  // Можна змінювати прапори за потреби
        }
        else if (fetch_attribute == "BODY[]")
        {
            imap_response += "BODY[] \"" + mail.body + "\" ";
        }
        else
        {
            Logger::LogError("Unknown fetch attribute: " + fetch_attribute);
            throw std::invalid_argument("Unknown fetch attribute");
        }
    }

    // Виводимо або відправляємо відповідь
    std::cout << imap_response << std::endl;

    Logger::LogProd("Exiting ClientSession::HandleFetch");
}

}  // namespace ISXCS
