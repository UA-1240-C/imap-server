#ifndef IMAP_REQUEST_H
#define IMAP_REQUEST_H

#include <string>

namespace ISXImapRequest
{
enum class IMAPCommand
{
    STARTTLS,
    CAPABILITY,
    LOGIN,
    BYE,
    SELECT,
    FETCH,
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

public:
    static ImapRequest Parse(const std::string& request);
};
}  // namespace ISXImapRequest

#endif  // IMAP_REQUEST_H
