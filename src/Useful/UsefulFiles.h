#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>

/**
* Gets all file paths in a directory and returns them in a vector.
*/
std::vector<std::string> GetFilesInDirectory(const std::string& directory, bool recurse);

/**
* Gets all file paths in a directory with a specific extension and returns them in a vector. Extension format should be ".wav" for example.
*/
std::vector<std::string> GetFilesInDirectory(const std::string& directory, const std::string& extension, bool recurse);

/**
* Gets all file paths in a directory with a specific extension and returns a map of paths and file modified times.
*/
std::map<std::string, std::filesystem::file_time_type> GetFilesInDirectoryWithModifiedTime(const std::string& directory, const std::string& extension, bool recurse);

/**
* Gets the current appication working directory.
*/
std::string GetWorkingDirectory();

/**
* Checks if a file exists. Returns true if it does, false if it doesn't.
*/
bool FileExists(const std::string& filePath);

/**
* Checks if a directory exists. Returns true if it does, false if it doesn't.
*/
bool DirectoryExists(const std::string& directoryPath);

/**
* Creates a directory at the specified path.
*/
void CreateNewDirectory(const std::string& directoryPath);

/**
* Deletes a directory at the specified path.
*/
void DeleteDirectory(const std::string& directoryPath);

/**
* Deletes a file at the specified path.
*/
void DeleteFilePath(const std::string& filePath);

/**
* Copies a file from one location to another.
*/
void CopyFileIntoDestination(const std::string& source, const std::string& destination);

/**
* Cuts a file from one location to another.
*/
void CutFile(const std::string& source, const std::string& destination);

/**
* Gets the size of a file in bytes.
*/
std::uintmax_t GetFileSize(const std::string& filePath);

/**
* Gets the last modified time of a file.
*/
std::filesystem::file_time_type GetFileModifiedTime(const std::string& filePath);

/**
* Copies a directory from one location to another.
*/
void CopyDirectory(const std::string& source, const std::string& destination);

/**
* Opens a file and reads all lines into a vector of strings.
*/
std::vector<std::string> OpenFileAndReadLines(const std::string& filePath);

/**
* Opens a file and reads the entire file into a string.
*/
std::string OpenFileAndReadString(const std::string& filePath);

/**
* Returns the file extension of a file.
*/
std::string GetFileExtension(const std::string& filePath);

/**
* Returns the path of a file relative to working directory.
*/
std::string GetPathRelativeToWorkingDirectory(const std::string& path);

/**
* Returns the path of a file relative to a directory.
*/
std::string GetPathRelativeToDirectory(const std::string& path, const std::string& directory);

/**
* Writes a vector of strings to a file.
*/
void WriteStringVectorToFile(const std::string& filePath, const std::vector<std::string>& data);

/**
* Gets the directory from a file path.
*/
std::string GetDirectoryFromPath(const std::string& path);