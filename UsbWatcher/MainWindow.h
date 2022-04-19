#pragma once
#include <robmikh.common/DesktopWindow.h>
#include "PopupMenu.h"
#include "UsbEvent.h"
#include "UsbEventWatcher.h"

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
	void CreateControls(HINSTANCE instance);
	void ResizeProcessListView();
	void OnUsbEventAdded(UsbEvent usbEvent);
	void OnListViewNotify(LPARAM const lparam);
	void WriteUsbEventData(std::wostream& stream, UsbEvent const& usbEvent, UsbEventColumn const& column);

private:
	bool m_isVisible = false;
	std::unique_ptr<PopupMenu> m_trayIconMenu;
	HWND m_usbEventsListView = nullptr;
	std::vector<UsbEventColumn> m_columns;
	std::vector<UsbEvent> m_usbEvents;
	std::unique_ptr<UsbEventWatcher> m_usbEventWatcher;
	winrt::Windows::System::DispatcherQueue m_dispatcherQueue{ nullptr };
	winrt::Windows::Globalization::DateTimeFormatting::DateTimeFormatter m_timestampFormatter{ nullptr };
};