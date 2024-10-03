add_library(SocketWrapper STATIC
    src/SocketWrapper.cpp
)

target_include_directories(SocketWrapper PUBLIC
    ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
    ${CMAKE_SOURCE_DIR}/include/Logger/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
)