#include "ClientSession.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <set>

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
    if (!m_socket_wrapper->IsOpen())
    {
        Logger::LogWarning("Client disconneced.");
        throw std::runtime_error("Client disconnected.");
    }

    std::string buffer = m_socket_wrapper->ReadFromSocketAsync().get();
    m_socket_wrapper->WhoIs();
    Logger::LogProd("Received data: " + buffer);

    if (buffer.find("STARTTLS") != std::string::npos)
    {
        std::cout << "IN IF" << std::endl;
        m_socket_wrapper->SendResponseAsync("* OK SOSI\r\n").get();
        AsyncPerformHandshake().get();
    }

    std::cout << "after IF" << std::endl;

    m_socket_wrapper->RestartTimeoutTimer(m_timeout_duration);

    std::string current_line{};
    current_line.append(buffer);

    std::size_t pos;
    while ((pos = current_line.find(".")) != std::string::npos)  // Шукаємо до '.'
    {
        std::string line = current_line.substr(0, pos);  // Обрізаємо рядок до '.'
        current_line.erase(0, pos + 1);  // Видаляємо оброблений рядок і '.' з current_line

        // Якщо line не порожній, обробляємо запит
        if (!line.empty())
        {
            try
            {
                Logger::LogProd("Line" + line);

                ProcessRequest(ImapParser::Parse(line));
                Logger::LogProd("Processed request successfully");
            }
            catch (const std::exception& e)
            {
                Logger::LogError("Error processing request: " + std::string(e.what()));
            }
        }
    }
}

void ClientSession::ProcessRequest(const ImapRequest& request)
{
    if (ImapParser::CommandToString(request.command) == "FETCH")
    {
        HandleFetch(request);
    }
        //std::cout << "command: " << ImapParser::CommandToString(request.command) << "\ndata: " << request.data
        //          << std::endl;
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
            m_socket_wrapper->SendResponseAsync("* OK 228\r\n");
            promise->set_value();
        });

    Logger::LogDebug("Exiting ClientSession::PerformTlsHandshake");
    return future;
}

void ClientSession::HandleCapability(const ImapRequest& request)
{
    Logger::LogDebug("Entering ClientSession::HandleCapability");
    std::future<void> wrft = m_socket_wrapper->SendResponseAsync("* CAPABILITY IMAP4rev1 STARTTLS\r\n");

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
