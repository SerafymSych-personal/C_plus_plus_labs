#include <windows.h>
#include <cstdio>
#include <string>

#define IDT_MAILSLOT_TIMER 1001

#define ID_MAILSLOT_CREATE 101
#define ID_MAILSLOT_CLOSE  102

const wchar_t* MAILSLOT_NAME = L"\\\\.\\mailslot\\lab5";

HWND hwndEdit = nullptr;
HANDLE hMailslot = INVALID_HANDLE_VALUE;
std::wstring g_logText;

// Append Unicode text to the read-only edit box and scroll down
void AppendLog(const std::wstring& text) {
    g_logText += text;
    SetWindowTextW(hwndEdit, g_logText.c_str());
    int len = GetWindowTextLengthW(hwndEdit);
    SendMessageW(hwndEdit, EM_SETSEL, len, len);
    SendMessageW(hwndEdit, EM_SCROLLCARET, 0, 0);
}

// Create the server mailslot and start polling timer
void MailslotCreate(HWND hWnd) {
    if (hMailslot != INVALID_HANDLE_VALUE) {
        MessageBoxW(hWnd, L"Поштова скринька вже створена.", L"Інформація", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Create mailslot with unlimited message size and non-blocking read timeout (0)
    hMailslot = CreateMailslotW(MAILSLOT_NAME, 0, 0, nullptr);
    if (hMailslot == INVALID_HANDLE_VALUE) {
        wchar_t errBuf[128];
        std::swprintf(errBuf, 128, L"Помилка створення скриньки! Код: %lu\r\n", GetLastError());
        AppendLog(errBuf);
        return;
    }

    // Start 1-second timer to check incoming messages
    SetTimer(hWnd, IDT_MAILSLOT_TIMER, 1000, nullptr);
    AppendLog(L"Поштову скриньку створено (\\\\.\\mailslot\\lab5).\r\nСервер запущено. Очікування повідомлень...\r\n");
}

// Stop polling timer and close the mailslot handle
void MailslotClose(HWND hWnd) {
    KillTimer(hWnd, IDT_MAILSLOT_TIMER);

    if (hMailslot != INVALID_HANDLE_VALUE) {
        CloseHandle(hMailslot);
        hMailslot = INVALID_HANDLE_VALUE;
        AppendLog(L"\r\nПоштовий сервер закрито.\r\n");
    }
}

// Check queue via GetMailslotInfo and read all pending messages
void ReadPendingMessages() {
    if (hMailslot == INVALID_HANDLE_VALUE) return;

    DWORD nextSize = 0;
    DWORD msgCount = 0;

    // Query current mailslot queue status
    while (GetMailslotInfo(hMailslot, nullptr, &nextSize, &msgCount, nullptr) && msgCount > 0) {
        if (nextSize == MAILSLOT_NO_MESSAGE) break;

        wchar_t wBuf[512] = {};
        DWORD bytesRead = 0;

        // Read one message from the head of the FIFO queue
        if (ReadFile(hMailslot, wBuf, sizeof(wBuf) - sizeof(wchar_t), &bytesRead, nullptr) && bytesRead > 0) {
            int wcharCount = bytesRead / sizeof(wchar_t);
            wBuf[wcharCount] = L'\0';
            AppendLog(std::wstring(L"\r\nОтримано повідомлення:\r\n") + wBuf + L"\r\n");
        } else {
            break;
        }
    }
}

// Main window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        // Create multiline Unicode output text control
        hwndEdit = CreateWindowW(
            L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, 0, 440, 280, hWnd, nullptr, nullptr, nullptr
        );
        AppendLog(L"Лабораторна робота №5 — Поштова скринька.\r\nНатисніть MailSlot -> Create для запуску.\r\n");
        break;
    case WM_SIZE:
        MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_MAILSLOT_CREATE: MailslotCreate(hWnd); break;
        case ID_MAILSLOT_CLOSE:  MailslotClose(hWnd); break;
        }
        break;
    case WM_TIMER:
        if (wParam == IDT_MAILSLOT_TIMER) {
            ReadPendingMessages();
        }
        break;
    case WM_DESTROY:
        MailslotClose(hWnd);
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
    wcex.lpszClassName = L"Lab5ServerClass";
    RegisterClassExW(&wcex);

    // Build top menu bar
    HMENU hMenuBar = CreateMenu();
    HMENU hMailMenu = CreatePopupMenu();
    AppendMenuW(hMailMenu, MF_STRING, ID_MAILSLOT_CREATE, L"Create");
    AppendMenuW(hMailMenu, MF_STRING, ID_MAILSLOT_CLOSE, L"Close");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hMailMenu, L"MailSlot");

    HWND hWnd = CreateWindowW(
        L"Lab5ServerClass", L"Сервер — Поштова скринька (Лаб 5)", WS_OVERLAPPEDWINDOW,
        200, 200, 460, 340, nullptr, hMenuBar, hInstance, nullptr
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