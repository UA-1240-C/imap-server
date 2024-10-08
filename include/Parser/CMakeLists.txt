add_library(Parser STATIC
        src/JSONParser.cpp
        src/ServerConfig.cpp
)

target_link_libraries(Parser PRIVATE Boost::log Boost::log_setup )

target_include_directories(Parser PUBLIC
        ${CMAKE_SOURCE_DIR}/include/Parser/include
)
