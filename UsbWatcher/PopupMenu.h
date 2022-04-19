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