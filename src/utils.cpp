#include "utils.h"
#include <commctrl.h>
#include <codecvt>

bool Utils::FileDialog(HWND owner, LPWSTR filePath, DWORD len, LPCWSTR filter, LPCWSTR defExt, int mode)
{
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = len;
    ofn.lpstrFilter = filter;
    ofn.lpstrDefExt = defExt;
    ofn.Flags = (mode == Open) ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT;
    return (mode == Open) ? GetOpenFileName(&ofn) : GetSaveFileName(&ofn);
}

std::string Utils::WStrToUtf8(const std::wstring &str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    return utf8_conv.to_bytes(str);
}
