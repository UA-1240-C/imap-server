add_library(SocketWrapper
    src/TcpSocketWrapper.cpp
    src/SslSocketWrapper.cpp
)

target_include_directories(SocketWrapper PUBLIC
    ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
    ${CMAKE_SOURCE_DIR}/include/Logger/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
)


if(WIN32 AND MINGW)
    message("Windows and mingw detected")
    find_package(OpenSSL REQUIRED)

    target_link_libraries(SocketWrapper PUBLIC 
        OpenSSL::SSL
        OpenSSL::Crypto
        ws2_32
    )
endif()