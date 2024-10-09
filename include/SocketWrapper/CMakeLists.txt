add_library(SocketWrapper
    src/TcpSocketWrapper.cpp
    src/SslSocketWrapper.cpp
)

target_include_directories(SocketWrapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
    ${CMAKE_SOURCE_DIR}/include/Logger/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
)

target_link_libraries(SocketWrapper PRIVATE
    Logger
    Boost::log 
    Boost::log_setup
    OpenSSL::SSL
    OpenSSL::Crypto
)