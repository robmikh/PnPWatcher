#include "pch.h"
#include "MainWindow.h"
#include "TrayIcon.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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

    // Load accelerators
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    wil::unique_haccel accel(winrt::check_pointer(LoadAcceleratorsW(instance, MAKEINTRESOURCEW(IDR_ACCELERATOR1))));

    // Message pump
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (!TranslateAcceleratorW(window.m_window, accel.get(), &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}