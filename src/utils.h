#ifndef UTILS_H
#define UTILS_H

#include <Windows.h>
#include <string>

namespace Utils
{
enum FileMode { Open, Save };

bool FileDialog(HWND owner, LPWSTR filePath, DWORD len, LPCWSTR filter, LPCWSTR defExt, int mode);
std::string WStrToUtf8(const std::wstring &str);
}

#endif // UTILS_H
