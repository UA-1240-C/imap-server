cmake_minimum_required(VERSION 3.28)
project(IMAP_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED system)

if (NOT Boost_FOUND)
    FetchContent_Declare(
            boost
            GIT_REPOSITORY https://github.com/boostorg/boost.git
            GIT_TAG boost-1.86.0
    )
    FetchContent_MakeAvailable(boost)
endif()

add_executable(IMAP_server src/main.cpp)

target_link_libraries(IMAP_server PRIVATE
    Boost::system
)

target_include_directories(IMAP_server PRIVATE
    ${Boost_INCLUDE_DIRS}
)