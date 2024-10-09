cmake_minimum_required(VERSION 3.28)
project(IMAP_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED COMPONENTS system thread log log_setup)
find_package(OpenSSL REQUIRED)


include(FetchContent)

if (NOT Boost_FOUND)
    FetchContent_Declare(
            boost
            GIT_REPOSITORY https://github.com/boostorg/boost.git
            GIT_TAG boost-1.86.0
    )
    FetchContent_MakeAvailable(boost)
endif()

if (NOT OpenSSL_FOUND)
    FetchContent_Declare(
            openssl
            GIT_REPOSITORY https://github.com/openssl/openssl.git
            GIT_TAG OpenSSL_1_1_1k
    )
    FetchContent_MakeAvailable(openssl)
endif()

FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest
	GIT_TAG v1.15.0)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BUILD_DIR})
endif()

add_subdirectory(include/SocketWrapper)
add_subdirectory(include/Logger)
add_subdirectory(include/Parser)
add_subdirectory(include/Utils)
add_subdirectory(include/Server)
add_subdirectory(include/Server/include/ClientSession)

if (NOT TARGET pqxx)
    FetchContent_Declare(
            libpqxx
            GIT_REPOSITORY https://github.com/jtv/libpqxx.git
            GIT_TAG 7.9.2
    )

    # Disable tests for libpqxx
    set(SKIP_BUILD_TEST ON CACHE BOOL "Skip build tests" FORCE)

    FetchContent_MakeAvailable(libpqxx)
endif()

add_subdirectory(DataBase/MailDB)

add_executable(IMAP_server src/main.cpp)

target_link_libraries(IMAP_server PRIVATE
    SocketWrapper
    Parser
    Logger
    Server
    Utils
    ClientSession
    Boost::system
    Boost::log
    Boost::thread
    Boost::log_setup
    ${OPENSSL_LIBRARIES}
    MailDB
)

target_include_directories(IMAP_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
    ${CMAKE_SOURCE_DIR}/include/Utils/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
    ${CMAKE_SOURCE_DIR}/include/Logger/include
    ${CMAKE_SOURCE_DIR}/include/Server/include
    ${CMAKE_SOURCE_DIR}/include/Server/include/ClientSession/include
    ${CMAKE_SOURCE_DIR}/DataBase/include
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)
