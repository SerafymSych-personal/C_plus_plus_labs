#include <winsock2.h>
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <string>

#define SERV_PORT 5000
#define WSA_NETEVENT (WM_USER + 2)

#define ID_CLIENT_CONNECT 201
#define ID_CLIENT_SEND    202

HWND hwndEdit = nullptr;
WSADATA wsaData;
SOCKET cln_socket = INVALID_SOCKET;
std::wstring g_logText;

// Append Unicode text to the output edit box
void AppendLog(const std::wstring& text) {
    g_logText += text;
    SetWindowTextW(hwndEdit, g_logText.c_str());
}

// Create client socket and connect to localhost:5000
BOOL SetConnection(HWND hWnd) {
    if (cln_socket != INVALID_SOCKET) {
        MessageBoxW(hWnd, L"З'єднання вже встановлено.", L"Інформація", MB_OK);
        return TRUE;
    }

    // Create IPv4 TCP socket
    cln_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cln_socket == INVALID_SOCKET) {
        MessageBoxW(hWnd, L"Помилка створення сокета!", L"Помилка", MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    // Set server address (127.0.0.1:5000)
    SOCKADDR_IN dest_sin = {};
    dest_sin.sin_family = AF_INET;
    dest_sin.sin_port = htons(SERV_PORT);
    dest_sin.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(cln_socket, (PSOCKADDR)&dest_sin, sizeof(dest_sin)) == SOCKET_ERROR) {
        closesocket(cln_socket);
        cln_socket = INVALID_SOCKET;
        MessageBoxW(hWnd, L"Помилка підключення до сервера!", L"Помилка", MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    // Track server disconnect event asynchronously
    WSAAsyncSelect(cln_socket, hWnd, WSA_NETEVENT, FD_CLOSE);
    AppendLog(L"Зв'язок встановлений!\r\n");
    return TRUE;
}

// Gather Variant 1 metrics and send them as wide-character text to the server
void SendMsg(HWND hWnd) {
    if (cln_socket == INVALID_SOCKET) {
        MessageBoxW(hWnd, L"Спочатку підключіться до сервера (Client -> SetConnection).", L"Інформація", MB_OK);
        return;
    }

    // Query system metrics via WinAPI
    int iconW = GetSystemMetrics(SM_CXICON);
    int iconH = GetSystemMetrics(SM_CYICON);
    int scrollW = GetSystemMetrics(SM_CXVSCROLL);
    HDC hdc = GetDC(hWnd);
    int bitsPixel = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(hWnd, hdc);

    // Format Unicode text payload
    wchar_t wBuf[256];
    std::swprintf(
        wBuf, 256,
        L"Ширина іконки: %d px\r\nВисота іконки: %d px\r\nШирина вертикальної полоси прокрутки: %d px\r\nГлибина кольору: %d біт",
        iconW, iconH, scrollW, bitsPixel
    );

    // Send raw wchar_t bytes over TCP socket
    int byteLen = (int)(std::wcslen(wBuf) * sizeof(wchar_t));
    if (send(cln_socket, (const char*)wBuf, byteLen, 0) != SOCKET_ERROR) {
        AppendLog(std::wstring(L"\r\nДані надіслані на сервер:\r\n") + wBuf + L"\r\n");
    } else {
        AppendLog(L"\r\nПомилка надсилання даних!\r\n");
    }
}

// Main window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Create multiline Unicode output text control
        hwndEdit = CreateWindowW(
            L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, 0, 400, 220, hWnd, nullptr, nullptr, nullptr
        );

        // Initialize Winsock 2.2
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            MessageBoxW(hWnd, L"Помилка WSAStartup", L"Помилка", MB_OK | MB_ICONSTOP);
            return -1;
        }
        AppendLog(L"Використовується WinSock 2.2\r\nСтатус: Running\r\n");
        break;
    }
    case WM_SIZE:
        MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_CLIENT_CONNECT: SetConnection(hWnd); break;
        case ID_CLIENT_SEND:    SendMsg(hWnd); break;
        }
        break;
    case WSA_NETEVENT:
        // Handle server disconnect
        if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) {
            closesocket(cln_socket);
            cln_socket = INVALID_SOCKET;
            AppendLog(L"\r\nСервер закрив з'єднання.\r\n");
        }
        break;
    case WM_DESTROY:
        if (cln_socket != INVALID_SOCKET) closesocket(cln_socket);
        WSACleanup();
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
    wcex.lpszClassName = L"Lab4Client2Class";
    RegisterClassExW(&wcex);

    // Build top menu bar
    HMENU hMenuBar = CreateMenu();
    HMENU hClnMenu = CreatePopupMenu();
    AppendMenuW(hClnMenu, MF_STRING, ID_CLIENT_CONNECT, L"SetConnection");
    AppendMenuW(hClnMenu, MF_STRING, ID_CLIENT_SEND, L"SendMsg");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hClnMenu, L"Client");

    HWND hWnd = CreateWindowW(
        L"Lab4Client2Class", L"Клієнт 2 (Варіант 1)", WS_OVERLAPPEDWINDOW,
        700, 420, 420, 260, nullptr, hMenuBar, hInstance, nullptr
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