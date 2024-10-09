#include "IMailDB.h"

#include <iostream>
#include <string>

namespace ISXMailDB
{

std::ostream& operator<<(std::ostream& os, const Mail& mail)
{
    os << "Recipient: " << mail.recipient << "\n"
       << "Sender: " << mail.sender << "\n"
       << "Subject: " << mail.subject << "\n"
       << "Body: " << mail.body << "\n";
    return os;
}

}