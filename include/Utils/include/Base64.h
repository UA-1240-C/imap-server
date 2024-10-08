#pragma once

#ifndef BASE64_H
#define BASE64_H

#include <boost/beast/core/detail/base64.hpp>
#include <string>

#include "Logger.h"

using std::string;

namespace ISXBase64
{
/**
 * @brief Encodes a string into Base64 format.
 * @param decoded The string to encode.
 * @return The Base64 encoded string.
 *
 * This function takes a string and encodes it using boost's Base64 encoding wrapper.
 * The encoded string can be used in contexts where Base64 encoding is required.
 */
string Base64Encode(const string& decoded);

/**
 * @brief Decodes a Base64 encoded string.
 * @param encoded The Base64 encoded string to decode.
 * @return The decoded string.
 *
 * This function takes a Base64 encoded string and decodes it back to its original format.
 * The result is a string that represents the decoded content.
 */
string Base64Decode(const string& encoded);
};  // namespace ISXBase64

#endif  // BASE64_H
