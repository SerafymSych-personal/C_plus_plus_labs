#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <string>

#define ID_CLIENT_SEND 201

const wchar_t* MAILSLOT_NAME = L"\\\\.\\mailslot\\lab5";

HWND hwndEdit = nullptr;
std::wstring g_logText;

// Append Unicode text to the read-only edit box
void AppendLog(const std::wstring& text) {
    g_logText += text;
    SetWindowTextW(hwndEdit, g_logText.c_str());
}

// Gather Variant 8 metrics and write them to the server mailslot
void SendToMailslot(HWND hWnd) {
    // Query system metrics via WinAPI
    int osMetric = GetSystemMetrics(SM_SERVERR2);
    int monitors = GetSystemMetrics(SM_CMONITORS);
    HDC hdc = GetDC(hWnd);
    int bitsPixel = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(hWnd, hdc);

    // Format Unicode message
    wchar_t wBuf[256];
    std::swprintf(
        wBuf, 256,
        L"Дані Клієнта №1:\r\n- версія ОС (SM_SERVERR2): %d\r\n- кількість моніторів: %d\r\n- глибина кольору: %d біт",
        osMetric, monitors, bitsPixel
    );

    // Open existing mailslot created by the server
    HANDLE hFile = CreateFileW(
        MAILSLOT_NAME,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        wchar_t errBuf[128];
        std::swprintf(errBuf, 128, L"\r\nПомилка відкриття поштової скриньки! Код: %lu\r\n", GetLastError());
        AppendLog(errBuf);
        return;
    }

    // Write wide-character message into the mailslot
    DWORD bytesToWrite = (DWORD)((std::wcslen(wBuf) + 1) * sizeof(wchar_t));
    DWORD bytesWritten = 0;

    if (WriteFile(hFile, wBuf, bytesToWrite, &bytesWritten, nullptr)) {
        AppendLog(std::wstring(L"\r\nДані надіслані на сервер:\r\n") + wBuf + L"\r\n");
    } else {
        AppendLog(L"\r\nПомилка запису у поштову скриньку!\r\n");
    }

    // Close mailslot file handle
    CloseHandle(hFile);
}

// Main window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        // Create multiline Unicode output text control
        hwndEdit = CreateWindowW(
            L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, 0, 400, 220, hWnd, nullptr, nullptr, nullptr
        );
        AppendLog(L"Клієнт №1 (Варіант 8) готовий.\r\nНатисніть Client -> SendMsg для відправки.\r\n");
        break;
    case WM_SIZE:
        MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_CLIENT_SEND) {
            SendToMailslot(hWnd);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Application entry point
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"Lab5Client1Class";
    RegisterClassExW(&wcex);

    // Build top menu bar
    HMENU hMenuBar = CreateMenu();
    HMENU hClnMenu = CreatePopupMenu();
    AppendMenuW(hClnMenu, MF_STRING, ID_CLIENT_SEND, L"SendMsg");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hClnMenu, L"Client");

    HWND hWnd = CreateWindowW(
        L"Lab5Client1Class", L"Клієнт 1 (Варіант 8)", WS_OVERLAPPEDWINDOW,
        700, 120, 420, 260, nullptr, hMenuBar, hInstance, nullptr
    );

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Run standard message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}