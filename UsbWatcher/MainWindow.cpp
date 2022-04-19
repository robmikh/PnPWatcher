#include "pch.h"
#include "MainWindow.h"

const std::wstring MainWindow::ClassName = L"UsbWatcher.MainWindow";
std::once_flag MainWindowClassRegistration;

void MainWindow::RegisterWindowClass()
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIconW(instance, MAIN_ICON);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = ClassName.c_str();
    wcex.hIconSm = LoadIconW(instance, MAIN_ICON);
    winrt::check_bool(RegisterClassExW(&wcex));
}

MainWindow::MainWindow(std::wstring const& titleString, int width, int height)
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));

    std::call_once(MainWindowClassRegistration, []() { RegisterWindowClass(); });

    winrt::check_bool(CreateWindowExW(0, ClassName.c_str(), titleString.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    CreateTrayIconMenu();
}

LRESULT MainWindow::MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam)
{
    switch (message)
    {
    case WM_CLOSE:
        // We don't want to destroy the window, just hide it.
        IsVisible(false);
        break;
    case WM_DESTROY:
        // TODO: PostQuitMessage? Does it matter with this configuration?
        break;
    case TrayIconMessage:
    {
        // We only have one icon, so we won't check the id.
        auto iconMessage = LOWORD(lparam);
        auto x = GET_X_LPARAM(wparam);
        auto y = GET_Y_LPARAM(wparam);
        switch (iconMessage)
        {
        case WM_CONTEXTMENU:
            ShowTrayIconMenu(x, y);
            break;
        }
    }
        break;
    case WM_MENUCOMMAND:
    {
        auto menu = reinterpret_cast<HMENU>(lparam);
        auto index = static_cast<int>(wparam);
        if (menu == m_trayIconMenu.get())
        {
            switch (index)
            {
            case 0:
                IsVisible(true);
                break;
            case 1:
                PostQuitMessage(0);
                break;
            }
        }
    }
        break;
    default:
        return base_type::MessageHandler(message, wparam, lparam);
    }
    return 0;
}

void MainWindow::IsVisible(bool value)
{
    if (value != m_isVisible)
    {
        m_isVisible = value;
        auto showWindowFlag = m_isVisible ? SW_SHOW : SW_HIDE;
        ShowWindow(m_window, showWindowFlag);
        UpdateWindow(m_window);
    }
}

void MainWindow::CreateTrayIconMenu()
{
    m_trayIconMenu.reset(winrt::check_pointer(CreatePopupMenu()));
    winrt::check_bool(AppendMenuW(m_trayIconMenu.get(), MF_STRING, 0, L"Open"));
    winrt::check_bool(AppendMenuW(m_trayIconMenu.get(), MF_STRING, 1, L"Exit"));
    MENUINFO menuInfo = {};
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIM_STYLE;
    menuInfo.dwStyle = MNS_NOTIFYBYPOS;
    winrt::check_bool(SetMenuInfo(m_trayIconMenu.get(), &menuInfo));
}

void MainWindow::ShowTrayIconMenu(int x, int y)
{
    winrt::check_bool(TrackPopupMenuEx(
        m_trayIconMenu.get(),
        TPM_LEFTALIGN | TPM_BOTTOMALIGN,
        x,
        y,
        m_window,
        nullptr));
}
