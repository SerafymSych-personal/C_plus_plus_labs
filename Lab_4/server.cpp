#include <winsock2.h>
#include <windows.h>
#include <cstdio>
#include <string>

#define SERV_PORT 5000
#define WSA_ACCEPT   (WM_USER + 1)
#define WSA_NETEVENT (WM_USER + 2)

#define ID_SERVER_START  101
#define ID_SERVER_STOP   102
#define ID_CLIENTOFF_1   103
#define ID_CLIENTOFF_2   104

HWND hwndEdit = nullptr;
HWND hMainWnd = nullptr;
WSADATA wsaData;
SOCKET srv_socket = INVALID_SOCKET;
int ClientNum = -1;
SOCKET sock[2] = { INVALID_SOCKET, INVALID_SOCKET };
SOCKADDR_IN clientAddr[2] = {};
std::wstring g_logText;

// Append Unicode text to the output edit box and scroll down
void AppendLog(const std::wstring& text) {
    g_logText += text;
    SetWindowTextW(hwndEdit, g_logText.c_str());
    int len = GetWindowTextLengthW(hwndEdit);
    SendMessageW(hwndEdit, EM_SETSEL, len, len);
    SendMessageW(hwndEdit, EM_SCROLLCARET, 0, 0);
}

// Create, bind, and start listening on the server TCP socket
void ServerStart(HWND hWnd) {
    if (srv_socket != INVALID_SOCKET) {
        MessageBoxW(hWnd, L"Сервер вже запущено.", L"Інформація", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Create IPv4 TCP socket
    srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_socket == INVALID_SOCKET) {
        MessageBoxW(hWnd, L"Помилка створення сокета!", L"Помилка", MB_OK | MB_ICONERROR);
        return;
    }

    // Bind socket to any local IP and port 5000
    SOCKADDR_IN srv_address = {};
    srv_address.sin_family = AF_INET;
    srv_address.sin_port = htons(SERV_PORT);
    srv_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv_socket, (LPSOCKADDR)&srv_address, sizeof(srv_address)) == SOCKET_ERROR) {
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        MessageBoxW(hWnd, L"Помилка зв'язування з портом 5000!", L"Помилка", MB_OK | MB_ICONSTOP);
        return;
    }

    // Start listening for incoming client connections
    if (listen(srv_socket, 4) == SOCKET_ERROR) {
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        MessageBoxW(hWnd, L"Помилка listen()!", L"Помилка", MB_OK | MB_ICONERROR);
        return;
    }

    // Register asynchronous accept event notification for the window
    if (WSAAsyncSelect(srv_socket, hWnd, WSA_ACCEPT, FD_ACCEPT) == SOCKET_ERROR) {
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        MessageBoxW(hWnd, L"Помилка WSAAsyncSelect!", L"Помилка", MB_OK | MB_ICONERROR);
        return;
    }

    AppendLog(L"Сервер запущено (порт 5000). Очікування клієнтів...\r\n");
}

// Close all client sockets and stop the server listening socket
void ServerStop(HWND hWnd) {
    for (int i = 0; i < 2; ++i) {
        if (sock[i] != INVALID_SOCKET) {
            WSAAsyncSelect(sock[i], hWnd, 0, 0);
            closesocket(sock[i]);
            sock[i] = INVALID_SOCKET;
        }
    }
    ClientNum = -1;

    if (srv_socket != INVALID_SOCKET) {
        WSAAsyncSelect(srv_socket, hWnd, 0, 0);
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        AppendLog(L"Сервер зупинено.\r\n");
    }
}

// Accept a pending client connection and subscribe to read/close events
BOOL AcceptClient(int j) {
    int addrLen = sizeof(clientAddr[j]);
    sock[j] = accept(srv_socket, (LPSOCKADDR)&clientAddr[j], &addrLen);
    if (sock[j] != INVALID_SOCKET) {
        if (WSAAsyncSelect(sock[j], hMainWnd, WSA_NETEVENT, FD_READ | FD_CLOSE) == 0) {
            return TRUE;
        }
        closesocket(sock[j]);
        sock[j] = INVALID_SOCKET;
    }
    return FALSE;
}

// Handle incoming connection request from a client
void OnWSAAccept(HWND hWnd, LPARAM lParam) {
    if (WSAGETSELECTERROR(lParam)) {
        MessageBoxW(hWnd, L"Помилка прийняття з'єднання", L"Помилка", MB_OK);
        return;
    }

    if (ClientNum >= 1) {
        MessageBoxW(hWnd, L"Досягнуто ліміт клієнтів (максимум 2).", L"Увага", MB_OK | MB_ICONWARNING);
        return;
    }

    ClientNum++;
    if (!AcceptClient(ClientNum)) {
        ClientNum--;
        MessageBoxW(hWnd, L"Помилка підключення клієнта", L"Помилка", MB_OK);
        return;
    }

    // Format IP octets directly without ASCII-to-Unicode conversion issues
    unsigned char* b = (unsigned char*)&clientAddr[ClientNum].sin_addr.s_addr;
    wchar_t szBuf[256];
    std::swprintf(szBuf, 256, L"\r\nДоданий клієнт %d (IP=%u.%u.%u.%u, Port=%u)\r\n",
                  ClientNum + 1, b[0], b[1], b[2], b[3], ntohs(clientAddr[ClientNum].sin_port));
    AppendLog(szBuf);
}

// Handle incoming data or disconnection from an active client
void OnWSANetEvent(WPARAM wParam, LPARAM lParam) {
    int number = -1;
    if (sock[0] == (SOCKET)wParam) number = 0;
    else if (sock[1] == (SOCKET)wParam) number = 1;
    if (number == -1) return;

    // Receive wide-character text sent directly by the client
    if (WSAGETSELECTEVENT(lParam) == FD_READ) {
        wchar_t wTemp[512];
        int bytesReceived = recv((SOCKET)wParam, (char*)wTemp, sizeof(wTemp) - sizeof(wchar_t), 0);
        if (bytesReceived > 0) {
            int wcharCount = bytesReceived / sizeof(wchar_t);
            wTemp[wcharCount] = L'\0';

            wchar_t header[64];
            std::swprintf(header, 64, L"\r\nДані від Клієнта %d:\r\n", number + 1);
            AppendLog(std::wstring(header) + wTemp + L"\r\n");
        }
    }

    // Clean up socket when client disconnects
    if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) {
        WSAAsyncSelect(sock[number], hMainWnd, 0, 0);
        closesocket(sock[number]);
        sock[number] = INVALID_SOCKET;

        wchar_t szTemp[64];
        std::swprintf(szTemp, 64, L"\r\nКлієнт %d завершив роботу.\r\n", number + 1);
        AppendLog(szTemp);
    }
}

// Disconnect the selected client manually
void ClientOff(HWND hWnd, int j) {
    if (j > ClientNum || sock[j] == INVALID_SOCKET) return;

    wchar_t szBuf[128];
    std::swprintf(szBuf, 128, L"Відключити клієнта %d?", j + 1);
    if (IDYES == MessageBoxW(hWnd, szBuf, L"Підтвердження", MB_YESNO | MB_ICONQUESTION)) {
        WSAAsyncSelect(sock[j], hMainWnd, 0, 0);
        closesocket(sock[j]);
        sock[j] = INVALID_SOCKET;

        std::swprintf(szBuf, 128, L"\r\nВідключено клієнта %d\r\n", j + 1);
        AppendLog(szBuf);
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
            0, 0, 440, 260, hWnd, nullptr, nullptr, nullptr
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
        case ID_SERVER_START: ServerStart(hWnd); break;
        case ID_SERVER_STOP:  ServerStop(hWnd); break;
        case ID_CLIENTOFF_1:  ClientOff(hWnd, 0); break;
        case ID_CLIENTOFF_2:  ClientOff(hWnd, 1); break;
        }
        break;
    case WSA_ACCEPT:
        OnWSAAccept(hWnd, lParam);
        break;
    case WSA_NETEVENT:
        OnWSANetEvent(wParam, lParam);
        break;
    case WM_DESTROY:
        ServerStop(hWnd);
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
    wcex.lpszClassName = L"Lab4ServerClass";
    RegisterClassExW(&wcex);

    // Build top menu bar
    HMENU hMenuBar = CreateMenu();
    HMENU hSrvMenu = CreatePopupMenu();
    AppendMenuW(hSrvMenu, MF_STRING, ID_SERVER_START, L"Start");
    AppendMenuW(hSrvMenu, MF_STRING, ID_SERVER_STOP, L"Stop");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hSrvMenu, L"Server");

    HMENU hOffMenu = CreatePopupMenu();
    AppendMenuW(hOffMenu, MF_STRING, ID_CLIENTOFF_1, L"Client 1");
    AppendMenuW(hOffMenu, MF_STRING, ID_CLIENTOFF_2, L"Client 2");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hOffMenu, L"ClientOff");

    hMainWnd = CreateWindowW(
        L"Lab4ServerClass", L"Сервер (Лаб 4)", WS_OVERLAPPEDWINDOW,
        200, 200, 460, 320, nullptr, hMenuBar, hInstance, nullptr
    );

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    // Run standard message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}