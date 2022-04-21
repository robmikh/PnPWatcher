#pragma once
#include <robmikh.common/DesktopWindow.h>
#include "PopupMenu.h"
#include "PnPEvent.h"
#include "PnPEventWatcher.h"
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

	static const std::wstring CopyDelim;
	static void RegisterWindowClass();
	void CreateTrayIconMenu();
	void CreateListViewItemMenu();
	void OnOpenMenuItemClicked();
	void CreateControls(HINSTANCE instance);
	void ResizeProcessListView();
	void OnPnPEventAdded(PnPEvent pnpEvent);
	void OnListViewNotify(LPARAM const lparam);
	void WritePnPEventData(std::wostream& stream, PnPEvent const& pnpEvent, PnPEventColumn const& column);
	void WritePnPEventData(std::wostream& stream, PnPEvent const& pnpEvent, std::wstring const& delim);
	void CopyStringToClipboard(std::wstring const& string);
	winrt::fire_and_forget ShowAbout();
	winrt::Windows::Foundation::IAsyncAction ExportToCsvAsync();

private:
	bool m_isVisible = false;
	std::unique_ptr<PopupMenu> m_trayIconMenu;
	HWND m_pnpEventsListView = nullptr;
	std::vector<PnPEventColumn> m_columns;
	RingList<PnPEvent> m_pnpEvents;
	std::unique_ptr<PnPEventWatcher> m_pnpEventWatcher;
	winrt::Windows::System::DispatcherQueue m_dispatcherQueue{ nullptr };
	winrt::Windows::Globalization::DateTimeFormatting::DateTimeFormatter m_timestampFormatter{ nullptr };
	std::unique_ptr<SyncPopupMenu<ListViewItemMenuItem>> m_eventItemMenu;
	wil::unique_hmenu m_mainMenu;
};