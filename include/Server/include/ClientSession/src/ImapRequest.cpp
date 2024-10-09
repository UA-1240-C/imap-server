#include "ImapRequest.h"

#include <regex>

namespace ISXImapRequest
{
ImapRequest ImapParser::Parse(const std::string& request) 
{
    ImapRequest imap_request;
    imap_request.data = request;

    std::regex regex("^\\S+\\s+(\\S.*?)(?:\\s.*)?$");

    if (ExtractCommand(request) == "STARTTLS")
    {
        imap_request.command = IMAPCommand::STARTTLS;
    }
    else if (request.find("SELECT") == 0)
    {
        imap_request.command = IMAPCommand::SELECT;
    }
    else if (request.find("FETCH") == 0)
    {
        imap_request.command = IMAPCommand::FETCH;
    }
    else if (request.find("BYE") == 0)
    {
        imap_request.command = IMAPCommand::BYE;
    }
    else if (request.find("CAPABILITY") == 0)
    {
        imap_request.command = IMAPCommand::CAPABILITY;
    }
    else if (request.find("LOGIN") == 0)
    {
        imap_request.command = IMAPCommand::LOGIN;
    }

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

std::pair<std::string, std::string> ImapParser::ExtractUserAndPass(const std::string& request)
{
    std::regex regex("^[A-Za-z0-9]+ LOGIN (\\S+) \"(.*)");
    std::smatch match;

    if (std::regex_search(request, match, regex))
    {
        return std::pair<std::string, std::string>(match[1], match[2]);
    }

    return std::pair<std::string, std::string>("", "");
}

}  // namespace ISXImapRequest
