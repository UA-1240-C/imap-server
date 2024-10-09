#include "ImapRequest.h"
namespace ISXImapRequest
{
ImapParser ImapParser::Parse(const std::string& request) 
{
    ImapRequest imap_request;
zquest.data = request;

    if (request.find("CAPABILITY") == 0)
    {
        imap_request.command = IMAPCommand::CAPABILITY;
    }
    else if (request.find("LOGIN") == 0)
    {
        imap_request.command = IMAPCommand::LOGIN;
    }
}
}  // namespace ISXImapRequest
