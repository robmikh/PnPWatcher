#include "pch.h"
#include "MainWindow.h"

namespace winrt
{
    using namespace Windows::System;
    using namespace Windows::Globalization::DateTimeFormatting;
}

const std::wstring MainWindow::ClassName = L"UsbWatcher.MainWindow";
std::once_flag MainWindowClassRegistration;

#define ID_LISTVIEW  2000 // ?????

void MainWindow::RegisterWindowClass()
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIconW(instance, MAIN_ICON);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = ClassName.c_str();
    wcex.hIconSm = LoadIconW(instance, MAIN_ICON);
    winrt::check_bool(RegisterClassExW(&wcex));
}

MainWindow::MainWindow(std::wstring const& titleString, int width, int height)
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    m_dispatcherQueue = winrt::DispatcherQueue::GetForCurrentThread();
    m_timestampFormatter = winrt::DateTimeFormatter(L"longtime");

    m_columns = { UsbEventColumn::Type, UsbEventColumn::Name, UsbEventColumn::DeviceId, UsbEventColumn::Timestamp };

    std::call_once(MainWindowClassRegistration, []() { RegisterWindowClass(); });

    winrt::check_bool(CreateWindowExW(0, ClassName.c_str(), titleString.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    CreateTrayIconMenu();
    CreateControls(instance);

    m_usbEventWatcher = std::make_unique<UsbEventWatcher>(
        m_dispatcherQueue, 
        std::bind(&MainWindow::OnUsbEventAdded, this, std::placeholders::_1));
}

LRESULT MainWindow::MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam)
{
    switch (message)
    {
    case WM_CLOSE:
        // We don't want to destroy the window, just hide it.
        IsVisible(false);
        break;
    case WM_DESTROY:
        // TODO: PostQuitMessage? Does it matter with this configuration?
        break;
    case TrayIconMessage:
    {
        // We only have one icon, so we won't check the id.
        auto iconMessage = LOWORD(lparam);
        auto x = GET_X_LPARAM(wparam);
        auto y = GET_Y_LPARAM(wparam);
        switch (iconMessage)
        {
        case WM_CONTEXTMENU:
            m_trayIconMenu->ShowMenu(m_window, x, y);
            break;
        }
    }
        break;
    case WM_MENUCOMMAND:
        return m_trayIconMenu->MessageHandler(wparam, lparam);
    case WM_NOTIFY:
        OnListViewNotify(lparam);
        break;
    case WM_SIZE:
        ResizeProcessListView();
        break;
    default:
        return base_type::MessageHandler(message, wparam, lparam);
    }
    return 0;
}

void MainWindow::IsVisible(bool value)
{
    if (value != m_isVisible)
    {
        m_isVisible = value;
        auto showWindowFlag = m_isVisible ? SW_SHOW : SW_HIDE;
        ShowWindow(m_window, showWindowFlag);
        UpdateWindow(m_window);
    }
}

void MainWindow::CreateTrayIconMenu()
{
    m_trayIconMenu = std::make_unique<PopupMenu>();
    m_trayIconMenu->AppendMenuItem(L"Open", std::bind(&MainWindow::OnOpenMenuItemClicked, this));
    m_trayIconMenu->AppendMenuItem(L"Exit", []() { PostQuitMessage(0); });
}

void MainWindow::OnOpenMenuItemClicked()
{
    IsVisible(true);
}

void MainWindow::CreateControls(HINSTANCE instance)
{
    auto style = WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_AUTOARRANGE | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | LVS_SINGLESEL;

    m_usbEventsListView = winrt::check_pointer(CreateWindowExW(
        0, //WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        L"",
        style,
        0, 0, 0, 0,
        m_window,
        (HMENU)ID_LISTVIEW,
        instance,
        nullptr));

    // Setup columns
    {
        LVCOLUMNW listViewColumn = {};
        std::vector<std::wstring> columnNames;
        for (auto&& column : m_columns)
        {
            std::wstringstream stream;
            stream << column;
            columnNames.push_back(stream.str());
        }

        listViewColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        listViewColumn.fmt = LVCFMT_LEFT;
        listViewColumn.cx = 120;
        for (auto i = 0; i < columnNames.size(); i++)
        {
            listViewColumn.pszText = columnNames[i].data();
            ListView_InsertColumn(m_usbEventsListView, i, &listViewColumn);
        }
        ListView_SetExtendedListViewStyle(m_usbEventsListView, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    }

    ResizeProcessListView();
}

void MainWindow::ResizeProcessListView()
{
    if (m_usbEventsListView)
    {
        RECT rect = {};
        winrt::check_bool(GetClientRect(m_window, &rect));
        winrt::check_bool(MoveWindow(
            m_usbEventsListView,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            true));
    }
}

void MainWindow::OnUsbEventAdded(UsbEvent usbEvent)
{
    auto newIndex = m_usbEvents.size();
    LVITEMW item = {};
    item.iItem = static_cast<int>(newIndex);
    m_usbEvents.push_back(std::move(usbEvent));
    ListView_InsertItem(m_usbEventsListView, &item);
}

void MainWindow::WriteUsbEventData(std::wostream& stream, UsbEvent const& usbEvent, UsbEventColumn const& column)
{
    switch (column)
    {
    case UsbEventColumn::Type:
        stream << usbEvent.Type;
        break;
    case UsbEventColumn::Name:
        stream << usbEvent.Name;
        break;
    case UsbEventColumn::DeviceId:
        stream << usbEvent.DeviceId;
        break;
    case UsbEventColumn::Timestamp:
        stream << std::wstring_view(m_timestampFormatter.Format(usbEvent.Timestamp));
        break;
    }
}

void MainWindow::OnListViewNotify(LPARAM const lparam)
{
    auto  lpnmh = reinterpret_cast<LPNMHDR>(lparam);
    //auto listView = winrt::check_pointer(GetDlgItem(m_window, ID_LISTVIEW));

    switch (lpnmh->code)
    {
    case LVN_GETDISPINFOW:
    {
        auto itemDisplayInfo = reinterpret_cast<NMLVDISPINFOW*>(lparam);
        auto itemIndex = itemDisplayInfo->item.iItem;
        auto subItemIndex = itemDisplayInfo->item.iSubItem;
        if (subItemIndex == 0)
        {
            if (itemDisplayInfo->item.mask & LVIF_TEXT)
            {
                auto& usbEvent = m_usbEvents[itemIndex];
                std::wstringstream stream;
                WriteUsbEventData(stream, usbEvent, m_columns[0]);
                auto string = stream.str();
                wcsncpy_s(itemDisplayInfo->item.pszText, itemDisplayInfo->item.cchTextMax, string.data(), _TRUNCATE);
            }
        }
        else if (subItemIndex > 0)
        {
            if (itemDisplayInfo->item.mask & LVIF_TEXT)
            {
                auto& usbEvent = m_usbEvents[itemIndex];
                auto& column = m_columns[subItemIndex];
                std::wstringstream stream;
                WriteUsbEventData(stream, usbEvent, column);
                auto string = stream.str();
                wcsncpy_s(itemDisplayInfo->item.pszText, itemDisplayInfo->item.cchTextMax, string.data(), _TRUNCATE);
            }
        }
    }
    break;
    }
}