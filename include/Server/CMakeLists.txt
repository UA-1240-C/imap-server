add_library(Server src/Server.cpp)

target_link_libraries(Server PRIVATE
    Boost::log
    Boost::log_setup
)

target_include_directories(Server PUBLIC
        ${CMAKE_SOURCE_DIR}/include/Server/include
        ${CMAKE_SOURCE_DIR}/include/Parser/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
)