# Logger/tests/CMakeLists.txt

cmake_minimum_required(VERSION 3.28)

# Include the path to Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

set(TEST_PROJECT_NAME
    TestLogger
)
set(LIBRARY_TESTS_SOURCE
    TestLogger.cpp
)
project(${TEST_PROJECT_NAME})

# Enable testing
enable_testing()

if (NOT (TARGET gtest_main))
    message(FATAL_ERROR "gtest_main target NOT found")
endif()

# Add the test executable
add_executable(${TEST_PROJECT_NAME} ${LIBRARY_TESTS_SOURCE})
target_link_libraries(${TEST_PROJECT_NAME} PRIVATE
    Logger
    ${GTEST_LIBRARIES}
)

# Add test cases
add_test(NAME TestLogger COMMAND TestLogger)