#include "unicodestuff.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <assert.h>

std::wstring utf8_to_wstring(const std::string &str) {
	auto bufferSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	assert(bufferSize != 0);
	
	auto buffer = new wchar_t[bufferSize];
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, bufferSize);
	assert(buffer[bufferSize - 1] == 0); // let's be certain ...

	auto ret = std::wstring(buffer);
	delete[] buffer;

	return ret;
}

std::string wstring_to_utf8(const std::wstring &str) {
	auto bufferSize = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
	assert(bufferSize != 0);

	auto buffer = new char[bufferSize];
	WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buffer, bufferSize, NULL, NULL);
	assert(buffer[bufferSize - 1] == 0); // let's be certain ...

	auto ret = std::string(buffer);
	delete[] buffer;

	return ret;
}
