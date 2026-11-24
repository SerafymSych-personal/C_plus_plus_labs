#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>

#define IDT_RENDER_TIMER 1

struct CircleData {
    int x;
    int y;
    int radius;
    int dx;
    int dy;
    COLORREF color;
};

int window_width = 800;
int window_height = 600;

HWND g_hWnd = nullptr;
HANDLE g_hMutex = nullptr;
HANDLE g_hStopEvent = nullptr;

std::vector<CircleData> g_circles;
std::vector<HANDLE> g_threads;

DWORD WINAPI CircleThreadProc(LPVOID lpParam) {
    size_t index = (size_t)lpParam;

    while (WaitForSingleObject(g_hStopEvent, 0) == WAIT_TIMEOUT) {
        WaitForSingleObject(g_hMutex, INFINITE);

        CircleData& c = g_circles[index];
        c.x += c.dx;
        c.y += c.dy;

        // Bounce off left and right borders
        if (c.x - c.radius <= 0) {
            c.x = c.radius;
            c.dx = -c.dx;
        } else if (c.x + c.radius >= window_width) {
            c.x = window_width - c.radius;
            c.dx = -c.dx;
        }

        // Bounce off top and bottom borders
        if (c.y - c.radius <= 0) {
            c.y = c.radius;
            c.dy = -c.dy;
        } else if (c.y + c.radius >= window_height) {
            c.y = window_height - c.radius;
            c.dy = -c.dy;
        }

        ReleaseMutex(g_hMutex);

        // Worker thread only updates coordinates; timer handles screen redraw
        Sleep(16);
    }

    ExitThread(0);
}

void SpawnCircleThread() {
    WaitForSingleObject(g_hMutex, INFINITE);

    if (g_threads.size() >= MAXIMUM_WAIT_OBJECTS) {
        ReleaseMutex(g_hMutex);
        return;
    }

    CircleData c;
    c.radius = 20;

    int maxX = (window_width > 2 * c.radius) ? (window_width - 2 * c.radius) : 1;
    int maxY = (window_height > 2 * c.radius) ? (window_height - 2 * c.radius) : 1;

    c.x = c.radius + (std::rand() % maxX);
    c.y = c.radius + (std::rand() % maxY);

    int speed = 4;
    int dir = (std::rand() % 2 == 0) ? 1 : -1;

    if (std::rand() % 2 == 0) {
        c.dx = speed * dir;
        c.dy = 0;
    } else {
        c.dx = 0;
        c.dy = speed * dir;
    }

    c.color = RGB(std::rand() % 220, std::rand() % 220, std::rand() % 220);

    size_t index = g_circles.size();
    g_circles.push_back(c);

    HANDLE hThread = CreateThread(nullptr, 0, CircleThreadProc, (LPVOID)index, 0, nullptr);
    if (hThread) {
        g_threads.push_back(hThread);
    }

    ReleaseMutex(g_hMutex);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Start render timer (~60 FPS)
        SetTimer(hWnd, IDT_RENDER_TIMER, 16, nullptr);
        break;
    }
    case WM_TIMER: {
        if (wParam == IDT_RENDER_TIMER) {
            // Request redraw without erasing background (FALSE)
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;
    }
    case WM_ERASEBKGND: {
        // Prevent background flicker; we fill background inside memory buffer
        return 1;
    }
    case WM_SIZE: {
        window_width = LOWORD(lParam);
        window_height = HIWORD(lParam);
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
    }
    case WM_KEYDOWN: {
        SpawnCircleThread();
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 1. Create off-screen memory DC and compatible bitmap
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, window_width, window_height);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        // 2. Fill white background in memory buffer
        RECT bgRect = { 0, 0, window_width, window_height };
        FillRect(memDC, &bgRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

        // 3. Draw all circles into memory buffer
        HPEN hPen = (HPEN)GetStockObject(NULL_PEN);
        HPEN hOldPen = (HPEN)SelectObject(memDC, hPen);

        WaitForSingleObject(g_hMutex, INFINITE);
        for (const auto& c : g_circles) {
            HBRUSH hBrush = CreateSolidBrush(c.color);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(memDC, hBrush);

            Ellipse(memDC, c.x - c.radius, c.y - c.radius, c.x + c.radius, c.y + c.radius);

            SelectObject(memDC, hOldBrush);
            DeleteObject(hBrush);
        }
        ReleaseMutex(g_hMutex);

        SelectObject(memDC, hOldPen);

        // 4. Copy finished frame from memory buffer to screen in one operation
        BitBlt(hdc, 0, 0, window_width, window_height, memDC, 0, 0, SRCCOPY);

        // 5. Cleanup off-screen GDI resources
        SelectObject(memDC, hOldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        KillTimer(hWnd, IDT_RENDER_TIMER);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    std::srand((unsigned int)std::time(nullptr));

    g_hMutex = CreateMutexW(nullptr, FALSE, nullptr);
    g_hStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    const wchar_t szWindowClass[] = L"SerafymLab3Class";

    WNDCLASSEXW wcex = {};
    wcex.cbSize        = sizeof(WNDCLASSEXW);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wcex)) return 0;

    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - window_width) / 2;
    int posY = (screenHeight - window_height) / 2;

    g_hWnd = CreateWindowW(
        szWindowClass,
        L"Лабораторна робота №3 - Варіант 1",
        WS_OVERLAPPEDWINDOW,
        posX, posY, window_width, window_height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hWnd) return 0;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SetEvent(g_hStopEvent);
    if (!g_threads.empty()) {
        WaitForMultipleObjects((DWORD)g_threads.size(), g_threads.data(), TRUE, 5000);
        for (HANDLE hThread : g_threads) {
            CloseHandle(hThread);
        }
    }

    CloseHandle(g_hStopEvent);
    CloseHandle(g_hMutex);

    return (int)msg.wParam;
}