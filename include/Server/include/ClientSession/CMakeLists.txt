add_library(ClientSession  STATIC
    src/ClientSession.cpp
    src/ImapRequest.cpp
)

target_include_directories(SocketWrapper PUBLIC
    ${CMAKE_SOURCE_DIR}/include/Server/include/ClientSession/include
    ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
    ${CMAKE_SOURCE_DIR}/include/Logger/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include

)
