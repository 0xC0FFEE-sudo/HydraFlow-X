/**
 * @file simple_json.hpp
 * @brief Simple JSON parser for production trading
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <sstream>

namespace hfx::utils {

class JsonValue {
public:
    enum Type { STRING, NUMBER, BOOLEAN, OBJECT, ARRAY, NULL_VALUE };
    
    JsonValue() : type_(NULL_VALUE) {}
    JsonValue(const std::string& str) : type_(STRING), string_value_(str) {}
    JsonValue(double num) : type_(NUMBER), number_value_(num) {}
    JsonValue(bool b) : type_(BOOLEAN), bool_value_(b) {}
    
    Type type() const { return type_; }
    
    std::string asString() const {
        if (type_ == STRING) return string_value_;
        if (type_ == NUMBER) return std::to_string(number_value_);
        if (type_ == BOOLEAN) return bool_value_ ? "true" : "false";
        return "";
    }
    
    double asDouble() const {
        if (type_ == NUMBER) return number_value_;
        if (type_ == STRING) return std::stod(string_value_);
        return 0.0;
    }
    
    bool asBool() const {
        if (type_ == BOOLEAN) return bool_value_;
        if (type_ == STRING) return string_value_ == "true";
        return false;
    }
    
    uint64_t asUInt64() const {
        return static_cast<uint64_t>(asDouble());
    }
    
    bool isArray() const { return type_ == ARRAY; }
    bool isObject() const { return type_ == OBJECT; }
    bool isMember(const std::string& key) const {
        return type_ == OBJECT && object_value_.find(key) != object_value_.end();
    }
    
    JsonValue& operator[](const std::string& key) {
        if (type_ != OBJECT) {
            type_ = OBJECT;
            object_value_.clear();
        }
        return object_value_[key];
    }
    
    const JsonValue& operator[](const std::string& key) const {
        static JsonValue null_value;
        auto it = object_value_.find(key);
        return (it != object_value_.end()) ? it->second : null_value;
    }
    
    JsonValue& operator[](size_t index) {
        if (type_ != ARRAY || index >= array_value_.size()) {
            static JsonValue null_value;
            return null_value;
        }
        return array_value_[index];
    }
    
    JsonValue get(const std::string& key, const JsonValue& default_value) const {
        if (type_ == OBJECT) {
            auto it = object_value_.find(key);
            if (it != object_value_.end()) {
                return it->second;
            }
        }
        return default_value;
    }
    
    size_t size() const {
        if (type_ == ARRAY) return array_value_.size();
        if (type_ == OBJECT) return object_value_.size();
        return 0;
    }
    
    void push_back(const JsonValue& value) {
        if (type_ != ARRAY) {
            type_ = ARRAY;
            array_value_.clear();
        }
        array_value_.push_back(value);
    }

private:
    Type type_;
    std::string string_value_;
    double number_value_ = 0.0;
    bool bool_value_ = false;
    std::unordered_map<std::string, JsonValue> object_value_;
    std::vector<JsonValue> array_value_;
};

class SimpleJson {
public:
    static JsonValue parse(const std::string& json_str) {
        size_t pos = 0;
        return parse_value(json_str, pos);
    }
    
    static std::string stringify(const JsonValue& value) {
        std::stringstream ss;
        stringify_value(value, ss);
        return ss.str();
    }

private:
    static JsonValue parse_value(const std::string& str, size_t& pos) {
        skip_whitespace(str, pos);
        
        if (pos >= str.length()) return JsonValue();
        
        char ch = str[pos];
        if (ch == '"') {
            return parse_string(str, pos);
        } else if (ch == '{') {
            return parse_object(str, pos);
        } else if (ch == '[') {
            return parse_array(str, pos);
        } else if (ch == 't' || ch == 'f') {
            return parse_boolean(str, pos);
        } else if (ch == 'n') {
            return parse_null(str, pos);
        } else if (ch == '-' || (ch >= '0' && ch <= '9')) {
            return parse_number(str, pos);
        }
        
        return JsonValue();
    }
    
    static JsonValue parse_string(const std::string& str, size_t& pos) {
        pos++; // Skip opening quote
        std::string result;
        
        while (pos < str.length() && str[pos] != '"') {
            if (str[pos] == '\\' && pos + 1 < str.length()) {
                pos++; // Skip escape
                char escaped = str[pos];
                if (escaped == 'n') result += '\n';
                else if (escaped == 't') result += '\t';
                else if (escaped == 'r') result += '\r';
                else if (escaped == '\\') result += '\\';
                else if (escaped == '"') result += '"';
                else result += escaped;
            } else {
                result += str[pos];
            }
            pos++;
        }
        
        if (pos < str.length()) pos++; // Skip closing quote
        return JsonValue(result);
    }
    
    static JsonValue parse_number(const std::string& str, size_t& pos) {
        size_t start = pos;
        
        if (str[pos] == '-') pos++;
        
        while (pos < str.length() && 
               ((str[pos] >= '0' && str[pos] <= '9') || str[pos] == '.')) {
            pos++;
        }
        
        std::string num_str = str.substr(start, pos - start);
        return JsonValue(std::stod(num_str));
    }
    
    static JsonValue parse_boolean(const std::string& str, size_t& pos) {
        if (str.substr(pos, 4) == "true") {
            pos += 4;
            return JsonValue(true);
        } else if (str.substr(pos, 5) == "false") {
            pos += 5;
            return JsonValue(false);
        }
        return JsonValue();
    }
    
    static JsonValue parse_null(const std::string& str, size_t& pos) {
        if (str.substr(pos, 4) == "null") {
            pos += 4;
        }
        return JsonValue();
    }
    
    static JsonValue parse_object(const std::string& str, size_t& pos) {
        JsonValue obj;
        pos++; // Skip opening brace
        
        skip_whitespace(str, pos);
        
        if (pos < str.length() && str[pos] == '}') {
            pos++;
            return obj;
        }
        
        while (pos < str.length()) {
            skip_whitespace(str, pos);
            
            if (str[pos] != '"') break;
            JsonValue key = parse_string(str, pos);
            
            skip_whitespace(str, pos);
            if (pos >= str.length() || str[pos] != ':') break;
            pos++; // Skip colon
            
            skip_whitespace(str, pos);
            JsonValue value = parse_value(str, pos);
            
            obj[key.asString()] = value;
            
            skip_whitespace(str, pos);
            if (pos >= str.length() || str[pos] == '}') break;
            
            if (str[pos] == ',') {
                pos++;
                continue;
            }
            break;
        }
        
        if (pos < str.length() && str[pos] == '}') pos++;
        return obj;
    }
    
    static JsonValue parse_array(const std::string& str, size_t& pos) {
        JsonValue arr;
        pos++; // Skip opening bracket
        
        skip_whitespace(str, pos);
        
        if (pos < str.length() && str[pos] == ']') {
            pos++;
            return arr;
        }
        
        while (pos < str.length()) {
            skip_whitespace(str, pos);
            JsonValue value = parse_value(str, pos);
            arr.push_back(value);
            
            skip_whitespace(str, pos);
            if (pos >= str.length() || str[pos] == ']') break;
            
            if (str[pos] == ',') {
                pos++;
                continue;
            }
            break;
        }
        
        if (pos < str.length() && str[pos] == ']') pos++;
        return arr;
    }
    
    static void skip_whitespace(const std::string& str, size_t& pos) {
        while (pos < str.length() && 
               (str[pos] == ' ' || str[pos] == '\t' || 
                str[pos] == '\n' || str[pos] == '\r')) {
            pos++;
        }
    }
    
    static void stringify_value(const JsonValue& value, std::stringstream& ss) {
        // Simple implementation - just enough for basic JSON generation
        switch (value.type()) {
            case JsonValue::STRING:
                ss << '"' << value.asString() << '"';
                break;
            case JsonValue::NUMBER:
                ss << value.asDouble();
                break;
            case JsonValue::BOOLEAN:
                ss << (value.asBool() ? "true" : "false");
                break;
            default:
                ss << "null";
                break;
        }
    }
};

} // namespace hfx::utils
