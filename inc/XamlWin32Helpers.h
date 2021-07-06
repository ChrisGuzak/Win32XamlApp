#pragma once
#include <winuser.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>

inline const wchar_t* MakeIntResourceId(int i)
{
    return reinterpret_cast<const wchar_t*>(static_cast<ULONG_PTR>(i));
}

#define XAMLRESOURCE  256 // see resource.h, try to get rid of this by using a standard "file" type.

template<typename T = winrt::Windows::UI::Xaml::UIElement>
winrt::Windows::UI::Xaml::UIElement LoadXamlResource(HMODULE mod, uint32_t id, const wchar_t* type = MakeIntResourceId(XAMLRESOURCE))
{
    auto rc = ::FindResourceW(mod, MakeIntResourceId(id), type);
    winrt::check_bool(rc != nullptr);
    HGLOBAL rcData = ::LoadResource(mod, rc);
    winrt::check_bool(rcData != nullptr);
    auto textStart = static_cast<wchar_t*>(::LockResource(rcData));
    auto size = SizeofResource(mod, rc);
    winrt::hstring text{ textStart, size / sizeof(*textStart) }; // need a copy to null terminate, see if this can be avoided
    return winrt::Windows::UI::Xaml::Markup::XamlReader::Load(text).as<T>();
}

template <typename T>
void RegisterWindowClass(const wchar_t* windowClassName)
{
    WNDCLASSEXW wcex{ sizeof(wcex) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = [](HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept -> LRESULT
    {
        if (message == WM_NCCREATE)
        {
            auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
            auto that = static_cast<T*>(cs->lpCreateParams);
            that->m_window.reset(window); // take ownership
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
        }
        else if (message == WM_NCDESTROY)
        {
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        }
        else if (auto that = reinterpret_cast<T*>(GetWindowLongPtrW(window, GWLP_USERDATA)))
        {
            return that->MessageHandler(message, wparam, lparam);
        }

        return DefWindowProcW(window, message, wparam, lparam);
    };
    wcex.hInstance = wil::GetModuleInstanceHandle();
    auto imageResMod = LoadLibraryExW(L"imageres.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_AS_DATAFILE);
    wcex.hIcon = LoadIconW(imageResMod, reinterpret_cast<const wchar_t*>(5206)); // App Icon
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.lpszClassName = windowClassName;

    RegisterClassExW(&wcex);
}

