add_library(Utils INTERFACE)

target_include_directories(Utils INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/BoostLockfree
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Parser/include
)

add_library(Encoding STATIC
        src/Base64.cpp
)

target_include_directories(Encoding PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Parser/include
)

target_link_libraries(Encoding PRIVATE Boost::log Boost::log_setup)