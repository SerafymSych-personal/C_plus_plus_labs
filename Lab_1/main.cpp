#include <windows.h>
#include <string>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);

        int screenWidth     = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight    = GetSystemMetrics(SM_CYSCREEN);
        int hScrollBtnWidth = GetSystemMetrics(SM_CXHSCROLL);
        int hScrollHeight   = GetSystemMetrics(SM_CYHSCROLL);
        int frameWidth      = GetSystemMetrics(SM_CXSIZEFRAME);
        int frameHeight     = GetSystemMetrics(SM_CYSIZEFRAME);

        std::wstring text =
            L"Системні метрики:\n\n"
            L"    1. Роздільна здатність екрану: " + std::to_wstring(screenWidth) + L" x " + std::to_wstring(screenHeight) + L" px\n"
            L"    2. Горизонтальна смуга прокрутки: " + std::to_wstring(hScrollBtnWidth) + L" x " + std::to_wstring(hScrollHeight) + L" px\n"
            L"    3. Рамка вікна додатка: " + std::to_wstring(frameWidth) + L" x " + std::to_wstring(frameHeight) + L" px";

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
        DrawTextW(hdc, text.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_WORDBREAK);

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
    const wchar_t szWindowClass[] = L"SerafymWindowClass";

    WNDCLASSEXW wcex = {};
    wcex.cbSize        = sizeof(WNDCLASSEXW);
    wcex.style         = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wcex)) return 0;

    HWND hWnd = CreateWindowW(
        szWindowClass,
        L"Лабораторна робота №1 - Варіант 1",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
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
