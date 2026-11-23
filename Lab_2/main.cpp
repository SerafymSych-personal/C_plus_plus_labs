#include <windows.h>

int window_width = 500;
int window_height = 300;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE: {
        // Get new window sizes on change
        window_width = LOWORD(lParam);
        window_height = HIWORD(lParam);

        // Redraw whole window on size change
        InvalidateRect(hWnd, nullptr, TRUE);
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        int midX = window_width / 2;
        int midY = window_height / 2;

        HPEN hPen = (HPEN)GetStockObject(NULL_PEN); // CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        HBRUSH hBrush1 = CreateSolidBrush(RGB(255, 200, 200));
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush1);
        Rectangle(hdc, 0, 0, midX, midY);
        DeleteObject(hBrush1);

        HBRUSH hBrush2 = CreateSolidBrush(RGB(200, 255, 200));
        SelectObject(hdc, hBrush2);
        Rectangle(hdc, midX, 0, window_width, midY);
        DeleteObject(hBrush2);

        HBRUSH hBrush3 = CreateSolidBrush(RGB(200, 200, 255));
        SelectObject(hdc, hBrush3);
        Rectangle(hdc, 0, midY, midX, window_height);
        DeleteObject(hBrush3);

        HBRUSH hBrush4 = CreateSolidBrush(RGB(255, 255, 200));
        SelectObject(hdc, hBrush4);
        Rectangle(hdc, midX, midY, window_width, window_height);
        DeleteObject(hBrush4);

        // Clear to default and free memory
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    const wchar_t szWindowClass[] = L"SerafymLab2Class";

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

    // Window in the center of screen
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - window_width) / 2;
    int posY = (screenHeight - window_height) / 2;

    HWND hWnd = CreateWindowW(
        szWindowClass,
        L"Лабораторна робота №2 - Варіант 1 (GDI)",
        WS_OVERLAPPEDWINDOW,
        posX, posY, window_width, window_height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}