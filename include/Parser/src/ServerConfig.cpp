#include "ServerConfig.h"

Config::Config(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open filename. Settings are set to default values" << std::endl;
    } else {
        std::stringstream buffer;
        buffer << file.rdbuf();
        JSON json = JSON::Parse(buffer.str());

        ParseJSON(json);
    }
}

Config::Server Config::get_server() const { return server; }
Config::CommunicationSettings Config::get_communication_settings() const { return communication_settings; }
Config::Logging Config::get_logging() const { return logging; }
Config::Time Config::get_time() const { return time; }
Config::ThreadPool Config::get_thread_pool() const { return thread_pool; }

void Config::ParseServerConfig(const JSON &root) {
    if (root.get_object_value().count("Server")) {
        const auto &server_obj = root.get_object_value().at("Server").get_object_value();
        server.server_name = GetValueOrDefault(server_obj, "servername", server.server_name);
        server.server_display_name = GetValueOrDefault(server_obj, "serverdisplayname", server.server_display_name);
        server.listener_port = GetValueOrDefault(server_obj, "listenerport", server.listener_port);
        server.ip_address = GetValueOrDefault(server_obj, "ipaddress", server.ip_address);
    } else {
        NotifyDefault("Server");
    }
}

void Config::ParseCommunicationSettings(const JSON &root) {
    if (root.get_object_value().count("communicationsettings")) {
        const auto &comm_settings_obj = root.get_object_value().at("communicationsettings").get_object_value();
        communication_settings.blocking = GetValueOrDefault(comm_settings_obj, "blocking", communication_settings.blocking);
        communication_settings.socket_timeout = GetValueOrDefault(comm_settings_obj, "socket_timeout", communication_settings.socket_timeout);
    } else {
        NotifyDefault("CommunicationSettings");
    }
}

void Config::ParseLoggingConfig(const JSON &root) {
    if (root.get_object_value().count("logging")) {
        const auto &logging_obj = root.get_object_value().at("logging").get_object_value();
        logging.filename = GetValueOrDefault(logging_obj, "filename", logging.filename);
        logging.log_level = GetValueOrDefault(logging_obj, "LogLevel", logging.log_level);
        logging.flush = GetValueOrDefault(logging_obj, "flush", logging.flush);
    } else {
        NotifyDefault("Logging");
    }
}

void Config::ParseTimeConfig(const JSON &root) {
    if (root.get_object_value().count("time")) {
        const auto &time_obj = root.get_object_value().at("time").get_object_value();
        time.period_time = GetValueOrDefault(time_obj, "Period_time", time.period_time);
    } else {
        NotifyDefault("Time");
    }
}

void Config::ParseThreadPoolConfig(const JSON &root) {
    if (root.get_object_value().count("threadpool")) {
        const auto &thread_pool_obj = root.get_object_value().at("threadpool").get_object_value();
        thread_pool.max_working_threads = GetValueOrDefault(thread_pool_obj, "maxworkingthreads", thread_pool.max_working_threads);
    } else {
        NotifyDefault("ThreadPool");
    }
}

void Config::ParseJSON(const JSON &json) {
    const auto &root = json.get_object_value().at("root");

    ParseServerConfig(root);
    ParseCommunicationSettings(root);
    ParseLoggingConfig(root);
    ParseTimeConfig(root);
    ParseThreadPoolConfig(root);
}

template<typename T>
T Config::GetValueOrDefault(const std::unordered_map<std::string, JSON> &obj, const std::string &key, T default_value) const {
    if (obj.count(key)) {
        const JSON &value_json = obj.at(key);
        if constexpr (std::is_same_v<T, std::string>) {
            return value_json.get_string_value();
        } else if constexpr (std::is_same_v<T, int>) {
            return static_cast<int>(value_json.get_number_value());
        }
    }
    NotifyDefault(key);
    return default_value;
}

void Config::NotifyDefault(const std::string &property) const {
    std::cout << "Warning: " << property << " is set to default value." << std::endl;
}
