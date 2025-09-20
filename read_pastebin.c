#include  <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "read_pastebin.h"

typedef HINTERNET (WINAPI *WinHttpOpen_t)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
typedef HINTERNET (WINAPI *WinHttpConnect_t)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
typedef HINTERNET (WINAPI *WinHttpOpenRequest_t)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
typedef BOOL (WINAPI *WinHttpSendRequest_t)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
typedef BOOL (WINAPI *WinHttpReceiveResponse_t)(HINTERNET, LPVOID);
typedef BOOL (WINAPI *WinHttpReadData_t)(HINTERNET, LPVOID, DWORD, LPDWORD);
typedef BOOL (WINAPI *WinHttpCloseHandle_t)(HINTERNET);

WinHttpOpen_t pWinHttpOpen = nullptr;
WinHttpConnect_t pWinHttpConnect = nullptr;
WinHttpOpenRequest_t pWinHttpOpenRequest = nullptr;
WinHttpSendRequest_t pWinHttpSendRequest = nullptr;
WinHttpReceiveResponse_t pWinHttpReceiveResponse = nullptr;
WinHttpReadData_t pWinHttpReadData = nullptr;
WinHttpCloseHandle_t pWinHttpCloseHandle = nullptr;

BOOL LoadWinHTTPFunctions() {
    HMODULE hWinHTTP = LoadLibraryA("winhttp.dll");
    if (!hWinHTTP) return FALSE;

    pWinHttpOpen = (WinHttpOpen_t)GetProcAddress(hWinHTTP, "WinHttpOpen");
    pWinHttpConnect = (WinHttpConnect_t)GetProcAddress(hWinHTTP, "WinHttpConnect");
    pWinHttpOpenRequest = (WinHttpOpenRequest_t)GetProcAddress(hWinHTTP, "WinHttpOpenRequest");
    pWinHttpSendRequest = (WinHttpSendRequest_t)GetProcAddress(hWinHTTP, "WinHttpSendRequest");
    pWinHttpReceiveResponse = (WinHttpReceiveResponse_t)GetProcAddress(hWinHTTP, "WinHttpReceiveResponse");
    pWinHttpReadData = (WinHttpReadData_t)GetProcAddress(hWinHTTP, "WinHttpReadData");
    pWinHttpCloseHandle = (WinHttpCloseHandle_t)GetProcAddress(hWinHTTP, "WinHttpCloseHandle");

    return (pWinHttpOpen && pWinHttpConnect && pWinHttpOpenRequest && pWinHttpSendRequest &&
            pWinHttpReceiveResponse && pWinHttpReadData && pWinHttpCloseHandle);
}

BOOL FetchHTTPSContent(LPCWSTR host, LPCWSTR path, char** content, DWORD* contentSize) {
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    DWORD dwDownloaded = 0;
    CHAR buffer[4096];
    char* result = NULL;
    DWORD totalSize = 0;
    BOOL success = FALSE;

    hSession = pWinHttpOpen(L"UserAgent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) goto cleanup;

    hConnect = pWinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) goto cleanup;

    hRequest = pWinHttpOpenRequest(hConnect, L"GET", path, NULL,
                                  WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                  WINHTTP_FLAG_SECURE);
    if (!hRequest) goto cleanup;

    if (!pWinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) goto cleanup;
    if (!pWinHttpReceiveResponse(hRequest, NULL)) goto cleanup;

    // First, determine the content size
    while (pWinHttpReadData(hRequest, buffer, sizeof(buffer), &dwDownloaded) && dwDownloaded > 0) {
        char* newResult = realloc(result, totalSize + dwDownloaded + 1);
        if (!newResult) goto cleanup;

        result = newResult;
        memcpy(result + totalSize, buffer, dwDownloaded);
        totalSize += dwDownloaded;

        result[totalSize] = '\0';
    }

    if (result) {
        *content = result;
        *contentSize = totalSize;
        success = TRUE;
    }

    cleanup:
        if (!success && result) free(result);
    if (hRequest) pWinHttpCloseHandle(hRequest);
    if (hConnect) pWinHttpCloseHandle(hConnect);
    if (hSession) pWinHttpCloseHandle(hSession);

    return success;
}

BOOL read_website(LPCWSTR host, LPCWSTR path, char** content, DWORD* contentSize) {
    if (!LoadWinHTTPFunctions()) {
        return FALSE;
    }
    return FetchHTTPSContent(host, path, content, contentSize);
}