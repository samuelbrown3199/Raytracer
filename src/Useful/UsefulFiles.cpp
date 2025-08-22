#include "UsefulFiles.h"

#include <sstream>
#include <string>
#include <locale>
#include <codecvt>

#include "UsefulStrings.h"

std::vector<std::string> GetFilesInDirectory(const std::string& directory, bool recurse)
{
	return GetFilesInDirectory(directory, "", recurse);
}

std::vector<std::string> GetFilesInDirectory(const std::string& directory, const std::string& extension, bool recurse)
{
	if (!DirectoryExists(directory))
		throw std::exception("Directory does not exist.");

	std::vector<std::string> files;
	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		if (entry.is_directory() && recurse)
		{
			std::vector<std::string> subFiles = GetFilesInDirectory(entry.path().string(), extension, recurse);
			files.insert(files.end(), subFiles.begin(), subFiles.end());
		}
		else if (entry.is_regular_file())
		{
			std::string fileExtension = entry.path().extension().string();
			if (extension != "" && fileExtension == extension)
			{
				files.push_back(entry.path().string());
				continue;
			}
			if(extension == "")
				files.push_back(entry.path().string());
		}
	}
	return files;
}

std::map<std::string, std::filesystem::file_time_type> GetFilesInDirectoryWithModifiedTime(const std::string& directory, const std::string& extension, bool recurse)
{
	if (!DirectoryExists(directory))
		throw std::exception("Directory does not exist.");

	std::vector<std::string> files = GetFilesInDirectory(directory, extension, recurse);
	std::map<std::string, std::filesystem::file_time_type> returnFiles;
		
	for(const auto& file : files)
	{
		std::filesystem::file_time_type modifiedTime = GetFileModifiedTime(file);
		returnFiles[file] = modifiedTime;
	}

	return returnFiles;
}

std::string GetWorkingDirectory()
{
	return std::filesystem::current_path().string();
}

bool FileExists(const std::string& filePath)
{
	return std::filesystem::exists(filePath);
}

bool DirectoryExists(const std::string& directoryPath)
{
	return std::filesystem::is_directory(directoryPath);
}

void CreateNewDirectory(const std::string& directoryPath)
{
	std::filesystem::create_directory(directoryPath);
}

void DeleteDirectory(const std::string& directoryPath)
{
	std::filesystem::remove_all(directoryPath);
}

void DeleteFilePath(const std::string& filePath)
{
	std::filesystem::remove(filePath);
}

void CopyFileIntoDestination(const std::string& source, const std::string& destination)
{
	if(!FileExists(source))
		throw std::exception("Source file does not exist.");

	if (FileExists(destination))
		throw std::exception("Destination file already exists.");

	std::filesystem::copy_file(source, destination);
}

void CutFile(const std::string& source, const std::string& destination)
{
	if (!FileExists(source))
		throw std::exception("Source file does not exist.");

	if (FileExists(destination))
		throw std::exception("Destination file already exists.");

	std::filesystem::rename(source, destination);
}

std::uintmax_t GetFileSize(const std::string& filePath)
{
	if(!FileExists(filePath))
		throw std::exception("File does not exist.");

	return std::filesystem::file_size(filePath);
}

std::filesystem::file_time_type GetFileModifiedTime(const std::string& filePath)
{
	if (!FileExists(filePath))
		throw std::exception("File does not exist.");

	return std::filesystem::last_write_time(filePath);
}

void CopyDirectory(const std::string& source, const std::string& destination)
{
	if (!DirectoryExists(source))
		throw std::exception("Source directory does not exist.");

	if (DirectoryExists(destination))
		throw std::exception("Destination directory already exists.");

	std::filesystem::copy(source, destination);
}

std::vector<std::string> OpenFileAndReadLines(const std::string& filePath)
{
	std::ifstream file;
	file.open(filePath);

	if (!file.is_open())
	{
		throw std::exception(FormatString("Trying to open file that does not exist. %s", filePath.c_str()).c_str());
	}

	std::vector<std::string> lines;

	std::string line;
	while (std::getline(file, line))
	{
		lines.push_back(line);
	}
	file.close();

	return lines;
}

std::string OpenFileAndReadString(const std::string& filePath)
{
	std::wifstream file(filePath, std::ios::binary); // Open in binary mode
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file " + filePath);
	}

	// Set the locale to UTF-8
	file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));

	std::wstringstream wss;
	wss << file.rdbuf(); // Read the file content into a wide string stream
	std::wstring ws = wss.str();

	// Convert wide string to narrow string
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(ws);
}

std::string GetFileExtension(const std::string& filePath)
{
	std::vector<std::string> splitPath = SplitString(filePath, ".");
	std::string extension = splitPath.back();
	return "." + extension;
}

std::string GetPathRelativeToWorkingDirectory(const std::string& path) 
{
	std::string workingDirectory = GetWorkingDirectory();
	return GetPathRelativeToDirectory(path, workingDirectory);
}

std::string GetPathRelativeToDirectory(const std::string& path, const std::string& directory)
{
	std::string relativePath = path.substr(directory.size());
	return relativePath;
}

void WriteStringVectorToFile(const std::string& filePath, const std::vector<std::string>& data)
{
	std::ofstream file;
	file.open(filePath);

	if (!file.is_open())
		throw std::exception(FormatString("Failed open file. %s", filePath.c_str()).c_str());

	for (const auto& line : data)
	{
		file << line << std::endl;
	}
	file.close();
}

std::string GetDirectoryFromPath(const std::string& path)
{
	std::string directory = path.substr(0, path.find_last_of("\\"));
	return directory;
}