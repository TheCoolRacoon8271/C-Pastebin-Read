#ifndef read_pastebin_h
#define read_pastebin_h
#include <windows.h>

BOOL read_website(LPCWSTR host, LPCWSTR path, char** content, DWORD* contentSize);

#endif