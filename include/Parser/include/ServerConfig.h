#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <iostream>
#include <string>
#include <fstream>
#include "JSONParser.h"

class Config {
public:
    struct Server {
        std::string server_name = "DefaultServer";
        std::string server_display_name = "DefaultServerDisplayName";
        int listener_port = 25000;
        std::string ip_address = "127.0.0.1";
    };

    struct CommunicationSettings {
        int blocking = 0;
        int socket_timeout = 5;
    };

    struct Logging {
        std::string filename = "serverlog.txt";
        int log_level = 2;
        int flush = 0;
    };

    struct Time {
        int period_time = 30;
    };

    struct ThreadPool {
        int max_working_threads = 10;
    };

    Config(const std::string &filename);

    Server get_server() const;
    CommunicationSettings get_communication_settings() const;
    Logging get_logging() const;
    Time get_time() const;
    ThreadPool get_thread_pool() const;

private:
    Server server;
    CommunicationSettings communication_settings;
    Logging logging;
    Time time;
    ThreadPool thread_pool;

    void ParseServerConfig(const JSON &root);
    void ParseCommunicationSettings(const JSON &root);
    void ParseLoggingConfig(const JSON &root);
    void ParseTimeConfig(const JSON &root);
    void ParseThreadPoolConfig(const JSON &root);
    void ParseJSON(const JSON &json);

    template<typename T>
    T GetValueOrDefault(const std::unordered_map<std::string, JSON> &obj, const std::string &key, T default_value) const;

    void NotifyDefault(const std::string &property) const;
};

#endif
