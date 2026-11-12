#ifndef TEST_MOCK_PREFERENCES_H
#define TEST_MOCK_PREFERENCES_H

/**
 * @file Preferences.h
 * @brief Mock implementation of ESP32 Preferences for host unit tests.
 * 
 * Simulates NVS key-value storage with in-memory maps per namespace.
 */

#include <string>
#include <map>
#include <vector>
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

public:
    Preferences() : readOnly(false), open(false) {}
    
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
    std::string getString(const char* key, const std::string& defaultValue = "") const {
        if (!open) return defaultValue;
        auto it = namespaces.find(currentNamespace);
        if (it == namespaces.end()) return defaultValue;
        auto strIt = it->second.strings.find(key);
        if (strIt == it->second.strings.end()) return defaultValue;
        return strIt->second;
    }
    
    bool putString(const char* key, const std::string& value) {
        if (!open || readOnly) return false;
        namespaces[currentNamespace].strings[key] = value;
        return true;
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
    
    bool putInt(const char* key, int value) {
        if (!open || readOnly) return false;
        namespaces[currentNamespace].ints[key] = value;
        return true;
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
    
    bool putULong(const char* key, unsigned long value) {
        if (!open || readOnly) return false;
        namespaces[currentNamespace].unsignedLongs[key] = value;
        return true;
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
    
    bool putFloat(const char* key, float value) {
        if (!open || readOnly) return false;
        namespaces[currentNamespace].floats[key] = value;
        return true;
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
    
    bool putBool(const char* key, bool value) {
        if (!open || readOnly) return false;
        namespaces[currentNamespace].bools[key] = value;
        return true;
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
    
    struct KeysView {
        size_t size() const { return 0; } // Simplified for testing
    };
    
    KeysView keys() const {
        return KeysView();
    }
};

#endif // TEST_MOCK_PREFERENCES_H
