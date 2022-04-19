#include "pch.h"
#include "PopupMenu.h"

PopupMenu::PopupMenu()
{
	m_menu.reset(winrt::check_pointer(CreatePopupMenu()));
    MENUINFO menuInfo = {};
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIM_STYLE;
    menuInfo.dwStyle = MNS_NOTIFYBYPOS;
    winrt::check_bool(SetMenuInfo(m_menu.get(), &menuInfo));
}

void PopupMenu::ShowMenu(HWND window, int x, int y)
{
    winrt::check_bool(TrackPopupMenuEx(
        m_menu.get(),
        TPM_LEFTALIGN | TPM_BOTTOMALIGN,
        x,
        y,
        window,
        nullptr));
}

void PopupMenu::AppendMenuItem(std::wstring const& name, std::function<void()> callback)
{
    auto id = m_menuItems.size();
    m_menuItems.push_back({ name, callback });
    winrt::check_bool(AppendMenuW(m_menu.get(), MF_STRING, id, name.c_str()));
}

LRESULT PopupMenu::MessageHandler(WPARAM const wparam, LPARAM const lparam)
{
    auto menu = reinterpret_cast<HMENU>(lparam);
    auto index = static_cast<int>(wparam);
    if (menu == m_menu.get())
    {
        const auto& menuItem = m_menuItems[index];
        menuItem.Callback();
    }
    return 0;
}