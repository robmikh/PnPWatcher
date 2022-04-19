#include "pch.h"
#include "MainWindow.h"
#include "TrayIcon.h"

namespace winrt
{
    using namespace Windows::Foundation;
}

namespace util
{
    using namespace robmikh::common::desktop;
}

int __stdcall WinMain(HINSTANCE, HINSTANCE, PSTR, int)
{
    // Initialize COM
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    // Create the DispatcherQueue that we'll use to dispatch things to the UI thread.
    auto controller = util::CreateDispatcherQueueControllerForCurrentThread();

    // Create our window
    auto window = MainWindow(L"UsbWatcher", 800, 600);

    // Create a tray icon
    auto trayIcon = TrayIcon(window.m_window, MainWindow::TrayIconMessage, 0, L"UsbWatcher");

    // Message pump
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}