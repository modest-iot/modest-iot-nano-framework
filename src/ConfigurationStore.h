#ifndef CONFIGURATION_STORE_H
#define CONFIGURATION_STORE_H

/**
 * @file ConfigurationStore.h
 * @brief Declares a persistent key-value configuration store using ESP32 NVS (Preferences).
 * @ingroup core_module
 * @date 2026-11-12
 * @version 1.0
 */

#include <Arduino.h>
#include <Preferences.h>

/**
 * @brief Maximum length for a configuration key name.
 * 
 * ESP32 NVS key names are limited to 15 bytes. This constant enforces
 * the limit at compile time to prevent silent truncation.
 */
static const size_t MAX_CONFIGURATION_KEY_LENGTH = 15;

/**
 * @brief Maximum length for a string configuration value.
 * 
 * Limits string values to prevent excessive NVS partition usage.
 */
static const size_t MAX_CONFIGURATION_STRING_VALUE_LENGTH = 256;

/**
 * @brief Persistent configuration store backed by ESP32 NVS (Non-Volatile Storage).
 * @ingroup core_module
 * 
 * Provides type-safe access to persistent key-value pairs with namespace-based
 * isolation. Each namespace (e.g., "wifi", "mqtt", "device") is stored in a
 * separate NVS partition, preventing key name collisions across subsystems.
 * 
 * All read operations support default fallback values, allowing graceful
 * degradation when configuration keys are missing (first boot scenario).
 * 
 * @par Thread Safety
 * The Preferences API is thread-safe for read operations. Write operations
 * should be performed on a single task context to avoid corruption.
 * 
 * @par Example
 * @code
 * ConfigurationStore store;
 * store.begin("wifi", true);  // Read-only namespace
 * 
 * const char* ssid = store.getString("ssid", "DefaultNetwork");
 * const char* password = store.getString("password", "");
 * 
 * // Write mode
 * store.begin("wifi", false);
 * store.setString("ssid", "MyNetwork");
 * store.setString("password", "secret123");
 * store.end();
 * @endcode
 */
class ConfigurationStore {
private:
    Preferences preferences; ///< Underlying NVS Preferences instance.
    char currentNamespace[MAX_CONFIGURATION_KEY_LENGTH + 1]; ///< Currently active namespace.
    bool initialized; ///< Tracks whether begin() was called successfully.
    bool readOnly; ///< Tracks whether the store was opened in read-only mode.
    mutable std::string lastStringValue; ///< Buffer for getString return value to avoid dangling pointers.

    /**
     * @brief Validates the key length against ESP32 NVS limits.
     * @param key The key to validate.
     * @return `true` if the key length is within bounds, `false` otherwise.
     */
    [[nodiscard]] bool isValidKey(const char* key) const;

    /**
     * @brief Checks preconditions for any read operation.
     * @param key The configuration key being read.
     * @return `true` if the store is initialized and the key is valid, `false` otherwise.
     */
    [[nodiscard]] bool canRead(const char* key) const;

    /**
     * @brief Checks preconditions for any write operation.
     * @param key The configuration key being written.
     * @return `true` if the store is initialized, not read-only, and the key is valid, `false` otherwise.
     */
    [[nodiscard]] bool canWrite(const char* key) const;

    /**
     * @brief Checks preconditions for namespace-level modifications (no key required).
     * @return `true` if the store is initialized and not read-only, `false` otherwise.
     */
    [[nodiscard]] bool canModify() const;

public:
    /**
     * @brief Constructs an uninitialized ConfigurationStore.
     * 
     * Call begin() before any read or write operations.
     */
    ConfigurationStore();

    /**
     * @brief Destructor closes the active namespace if open.
     */
    ~ConfigurationStore();

    /**
     * @brief Opens a configuration namespace for read/write access.
     * @param namespaceName Logical group name (e.g., "wifi", "mqtt", "device").
     * @param readOnly If `true`, opens the namespace in read-only mode.
     * @return `true` if the namespace was opened successfully, `false` otherwise.
     * 
     * Must be called before any read or write operations. The namespace name
     * is limited to 15 characters per ESP32 NVS constraints.
     * 
     * @note Calling begin() on an already-open store will close the current
     * namespace first, then open the new one.
     */
    bool begin(const char* namespaceName, bool readOnly = false);

    /**
     * @brief Closes the currently open namespace and releases NVS resources.
     * 
     * Should be called after all write operations to ensure data is flushed
     * to non-volatile memory. Safe to call on an uninitialized store.
     */
    void end();

    /**
     * @brief Returns whether the store is currently initialized and ready.
     * @return `true` if begin() was called successfully, `false` otherwise.
     */
    [[nodiscard]] bool isInitialized() const;

    /**
     * @brief Returns the currently active namespace name.
     * @return The namespace name, or an empty string if not initialized.
     */
    [[nodiscard]] const char* getNamespace() const;

    /**
     * @brief Returns whether the store is in read-only mode.
     * @return `true` if the store was opened with readOnly = true.
     */
    [[nodiscard]] bool isReadOnly() const;

    /** @name String Operations
     *  @{ */
    /**
     * @brief Reads a string value from the configuration store.
     * @param key The configuration key.
     * @param defaultValue Fallback value if the key doesn't exist.
     * @return The stored string value, or `defaultValue` if not found.
     * 
     * The returned pointer is valid until the next begin()/end() call or
     * until the store is destroyed. For persistent storage, copy the value.
     */
    [[nodiscard]] const char* getString(const char* key, const char* defaultValue = "") const;

    /**
     * @brief Writes a string value to the configuration store.
     * @param key The configuration key.
     * @param value The string value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     * 
     * @note Returns `false` if the store is in read-only mode, not initialized,
     * or the key/value exceeds NVS limits.
     */
    bool setString(const char* key, const char* value);

    /**
     * @brief Writes a string value using the unified setValue() API.
     * @param key The configuration key.
     * @param value The string value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     * @see setString
     */
    bool setValue(const char* key, const char* value);
    /** @} */

    /** @name Integer Operations
     *  @{ */
    /**
     * @brief Reads an integer value from the configuration store.
     * @param key The configuration key.
     * @param defaultValue Fallback value if the key doesn't exist.
     * @return The stored integer value, or `defaultValue` if not found.
     */
    [[nodiscard]] int getInt(const char* key, int defaultValue = 0) const;

    /**
     * @brief Writes an integer value to the configuration store.
     * @param key The configuration key.
     * @param value The integer value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     */
    bool setInt(const char* key, int value);

    /**
     * @brief Writes an integer value using the unified setValue() API.
     * @param key The configuration key.
     * @param value The integer value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     * @see setInt
     */
    bool setValue(const char* key, int value);

    /**
     * @brief Reads an unsigned long value from the configuration store.
     * @param key The configuration key.
     * @param defaultValue Fallback value if the key doesn't exist.
     * @return The stored unsigned long value, or `defaultValue` if not found.
     */
    [[nodiscard]] unsigned long getUnsignedLong(const char* key, unsigned long defaultValue = 0) const;

    /**
     * @brief Writes an unsigned long value to the configuration store.
     * @param key The configuration key.
     * @param value The unsigned long value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     */
    bool setUnsignedLong(const char* key, unsigned long value);

    /**
     * @brief Writes an unsigned long value using the unified setValue() API.
     * @param key The configuration key.
     * @param value The unsigned long value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     * @see setUnsignedLong
     */
    bool setValue(const char* key, unsigned long value);
    /** @} */

    /** @name Float Operations
     *  @{ */
    /**
     * @brief Reads a float value from the configuration store.
     * @param key The configuration key.
     * @param defaultValue Fallback value if the key doesn't exist.
     * @return The stored float value, or `defaultValue` if not found.
     */
    [[nodiscard]] float getFloat(const char* key, float defaultValue = 0.0f) const;

    /**
     * @brief Writes a float value to the configuration store.
     * @param key The configuration key.
     * @param value The float value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     */
    bool setFloat(const char* key, float value);

    /**
     * @brief Writes a float value using the unified setValue() API.
     * @param key The configuration key.
     * @param value The float value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     * @see setFloat
     */
    bool setValue(const char* key, float value);
    /** @} */

    /** @name Boolean Operations
     *  @{ */
    /**
     * @brief Reads a boolean value from the configuration store.
     * @param key The configuration key.
     * @param defaultValue Fallback value if the key doesn't exist.
     * @return The stored boolean value, or `defaultValue` if not found.
     */
    [[nodiscard]] bool getBool(const char* key, bool defaultValue = false) const;

    /**
     * @brief Writes a boolean value to the configuration store.
     * @param key The configuration key.
     * @param value The boolean value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     */
    bool setBool(const char* key, bool value);

    /**
     * @brief Writes a boolean value using the unified setValue() API.
     * @param key The configuration key.
     * @param value The boolean value to store.
     * @return `true` if the value was written successfully, `false` otherwise.
     * @see setBool
     */
    bool setValue(const char* key, bool value);
    /** @} */

    /** @name Schema Validation
     *  @{ */
    /**
     * @brief Checks if a key exists in the current namespace.
     * @param key The configuration key to check.
     * @return `true` if the key exists, `false` otherwise.
     * 
     * Useful for schema validation: verify expected keys are present
     * before reading their values.
     */
    [[nodiscard]] bool containsKey(const char* key) const;

    /**
     * @brief Removes a key from the configuration store.
     * @param key The configuration key to remove.
     * @return `true` if the key was removed, `false` if not found or read-only.
     */
    bool removeKey(const char* key);

    /**
     * @brief Removes all keys in the current namespace.
     * @return `true` if the namespace was cleared, `false` if read-only.
     * 
     * @warning This operation is irreversible. Use with caution.
     */
    bool clearNamespace();

    /**
     * @brief Returns the number of keys in the current namespace.
     * @return The count of keys, or 0 if not initialized.
     */
    [[nodiscard]] size_t keyCount() const;
    /** @} */
};

#endif // CONFIGURATION_STORE_H
