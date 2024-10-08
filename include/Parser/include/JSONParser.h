#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

class JSON {
public:
    enum Type { OBJECT, STRING, NUMBER, BOOL, NIL };

    JSON();
    JSON(Type t);
    JSON(const std::string &value);
    JSON(double value);
    JSON(bool value);

    Type get_type() const;

    const std::unordered_map<std::string, JSON>& get_object_value() const;
    const std::string& get_string_value() const;
    double get_number_value() const;
    bool get_bool_value() const;

    static JSON Parse(const std::string &content);

private:
    static JSON ParseObject(std::stringstream &ss);
    static JSON ParseString(std::stringstream &ss);
    static JSON ParseNumber(std::stringstream &ss);
    static JSON ParseBool(std::stringstream &ss);
    static JSON ParseNull(std::stringstream &ss);
    static void SkipWhitespace(std::stringstream &ss);

    Type m_type;
    std::unordered_map<std::string, JSON> m_object_value;
    std::string m_str_value;
    double m_num_value;
    bool m_bool_value;
};

#endif // PARSER_H
