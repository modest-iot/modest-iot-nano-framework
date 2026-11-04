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
};

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

#endif // FAKE_ARDUINO_JSON_H
