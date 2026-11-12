#ifndef FAKE_HTTP_CLIENT_H
#define FAKE_HTTP_CLIENT_H

#include "Client.h"
#include <cstdint>
#include <string>

class HTTPClient {
private:
    std::string url;
    bool isConnected;
    std::string responseBody;
    int responseStatusCode;
    std::string lastMethod;
    std::string lastPayload;

public:
    // Static simulation controls
    static inline int beginCount = 0;
    static inline std::string lastUrl = "";
    static inline int getCount = 0;
    static inline int postCount = 0;
    static inline int putCount = 0;
    static inline int deleteCount = 0;
    static inline int sendRequestCount = 0;
    static inline std::string lastSendRequestMethod = "";
    static inline std::string lastSendRequestPayload = "";
    static inline std::string lastPostPayload = "";
    static inline std::string lastPutPayload = "";
    static inline bool simulateSuccess = true;
    static inline std::string customResponse = "";
    static inline int customStatusCode = 200;

    static void resetMockState() {
        beginCount = 0;
        lastUrl.clear();
        getCount = 0;
        postCount = 0;
        putCount = 0;
        deleteCount = 0;
        sendRequestCount = 0;
        lastSendRequestMethod.clear();
        lastSendRequestPayload.clear();
        lastPostPayload.clear();
        lastPutPayload.clear();
        simulateSuccess = true;
        customResponse.clear();
        customStatusCode = 200;
    }

    static void simulateSuccessResponse(int statusCode = 200, const std::string& body = "") {
        simulateSuccess = true;
        customStatusCode = statusCode;
        customResponse = body;
    }

    static void setResponse(int statusCode, const std::string& body = "") {
        customStatusCode = statusCode;
        customResponse = body;
        simulateSuccess = (statusCode > 0);
    }

    HTTPClient()
        : isConnected(false), responseStatusCode(200) {}

    ~HTTPClient() = default;

    void begin(const std::string& aUrl) {
        beginCount++;
        url = aUrl;
        lastUrl = url;
        isConnected = true;
    }

    void begin(const char* aUrl) {
        begin(std::string(aUrl ? aUrl : ""));
    }

    void end() {
        isConnected = false;
    }

    bool connected() {
        return isConnected;
    }

    void addHeader(const char* name, const char* value) {
        (void)name;
        (void)value;
    }

    void setAuthorization(const char* auth) {
        (void)auth;
    }

    int GET() {
        getCount++;
        lastMethod = "GET";
        responseStatusCode = simulateSuccess ? customStatusCode : 0;
        return responseStatusCode;
    }

    int POST(const std::string& payload) {
        postCount++;
        lastMethod = "POST";
        lastPostPayload = payload;
        responseStatusCode = simulateSuccess ? customStatusCode : 0;
        responseBody = customResponse;
        return responseStatusCode;
    }

    int POST(const char* payload) {
        return POST(std::string(payload ? payload : ""));
    }

    int PUT(const std::string& payload) {
        putCount++;
        lastMethod = "PUT";
        lastPutPayload = payload;
        responseStatusCode = simulateSuccess ? customStatusCode : 0;
        responseBody = customResponse;
        return responseStatusCode;
    }

    int PUT(const char* payload) {
        return PUT(std::string(payload ? payload : ""));
    }

    int sendRequest(const char* method, const std::string& payload) {
        sendRequestCount++;
        lastMethod = method ? method : "";
        lastSendRequestMethod = lastMethod;
        lastSendRequestPayload = payload;
        if (std::string(method) == "DELETE") {
            deleteCount++;
        }
        responseStatusCode = simulateSuccess ? customStatusCode : 0;
        responseBody = customResponse;
        return responseStatusCode;
    }

    int sendRequest(const char* method, const char* payload) {
        return sendRequest(method, std::string(payload ? payload : ""));
    }

    String getString() {
        return String(responseBody.c_str());
    }
};

#endif // FAKE_HTTP_CLIENT_H
