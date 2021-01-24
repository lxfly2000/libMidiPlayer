#ifndef OPENWL_UNICODESTUFF_H
#define OPENWL_UNICODESTUFF_H

#include <string>

std::wstring utf8_to_wstring(const std::string &str);
std::string wstring_to_utf8(const std::wstring &str);

#endif //OPENWL_UNICODESTUFF_H
