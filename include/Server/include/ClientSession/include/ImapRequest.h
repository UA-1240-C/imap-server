#ifndef IMAP_REQUEST_H
#define IMAP_REQUEST_H

#include <string>

namespace ISXImapRequest
{
enum class IMAPCommand
{
    CAPABILITY,
    LOGIN,
    LOGOUT,
    BYE,
    SELECT,
    EXAMINE,
    FETCH,
    STORE,
    COPY,
    CREATE,
    DELETE,
    RENAME,
    SEARCH,
    IDLE,
    APPEND
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
    static ImapParser Parse(const std::string& request);
};
}  // namespace ISXImapRequest

#endif  // IMAP_REQUEST_H