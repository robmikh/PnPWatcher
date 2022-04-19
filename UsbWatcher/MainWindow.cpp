#include "pch.h"
#include "MainWindow.h"

namespace winrt
{
    using namespace Windows::ApplicationModel::DataTransfer;
    using namespace Windows::Foundation;
    using namespace Windows::Globalization::DateTimeFormatting;
    using namespace Windows::System;
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
    m_timestampFormatter = winrt::DateTimeFormatter(L"year.full month.numeric day hour minute second timezone.abbreviated");

    size_t maxEvents = 100;
#ifdef _DEBUG
    maxEvents = 3;
#endif
    m_usbEvents = RingList<UsbEvent>(maxEvents);

    m_columns = 
    { 
        UsbEventColumn::Type, 
        UsbEventColumn::Name,
        UsbEventColumn::Description,
        UsbEventColumn::DeviceId, 
        UsbEventColumn::Timestamp 
    };

    std::call_once(MainWindowClassRegistration, []() { RegisterWindowClass(); });

    winrt::check_bool(CreateWindowExW(0, ClassName.c_str(), titleString.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    CreateTrayIconMenu();
    CreateListViewItemMenu();
    CreateControls(instance);

    m_usbEventWatcher = std::make_unique<UsbEventWatcher>(
        m_dispatcherQueue, 
        std::bind(&MainWindow::OnUsbEventAdded, this, std::placeholders::_1));

#ifdef _DEBUG
    for (auto i = 0; i < maxEvents + 2; i++)
    {
        std::wstringstream stream;
        stream << L"Debug Name (" << i << L")";
        auto name = stream.str();
        OnUsbEventAdded(
            {
                UsbEventType::Added,
                name,
                L"Debug Description",
                L"Debug DeviceId",
                winrt::clock::now(),
            });
    }
    IsVisible(true);
#endif
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

void MainWindow::CreateListViewItemMenu()
{
    std::vector<SyncPopupMenu<ListViewItemMenuItem>::MenuItem> items =
    {
        { L"Copy", ListViewItemMenuItem::Copy },
        { L"Copy Name", ListViewItemMenuItem::CopyName },
        { L"Copy Description", ListViewItemMenuItem::CopyDescription },
        { L"Copy DeviceId", ListViewItemMenuItem::CopyDeviceId },
        { L"Copy Timestamp", ListViewItemMenuItem::CopyTimestamp },
    };
    m_eventItemMenu = std::make_unique<SyncPopupMenu<ListViewItemMenuItem>>(items);
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
    auto newIndex = m_usbEvents.Size();
    LVITEMW item = {};
    item.iItem = static_cast<int>(newIndex);
    auto adjusted = m_usbEvents.Add(std::move(usbEvent));
    ListView_InsertItem(m_usbEventsListView, &item);
    if (adjusted)
    {
        ListView_DeleteItem(m_usbEventsListView, 0);
        ListView_RedrawItems(m_usbEventsListView, 0, m_usbEvents.Size() - 1);
        UpdateWindow(m_usbEventsListView);
    }
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
    case UsbEventColumn::Description:
        stream << usbEvent.Description;
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
    case NM_RCLICK:
    {
        auto itemInfo = reinterpret_cast<NMITEMACTIVATE*>(lparam);
        auto itemIndex = itemInfo->iItem;
        if (itemIndex >= 0)
        {
            auto point = itemInfo->ptAction;
            winrt::check_bool(ClientToScreen(m_usbEventsListView, &point));
            auto result = m_eventItemMenu->ShowMenu(m_window, point.x, point.y);
            if (result.has_value())
            {
                const auto& usbEvent = m_usbEvents[itemIndex];
                std::wstringstream stream;
                switch (result.value())
                {
                case ListViewItemMenuItem::Copy:
                    WriteUsbEventData(stream, usbEvent);
                    break;
                case ListViewItemMenuItem::CopyName:
                    WriteUsbEventData(stream, usbEvent, UsbEventColumn::Name);
                    break;
                case ListViewItemMenuItem::CopyDescription:
                    WriteUsbEventData(stream, usbEvent, UsbEventColumn::Description);
                    break;
                case ListViewItemMenuItem::CopyDeviceId:
                    WriteUsbEventData(stream, usbEvent, UsbEventColumn::DeviceId);
                    break;
                case ListViewItemMenuItem::CopyTimestamp:
                    WriteUsbEventData(stream, usbEvent, UsbEventColumn::Timestamp);
                    break;
                }
                CopyStringToClipboard(stream.str());
            }
        }
    }
        break;
    }
}

void MainWindow::WriteUsbEventData(std::wostream& stream, UsbEvent const& usbEvent)
{
    for (auto i = 0; i < m_columns.size(); i++)
    {
        const auto& column = m_columns[i];
        WriteUsbEventData(stream, usbEvent, column);
        if (i != m_columns.size() - 1)
        {
            stream << L"  |  ";
        }
    }
}

void MainWindow::CopyStringToClipboard(std::wstring const& string)
{
    auto dataPackage = winrt::DataPackage();
    dataPackage.RequestedOperation(winrt::DataPackageOperation::Copy);
    dataPackage.SetText(string);
    winrt::Clipboard::SetContent(dataPackage);
}