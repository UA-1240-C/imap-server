#ifndef IMAP_REQUEST_H
#define IMAP_REQUEST_H

#include <set>
#include <sstream>
#include <string>
#include <tuple>

namespace ISXImapRequest
{
enum class IMAPCommand
{
    STARTTLS,
    CAPABILITY,
    LOGIN,
    LOGOUT,
    SELECT,
    FETCH,
    UNKNOWN
};

struct ImapRequest
{
    IMAPCommand command;
    std::string data;
};

class ImapParser
{
private:
    ImapParser() = delete;
    ImapParser(const ImapParser&) = delete;
    ImapParser(ImapParser&&) = delete;
    ImapParser& operator=(const ImapParser&) = delete;
    ~ImapParser() = delete;

    std::string m_message_set;
    std::string m_fetch_attribute;

public:
    static ImapRequest Parse(const std::string& request);
    static std::string ExtractCommand(const std::string& request);
    static std::pair<std::string, std::string> ExtractUserAndPass(const std::string& request);

    inline static std::tuple<std::string, std::string, std::string> ParseFetchRequest(const std::string& raw_request)
    {
        std::istringstream iss(raw_request);
        std::string command, message_set, fetch_attribute, request_id;

        iss >> request_id;
        iss >> command;
        iss >> message_set;
        iss >> fetch_attribute;

        if (message_set.empty() || fetch_attribute.empty())
        {
            throw std::runtime_error("Invalid FETCH request format.");
        }

        return {request_id, message_set, fetch_attribute};
    }

    inline static std::set<int> ParseMessageSet(const std::string& message_set)
    {
        std::set<int> indices;
        std::istringstream iss(message_set);
        std::string token;

        while (std::getline(iss, token, ','))
        {
            if (token.find(':') != std::string::npos)
            {
                auto range = token.substr(0, token.find(':'));
                auto end = token.substr(token.find(':') + 1);
                int start_index = std::stoi(range);
                int end_index = std::stoi(end);

                for (int i = start_index; i <= end_index; ++i)
                {
                    indices.insert(i);
                }
            }
            else
            {
                indices.insert(std::stoi(token));
            }
        }

        return indices;
    }

    inline static std::string CommandToString(IMAPCommand command)
    {
        switch (command)
        {
            case IMAPCommand::STARTTLS:
                return "STARTTLS";
            case IMAPCommand::CAPABILITY:
                return "CAPABILITY";
            case IMAPCommand::LOGIN:
                return "LOGIN";
            case IMAPCommand::LOGOUT:
                return "LOGOUT";
            case IMAPCommand::SELECT:
                return "SELECT";
            case IMAPCommand::FETCH:
                return "FETCH";
            case IMAPCommand::UNKNOWN:
                return "UNKNOWN";
            default:
                return "INVALID COMMAND";
        }
    }
};
}  // namespace ISXImapRequest

#endif  // IMAP_REQUEST_H
