cmake_minimum_required(VERSION 3.13.0)

set(PROJECT_NAME
    MailDBLibrary
)
set(LIBRARY_NAME
    MailDB
)
set(LIBRARY_HEADERS_DIR
    include/${LIBRARY_NAME}
)
set(LIBRARY_HEADERS
    ${LIBRARY_HEADERS_DIR}/IMailDB.h
    ${LIBRARY_HEADERS_DIR}/PgMailDB.h
    ${LIBRARY_HEADERS_DIR}/MailException.h
    ${LIBRARY_HEADERS_DIR}/ConnectionPool.h
    ${LIBRARY_HEADERS_DIR}/ConnectionPoolWrapper.h
    ${LIBRARY_HEADERS_DIR}/PgEmailsWriter.h
    ${LIBRARY_HEADERS_DIR}/PgManager.h
    ${LIBRARY_HEADERS_DIR}/EmailsInstance.h
)

set(LIBRARY_SOURCE_DIR
    src
)
set(LIBRARY_SOURCE
    ${LIBRARY_SOURCE_DIR}/PgMailDB.cpp
    ${LIBRARY_SOURCE_DIR}/IMailDB.cpp
    ${LIBRARY_SOURCE_DIR}/PgEmailsWriter.cpp
    ${LIBRARY_SOURCE_DIR}/PgManager.cpp

)

project(${PROJECT_NAME})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

# Check if libpqxx is available
if (NOT TARGET pqxx)
    message(FATAL_ERROR "libpqxx is not available")
endif()


add_library(${LIBRARY_NAME} STATIC
    ${LIBRARY_HEADERS}  
    ${LIBRARY_SOURCE}
)

target_link_libraries(${LIBRARY_NAME} PUBLIC pqxx ${LIBSODIUM_LIBRARIES} Boost::log Boost::log_setup)

target_include_directories(${LIBRARY_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${LIBRARY_NAME}>
    $<INSTALL_INTERFACE:include/${LIBRARY_NAME}>
)

target_include_directories(${LIBRARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)