#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cwchar>

/**
* Splits a string by a delimiter and returns a vector of strings.
*/
std::vector<std::string> SplitString(const std::string& string, const std::string& delimiter);

std::vector<std::vector<unsigned char>> SplitUnsignedCharArray(const std::vector<unsigned char>& array, const unsigned char delimiter);

/**
* Formats a string using a format string and arguments.
*/
template<typename ... Args>
std::string FormatString(const std::string& _format, Args ... _args)
{
	int size_s = std::snprintf(nullptr, 0, _format.c_str(), _args ...) + 1; // Extra space for '\0'
	if (size_s <= 0) { throw std::exception("Error during formatting."); }
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, _format.c_str(), _args ...);
	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

/**
* Formats a string using a format string and arguments.
*/
template<typename ... Args>
std::wstring FormatString(const std::wstring& _format, Args ... _args)
{
	int size_s = std::swprintf(nullptr, 0, _format.c_str(), _args ...) + 1; // Extra space for '\0'
	if (size_s <= 0) { throw std::exception("Error during formatting."); }
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	std::swprintf(buf.get(), size, _format.c_str(), _args ...);
	return std::wstring(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

/**
* Trims a string of whitespace.
*/
std::string TrimString(const std::string& string);

/**
* Removes characters from a string.
*/
std::string RemoveCharactersFromString(const std::string& string, const std::string& characters);

/**
* Gets the current date and time as a string.
*/
std::string GetDateTimeString();

/**
* Gets the current date and time as a string with a given format.
*/
std::string GetDateTimeString(const std::string& format);

/**
* Generates a random string of a given length.
*/
std::string GenerateRandomString(int length);

/**
* Converts a string to Lowercase.
*/
std::string ToLower(const std::string& string);

/**
* Converts a string to Uppercase.
*/
std::string ToUpper(const std::string& string);

/**
* Finds a string in a string.
*/
bool FindInString(const std::string& string, const std::string& toFind);

/**
* Converts an unsigned char array to a string.
*/
std::string ConvertUnsignedCharArrayToString(const unsigned char* array, size_t size);

/**
* Converts a string to an unsigned char array.
*/
std::vector<unsigned char> ConvertStringToUnsignedCharArray(const std::string& string);

/**
* Prints an unsigned char array.
*/
void PrintUnsignedCharArray(const std::vector<unsigned char>& array);

/**
* Prints an unsigned char vector as a hex string.
*/
void PrintHex(const std::vector<unsigned char>& data);

std::string ToHexString(const std::vector<unsigned char>& data);

/**
* Converts a string to a wide string.
*/
std::wstring ConvertStringToWideString(const std::string& str);