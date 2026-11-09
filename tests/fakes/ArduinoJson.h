#ifndef FAKE_ARDUINO_JSON_H
#define FAKE_ARDUINO_JSON_H

#include "Arduino.h"
#include <map>
#include <sstream>
#include <string>

class JsonVariant {
private:
    std::string val;

public:
    JsonVariant() = default;

    JsonVariant& operator=(int v) {
        val = std::to_string(v);
        return *this;
    }

    JsonVariant& operator=(uint32_t v) {
        val = std::to_string(v);
        return *this;
    }

    JsonVariant& operator=(float v) {
        std::ostringstream ss;
        ss << v;
        val = ss.str();
        return *this;
    }

    JsonVariant& operator=(double v) {
        std::ostringstream ss;
        ss << v;
        val = ss.str();
        return *this;
    }

    JsonVariant& operator=(const char* s) {
        val = s != nullptr ? std::string("\"") + s + "\"" : "null";
        return *this;
    }

    JsonVariant& operator=(const std::string& s) {
        val = "\"" + s + "\"";
        return *this;
    }

    JsonVariant& operator=(bool b) {
        val = b ? "true" : "false";
        return *this;
    }

    [[nodiscard]] const std::string& raw() const {
        return val;
    }

    // W15: Type conversion for deserialization
    template<typename T>
    [[nodiscard]] T as() const {
        if (std::is_integral<T>::value) {
            return static_cast<T>(std::stoll(val));
        }
        if (std::is_floating_point<T>::value) {
            return static_cast<T>(std::stod(val));
        }
        return T{};
    }

    [[nodiscard]] int asInt() const {
        return std::stoi(val);
    }

    // W15: Type check (ArduinoJson 7.x replacement for deprecated containsKey)
    template<typename T>
    [[nodiscard]] bool is() const {
        if (val.empty()) {
            return false;
        }
        if (std::is_integral<T>::value) {
            try {
                (void)std::stoll(val);
                return true;
            } catch (...) {
                return false;
            }
        }
        if (std::is_floating_point<T>::value) {
            try {
                (void)std::stod(val);
                return true;
            } catch (...) {
                return false;
            }
        }
        return !val.empty();
    }
};

class JsonDocument {
private:
    std::map<std::string, JsonVariant> fields;

public:
    JsonVariant& operator[](const char* key) {
        return fields[key != nullptr ? key : ""];
    }

    JsonVariant& operator[](const std::string& key) {
        return fields[key];
    }

    [[nodiscard]] const std::map<std::string, JsonVariant>& getFields() const {
        return fields;
    }

    std::map<std::string, JsonVariant>& getFieldsMutable() {
        return fields;
    }

    // W15: Key existence check for deserialization
    [[nodiscard]] bool containsKey(const char* key) const {
        return fields.find(key != nullptr ? key : "") != fields.end();
    }
};

// W15: Deserialization error enum
enum class DeserializationError {
    Ok = 0,
    InvalidInput,
    NoMemory,
    TooDeep
};

// W15: Simple JSON deserializer for the fake
inline DeserializationError deserializeJson(JsonDocument& doc, const char* input) {
    if (input == nullptr || input[0] == '\0') {
        return DeserializationError::InvalidInput;
    }
    
    // Clear existing fields
    doc.getFieldsMutable().clear();
    
    // Simple parser: find "key":value pairs
    std::string str(input);
    size_t pos = 1; // Skip opening brace
    
    while (pos < str.size()) {
        // Skip whitespace and commas
        while (pos < str.size() && (str[pos] == ' ' || str[pos] == ',' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\t')) {
            pos++;
        }
        if (pos >= str.size() || str[pos] == '}') break;
        
        // Expect opening quote for key
        if (str[pos] != '"') return DeserializationError::InvalidInput;
        pos++; // Skip opening quote
        
        size_t keyStart = pos;
        while (pos < str.size() && str[pos] != '"') pos++;
        if (pos >= str.size()) return DeserializationError::InvalidInput;
        std::string key = str.substr(keyStart, pos - keyStart);
        pos++; // Skip closing quote
        
        // Skip colon and whitespace
        while (pos < str.size() && (str[pos] == ' ' || str[pos] == ':')) pos++;
        
        // Parse value
        if (str[pos] == '"') {
            // String value — store with quotes via string assignment
            pos++; // Skip opening quote
            size_t valStart = pos;
            while (pos < str.size() && str[pos] != '"') pos++;
            std::string stringValue = str.substr(valStart, pos - valStart);
            pos++; // Skip closing quote
            doc[key] = stringValue;
        } else {
            // Number, bool, or null — store raw to preserve numeric type
            size_t valStart = pos;
            while (pos < str.size() && str[pos] != ',' && str[pos] != '}' && str[pos] != ' ') pos++;
            std::string rawValue = str.substr(valStart, pos - valStart);
            
            // Try to parse as number to avoid quote wrapping
            try {
                // Check for float first (contains '.' or 'e')
                if (rawValue.find('.') != std::string::npos || rawValue.find('e') != std::string::npos) {
                    doc[key] = std::stod(rawValue);
                } else {
                    doc[key] = static_cast<int>(std::stoll(rawValue));
                }
            } catch (...) {
                // Not a number — check for bool/null
                if (rawValue == "true") {
                    doc[key] = true;
                } else if (rawValue == "false") {
                    doc[key] = false;
                } else if (rawValue == "null") {
                    // Leave as empty
                } else {
                    // Fallback: unknown value type, store as 0
                    doc[key] = 0;
                }
            }
        }
    }
    
    return DeserializationError::Ok;
}

inline void serializeJson(const JsonDocument& doc, String& output) {
    std::string res = "{";
    bool first = true;
    for (const auto& kv : doc.getFields()) {
        if (!first) {
            res += ",";
        }
        first = false;
        res += "\"" + kv.first + "\":" + kv.second.raw();
    }
    res += "}";
    output = res;
}

inline size_t serializeJson(const JsonDocument& doc, char* buffer, size_t bufferSize) {
    std::string res = "{";
    bool first = true;
    for (const auto& kv : doc.getFields()) {
        if (!first) {
            res += ",";
        }
        first = false;
        res += "\"" + kv.first + "\":" + kv.second.raw();
    }
    res += "}";
    if (res.size() + 1 > bufferSize) {
        return 0; // Buffer too small
    }
    std::snprintf(buffer, bufferSize, "%s", res.c_str());
    return res.size();
}

#endif // FAKE_ARDUINO_JSON_H
