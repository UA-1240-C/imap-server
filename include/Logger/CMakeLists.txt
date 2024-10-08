cmake_minimum_required(VERSION 3.28)

set(LOGFILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/serverlog.txt")

add_compile_definitions(LOGFILE_PATH=\"${LOGFILE_PATH}\")

add_library(Logger STATIC
    src/Logger.cpp
)

target_include_directories(Logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(Logger PUBLIC 
    Boost::log
    Boost::log_setup
    Boost::dynamic_linking
    Boost::thread 
    Threads::Threads
)

add_subdirectory(tests)