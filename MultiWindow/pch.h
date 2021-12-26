#pragma once

#include <wil/cppwinrt.h>
#include <wil/com.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef GetCurrentTime

#include <wil/win32_helpers.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include <winrt/Windows.UI.Xaml.Markup.h>

#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h> // COM interop

#include <wil/resource.h>
