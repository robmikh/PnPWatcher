#include "pch.h"
#include "MainWindow.h"

namespace winrt
{
    using namespace Windows::ApplicationModel::DataTransfer;
    using namespace Windows::Foundation;
    using namespace Windows::Globalization::DateTimeFormatting;
    using namespace Windows::Storage;
    using namespace Windows::Storage::Pickers;
    using namespace Windows::Storage::Streams;
    using namespace Windows::System;
    using namespace Windows::UI::Popups;
}

const std::wstring MainWindow::ClassName = L"PnPWatcher.MainWindow";
const std::wstring MainWindow::CopyDelim = L"  |  ";
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
    maxEvents = 10;
#endif
    m_pnpEvents = RingList<PnPEvent>(maxEvents);

    m_columns = 
    { 
        PnPEventColumn::Type, 
        PnPEventColumn::Name,
        PnPEventColumn::Description,
        PnPEventColumn::DeviceId, 
        PnPEventColumn::Timestamp 
    };

    std::call_once(MainWindowClassRegistration, []() { RegisterWindowClass(); });

    winrt::check_bool(CreateWindowExW(0, ClassName.c_str(), titleString.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    CreateTrayIconMenu();
    CreateListViewItemMenu();
    m_mainMenu.reset(winrt::check_pointer(LoadMenuW(instance, MAKEINTRESOURCEW(IDR_MAIN_WINDOW_MENU))));
    winrt::check_bool(SetMenu(m_window, m_mainMenu.get()));
    CreateControls(instance);

    m_pnpEventWatcher = std::make_unique<PnPEventWatcher>(
        m_dispatcherQueue, 
        std::bind(&MainWindow::OnPnPEventAdded, this, std::placeholders::_1));

#ifdef _DEBUG
    for (auto i = 0; i < maxEvents + 2; i++)
    {
        std::wstringstream stream;
        stream << L"Debug Name (" << i << L")";
        auto name = stream.str();
        OnPnPEventAdded(
            {
                PnPEventType::Added,
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
        if (auto result = m_trayIconMenu->MessageHandler(wparam, lparam))
        {
            return result.value();
        }
        break;
    case WM_NOTIFY:
        OnListViewNotify(lparam);
        break;
    case WM_SIZE:
        ResizeProcessListView();
        break;
    case WM_COMMAND:
    {
        switch (LOWORD(wparam))
        {
        case ID_EDIT_COPY:
        case ID_COPY_CMD:
        {
            auto index = ListView_GetSelectionMark(m_pnpEventsListView);
            if (index >= 0)
            {
                const auto& pnpEvent = m_pnpEvents[index];
                std::wstringstream stream;
                WritePnPEventData(stream, pnpEvent, CopyDelim);
                CopyStringToClipboard(stream.str());
            }
        }
            break;
        case ID_FILE_MINIMIZETOTRAY:
            IsVisible(false);
            break;
        case ID_FILE_EXIT:
            PostQuitMessage(0);
            break;
        case ID_EDIT_CLEAR:
        {
            m_pnpEvents.Clear();
            ListView_DeleteAllItems(m_pnpEventsListView);
        }
            break;
        case ID_TOOLS_EXPORTTOCSV:
            ExportToCsvAsync();
            break;
        case ID_HELP_ABOUT:
            ShowAbout();
            break;
        }
    }
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

    m_pnpEventsListView = winrt::check_pointer(CreateWindowExW(
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
            ListView_InsertColumn(m_pnpEventsListView, i, &listViewColumn);
        }
        ListView_SetExtendedListViewStyle(m_pnpEventsListView, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    }

    ResizeProcessListView();
}

void MainWindow::ResizeProcessListView()
{
    if (m_pnpEventsListView)
    {
        RECT rect = {};
        winrt::check_bool(GetClientRect(m_window, &rect));
        winrt::check_bool(MoveWindow(
            m_pnpEventsListView,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            true));
    }
}

void MainWindow::OnPnPEventAdded(PnPEvent pnpEvent)
{
    auto newIndex = m_pnpEvents.Size();
    LVITEMW item = {};
    item.iItem = static_cast<int>(newIndex);
    auto adjusted = m_pnpEvents.Add(std::move(pnpEvent));
    ListView_InsertItem(m_pnpEventsListView, &item);
    if (adjusted)
    {
        ListView_DeleteItem(m_pnpEventsListView, 0);
        ListView_RedrawItems(m_pnpEventsListView, 0, m_pnpEvents.Size() - 1);
        UpdateWindow(m_pnpEventsListView);
    }
}

void MainWindow::WritePnPEventData(std::wostream& stream, PnPEvent const& pnpEvent, PnPEventColumn const& column)
{
    switch (column)
    {
    case PnPEventColumn::Type:
        stream << pnpEvent.Type;
        break;
    case PnPEventColumn::Name:
        stream << pnpEvent.Name;
        break;
    case PnPEventColumn::Description:
        stream << pnpEvent.Description;
        break;
    case PnPEventColumn::DeviceId:
        stream << pnpEvent.DeviceId;
        break;
    case PnPEventColumn::Timestamp:
        stream << std::wstring_view(m_timestampFormatter.Format(pnpEvent.Timestamp));
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
                auto& pnpEvent = m_pnpEvents[itemIndex];
                std::wstringstream stream;
                WritePnPEventData(stream, pnpEvent, m_columns[0]);
                auto string = stream.str();
                wcsncpy_s(itemDisplayInfo->item.pszText, itemDisplayInfo->item.cchTextMax, string.data(), _TRUNCATE);
            }
        }
        else if (subItemIndex > 0)
        {
            if (itemDisplayInfo->item.mask & LVIF_TEXT)
            {
                auto& pnpEvent = m_pnpEvents[itemIndex];
                auto& column = m_columns[subItemIndex];
                std::wstringstream stream;
                WritePnPEventData(stream, pnpEvent, column);
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
            winrt::check_bool(ClientToScreen(m_pnpEventsListView, &point));
            auto result = m_eventItemMenu->ShowMenu(m_window, point.x, point.y);
            if (result.has_value())
            {
                const auto& pnpEvent = m_pnpEvents[itemIndex];
                std::wstringstream stream;
                switch (result.value())
                {
                case ListViewItemMenuItem::Copy:
                    WritePnPEventData(stream, pnpEvent, CopyDelim);
                    break;
                case ListViewItemMenuItem::CopyName:
                    WritePnPEventData(stream, pnpEvent, PnPEventColumn::Name);
                    break;
                case ListViewItemMenuItem::CopyDescription:
                    WritePnPEventData(stream, pnpEvent, PnPEventColumn::Description);
                    break;
                case ListViewItemMenuItem::CopyDeviceId:
                    WritePnPEventData(stream, pnpEvent, PnPEventColumn::DeviceId);
                    break;
                case ListViewItemMenuItem::CopyTimestamp:
                    WritePnPEventData(stream, pnpEvent, PnPEventColumn::Timestamp);
                    break;
                }
                CopyStringToClipboard(stream.str());
            }
        }
    }
        break;
    }
}

void MainWindow::WritePnPEventData(std::wostream& stream, PnPEvent const& pnpEvent, std::wstring const& delim)
{
    for (auto i = 0; i < m_columns.size(); i++)
    {
        const auto& column = m_columns[i];
        WritePnPEventData(stream, pnpEvent, column);
        if (i != m_columns.size() - 1)
        {
            stream << delim;
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

winrt::fire_and_forget MainWindow::ShowAbout()
{
    auto dialog = winrt::MessageDialog(L"PnPWatcher is an open source application written by Robert Mikhayelyan", L"About");
    auto commands = dialog.Commands();
    commands.Append(winrt::UICommand(L"Open GitHub", [](auto&) -> winrt::fire_and_forget
        {
            co_await winrt::Launcher::LaunchUriAsync(winrt::Uri(L"https://github.com/robmikh/PnPWatcher"));
        }));
    commands.Append(winrt::UICommand(L"Close"));

    dialog.DefaultCommandIndex(1);
    dialog.CancelCommandIndex(1);

    InitializeObjectWithWindowHandle(dialog);
    co_await dialog.ShowAsync();
}

winrt::IAsyncAction MainWindow::ExportToCsvAsync()
{
    // Write our data to a string
    std::wstringstream sstream;
    for (auto i = 0; i < m_columns.size(); i++)
    {
        const auto& column = m_columns[i];
        sstream << column;
        if (i != m_columns.size() - 1)
        {
            sstream << L",";
        }
    }
    sstream << std::endl;
    for (auto i = 0; i < m_pnpEvents.Size(); i++)
    {
        const auto& pnpEvent = m_pnpEvents[i];
        WritePnPEventData(sstream, pnpEvent, L",");
        sstream << std::endl;
    }
    auto text = sstream.str();

    auto picker = winrt::FileSavePicker();
    InitializeObjectWithWindowHandle(picker);
    picker.SuggestedStartLocation(winrt::PickerLocationId::PicturesLibrary);
    picker.SuggestedFileName(L"pnpEvents");
    picker.DefaultFileExtension(L".csv");
    picker.FileTypeChoices().Clear();
    picker.FileTypeChoices().Insert(L"CSV Data", winrt::single_threaded_vector<winrt::hstring>({ L".csv" }));
    auto file = co_await picker.PickSaveFileAsync();
    if (file != nullptr)
    {
        co_await winrt::FileIO::WriteTextAsync(file, text, winrt::UnicodeEncoding::Utf8);

        co_await m_dispatcherQueue;
        MessageBoxW(m_window, L"Export completed!", L"PnPWatcher", MB_OK | MB_APPLMODAL);
    }
}
