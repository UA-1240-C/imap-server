cmake_minimum_required(VERSION 3.28)
project(IMAP_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED system log log_setup)

include(FetchContent)

if (NOT Boost_FOUND)
    FetchContent_Declare(
            boost
            GIT_REPOSITORY https://github.com/boostorg/boost.git
            GIT_TAG boost-1.86.0
    )
    FetchContent_MakeAvailable(boost)
endif()

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
    MailDB
    Boost::system
    Boost::log
    Boost::log_setup
)

target_include_directories(IMAP_server PRIVATE
    ${CMAKE_SOURCE_DIR}/DataBase/include
    ${Boost_INCLUDE_DIRS}
)