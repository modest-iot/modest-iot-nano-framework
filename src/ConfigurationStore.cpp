/**
 * @file ConfigurationStore.cpp
 * @brief Implements persistent key-value configuration storage using ESP32 NVS.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-12
 * @version 1.0
 */

#include "ConfigurationStore.h"

ConfigurationStore::ConfigurationStore()
    : currentNamespace(),
      initialized(false),
      readOnly(false)
{
}

ConfigurationStore::~ConfigurationStore() {
    end();
}

bool ConfigurationStore::isValidKey(const char* key) {
    if (key == nullptr) {
        return false;
    }
    size_t length = strlen(key);
    return length > 0 && length <= MAX_CONFIGURATION_KEY_LENGTH;
}

bool ConfigurationStore::canRead(const char* key) const {
    return initialized && isValidKey(key);
}

bool ConfigurationStore::canWrite(const char* key) const {
    return initialized && !readOnly && isValidKey(key);
}

bool ConfigurationStore::canModify() const {
    return initialized && !readOnly;
}

bool ConfigurationStore::begin(const char* namespaceName, const bool readOnlyMode) {
    if (namespaceName == nullptr) {
        return false;
    }
    
    size_t namespaceLength = strlen(namespaceName);
    if (namespaceLength == 0 || namespaceLength > MAX_CONFIGURATION_KEY_LENGTH) {
        return false;
    }
    
    // Close any previously open namespace
    if (initialized) {
        preferences.end();
    }
    
    bool success = preferences.begin(namespaceName, readOnlyMode);
    if (success) {
        strncpy(currentNamespace, namespaceName, MAX_CONFIGURATION_KEY_LENGTH);
        currentNamespace[MAX_CONFIGURATION_KEY_LENGTH] = '\0';
        initialized = true;
        this->readOnly = readOnlyMode;
    } else {
        initialized = false;
        currentNamespace[0] = '\0';
    }
    
    return success;
}

void ConfigurationStore::end() {
    if (initialized) {
        preferences.end();
        initialized = false;
        currentNamespace[0] = '\0';
        readOnly = false;
    }
}

bool ConfigurationStore::isInitialized() const {
    return initialized;
}

const char* ConfigurationStore::getNamespace() const {
    return currentNamespace;
}

bool ConfigurationStore::isReadOnly() const {
    return readOnly;
}

const char* ConfigurationStore::getString(const char* key, const char* defaultValue) const {
    if (!canRead(key)) {
        return defaultValue;
    }
    lastStringValue = preferences.getString(key, defaultValue);
    return lastStringValue.c_str();
}

bool ConfigurationStore::setString(const char* key, const char* value) const {
    if (!canWrite(key) || value == nullptr) {
        return false;
    }
    if (strlen(value) > MAX_CONFIGURATION_STRING_VALUE_LENGTH) {
        return false;
    }
    preferences.putString(key, value);
    return true;
}

bool ConfigurationStore::setValue(const char* key, const char* value) const {
    return setString(key, value);
}

int ConfigurationStore::getInt(const char* key, int defaultValue) const {
    if (!canRead(key)) {
        return defaultValue;
    }
    return preferences.getInt(key, defaultValue);
}

bool ConfigurationStore::setInt(const char* key, int value) const {
    if (!canWrite(key)) {
        return false;
    }
    preferences.putInt(key, value);
    return true;
}

bool ConfigurationStore::setValue(const char* key, const int value) const {
    return setInt(key, value);
}

unsigned long ConfigurationStore::getUnsignedLong(const char* key, unsigned long defaultValue) const {
    if (!canRead(key)) {
        return defaultValue;
    }
    return preferences.getULong(key, defaultValue);
}

bool ConfigurationStore::setUnsignedLong(const char* key, const unsigned long value) const {
    if (!canWrite(key)) {
        return false;
    }
    preferences.putULong(key, value);
    return true;
}

bool ConfigurationStore::setValue(const char* key, const unsigned long value) const {
    return setUnsignedLong(key, value);
}

float ConfigurationStore::getFloat(const char* key, const float defaultValue) const {
    if (!canRead(key)) {
        return defaultValue;
    }
    return preferences.getFloat(key, defaultValue);
}

bool ConfigurationStore::setFloat(const char* key, const float value) const {
    if (!canWrite(key)) {
        return false;
    }
    preferences.putFloat(key, value);
    return true;
}

bool ConfigurationStore::setValue(const char* key, const float value) const {
    return setFloat(key, value);
}

bool ConfigurationStore::getBool(const char* key, const bool defaultValue) const {
    if (!canRead(key)) {
        return defaultValue;
    }
    return preferences.getBool(key, defaultValue);
}

bool ConfigurationStore::setBool(const char* key, bool value) const {
    if (!canWrite(key)) {
        return false;
    }
    preferences.putBool(key, value);
    return true;
}

bool ConfigurationStore::setValue(const char* key, const bool value) const {
    return setBool(key, value);
}

bool ConfigurationStore::containsKey(const char* key) const {
    if (!canRead(key)) {
        return false;
    }
    return preferences.isKey(key);
}

bool ConfigurationStore::removeKey(const char* key) const {
    if (!canWrite(key)) {
        return false;
    }
    return preferences.remove(key);
}

bool ConfigurationStore::clearNamespace() const {
    if (!canModify()) {
        return false;
    }
    preferences.clear();
    return true;
}

