#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace ISXMailDB
{

struct EmailsInstance
{
    struct LoggedInUser
    {
        uint32_t sender_id;
        std::string sender_name;
    } sender;
    std::vector<std::string> receivers;
    std::string subject;
    std::string body;
    std::vector<std::string> attachments;
};

}