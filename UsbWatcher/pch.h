#pragma once

// Windows
#include <windows.h>
#include <windowsx.h>

// Must come before C++/WinRT
#include <wil/cppwinrt.h>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>

// WIL
#include <wil/resource.h>

// DirectX
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <wincodec.h>

// Common Controls
#include <CommCtrl.h>

// WMI
#include <Wbemidl.h>

// Shell
#include <shellapi.h>

// STL
#include <vector>
#include <string>
#include <atomic>
#include <memory>
#include <algorithm>
#include <mutex>
#include <functional>
#include <sstream>
#include <ostream>

// robmikh.common
#include <robmikh.common/dispatcherqueue.desktop.interop.h>
#include <robmikh.common/hwnd.interop.h>
#include <robmikh.common/DesktopWindow.h>

// Application resources
#include "resource.h"
#define MAIN_ICON MAKEINTRESOURCEW(IDI_MAINICON)

// Application helpers
#include "wmiHelpers.h"
