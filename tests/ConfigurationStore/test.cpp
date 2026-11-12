/**
 * @file test.cpp
 * @brief Host unit tests for ConfigurationStore persistent configuration.
 * 
 * Tests namespace isolation, type-safe getters/setters, default fallbacks,
 * read-only mode, schema validation, and error handling.
 */

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include "ConfigurationStore.h"

int testsPassed = 0;
int testsFailed = 0;

template<typename T, typename U>
void assertEqual(const T& a, const U& b, const char* expression, const char* file, int line) {
    if (a == b) {
        testsPassed++;
    } else {
        testsFailed++;
        std::cerr << "FAIL: " << expression << " at " << file << ":" << line << std::endl;
    }
}

template<typename T, typename U>
void assertNotEqual(const T& a, const U& b, const char* expression, const char* file, int line) {
    if (a != b) {
        testsPassed++;
    } else {
        testsFailed++;
        std::cerr << "FAIL: " << expression << " at " << file << ":" << line << std::endl;
    }
}

#define ASSERT_EQ(a, b) assertEqual(a, b, #a " == " #b, __FILE__, __LINE__)
#define ASSERT_NE(a, b) assertNotEqual(a, b, #a " != " #b, __FILE__, __LINE__)
#define ASSERT_TRUE(x) assertEqual((x), true, #x " == true", __FILE__, __LINE__)
#define ASSERT_FALSE(x) assertNotEqual((x), true, #x " != true", __FILE__, __LINE__)

void testConstructorInitialization() {
    ConfigurationStore store;
    ASSERT_FALSE(store.isInitialized());
    ASSERT_FALSE(store.isReadOnly());
    ASSERT_EQ(std::string(store.getNamespace()), "");
}

void testBeginSuccess() {
    ConfigurationStore store;
    bool result = store.begin("test_ns");
    ASSERT_TRUE(result);
    ASSERT_TRUE(store.isInitialized());
    ASSERT_FALSE(store.isReadOnly());
    ASSERT_EQ(std::string(store.getNamespace()), "test_ns");
    store.end();
}

void testBeginReadOnly() {
    ConfigurationStore store;
    bool result = store.begin("test_ns", true);
    ASSERT_TRUE(result);
    ASSERT_TRUE(store.isReadOnly());
    store.end();
}

void testBeginNullNamespace() {
    ConfigurationStore store;
    bool result = store.begin(nullptr);
    ASSERT_FALSE(result);
    ASSERT_FALSE(store.isInitialized());
}

void testBeginEmptyNamespace() {
    ConfigurationStore store;
    bool result = store.begin("");
    ASSERT_FALSE(result);
}

void testBeginNamespaceTooLong() {
    ConfigurationStore store;
    // Namespace longer than 15 characters should fail
    bool result = store.begin("this_is_way_too_long");
    ASSERT_FALSE(result);
}

void testEndClosesNamespace() {
    ConfigurationStore store;
    store.begin("test_ns");
    ASSERT_TRUE(store.isInitialized());
    store.end();
    ASSERT_FALSE(store.isInitialized());
    ASSERT_EQ(std::string(store.getNamespace()), "");
}

void testEndOnUninitialized() {
    ConfigurationStore store;
    // Should not crash
    store.end();
    ASSERT_FALSE(store.isInitialized());
}

void testStringOperations() {
    ConfigurationStore store;
    store.begin("string_test");
    
    // Set string
    bool setResult = store.setString("key1", "value1");
    ASSERT_TRUE(setResult);
    
    // Get string
    const char* value = store.getString("key1");
    ASSERT_EQ(std::string(value), "value1");
    
    // Get with default fallback
    const char* missing = store.getString("missing_key", "default");
    ASSERT_EQ(std::string(missing), "default");
    
    store.end();
}

void testStringGetDefaultEmpty() {
    ConfigurationStore store;
    store.begin("string_test");
    
    // Default fallback is empty string
    const char* value = store.getString("nonexistent");
    ASSERT_EQ(std::string(value), "");
    
    store.end();
}

void testStringSetReadOnlyFails() {
    ConfigurationStore store;
    store.begin("ro_test", true);
    
    bool setResult = store.setString("key", "value");
    ASSERT_FALSE(setResult);
    
    store.end();
}

void testStringSetNotInitializedFails() {
    ConfigurationStore store;
    // Don't call begin()
    bool setResult = store.setString("key", "value");
    ASSERT_FALSE(setResult);
}

void testStringGetNotInitializedReturnsDefault() {
    ConfigurationStore store;
    // Don't call begin()
    const char* value = store.getString("key", "fallback");
    ASSERT_EQ(std::string(value), "fallback");
}

void testIntegerOperations() {
    ConfigurationStore store;
    store.begin("int_test");
    
    // Set int
    bool setResult = store.setInt("count", 42);
    ASSERT_TRUE(setResult);
    
    // Get int
    int value = store.getInt("count");
    ASSERT_EQ(value, 42);
    
    // Get with default fallback
    int missing = store.getInt("missing", -1);
    ASSERT_EQ(missing, -1);
    
    store.end();
}

void testIntDefaultZero() {
    ConfigurationStore store;
    store.begin("int_test");
    
    int value = store.getInt("nonexistent");
    ASSERT_EQ(value, 0);
    
    store.end();
}

void testUnsignedLongOperations() {
    ConfigurationStore store;
    store.begin("ulong_test");
    
    bool setResult = store.setUnsignedLong("interval", 30000);
    ASSERT_TRUE(setResult);
    
    unsigned long value = store.getUnsignedLong("interval");
    ASSERT_EQ(value, 30000UL);
    
    unsigned long missing = store.getUnsignedLong("missing", 1000UL);
    ASSERT_EQ(missing, 1000UL);
    
    store.end();
}

void testFloatOperations() {
    ConfigurationStore store;
    store.begin("float_test");
    
    bool setResult = store.setFloat("threshold", 25.5f);
    ASSERT_TRUE(setResult);
    
    float value = store.getFloat("threshold");
    ASSERT_EQ(value, 25.5f);
    
    float missing = store.getFloat("missing", 0.0f);
    ASSERT_EQ(missing, 0.0f);
    
    store.end();
}

void testBooleanOperations() {
    ConfigurationStore store;
    store.begin("bool_test");
    
    bool setResult = store.setBool("enabled", true);
    ASSERT_TRUE(setResult);
    
    bool value = store.getBool("enabled");
    ASSERT_TRUE(value);
    
    bool missing = store.getBool("missing", false);
    ASSERT_FALSE(missing);
    
    store.end();
}

void testNamespaceIsolation() {
    ConfigurationStore store;
    
    // Write to namespace A
    store.begin("namespace_a");
    store.setString("key", "value_a");
    store.end();
    
    // Read from namespace B
    store.begin("namespace_b");
    const char* value = store.getString("key", "not_found");
    ASSERT_EQ(std::string(value), "not_found");
    store.end();
    
    // Re-read namespace A
    store.begin("namespace_a");
    const char* valueA = store.getString("key");
    ASSERT_EQ(std::string(valueA), "value_a");
    store.end();
}

void testContainsKey() {
    ConfigurationStore store;
    store.begin("contains_test");
    
    store.setString("existing", "value");
    
    ASSERT_TRUE(store.containsKey("existing"));
    ASSERT_FALSE(store.containsKey("nonexistent"));
    
    store.end();
}

void testContainsKeyNotInitialized() {
    ConfigurationStore store;
    ASSERT_FALSE(store.containsKey("key"));
}

void testRemoveKey() {
    ConfigurationStore store;
    store.begin("remove_test");
    
    store.setString("to_remove", "value");
    ASSERT_TRUE(store.containsKey("to_remove"));
    
    bool removed = store.removeKey("to_remove");
    ASSERT_TRUE(removed);
    ASSERT_FALSE(store.containsKey("to_remove"));
    
    store.end();
}

void testRemoveKeyReadOnlyFails() {
    ConfigurationStore store;
    store.begin("ro_test", true);
    
    bool removed = store.removeKey("key");
    ASSERT_FALSE(removed);
    
    store.end();
}

void testClearNamespace() {
    ConfigurationStore store;
    store.begin("clear_test");
    
    store.setString("key1", "value1");
    store.setInt("key2", 42);
    ASSERT_TRUE(store.containsKey("key1"));
    ASSERT_TRUE(store.containsKey("key2"));
    
    bool cleared = store.clearNamespace();
    ASSERT_TRUE(cleared);
    ASSERT_FALSE(store.containsKey("key1"));
    ASSERT_FALSE(store.containsKey("key2"));
    
    store.end();
}

void testClearNamespaceReadOnlyFails() {
    ConfigurationStore store;
    store.begin("ro_test", true);
    
    bool cleared = store.clearNamespace();
    ASSERT_FALSE(cleared);
    
    store.end();
}

void testBeginClosesPreviousNamespace() {
    ConfigurationStore store;
    
    store.begin("first");
    store.setString("key", "first_value");
    
    // Opening a new namespace should close the previous one
    store.begin("second");
    store.setString("key", "second_value");
    
    // Verify second namespace has its own value
    const char* value = store.getString("key");
    ASSERT_EQ(std::string(value), "second_value");
    
    store.end();
}

void testNullValueStringFails() {
    ConfigurationStore store;
    store.begin("null_test");
    
    bool setResult = store.setString("key", nullptr);
    ASSERT_FALSE(setResult);
    
    store.end();
}

void testInvalidKeyFails() {
    ConfigurationStore store;
    store.begin("invalid_key_test");
    
    // Null key should fail
    bool setResult = store.setString(nullptr, "value");
    ASSERT_FALSE(setResult);
    
    // Empty key should fail
    setResult = store.setString("", "value");
    ASSERT_FALSE(setResult);
    
    store.end();
}

void testWifiCredentialsScenario() {
    ConfigurationStore store;
    
    // Simulate first boot — no credentials stored
    store.begin("wifi");
    ASSERT_EQ(std::string(store.getString("ssid", "DefaultNetwork")), "DefaultNetwork");
    ASSERT_EQ(std::string(store.getString("password", "")), "");
    
    // Store credentials
    store.setString("ssid", "MyHomeNetwork");
    store.setString("password", "secret123");
    
    // Simulate reboot — re-read credentials
    store.end();
    store.begin("wifi");
    ASSERT_EQ(std::string(store.getString("ssid", "DefaultNetwork")), "MyHomeNetwork");
    ASSERT_EQ(std::string(store.getString("password", "")), "secret123");
    
    store.end();
}

void testMqttEndpointScenario() {
    ConfigurationStore store;
    
    store.begin("mqtt");
    store.setString("broker", "test.mosquitto.org");
    store.setInt("port", 8883);
    store.setString("topic", "device/telemetry");
    store.setBool("use_tls", true);
    
    // Verify all values
    ASSERT_EQ(std::string(store.getString("broker")), "test.mosquitto.org");
    ASSERT_EQ(store.getInt("port"), 8883);
    ASSERT_EQ(std::string(store.getString("topic")), "device/telemetry");
    ASSERT_TRUE(store.getBool("use_tls"));
    
    store.end();
}

void testSetValueString() {
    ConfigurationStore store;
    store.begin("setValue_str");
    
    bool setResult = store.setValue("name", "TestDevice");
    ASSERT_TRUE(setResult);
    ASSERT_EQ(std::string(store.getString("name")), "TestDevice");
    
    store.end();
}

void testSetValueInt() {
    ConfigurationStore store;
    store.begin("setValue_int");
    
    bool setResult = store.setValue("port", 1883);
    ASSERT_TRUE(setResult);
    ASSERT_EQ(store.getInt("port"), 1883);
    
    store.end();
}

void testSetValueUnsignedLong() {
    ConfigurationStore store;
    store.begin("setValue_ul");
    
    bool setResult = store.setValue("interval", 60000UL);
    ASSERT_TRUE(setResult);
    ASSERT_EQ(store.getUnsignedLong("interval"), 60000UL);
    
    store.end();
}

void testSetValueFloat() {
    ConfigurationStore store;
    store.begin("setValue_fl");
    
    bool setResult = store.setValue("threshold", 42.5f);
    ASSERT_TRUE(setResult);
    ASSERT_EQ(store.getFloat("threshold"), 42.5f);
    
    store.end();
}

void testSetValueBool() {
    ConfigurationStore store;
    store.begin("setValue_bl");
    
    bool setResult = store.setValue("enabled", true);
    ASSERT_TRUE(setResult);
    ASSERT_TRUE(store.getBool("enabled"));
    
    store.end();
}

void testSetValueReadOnlyFails() {
    ConfigurationStore store;
    store.begin("setValue_ro", true);
    
    ASSERT_FALSE(store.setValue("key", "value"));
    ASSERT_FALSE(store.setValue("key", 42));
    ASSERT_FALSE(store.setValue("key", 100UL));
    ASSERT_FALSE(store.setValue("key", 3.14f));
    ASSERT_FALSE(store.setValue("key", true));
    
    store.end();
}

void testSetValueNotInitializedFails() {
    ConfigurationStore store;
    // Don't call begin()
    ASSERT_FALSE(store.setValue("key", "value"));
    ASSERT_FALSE(store.setValue("key", 42));
    ASSERT_FALSE(store.setValue("key", 100UL));
    ASSERT_FALSE(store.setValue("key", 3.14f));
    ASSERT_FALSE(store.setValue("key", true));
}

int main() {
    std::cout << "Running ConfigurationStore tests..." << std::endl;
    
    testConstructorInitialization();
    testBeginSuccess();
    testBeginReadOnly();
    testBeginNullNamespace();
    testBeginEmptyNamespace();
    testBeginNamespaceTooLong();
    testEndClosesNamespace();
    testEndOnUninitialized();
    testStringOperations();
    testStringGetDefaultEmpty();
    testStringSetReadOnlyFails();
    testStringSetNotInitializedFails();
    testStringGetNotInitializedReturnsDefault();
    testIntegerOperations();
    testIntDefaultZero();
    testUnsignedLongOperations();
    testFloatOperations();
    testBooleanOperations();
    testNamespaceIsolation();
    testContainsKey();
    testContainsKeyNotInitialized();
    testRemoveKey();
    testRemoveKeyReadOnlyFails();
    testClearNamespace();
    testClearNamespaceReadOnlyFails();
    testBeginClosesPreviousNamespace();
    testNullValueStringFails();
    testInvalidKeyFails();
    testWifiCredentialsScenario();
    testMqttEndpointScenario();
    testSetValueString();
    testSetValueInt();
    testSetValueUnsignedLong();
    testSetValueFloat();
    testSetValueBool();
    testSetValueReadOnlyFails();
    testSetValueNotInitializedFails();
    
    std::cout << testsPassed << " tests passed, " << testsFailed << " tests failed." << std::endl;
    
    return testsFailed > 0 ? 1 : 0;
}
