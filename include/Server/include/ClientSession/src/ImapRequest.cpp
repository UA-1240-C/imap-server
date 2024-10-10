#include "ImapRequest.h"
#include <iostream>
#include <regex>

namespace ISXImapRequest
{
ImapRequest ImapParser::Parse(const std::string& request) 
{
    ImapRequest imap_request;
    imap_request.data = request;

    std::cout << "Before ExtractCommand" << std::endl;
    std::string command = ExtractCommand(request);
    std::cout << "After ExtractCommand" << std::endl;

    if (command == "STARTTLS")
    {
        imap_request.command = IMAPCommand::STARTTLS;
    }
    else if (command == "LOGIN")
    {
        imap_request.command = IMAPCommand::LOGIN;
    }
    else if (command == "CAPABILITY")
    {
        imap_request.command = IMAPCommand::CAPABILITY;
    }
    else
    {
        throw std::runtime_error("Invalid command");
    }

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
