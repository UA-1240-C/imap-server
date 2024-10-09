#include <iostream>
#include <vector>

#include "MailDB/PgMailDB.h"
#include "MailDB/PgManager.h"

int main() {

    ISXMailDB::PgManager manager(
        "postgresql://postgres.qotrdwfvknwbfrompcji:"
        "yUf73LWenSqd9Lt4@aws-0-eu-central-1.pooler."
        "supabase.com:6543/postgres?sslmode=require",
            "localhost",
            true);

    ISXMailDB::PgMailDB db(manager);

    db.Login("user@gmail.com2", "password");

    // std::vector<ISXMailDB::Mail> mails = db.RetrieveEmails(true);

    // Custom date: 2024-10-08
    std::tm custom_tm = {};
    int year = 2024;
    int month = 10;

    custom_tm.tm_year = year - 1900;
    custom_tm.tm_mon = month - 1;
    custom_tm.tm_mday = 8;

    std::time_t time = std::mktime(&custom_tm);
    std::chrono::system_clock::time_point custom_time_point = std::chrono::system_clock::from_time_t(time);

    // Call function with custom time
    auto messages = db.RetrieveMessagesWithSenderAndDate("user@gmail.com", custom_time_point);

    for (const auto& id : messages) {
        std::cout << "Message ID: " << id << std::endl;
    }


    return 0;
}
