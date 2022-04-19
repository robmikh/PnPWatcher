#pragma once

struct PopupMenu
{
	PopupMenu();
	void ShowMenu(HWND window, int x, int y);
	void AppendMenuItem(std::wstring const& name, std::function<void()> callback);
	LRESULT MessageHandler(WPARAM const wparam, LPARAM const lparam);

private:
	struct MenuItem
	{
		std::wstring Name;
		std::function<void()> Callback;
	};

	wil::unique_hmenu m_menu;
	std::vector<MenuItem> m_menuItems;
};

template<typename T>
struct SyncPopupMenu
{
    struct MenuItem
    {
        std::wstring Name;
        T Value;
    };

	SyncPopupMenu(std::vector<MenuItem> menuItems);
	std::optional<T> ShowMenu(HWND window, int x, int y);

private:
	wil::unique_hmenu m_menu;
	std::vector<MenuItem> m_menuItems;
};

template<typename T>
inline SyncPopupMenu<T>::SyncPopupMenu(std::vector<SyncPopupMenu<T>::MenuItem> menuItems)
{
    m_menu.reset(winrt::check_pointer(CreatePopupMenu()));
    m_menuItems = menuItems;
    for (auto i = 0; i < m_menuItems.size(); i++)
    {
        const auto& item = m_menuItems[i];
        auto id = i + 1;
        winrt::check_bool(AppendMenuW(m_menu.get(), MF_STRING, id, item.Name.c_str()));
    }
}

template<typename T>
inline std::optional<T> SyncPopupMenu<T>::ShowMenu(HWND window, int x, int y)
{
    int result = TrackPopupMenuEx(
        m_menu.get(),
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        x,
        y,
        window,
        nullptr);

    if (result > 0)
    {
        auto index = result - 1;
        const auto& item = m_menuItems[index];
        return std::optional(item.Value);
    }
    else
    {
        return std::nullopt;
    }
}
