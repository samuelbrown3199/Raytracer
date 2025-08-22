#include "UsefulStrings.h"

#include <iostream>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <cstdlib>

std::vector<std::string> SplitString(const std::string& string, const std::string& delimiter)
{
	std::vector<std::string> strings;
	size_t start = 0;
	size_t end = string.find(delimiter);
	while (end != std::string::npos)
	{
		strings.push_back(string.substr(start, end - start));
		start = end + delimiter.length();
		end = string.find(delimiter, start);
	}
	strings.push_back(string.substr(start, end));
	return strings;
}

std::vector<std::vector<unsigned char>> SplitUnsignedCharArray(const std::vector<unsigned char>& array, const unsigned char delimiter)
{
	std::vector<std::vector<unsigned char>> splitArray;
	std::vector<unsigned char> currentArray;
	for (unsigned char c : array)
	{
		if (c == delimiter)
		{
			splitArray.push_back(currentArray);
			currentArray.clear();
		}
		else
		{
			currentArray.push_back(c);
		}
	}
	splitArray.push_back(currentArray);
	return splitArray;
}

std::string TrimString(const std::string& string)
{
	std::string trimmedString = string;
	trimmedString.erase(0, trimmedString.find_first_not_of(" \t\n\r\f\v"));
	trimmedString.erase(trimmedString.find_last_not_of(" \t\n\r\f\v") + 1);
	return trimmedString;
}

std::string RemoveCharactersFromString(const std::string& string, const std::string& characters)
{
	std::string newString = string;
	for (char c : characters)
	{
		newString.erase(std::remove(newString.begin(), newString.end(), c), newString.end());
	}
	return newString;
}

std::string GetDateTimeString()
{
	time_t rawtime;
	struct tm timeinfo;
	char buffer[80];

	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);

	strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", &timeinfo);
	return std::string(buffer);
}

std::string GetDateTimeString(const std::string& format)
{
	time_t rawtime;
	struct tm timeinfo;
	char buffer[80];

	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);

	strftime(buffer, sizeof(buffer), format.c_str(), &timeinfo);
	return std::string(buffer);
}

std::string GenerateRandomString(int length)
{
	std::string randomString;
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < length; ++i)
	{
		randomString += alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	return randomString;
}

std::string ToLower(const std::string& string)
{
	std::string lowerString = string;
	std::transform(lowerString.begin(), lowerString.end(), lowerString.begin(), ::tolower);
	return lowerString;
}

std::string ToUpper(const std::string& string)
{
	std::string upperString = string;
	std::transform(upperString.begin(), upperString.end(), upperString.begin(), ::toupper);
	return upperString;
}

bool FindInString(const std::string& string, const std::string& toFind)
{
	return string.find(toFind) != std::string::npos;
}

std::string ConvertUnsignedCharArrayToString(const unsigned char* array, size_t size)
{
	return std::string(reinterpret_cast<const char*>(array), size);
}

std::vector<unsigned char> ConvertStringToUnsignedCharArray(const std::string& string)
{
	std::vector<unsigned char> ucharArray(string.size());
	// Copy the data byte-by-byte
	for (size_t i = 0; i < string.size(); ++i)
	{
		ucharArray[i] = static_cast<unsigned char>(string[i]);
	}
	return ucharArray;
}

void PrintUnsignedCharArray(const std::vector<unsigned char>& array)
{
	for (unsigned char c : array)
	{
		std::cout << c;
	}
	std::cout << std::endl;
}

void PrintHex(const std::vector<unsigned char>& data)
{
	for (unsigned char c : data) {
		printf("%02x", c);
	}
	printf("\n");
}

std::string ToHexString(const std::vector<unsigned char>& data)
{
	std::string hexString;
	for (unsigned char c : data)
	{
		char buffer[3];
		sprintf_s(buffer, sizeof(buffer), "%02x", c);
		hexString.append(buffer);
	}

	return hexString;
}

std::wstring ConvertStringToWideString(const std::string& str)
{
	std::mbstate_t state = std::mbstate_t();
	const char* src = str.data();
	size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);
	if (len == static_cast<size_t>(-1)) {
		throw std::exception("Error converting string to wide string");
	}
	std::wstring wstr(len, 0);
	std::mbsrtowcs(&wstr[0], &src, len, &state);
	return wstr;
}