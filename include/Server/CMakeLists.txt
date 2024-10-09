add_library(Server src/Server.cpp)

target_link_libraries(Server PRIVATE
    Boost::log
    Boost::log_setup
    ClientSession
    SocketWrapper
    Parser
    MailDB
    ${OPENSSL_LIBRARIES}
)

target_include_directories(Server PUBLIC
        ${CMAKE_SOURCE_DIR}/include/Server/include
        ${CMAKE_SOURCE_DIR}/include/Server/include/ClientSession/include
        ${CMAKE_SOURCE_DIR}/include/Parser/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/Encoding/include
        ${CMAKE_SOURCE_DIR}/Database/MailDB/include
)
