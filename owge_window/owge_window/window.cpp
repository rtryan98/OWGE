#include "owge_window/window.hpp"

namespace owge
{
LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    Window_Data* data = reinterpret_cast<Window_Data*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (data && data->userproc && data->userproc(hwnd, msg, wparam, lparam))
        return true;

    switch (msg)
    {
    case WM_CLOSE:
        if (data)
        {
            data->alive = false;
        }
        break;
    case WM_GETMINMAXINFO:
    {
        auto& size = reinterpret_cast<LPMINMAXINFO>(lparam)->ptMinTrackSize;
        size.x = 256;
        size.y = 144;
        break;
    }
    case WM_SIZE:
    {
        if (data)
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            data->width = rect.right;
            data->height = rect.bottom;
        }
        break;
    }
    default:
        break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

Window::Window(const Window_Settings& settings)
    : m_hwnd(),
    m_data{
        .width = settings.width,
        .height = settings.height,
        .title = std::string(settings.title),
        .userproc = settings.userproc
    }
{
    RECT wr = {
        .left = LONG((GetSystemMetrics(SM_CXSCREEN) - m_data.width) / 2),
        .top = LONG((GetSystemMetrics(SM_CYSCREEN) - m_data.height) / 2),
        .right = LONG(m_data.width),
        .bottom = LONG(m_data.height)
    };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRectEx(&wr, style, FALSE, WS_EX_TOOLWINDOW);
    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = 0,
        .lpfnWndProc = wnd_proc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = GetModuleHandle(NULL),
        .hIcon = LoadIcon(NULL, IDI_WINLOGO),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = HBRUSH(GetStockObject(BLACK_BRUSH)),
        .lpszMenuName = nullptr,
        .lpszClassName = m_data.title.c_str(),
        .hIconSm = wc.hIcon
    };
    RegisterClassEx(&wc);
    m_hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        wc.lpszClassName,
        style,
        wr.left,
        wr.top,
        wr.right,
        wr.bottom,
        nullptr,
        nullptr,
        GetModuleHandle(NULL),
        0);
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)(&m_data));
    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
    m_data.alive = true;
}

Window::~Window()
{
    DestroyWindow(m_hwnd);
}

void Window::poll_events()
{
    MSG message = {};
    ZeroMemory(&message, sizeof(MSG));
    while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}
}
