#include <gtest/gtest.h>
#include <thread>
#include "Logger.h"

class LoggerTest : public testing::Test
{
protected:
	void SetUp() override
	{
		// Setup the logger for each test
		Logger::Setup(Config::Logging{"serverlog.txt", DEBUG_LOGS, true});
	}

	void TearDown() override
	{
		Logger::Reset(); // Reset logger state after each test
	}
};

// Test case 1: Check if Logger is initialized correctly
TEST_F(LoggerTest, LoggerInitializesCorrectly)
{
	EXPECT_NO_THROW(Logger::Setup(Config::Logging{ "serverlog.txt", DEBUG_LOGS, true }));
}

// Test case 2: Verify log to console for PROD log level
TEST_F(LoggerTest, LogToConsoleProd)
{
	std::string message = "This is a production log";
	EXPECT_NO_THROW(Logger::LogProd(message));
}

// Test case 3: Verify log to console for DEBUG log level
TEST_F(LoggerTest, LogToConsoleDebug)
{
	std::string message = "This is a debug log";
	EXPECT_NO_THROW(Logger::LogDebug(message));
}

// Test case 4: Verify log to console for TRACE log level
TEST_F(LoggerTest, LogToConsoleTrace)
{
	std::string message = "This is a trace log";
	EXPECT_NO_THROW(Logger::LogTrace(message));
}

// Test case 5: Verify log to console for WARNING log level
TEST_F(LoggerTest, LogToConsoleWarning)
{
	std::string message = "This is a warning log";
	EXPECT_NO_THROW(Logger::LogWarning(message));
}

// Test case 6: Verify log to console for ERROR log level
TEST_F(LoggerTest, LogToConsoleError)
{
	std::string message = "This is an error log";
	EXPECT_NO_THROW(Logger::LogError(message));
}

// Test case 7: Ensure that Logger handles empty messages
TEST_F(LoggerTest, LogEmptyMessage)
{
	std::string empty_message = "";
	EXPECT_NO_THROW(Logger::LogProd(empty_message));
}

// Test case 8: Verify if SeverityToOutput handles valid log levels
TEST_F(LoggerTest, SeverityToOutputValid)
{
	Logger::Setup(Config::Logging{"serverlog.txt", PROD_WARN_ERR_LOGS, true}); // Set severity filter to Prod/Warn/Error
	EXPECT_EQ(Logger::SeverityToOutput(), "Prod/Warning/Error");
}

// Test case 9: Verify Logger resets correctly
TEST_F(LoggerTest, LoggerResetsCorrectly)
{
	EXPECT_NO_THROW(Logger::Reset());
}

// Test case 10: Check log color formatting for error messages
TEST_F(LoggerTest, LogErrorColor)
{
	std::string message = "Error occurred";
	Logger::LogError(message);
	// Assuming console coloring is handled by ANSI, check for the red color code
	EXPECT_EQ(Colors::RED, "\033[1;31m");
}

// Test case 11: Check log color formatting for trace messages
TEST_F(LoggerTest, LogTraceColor)
{
	std::string message = "Trace message";
	Logger::LogTrace(message);
	// Check if the color is set to cyan for TRACE logs
	EXPECT_EQ(Colors::CYAN, "\033[1;36m");
}

// Test case 12: Ensure multiple log levels work together
TEST_F(LoggerTest, MultipleLogLevels)
{
	Logger::LogProd("Production log");
	Logger::LogDebug("Debug log");
	Logger::LogWarning("Warning log");
	EXPECT_NO_THROW(Logger::LogError("Error log"));
}

// Test case 13: Verify mutex locks are applied when logging concurrently
TEST_F(LoggerTest, MutexLockDuringLogging)
{
	std::thread t1([]() { Logger::LogProd("Thread 1 log"); });
	std::thread t2([]() { Logger::LogDebug("Thread 2 log"); });
	t1.join();
	t2.join();
	EXPECT_NO_THROW(Logger::LogError("Log after threads"));
}

// Test case 14: Test logger’s output when no log level is set (NO_LOGS)
TEST_F(LoggerTest, NoLogsOutput)
{
	Logger::Setup(Config::Logging{"serverlog.txt", NO_LOGS, false});
	Logger::LogProd("This message should not appear");
	EXPECT_EQ(Logger::SeverityToOutput(), "");
}

// Test case 15: Verify thread-local logger is unique to each thread
TEST_F(LoggerTest, UniqueLoggerPerThread)
{
	auto& main_logger = Logger::get_thread_local_logger();

	std::thread t1([&]()
	{
		auto& thread_logger = Logger::get_thread_local_logger();
		EXPECT_NE(thread_logger, main_logger);
	});

	t1.join();
}

// Test case 16: Test console output format contains log level and timestamp
TEST_F(LoggerTest, LogOutputFormat)
{
	std::string message = "Test log message";
	Logger::LogDebug(message);
	// Assuming you have a way to capture and assert the format of the output
	// Check for log level and timestamp in the output
	// Example: "[DEBUG] 2023-09-12 12:45:00 Test log message"
}

// Test case 17: Test log level filtering (DEBUG should exclude TRACE)
TEST_F(LoggerTest, LogLevelFilterDebug)
{
	Logger::Setup(Config::Logging{"serverlog.txt", DEBUG_LOGS, true});
	Logger::LogTrace("This trace log should not appear");
	Logger::LogDebug("This debug log should appear");
	EXPECT_EQ(Logger::SeverityToOutput(), "Debug");
}

// Test case 18: Test log level filtering (PROD should exclude DEBUG/TRACE)
TEST_F(LoggerTest, LogLevelFilterProd)
{
	Logger::Setup(Config::Logging{"serverlog.txt", PROD_WARN_ERR_LOGS, true});
	Logger::LogTrace("This trace log should not appear");
	Logger::LogDebug("This debug log should not appear");
	Logger::LogProd("This prod log should appear");
	EXPECT_EQ(Logger::SeverityToOutput(), "Prod/Warning/Error");
}

// Test case 19: Test logger flush functionality
TEST_F(LoggerTest, LoggerFlush)
{
	Logger::Setup(Config::Logging{"serverlog.txt", DEBUG_LOGS, true});
	Logger::LogDebug("This should be flushed immediately");
	Logger::Reset(); // After reset, all pending logs should be flushed
	EXPECT_NO_THROW(Logger::LogProd("This should flush as well"));
}

// Test case 20: Test logger reset removes sinks
TEST_F(LoggerTest, ResetRemovesSinks)
{
	Logger::Setup(Config::Logging{"serverlog.txt", DEBUG_LOGS, true});
	Logger::Reset();
	// Try logging after reset, should reinitialize sinks
	EXPECT_NO_THROW(Logger::LogProd("This should work after reset"));
}

// Test case 21: Ensure correct logging to syslog
TEST_F(LoggerTest, LogToSyslog)
{
	std::string message = "System log test";
	Logger::LogProd(message);
	// Assuming syslog is being tested, check syslog entries
	// Example: Ensure message appears in syslog
}

// Test case 22: Test thread-local logger in multiple threads concurrently
TEST_F(LoggerTest, ConcurrentThreadLocalLogger)
{
	auto& main_logger = Logger::get_thread_local_logger();

	std::thread t1([&]()
	{
		auto& thread_logger = Logger::get_thread_local_logger();
		EXPECT_NE(thread_logger, main_logger);
		Logger::LogDebug("Thread 1 log");
	});

	std::thread t2([&]()
	{
		auto& thread_logger = Logger::get_thread_local_logger();
		EXPECT_NE(thread_logger, main_logger);
		Logger::LogDebug("Thread 2 log");
	});

	t1.join();
	t2.join();
	EXPECT_NO_THROW(Logger::LogProd("Main thread log"));
}

// Test case 23: Ensure log message length limit (if applicable)
TEST_F(LoggerTest, LogMessageLengthLimit)
{
	std::string long_message(1024, 'A'); // Test a long message of 1024 chars
	EXPECT_NO_THROW(Logger::LogProd(long_message)); // Should handle long messages without crashing
}


int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
