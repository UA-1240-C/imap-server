#pragma once

#include <string>
#include <exception>

namespace ISXMailDB
{
/**
 * @class MailException
 * @brief Custom exception class for handling mail-related errors.
 * 
 * The `MailException` class is a custom exception used to represent errors that occur during 
 * mail operations within the `ISXMailDB` namespace. It inherits from `std::exception` and provides
 * additional functionality to carry an error message.
 * 
 * @note This class is intended to be used to signal issues specific to mail database operations,
 *       such as failures in database queries, invalid data, or connection problems.
 */
class MailException : public std::exception
{
public:
	/**
     * @brief Constructs a `MailException` with a given error message.
     * 
     * @param message A `std::string` containing the error message to be associated with this exception.
     */
	MailException(const std::string& message) : m_message(message) {}
	/**
     * @brief Constructs a `MailException` with a given error message (rvalue version).
     * 
     * @param message A `std::string` containing the error message to be associated with this exception.
     *                This constructor takes the message by rvalue reference to enable move semantics.
     */
	MailException(std::string&& message) : m_message(std::move(message)) {}

	/**
     * @brief Returns a C-style character string describing the error.
     * 
     * This method overrides the `what` method from `std::exception` to provide a custom error message.
     * 
     * @return A `const char*` pointing to a null-terminated character array containing the error message.
     */
	const char* what() const noexcept override {
        return m_message.c_str();
    }
private:
	std::string m_message;
};
};