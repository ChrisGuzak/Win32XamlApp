#pragma once
#include <combaseapi.h>

// Proposed addition to Xaml Islands to get the HWNDs in the graph of objects.

DECLARE_INTERFACE_IID_(IXamlRootInterop, IUnknown, "68601abf-23cc-4017-a946-fb25c574fbab")
{
    virtual HRESULT STDMETHODCALLTYPE GetHostingChidWindow(_Out_ HWND * childHwnd) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTopLevelWindow(_Out_ HWND * topLevelHwnd) = 0;
};

/* Example usage:

    HWND ownerWindow{};
    page.as<xaml::UIElement>().XamlRoot().as<IXamlRootInterop>()->GetTopLevelWindow(&ownerWindow);
    MessageBoxW(ownerWindow, L"Hello from XamlControl!", L"XamlControl", MB_OK);
*/


