#pragma once
#include <robmikh.common/DesktopWindow.h>
#include "PopupMenu.h"

struct MainWindow : robmikh::common::desktop::DesktopWindow<MainWindow>
{
	static const uint32_t TrayIconMessage = WM_USER + 1;
	static const std::wstring ClassName;
	MainWindow(std::wstring const& titleString, int width, int height);
	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);

	bool IsVisible() { return m_isVisible; }
	void IsVisible(bool value);

private:
	static void RegisterWindowClass();
	void CreateTrayIconMenu();
	void OnOpenMenuItemClicked();

private:
	bool m_isVisible = false;
	std::unique_ptr<PopupMenu> m_trayIconMenu;
};