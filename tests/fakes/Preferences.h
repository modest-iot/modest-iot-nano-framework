#ifndef TEST_MOCK_PREFERENCES_H
#define TEST_MOCK_PREFERENCES_H

/**
 * @file Preferences.h
 * @brief Mock implementation of ESP32 Preferences for host unit tests.
 * 
 * Simulates NVS key-value storage with in-memory maps per namespace.
 */

#include <time.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <map>
#include <cstring>

class Preferences {
private:
    struct NamespaceData {
        std::map<std::string, std::string> strings;
        std::map<std::string, int> ints;
        std::map<std::string, unsigned long> unsignedLongs;
        std::map<std::string, float> floats;
        std::map<std::string, bool> bools;
    };
    
    std::map<std::string, NamespaceData> namespaces;
    std::string currentNamespace;
    bool readOnly;
    bool open;

    static bool& hasNextWriteResultOverride() {
        static bool value = false;
        return value;
    }

    static size_t& nextWriteResultOverride() {
        static size_t value = 0;
        return value;
    }

    static size_t takeWriteResult(const size_t successfulByteCount) {
        if (!hasNextWriteResultOverride()) {
            return successfulByteCount;
        }
        hasNextWriteResultOverride() = false;
        return nextWriteResultOverride();
    }

public:
    Preferences() : readOnly(false), open(false) {}

    static void setNextWriteResult(const size_t byteCount) {
        nextWriteResultOverride() = byteCount;
        hasNextWriteResultOverride() = true;
    }

    static void failNextWrite() {
        setNextWriteResult(0);
    }
    
    bool begin(const char* namespaceName, bool readOnly = false) {
        if (namespaceName == nullptr || strlen(namespaceName) == 0) {
            return false;
        }
        currentNamespace = namespaceName;
        this->readOnly = readOnly;
        open = true;
        // Ensure namespace exists
        namespaces[currentNamespace];
        return true;
    }
    
    void end() {
        open = false;
        currentNamespace.clear();
        readOnly = false;
    }
    
    // String operations
    String getString(const char* key, const String& defaultValue = "") const {
        if (!open) return defaultValue;
        auto it = namespaces.find(currentNamespace);
        if (it == namespaces.end()) return defaultValue;
        auto strIt = it->second.strings.find(key);
        if (strIt == it->second.strings.end()) return defaultValue;
        return String(strIt->second.c_str());
    }
    
    size_t putString(const char* key, const char* value) {
        if (!open || readOnly || key == nullptr || value == nullptr) return 0;
        const size_t byteCount = std::strlen(value);
        const size_t writeResult = takeWriteResult(byteCount);
        if (writeResult != byteCount) return writeResult;
        namespaces[currentNamespace].strings[key] = value;
        return byteCount;
    }
    
    // Integer operations
    int getInt(const char* key, int defaultValue = 0) const {
        if (!open) return defaultValue;
        auto it = namespaces.find(currentNamespace);
        if (it == namespaces.end()) return defaultValue;
        auto intIt = it->second.ints.find(key);
        if (intIt == it->second.ints.end()) return defaultValue;
        return intIt->second;
    }
    
    size_t putInt(const char* key, int value) {
        if (!open || readOnly || key == nullptr) return 0;
        const size_t byteCount = sizeof(int32_t);
        const size_t writeResult = takeWriteResult(byteCount);
        if (writeResult != byteCount) return writeResult;
        namespaces[currentNamespace].ints[key] = value;
        return byteCount;
    }
    
    // Unsigned long operations
    unsigned long getULong(const char* key, unsigned long defaultValue = 0) const {
        if (!open) return defaultValue;
        auto it = namespaces.find(currentNamespace);
        if (it == namespaces.end()) return defaultValue;
        auto ulIt = it->second.unsignedLongs.find(key);
        if (ulIt == it->second.unsignedLongs.end()) return defaultValue;
        return ulIt->second;
    }
    
    size_t putULong(const char* key, unsigned long value) {
        if (!open || readOnly || key == nullptr) return 0;
        const size_t byteCount = sizeof(uint32_t);
        const size_t writeResult = takeWriteResult(byteCount);
        if (writeResult != byteCount) return writeResult;
        namespaces[currentNamespace].unsignedLongs[key] = value;
        return byteCount;
    }
    
    // Float operations
    float getFloat(const char* key, float defaultValue = 0.0f) const {
        if (!open) return defaultValue;
        auto it = namespaces.find(currentNamespace);
        if (it == namespaces.end()) return defaultValue;
        auto flIt = it->second.floats.find(key);
        if (flIt == it->second.floats.end()) return defaultValue;
        return flIt->second;
    }
    
    size_t putFloat(const char* key, float value) {
        if (!open || readOnly || key == nullptr) return 0;
        const size_t byteCount = sizeof(float);
        const size_t writeResult = takeWriteResult(byteCount);
        if (writeResult != byteCount) return writeResult;
        namespaces[currentNamespace].floats[key] = value;
        return byteCount;
    }
    
    // Boolean operations
    bool getBool(const char* key, bool defaultValue = false) const {
        if (!open) return defaultValue;
        auto it = namespaces.find(currentNamespace);
        if (it == namespaces.end()) return defaultValue;
        auto boolIt = it->second.bools.find(key);
        if (boolIt == it->second.bools.end()) return defaultValue;
        return boolIt->second;
    }
    
    size_t putBool(const char* key, bool value) {
        if (!open || readOnly || key == nullptr) return 0;
        const size_t byteCount = sizeof(uint8_t);
        const size_t writeResult = takeWriteResult(byteCount);
        if (writeResult != byteCount) return writeResult;
        namespaces[currentNamespace].bools[key] = value;
        return byteCount;
    }
    
    // Schema validation
    bool isKey(const char* key) const {
        if (!open) return false;
        auto it = namespaces.find(currentNamespace);
        if (it == namespaces.end()) return false;
        return it->second.strings.count(key) > 0 ||
               it->second.ints.count(key) > 0 ||
               it->second.unsignedLongs.count(key) > 0 ||
               it->second.floats.count(key) > 0 ||
               it->second.bools.count(key) > 0;
    }
    
    bool remove(const char* key) {
        if (!open || readOnly) return false;
        auto it = namespaces.find(currentNamespace);
        if (it == namespaces.end()) return false;
        bool found = it->second.strings.erase(key) > 0 ||
                     it->second.ints.erase(key) > 0 ||
                     it->second.unsignedLongs.erase(key) > 0 ||
                     it->second.floats.erase(key) > 0 ||
                     it->second.bools.erase(key) > 0;
        return found;
    }
    
    void clear() {
        if (!open || readOnly) return;
        namespaces[currentNamespace] = NamespaceData();
    }
};

#endif // TEST_MOCK_PREFERENCES_H
