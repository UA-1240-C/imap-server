#include "ImapRequest.h"

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
        {"LOGOUT", IMAPCommand::LOGOUT},           {"SELECT", IMAPCommand::SELECT},         {"FETCH", IMAPCommand::FETCH}};

    std::string command = ExtractCommand(request);
    auto it = command_map.find(command);
    imap_request.command = (it != command_map.end()) ? it->second : IMAPCommand::UNKNOWN;

    return imap_request;
}

std::string ImapParser::ExtractCommand(const std::string& request)
{
    std::regex regex("^\\S+\\s+(\\S.*?)(?:\\s.*)?$");
    std::smatch match;

    if (std::regex_search(request, match, regex))
    {
        return match[1];
    }

    return "";
}
}  // namespace ISXImapRequest