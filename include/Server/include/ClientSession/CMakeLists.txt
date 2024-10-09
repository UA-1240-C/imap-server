add_library(ClientSession  STATIC
    src/ClientSession.cpp
    src/ImapRequest.cpp
)

target_link_libraries(ClientSession PRIVATE
    Boost::log
    Boost::log_setup
    ${OPENSSL_LIBRARIES}
)

target_include_directories(ClientSession PUBLIC
    ${CMAKE_SOURCE_DIR}/include/Server/include/ClientSession/include
    ${CMAKE_SOURCE_DIR}/include/Server/include
    ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
    ${CMAKE_SOURCE_DIR}/include/Logger/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
    ${CMAKE_SOURCE_DIR}/include/Utils/include
)
