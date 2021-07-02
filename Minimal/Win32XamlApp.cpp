#include "pch.h"
#include <XamlWin32Helpers.h>

struct AppWindow
{
    LRESULT OnCreate()
    {
        m_xamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();

        auto interop = m_xamlSource.as<IDesktopWindowXamlSourceNative>();
        THROW_IF_FAILED(interop->AttachToWindow(m_window.get()));
        THROW_IF_FAILED(interop->get_WindowHandle(&m_xamlSourceWindow));

        // When this fails look in the debug output window, it shows the line and offset
        // that has the parsing problem.
        auto content = LoadXamlResource(nullptr, IDR_APP_XAML);

        m_pointerPressedRevoker = content.PointerPressed(winrt::auto_revoke, [](auto&&, auto&& args)
        {
            auto poitnerId = args.Pointer().PointerId();
        });
        
        m_xamlSource.Content(content);

        return 0;
    }

    LRESULT OnSize(WPARAM /* SIZE_XXX */wparam, LPARAM /* x, y */ lparam)
    {
        const auto dx = LOWORD(lparam), dy = HIWORD(lparam);
        SetWindowPos(m_xamlSourceWindow, nullptr, 0, 0, dx, dy, SWP_SHOWWINDOW);
        return 0;
    }

    LRESULT OnDestroy()
    {
        PostQuitMessage(0);
        return 0;
    }

    LRESULT MessageHandler(UINT message, WPARAM wparam, LPARAM lparam) noexcept
    {
        switch (message)
        {
        case WM_CREATE:
            return OnCreate();

        case WM_SIZE:
            return OnSize(wparam, lparam);

        case WM_DESTROY:
            return OnDestroy();
        }
        return DefWindowProcW(m_window.get(), message, wparam, lparam);
    }

    void Show(int nCmdShow)
    {
        const PCWSTR className = L"Win32XamlAppWindow";
        RegisterWindowClass<AppWindow>(className);

        auto hwnd = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, className, L"Win32 Xaml App", WS_OVERLAPPEDWINDOW,
           CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, wil::GetModuleInstanceHandle(), this);
        THROW_LAST_ERROR_IF(!hwnd);

        ShowWindow(m_window.get(), nCmdShow);
        UpdateWindow(m_window.get());
        MessageLoop();
    }

    void MessageLoop()
    {
        MSG msg = {};
        while (GetMessageW(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        m_xamlSource.Close(); // Work around http://task.ms/33900412, to be fixed
    }

    wil::unique_hwnd m_window;
    HWND m_xamlSourceWindow{}; // This is owned by m_xamlSource, destroyed when Close() is called.

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };

    winrt::Windows::UI::Xaml::UIElement::PointerPressed_revoker m_pointerPressedRevoker;
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    auto coInit = wil::CoInitializeEx(COINIT_APARTMENTTHREADED);
    std::make_unique<AppWindow>()->Show(nCmdShow);
    return 0;
}
