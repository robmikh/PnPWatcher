#pragma once
#include <robmikh.common/DesktopWindow.h>
#include "PopupMenu.h"
#include "UsbEvent.h"
#include "UsbEventWatcher.h"
#include "RingList.h"

struct MainWindow : robmikh::common::desktop::DesktopWindow<MainWindow>
{
	static const uint32_t TrayIconMessage = WM_USER + 1;
	static const std::wstring ClassName;
	MainWindow(std::wstring const& titleString, int width, int height);
	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);

	bool IsVisible() { return m_isVisible; }
	void IsVisible(bool value);

private:
	enum class ListViewItemMenuItem
	{
		Copy,
		CopyName,
		CopyDescription,
		CopyDeviceId,
		CopyTimestamp,
	};

	static void RegisterWindowClass();
	void CreateTrayIconMenu();
	void CreateListViewItemMenu();
	void OnOpenMenuItemClicked();
	void CreateControls(HINSTANCE instance);
	void ResizeProcessListView();
	void OnUsbEventAdded(UsbEvent usbEvent);
	void OnListViewNotify(LPARAM const lparam);
	void WriteUsbEventData(std::wostream& stream, UsbEvent const& usbEvent, UsbEventColumn const& column);
	void WriteUsbEventData(std::wostream& stream, UsbEvent const& usbEvent);
	void CopyStringToClipboard(std::wstring const& string);
	winrt::fire_and_forget ShowAbout();

private:
	bool m_isVisible = false;
	std::unique_ptr<PopupMenu> m_trayIconMenu;
	HWND m_usbEventsListView = nullptr;
	std::vector<UsbEventColumn> m_columns;
	RingList<UsbEvent> m_usbEvents;
	std::unique_ptr<UsbEventWatcher> m_usbEventWatcher;
	winrt::Windows::System::DispatcherQueue m_dispatcherQueue{ nullptr };
	winrt::Windows::Globalization::DateTimeFormatting::DateTimeFormatter m_timestampFormatter{ nullptr };
	std::unique_ptr<SyncPopupMenu<ListViewItemMenuItem>> m_eventItemMenu;
	wil::unique_hmenu m_mainMenu;
};