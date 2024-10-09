#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <source_location>

#include <boost/core/null_deleter.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include "ServerConfig.h"


namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;

/**
 * @enum LogLevel
 * @brief Enum class for log levels.
 *
 * This enum consists of log levels that are used directly in the logger.
 */
enum LogLevel
{
	TRACE, // Trace log level: for e.g. Detailed information about the function
	// (input params, and return value (still to be added))
	DEBUG, // Debug log level: for e.g. Start and End functions, if configuration is successfully parsed
	PROD, // Production log level: for e.g. Server start/stop/restart
	WARNING, // Warning log level: for e.g. Invalid input
	ERR // Error log level: for e.g. Server crash
};

/**
 * @enum SeverityFilter
 * @brief Enum class for severity filter.
 *
 * This enum represents four possible severity levels in config.txt,
 * from which all the settings are being parsed.
 */
enum SeverityFilter
{
	NO_LOGS, // No logs, 0 in config.txt
	PROD_WARN_ERR_LOGS, // Production, Warning and Error logs, 1 in config.txt
	DEBUG_LOGS, // Debug logs, 2 in config.txt
	TRACE_LOGS // Trace logs, 3 in config.txt
};

/**
 * @struct Colors
 * @brief Struct for coloring console output.
 *
 * This struct consists of four colors that are used to color the console output,
 * the colors are being set depending on current severity level set. Colors are
 * represented as ANSI escape sequences.
 */
struct Colors
{
	static const std::string RED; // Red for Production/Warning/Error logs
	static const std::string BLUE; // Blue for Debug logs
	static const std::string CYAN; // Cyan for Trace logs
	static const std::string RESET; // Reset for clean overall look.
};

/**
 * @brief Global severity logger instance for multithreaded logging. This logger uses the specified LogLevel for filtering log messages.
 *
 * @note This instance is thread-safe due to `src::severity_logger_mt`.
 */
inline src::severity_logger_mt<LogLevel> g_slg;

/**
 * @brief Logs a message to the system log with the specified log level.
 *
 * @param message The message to be logged to the syslog.
 * @param log_level The severity level of the log, represented by the LogLevel enum.
 * @param location The source location where the log was invoked. Defaults to the current location.
 */
void Syslog(const std::string& message, const LogLevel& log_level, const std::source_location& location);

/**
 * @class Logger
 * @brief Logger class for managing and writing log messages.
 *
 * This class handles all the logging operations, such as setting up the logger,
 * resetting it, working with different log levels, and writing log messages to the console.
 * Class implements Singleton pattern, so only one instance of the logger can be created.
 *
 */
class Logger
{
	// Boost sink pointer for synchronous operations
	static boost::shared_ptr<sinks::asynchronous_sink<sinks::text_ostream_backend>> s_sink_pointer;
	static logging::formatter s_sink_formatter; // Formatter for the sinks
	static uint8_t s_severity_filter; // Severity filter for the sink
	static std::ofstream s_log_file; // Log file path, in development
	static uint8_t s_flush; // Auto flushing for console output
	static std::mutex s_logging_mutex; // Mutex for thread safety
	static thread_local std::unique_ptr<Logger> s_thread_local_logger; // Thread-local logger instance

public:
	Logger() = default; // Default constructor for the Logger class
	Logger(const Logger&) = delete; // Deleted copy constructor
	Logger& operator=(const Logger&) = delete; // Deleted assignment operator

	/**
	* @brief Destructor for the Logger class.
	*
	* Waits for any ongoing logs to end, then cleans the sink up and resets the logger.
	*/
	~Logger();

	/**
	* @brief Get current sink pointer from the Logger class
	* @return Sink pointer from the Logger class; used for testing
	*/
	static boost::shared_ptr<sinks::asynchronous_sink<sinks::text_ostream_backend>> get_sink_pointer()
	{
		return s_sink_pointer;
	}

	/**
	 * @brief Get current severity filter from the Logger class
	 * @return Severity filter from the Logger class; used for testing
	 */
	static uint8_t get_severity_filter() { return s_severity_filter; }

	/**
	* @brief Get current flush setting from the Logger class
	* @return Flush setting from the Logger class; used for testing
	*/
	static uint8_t get_flush() { return s_flush; }

	/**
	* @brief Get current thread-local logger instance from the Logger class
	* @return Thread-local logger instance from the Logger class if it exists; used for testing
	*/
	static std::unique_ptr<Logger>& get_thread_local_logger()
	{
		return s_thread_local_logger;
	}

	/**
	 * @brief Set sink for the logger.
	 *
	 * Function sets up the sink as shared pointer to the backend.
	 * Auto flushing is also being set here (if enabled).
	 */
	static boost::shared_ptr<sinks::asynchronous_sink<sinks::text_ostream_backend>> set_sink();

	/**
	 * @brief Set global attributes for the logger.
	 *
	 * Function sets additional needed attributes to the logger:
	 *  - TimeStamp: current time
	 *	- Scope: current function scope
	 */
	static void set_attributes();

	/**
	 * @brief Set sink filter for the logger.
	 *
	 * Function sets the sink filter based on the severity filter set in the config.txt.
	 * Filter only passes those severity levels that are set in the config.txt and ignores the rest.
	 */
	static void set_sink_filter();

	/**
	 * @brief Set sink formatter for the logger.
	 *
	 * Sets the formatter for the sink as following:
	 * Thread ID - dd/mm/yyyy hh:mm:ss:mmm [LogLevel] - [Function/Method name] Log message.
	 */
	static void set_sink_formatter();

	/**
	 * @brief Setups the logger.
	 * @param logging_config Config::Logging instance that consists log level, log file name, log flush
	 */
	static void Setup(const Config::Logging& logging_config);

	/**
	 * @brief Resets the logger.
	 *
	 * Performs flushing to ensure that all logs are written, then removes all the sinks and resets the logger.
	 */
	static void Reset();

	/**
	* @brief Converts a SeverityFilter enum value to its corresponding string representation.
	* @return A string representing the severity level.
	*/
	static std::string SeverityToOutput();

	/**
	 * @brief Logs a message to the console with the specified log level.
	 * @param message User-defined message to log
	 * @param log_level Log level of the message, one from LogLevel enum
	 * @param location The source location where the log was invoked. Defaults to the current location.
	 */
	void LogToConsole(const std::string& message, const LogLevel& log_level,
					const std::source_location& location = std::source_location::current());

	/**
	 * @brief Logs a message to the file with the specified log level.
	 * @param message The log message to be written to the file.
	 * @param log_level The severity level of the log message.
	 * @param location The source location where the log was invoked. Defaults to the current location.
	 */
	void LogToFile(const std::string& message, const LogLevel& log_level,
					const std::source_location& location = std::source_location::current());

	/**
	 * @brief Logs a message with the specified log level, both to the console and the system log.
	 *
	 * This template function logs a message to the console and the syslog. It automatically associates the message
	 * with the provided log level and captures the location in the source code where the log was invoked.
	 * A thread-local instance of the logger is created if it does not already exist.
	 *
	 * @tparam log_level The severity level of the log, specified as a LogLevel enum.
	 * @param message The message to be logged.
	 * @param location The source location where the log was invoked. Defaults to the current location.
	 */
	template <LogLevel log_level>
	static void Log(const std::string& message, const std::source_location& location = std::source_location::current())
	{
		if (!s_thread_local_logger)
		{
			s_thread_local_logger = std::make_unique<Logger>();
		}
		s_thread_local_logger->LogToConsole(message, log_level, location);
		s_thread_local_logger->LogToFile(message, log_level, location);
		Syslog(message, log_level, location);
	}

	/**
	 * @brief Logs the message with the DEBUG log level.
	 * @param message User-defined message to log
	 * @param location The source location where the log was invoked. Defaults to the current location.
	 */
	static void LogDebug(const std::string& message, const std::source_location& location =
							std::source_location::current());

	/**
	 * @brief Logs the message with the TRACE log level.
	 * @param message User-defined message to log
	 * @param location The source location where the log was invoked. Defaults to the current location.
	 */
	static void LogTrace(const std::string& message, const std::source_location& location =
							std::source_location::current());

	/**
	* @brief Logs the message with the PRODUCTION log level.
	* @param message User-defined message to log
	* @param location The source location where the log was invoked. Defaults to the current location.
	*/
	static void LogProd(const std::string& message, const std::source_location& location =
							std::source_location::current());

	/**
	* @brief Logs the message with the WARNING log level.
	* @param message User-defined message to log
	* @param location The source location where the log was invoked. Defaults to the current location.
	*/
	static void LogWarning(const std::string& message, const std::source_location& location =
								std::source_location::current());

	/**
	* @brief Logs the message with the ERROR log level.
	* @param message User-defined message to log
	* @param location The source location where the log was invoked. Defaults to the current location.
	*/
	static void LogError(const std::string& message, const std::source_location& location =
							std::source_location::current());
};


#if defined (_WIN32) || (_WIN64)
#include <Windows.h>

inline void Syslog(const std::string& message, const LogLevel& log_level, const std::source_location& location)
{
	DWORD event_type = EVENTLOG_INFORMATION_TYPE;

	// Set the event type based on the log level
	switch (log_level)
	{
	case TRACE:
	case DEBUG:
		event_type = EVENTLOG_INFORMATION_TYPE;
		break;
	case PROD:
	case WARNING:
		event_type = EVENTLOG_WARNING_TYPE;
		break;
	case ERR:
		event_type = EVENTLOG_ERROR_TYPE;
		break;
	default:
		event_type = EVENTLOG_INFORMATION_TYPE;
		break;
	}

	const HANDLE event_source = RegisterEventSource(nullptr, TEXT("Logger"));
	if (event_source != nullptr)
	{
		LPCTSTR lpsz_strings[2];
		lpsz_strings[0] = Logger::SeverityToOutput().c_str();
		lpsz_strings[1] = message.c_str();
		ReportEvent(event_source, // Event log handle
					event_type, // Event type
					0, // Event category
					0, // Event identifier
					nullptr, // No security identifier
					2, // Size of lpsz_strings array
					0, // No binary data
					lpsz_strings, // Array of strings
					nullptr); // No binary data
		DeregisterEventSource(event_source);
	}
}
#else
#include <syslog.h>
inline void Syslog(const std::string &message, const LogLevel &log_level, const std::source_location &location)
{
	// Open the syslog
	openlog("Logger", LOG_PID, LOG_USER);
	uint8_t syslog_level = LOG_INFO;

	// Set the syslog level based on the log level
	switch (log_level)
	{
	case LogLevel::TRACE:
		syslog_level = LOG_INFO;
			break;
	case LogLevel::DEBUG:
		syslog_level = LOG_DEBUG;
		break;
	case LogLevel::PROD:
	case LogLevel::WARNING:
		syslog_level = LOG_WARNING;
		break;
	case LogLevel::ERR:
		syslog_level = LOG_ERR;
		break;
	default:
		syslog_level = LOG_INFO;
		break;
	}

	// Log the message to the syslog using following formatting:
	// [LogLevel] - [Function/Method name] Log message
	syslog(syslog_level, "[ %s ] - [ %s ] %s",
		Logger::SeverityToOutput().c_str(),
		location.function_name(),
		message.c_str()
	);
	closelog();
}
#endif
