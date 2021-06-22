#include "pch.h"

namespace winrt
{
    using namespace winrt::Windows::UI::Xaml;
    using namespace winrt::Windows::UI::Xaml::Controls;
    using namespace winrt::Windows::UI::Xaml::Hosting;
    using namespace winrt::Windows::UI::Xaml::Input;
    using namespace winrt::Windows::UI::Xaml::Markup;
}

PCWSTR contentText = LR"(
<StackPanel xmlns = "http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    Margin = "20">
    <Rectangle Fill = "Red" Width = "100" Height = "100" Margin = "5" />
    <Rectangle Fill = "Blue" Width = "100" Height = "100" Margin = "5" />
    <Rectangle Fill = "Green" Width = "100" Height = "100" Margin = "5" />
    <Rectangle Fill = "Purple" Width = "100" Height = "100" Margin = "5" />
    <TextBlock TextAlignment="Center">Multi-Window App, Click on a box to create a new window</TextBlock>
</StackPanel>
)";

struct AppWindow
{
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

    void RegisterWindowClass()
    {
        WNDCLASSEXW wcex{ sizeof(wcex) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = [](HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept -> LRESULT
        {
            if (message == WM_NCCREATE)
            {
                auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
                auto that = static_cast<AppWindow*>(cs->lpCreateParams);
                that->m_window.reset(window); // take ownership
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
            }
            else if (message == WM_NCDESTROY)
            {
                SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            }
            else if (auto that = reinterpret_cast<AppWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA)))
            {
                return that->MessageHandler(message, wparam, lparam);
            }

            return DefWindowProcW(window, message, wparam, lparam);
        };
        wcex.hInstance = wil::GetModuleInstanceHandle();
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.lpszClassName = WindowClassName;

        RegisterClassExW(&wcex);
    }

    LRESULT OnCreate()
    {
        // WindowsXamlManager must be used if multiple islands are created on the thread
        // and it must be constructed before the first DesktopWindowXamlSource.
        m_xamlManager = winrt::WindowsXamlManager::InitializeForCurrentThread();
        m_xamlSource = winrt::DesktopWindowXamlSource();

        auto interop = m_xamlSource.as<IDesktopWindowXamlSourceNative>();
        THROW_IF_FAILED(interop->AttachToWindow(m_window.get()));
        THROW_IF_FAILED(interop->get_WindowHandle(&m_xamlSourceWindow));

        auto content = winrt::XamlReader::Load(contentText).as<winrt::UIElement>();

        m_pointerPressedRevoker = content.PointerPressed(winrt::auto_revoke, [](const auto&, const winrt::PointerRoutedEventArgs& args)
        {
            auto poitnerId = args.Pointer().PointerId();

            std::thread([]()
            {
                auto coInit = wil::CoInitializeEx(COINIT_APARTMENTTHREADED);
                std::make_unique<AppWindow>()->Show(SW_SHOWNORMAL);
            }).detach();
        });
        
        m_xamlSource.Content(content);

        return 0;
    }

    LRESULT OnSize(WPARAM /* SIZE_XXX */wparam, LPARAM /* x, y */ lparam)
    {
        auto dx = LOWORD(lparam);
        auto dy = HIWORD(lparam);
        SetWindowPos(m_xamlSourceWindow, nullptr, 0, 0, dx, dy, SWP_SHOWWINDOW);
        return 0;
    }

    LRESULT OnDestroy()
    {
        PostQuitMessage(0);
        return 0;
    }

    void Show(int nCmdShow)
    {
        RegisterWindowClass();

        auto hwnd = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, WindowClassName, L"Win32 Xaml App", WS_OVERLAPPEDWINDOW,
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
            if (!TranslateAcceleratorW(msg.hwnd, nullptr, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        // In this use scenario we need to work around http://task.ms/33900412 (XamlCore leaked)
        // Verify Windows.UI.Xaml.dll!DirectUI::WindowsXamlManager instance counters are 
        // zero or that Windows.UI.Xaml.dll!DirectUI::WindowsXamlManager::XamlCore::Close is called.
        m_xamlSource.Close();

        // Drain the message queue since Xaml rundown is async, verify with a break point
        // on Windows.UI.Xaml.dll!DirectUI::WindowsXamlManager::XamlCore::Close.
        // http://task.ms/33731494
        // m_xamlManager = nullptr;

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::DispatchMessageW(&msg);
        }
    }

    const PCWSTR WindowClassName = L"Win32XamlAppWindow";
    wil::unique_hwnd m_window;
    HWND m_xamlSourceWindow{}; // This is owned by m_xamlSource, destroyed when Close() is called.

    winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_xamlManager{ nullptr };
    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };

    winrt::Windows::UI::Xaml::UIElement::PointerPressed_revoker m_pointerPressedRevoker;
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    auto coInit = wil::CoInitializeEx(COINIT_APARTMENTTHREADED);

    std::make_unique<AppWindow>()->Show(nCmdShow);

    return 0;
}
