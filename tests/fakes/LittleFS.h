#ifndef TEST_FAKE_LITTLEFS_H
#define TEST_FAKE_LITTLEFS_H

#include <Arduino.h>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

class FakeLittleFSClass;

class File {
private:
    FakeLittleFSClass* filesystem = nullptr;
    std::string path;
    bool directory = false;
    bool opened = false;
    std::vector<std::string> entries;
    size_t nextEntry = 0;

    File(FakeLittleFSClass* filesystemValue, std::string pathValue, bool isDirectory);
    friend class FakeLittleFSClass;

public:
    File() = default;

    explicit operator bool() const;
    bool isDirectory() const;
    const char* name() const;
    File openNextFile();
    String readString();
    size_t print(const String& contents);
    size_t size() const;
    void close();
};

class FakeLittleFSClass {
private:
    std::map<std::string, std::string> files;
    std::set<std::string> directories;
    std::string failedOpenPath;
    std::string failedReadPath;
    std::string failedRemovePath;
    bool mounted = false;
    bool mountAvailable = true;
    bool directoryCreationAvailable = true;
    bool writeOverridePending = false;
    size_t nextWriteByteCount = 0;
    bool readFailurePending = false;
    int formatCount = 0;

    bool existsPath(const std::string& path) const;
    std::vector<std::string> childPaths(const std::string& path) const;
    String readPath(const std::string& path);
    size_t writePath(const std::string& path, const String& contents);
    friend class File;

public:
    bool begin(bool formatOnFailure = true);
    bool exists(const String& path) const;
    bool mkdir(const String& path);
    File open(const String& path, const char* mode);
    bool remove(const String& path);

    void reset();
    void setMountAvailable(bool available);
    void setDirectoryCreationAvailable(bool available);
    void failOpenFor(const String& path);
    void failReadFor(const String& path);
    void failRemoveFor(const String& path);
    void failNextWriteAfter(size_t byteCount);
    void addFileForTest(const String& path, const String& contents);
    String getFirstStoredFilePath() const;
    int getFormatCount() const;
    size_t getStoredFileCount() const;
};

extern FakeLittleFSClass LittleFS;

#endif // TEST_FAKE_LITTLEFS_H