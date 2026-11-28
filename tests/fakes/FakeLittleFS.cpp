#include "LittleFS.h"

#include <algorithm>
#include <utility>

FakeLittleFSClass LittleFS;

File::File(FakeLittleFSClass* filesystemValue, std::string pathValue, const bool isDirectory)
    : filesystem(filesystemValue), path(std::move(pathValue)), directory(isDirectory), opened(true) {}

File::operator bool() const {
    return opened;
}

bool File::isDirectory() const {
    return opened && directory;
}

const char* File::name() const {
    return path.c_str();
}

File File::openNextFile() {
    if (!opened || !directory || nextEntry >= entries.size()) {
        return {};
    }
    return filesystem->open(String(entries[nextEntry++]), "r");
}

String File::readString() {
    if (!opened || directory || filesystem == nullptr) {
        return {};
    }
    return filesystem->readPath(path);
}

size_t File::print(const String& contents) {
    if (!opened || directory || filesystem == nullptr) {
        return 0;
    }
    return filesystem->writePath(path, contents);
}

size_t File::size() const {
    if (!opened || directory || filesystem == nullptr) {
        return 0;
    }
    const auto found = filesystem->files.find(path);
    return found == filesystem->files.end() ? 0 : found->second.size();
}

void File::close() {
    opened = false;
}

bool FakeLittleFSClass::begin(const bool formatOnFailure) {
    if (!mountAvailable) {
        mounted = false;
        if (formatOnFailure) {
            files.clear();
            directories.clear();
            ++formatCount;
        }
        return false;
    }
    mounted = true;
    return true;
}

bool FakeLittleFSClass::exists(const String& path) const {
    return mounted && existsPath(path);
}

bool FakeLittleFSClass::mkdir(const String& path) {
    if (!mounted || !directoryCreationAvailable) {
        return false;
    }
    directories.insert(path);
    return true;
}

File FakeLittleFSClass::open(const String& requestedPath, const char* mode) {
    const std::string path(requestedPath.c_str());
    if (!mounted || mode == nullptr) {
        return {};
    }
    if (failedOpenPath == path) {
        failedOpenPath.clear();
        return {};
    }
    if (mode[0] == 'w') {
        files[path] = "";
        return File(this, path, false);
    }
    if (mode[0] != 'r' || !existsPath(path)) {
        return {};
    }
    const bool isDirectory = directories.find(path) != directories.end();
    File file(this, path, isDirectory);
    if (isDirectory) {
        file.entries = childPaths(path);
    }
    return file;
}

bool FakeLittleFSClass::remove(const String& requestedPath) {
    const std::string path(requestedPath.c_str());
    if (!mounted) {
        return false;
    }
    if (failedRemovePath == path) {
        failedRemovePath.clear();
        return false;
    }
    return files.erase(path) > 0;
}

void FakeLittleFSClass::reset() {
    files.clear();
    directories.clear();
    failedOpenPath.clear();
    failedReadPath.clear();
    failedRemovePath.clear();
    mounted = false;
    mountAvailable = true;
    directoryCreationAvailable = true;
    writeOverridePending = false;
    nextWriteByteCount = 0;
    readFailurePending = false;
    formatCount = 0;
}

void FakeLittleFSClass::setMountAvailable(const bool available) {
    mountAvailable = available;
}

void FakeLittleFSClass::setDirectoryCreationAvailable(const bool available) {
    directoryCreationAvailable = available;
}

void FakeLittleFSClass::failOpenFor(const String& path) {
    failedOpenPath = path;
}

void FakeLittleFSClass::failReadFor(const String& path) {
    failedReadPath = path;
    readFailurePending = true;
}

void FakeLittleFSClass::failRemoveFor(const String& path) {
    failedRemovePath = path;
}

void FakeLittleFSClass::failNextWriteAfter(const size_t byteCount) {
    writeOverridePending = true;
    nextWriteByteCount = byteCount;
}

void FakeLittleFSClass::addFileForTest(const String& path, const String& contents) {
    files[path] = contents;
}

String FakeLittleFSClass::getFirstStoredFilePath() const {
    return files.empty() ? String() : String(files.begin()->first);
}

int FakeLittleFSClass::getFormatCount() const {
    return formatCount;
}

size_t FakeLittleFSClass::getStoredFileCount() const {
    return files.size();
}

bool FakeLittleFSClass::existsPath(const std::string& path) const {
    return files.find(path) != files.end() || directories.find(path) != directories.end();
}

std::vector<std::string> FakeLittleFSClass::childPaths(const std::string& path) const {
    std::vector<std::string> children;
    const std::string prefix = path + "/";
    for (const auto& file : files) {
        if (file.first.compare(0, prefix.size(), prefix) == 0 &&
            file.first.find('/', prefix.size()) == std::string::npos) {
            children.push_back(file.first);
        }
    }
    for (const auto& directory : directories) {
        if (directory.compare(0, prefix.size(), prefix) == 0 &&
            directory.find('/', prefix.size()) == std::string::npos) {
            children.push_back(directory);
        }
    }
    std::sort(children.begin(), children.end());
    return children;
}

String FakeLittleFSClass::readPath(const std::string& path) {
    if (readFailurePending && failedReadPath == path) {
        readFailurePending = false;
        failedReadPath.clear();
        return {};
    }
    const auto found = files.find(path);
    return found == files.end() ? String() : String(found->second);
}

size_t FakeLittleFSClass::writePath(const std::string& path, const String& contents) {
    const size_t byteCount = writeOverridePending
        ? std::min(nextWriteByteCount, contents.size())
        : contents.size();
    writeOverridePending = false;
    nextWriteByteCount = 0;
    files[path] = contents.substr(0, byteCount);
    return byteCount;
}