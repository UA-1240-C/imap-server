#include "JSONParser.h"

JSON::JSON() : m_type(NIL) {}

JSON::JSON(Type t) : m_type(t) {}

JSON::JSON(const std::string &value) : m_type(STRING), m_str_value(value) {}

JSON::JSON(double value) : m_type(NUMBER), m_num_value(value) {}

JSON::JSON(bool value) : m_type(BOOL), m_bool_value(value) {}

JSON::Type JSON::get_type() const { return m_type; }

const std::unordered_map<std::string, JSON>& JSON::get_object_value() const { return m_object_value; }

const std::string& JSON::get_string_value() const { return m_str_value; }

double JSON::get_number_value() const { return m_num_value; }

bool JSON::get_bool_value() const { return m_bool_value; }

void JSON::SkipWhitespace(std::stringstream &ss) {
    while (ss.peek() == ' ' || ss.peek() == '\n' || ss.peek() == '\t' || ss.peek() == '\r') {
        ss.get();
    }
}


JSON JSON::Parse(const std::string &content) {
    std::stringstream ss(content);
    SkipWhitespace(ss);
    char c = ss.peek();
    if (c == '{') {
        return ParseObject(ss);
    } else if (c == '"') {
        return ParseString(ss);
    } else if (isdigit(c) || c == '-') {
        return ParseNumber(ss);
    } else if (c == 't' || c == 'f') {
        return ParseBool(ss);
    } else if (c == 'n') {
        return ParseNull(ss);
    } else {
        throw std::runtime_error("Invalid JSON input");
    }
}

JSON JSON::ParseObject(std::stringstream &ss) {
    JSON obj(OBJECT);
    bool firstItem = true;
    ss.get(); 
    SkipWhitespace(ss);
    while (ss.peek() != '}') {
        if (firstItem) {
            firstItem = false;
        } else {
            while (ss.peek() != ',' && ss.peek() != -1) {
                ss.get();
            }
            if (ss.peek() == -1) {
                break;
            }
            ss.get(); 
        }
        SkipWhitespace(ss);
        JSON key = ParseString(ss); 
        SkipWhitespace(ss);
        ss.get(); 
        SkipWhitespace(ss);
        obj.m_object_value[key.get_string_value()] = Parse(ss.str().substr(ss.tellg())); 
        SkipWhitespace(ss);
        if (ss.peek() == ',') {
            ss.get();
            SkipWhitespace(ss);
        }
    }
    ss.get(); 
    return obj;
}

JSON JSON::ParseString(std::stringstream &ss) {
    ss.get(); 
    std::string value;
    while (ss.peek() != '"') {
        value += ss.get();
    }
    ss.get(); 
    return JSON(value);
}

JSON JSON::ParseNumber(std::stringstream &ss) {
    std::string value;
    while (isdigit(ss.peek()) || ss.peek() == '.' || ss.peek() == '-') {
        value += ss.get();
    }
    double value_double = std::stod(value);
    return JSON(value_double);
}

JSON JSON::ParseBool(std::stringstream &ss) {
    std::string value;
    while (isalpha(ss.peek())) {
        value += ss.get();
    }
    if (value == "true") {
        return JSON(true);
    } else if (value == "false") {
        return JSON(false);
    } else {
        throw std::runtime_error("Invalid JSON input");
    }
}

JSON JSON::ParseNull(std::stringstream &ss) {
    std::string value;
    while (isalpha(ss.peek())) {
        value += ss.get();
    }
    if (value == "null") {
        return JSON();
    } else {
        throw std::runtime_error("Invalid JSON input");
    }
}
