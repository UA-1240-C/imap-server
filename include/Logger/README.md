# Logger Static Library

This is a C++ logger for managing and writing log messages with various severity levels, including multi-threaded logging support and optional logging to the system log. The logger uses the Boost.Log library and can be integrated into any server application to ensure detailed and organized logging of application events.

## Table of Contents

1. [Features](#features)
2. [Installation](#installation)
3. [Usage](#usage)
   - [Setting Up the Logger](#setting-up-the-logger)
   - [Logging Messages](#logging-messages)
4. [Configuration](#configuration)
5. [Customization](#customization)

## Features

- **Multiple Log Levels:** Supports `TRACE`, `DEBUG`, `PROD`, `WARNING`, and `ERROR` levels.
- **Severity Filtering:** Logs can be filtered based on the severity level set in the configuration file.
- **Console Coloring:** Different log levels are displayed in different colors for easier readability.
- **System Log Integration:** Logs are directed to system logs (Windows Event Log or syslog on Unix-based systems).
- **Thread-Local Logging:** Each server thread has its own logger instance.
- **Singleton Pattern:** Ensures only one logger instance is active.
- **Multi-Threaded:** This logger is designed for use in multi-threaded environments.
- **Multiple Output Destinations:** Supports logging to a console and system log. File logging is in development.

## Installation

### Prerequisites

- C++20 or later
- Boost.Log library
- A build system (e.g., CMake)
- A compiler supporting C++20 (e.g., GCC, Clang, MSVC)

### Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/UA-1240-C/smtp-server/tree/SCRUM-25/Logger.git
   cd Logger
   ``` 
2. Build the library:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

## Usage

### Setting Up the Logger

Before using the logger, it must be set up with the desired configuration. The configuration can be loaded from an external source, such as an XML or JSON file. The following example demonstrates how to set up the logger with a basic configuration:

```cpp
#include "Logger.h"
#include "ServerConfig.h"

int main() {
    Config::Logging logging_config;
    // Assume logging_config is loaded from a configuration file
    Logger::Setup(logging_config);
    
    // Start logging messages
    Logger::LogProd("Server started.");
    return 0;
}
```

### Different Log Levels

The logger provides several levels for logging messages at different severity levels. Each class method also captures the function where the log was invoked.

```cpp
Logger::LogTrace("This is a trace log message.");
Logger::LogDebug("Debugging application flow.");
Logger::LogProd("Production log message.");
Logger::LogWarning("Potential issue detected.");
Logger::LogError("An error has occurred.");
```

Also there is a `Log` method that can be used to log messages at any severity level:

```cpp
Logger::Log<LogLevel::DEBUG>("This is a generic debug log.");
```

## Configuration

The logger's behavior is determined by a configuration file, typically named config.txt or similar. The configuration file defines the severity filter and other settings. The following is an example configuration file:

```xml
	<logging>
		<!--path to log file-->
		<filename>"serverlog.txt"</filename>
		<!--supports only 3 levels of logging: 
		0 - No logs, 
		1 - Production logs/Warning/Erros, 
		2 - Debug logs, 
		3 - Trace logs-->
		<LogLevel>"2"</LogLevel>
		<!--parameter for flushing output to log file (1 - enable / 0 - disable). 
		Warning: this parameter will slow dowm output!-->
		<flush>"0"</flush>
	</logging>
```

## Customization

### Console messages coloring

The logger is customized to display log messages in different colors based on their severity level. Coloring is achieved using ANSI escape codes:

- **TRACE:** Cyan
- **DEBUG:** Blue
- **PROD/WARNING/ERROR:** Red

### System Log Integration

On Windows, log messages are sent to the Event Log. On Unix-based systems, messages are sent to the syslog.
