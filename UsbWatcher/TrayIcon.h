#pragma once

struct TrayIcon
{
public:
    TrayIcon(HWND window, uint32_t message, uint32_t id, std::wstring const& tip)
    {
        m_window = window;
        m_id = id;
        RegisterTrayIcon(window, message, id, tip);
    }
    ~TrayIcon()
    {
        UnregisterRayIcon(m_id);
    }

private:
    void RegisterTrayIcon(HWND window, uint32_t message, uint32_t id, std::wstring const& tip)
    {
        auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
        NOTIFYICONDATAW trayIconDesc = {};
        trayIconDesc.cbSize = sizeof(trayIconDesc);
        trayIconDesc.hWnd = window;
        trayIconDesc.uID = id;
        trayIconDesc.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
        trayIconDesc.uCallbackMessage = message;
        trayIconDesc.hIcon = LoadIconW(instance, MAIN_ICON);
        memcpy(trayIconDesc.szTip, tip.data(), tip.size() * sizeof(wchar_t));
        winrt::check_bool(Shell_NotifyIconW(NIM_ADD, &trayIconDesc));
        trayIconDesc.uVersion = NOTIFYICON_VERSION_4;
        winrt::check_bool(Shell_NotifyIconW(NIM_SETVERSION, &trayIconDesc));
    }

    void UnregisterRayIcon(uint32_t id)
    {
        NOTIFYICONDATAW trayIconDesc = {};
        trayIconDesc.cbSize = sizeof(trayIconDesc);
        trayIconDesc.hWnd = m_window;
        trayIconDesc.uID = id;
        winrt::check_bool(Shell_NotifyIconW(NIM_DELETE, &trayIconDesc));
    }

private:
    HWND m_window = nullptr;
    uint32_t m_id = 0;
};