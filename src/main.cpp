#include <boost/asio.hpp>

#include "Logger.h"
#include "Server.h"

using boost::asio::ip::tcp;
using namespace ISXSS;

int main() {
    try {
        // Server setup
        boost::asio::io_context io_context;
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12_server);

        // Load server certificates and private key
        #ifdef _WIN32
        ssl_context.use_certificate_chain_file("server.crt");    // public key
        ssl_context.use_private_key_file("server.key", boost::asio::ssl::context::pem);
        #else
        ssl_context.use_certificate_chain_file("../server.crt");    // public key
        ssl_context.use_private_key_file("../server.key", boost::asio::ssl::context::pem);
        #endif

        Server server(io_context, ssl_context);
        server.Start();

        io_context.run();
    } catch (const std::exception& e) {
        Logger::LogError("Exception caught in entry point: " + std::string(e.what()));
    }
    return 0;
}
