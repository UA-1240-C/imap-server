#include "ImapRequest.h"

#include <iostream>
#include <map>
#include <regex>

#include "Logger.h"

namespace ISXImapRequest
{
ImapRequest ImapParser::Parse(const std::string& request)
{
    ImapRequest imap_request;
    imap_request.data = request;

    static const std::map<std::string, IMAPCommand> command_map = {
        {"STARTTLS", IMAPCommand::STARTTLS}, {"CAPABILITY", IMAPCommand::CAPABILITY}, {"LOGIN", IMAPCommand::LOGIN},
        {"LOGOUT", IMAPCommand::LOGOUT},     {"SELECT", IMAPCommand::SELECT},         {"FETCH", IMAPCommand::FETCH}};

    std::string command = ExtractCommand(request);
    auto it = command_map.find(command);
    imap_request.command = (it != command_map.end()) ? it->second : IMAPCommand::UNKNOWN;

    return imap_request;
}

std::string ImapParser::ExtractCommand(const std::string& request)
{
    std::regex pattern(R"((^(\*|\w+)\s+([A-Z]+)\s*(.*)\r?\n?))");
    std::smatch match;

    if (std::regex_search(request, match, pattern))
    {
        return match[3];
    }
    else
    {
        throw std::runtime_error("Invalid command");
    }
}

std::pair<std::string, std::string> ImapParser::ExtractUserAndPass(const std::string& request)
{
    std::regex pattern(R"(^([A]\d+)\s+(LOGIN)\s+([^\s]+)\s+([^"]+)$)");

    std::smatch match;

    if (std::regex_search(request, match, pattern))
    {
        return std::pair<std::string, std::string>(match[3], match[4]);
    }
    else
    {
        throw std::runtime_error("Invalid command");
    }
}

}  // namespace ISXImapRequest
