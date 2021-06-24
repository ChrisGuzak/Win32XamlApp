#include "pch.h"

namespace winrt
{
    using namespace winrt::Windows::UI::Xaml;
    using namespace winrt::Windows::UI::Xaml::Controls;
    using namespace winrt::Windows::UI::Xaml::Hosting;
    using namespace winrt::Windows::UI::Xaml::Input;
    using namespace winrt::Windows::UI::Xaml::Markup;
}

const PCWSTR contentText = LR"(
<StackPanel
    xmlns = "http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'  
    Margin = "20">
    <Rectangle Fill = "Red" Width = "100" Height = "100" Margin = "5" />
    <Rectangle Fill = "Blue" Width = "100" Height = "100" Margin = "5" />
    <Rectangle Fill = "Green" Width = "100" Height = "100" Margin = "5" />
    <Rectangle Fill = "Purple" Width = "100" Height = "100" Margin = "5" />
    <TextBlock TextAlignment="Center">Multi-Window App, Click on a box to create a new window</TextBlock>
    <TextBlock x:Name="Status" TextAlignment="Left"></TextBlock>
</StackPanel>
)";

struct AppWindow
{
    AppWindow(bool rightClickLaunch = false) : m_rightClickLaunch(rightClickLaunch)
    {
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
        // WindowsXamlManager must be used if multiple islands are created on the thread or in the process.
        // It must be constructed before the first DesktopWindowXamlSource.
        m_xamlSource = winrt::DesktopWindowXamlSource();

        auto interop = m_xamlSource.as<IDesktopWindowXamlSourceNative>();
        THROW_IF_FAILED(interop->AttachToWindow(m_window.get()));
        THROW_IF_FAILED(interop->get_WindowHandle(&m_xamlSourceWindow));

        auto content = winrt::XamlReader::Load(contentText).as<winrt::UIElement>();

        m_xamlSource.Content(content);

        m_status = content.as<winrt::FrameworkElement>().FindName(L"Status").as<winrt::TextBlock>();

        m_rootChangedRevoker = content.XamlRoot().Changed(winrt::auto_revoke, [this](const auto& sender, const winrt::XamlRootChangedEventArgs& args)
        {
            auto scale = sender.RasterizationScale();
            auto visible = sender.IsHostVisible();

            m_status.Text(std::to_wstring(scale));
        });

        m_pointerPressedRevoker = content.PointerPressed(winrt::auto_revoke, [](const auto& sender, const winrt::PointerRoutedEventArgs& args)
        {
            const bool isRightClick = args.GetCurrentPoint(sender.as<winrt::UIElement>()).Properties().IsRightButtonPressed();

            StartAppThread([isRightClick]()
            {
                auto coInit = wil::CoInitializeEx(COINIT_APARTMENTTHREADED);
                std::make_unique<AppWindow>(isRightClick)->Show(SW_SHOWNORMAL);
            });
        });

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
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // In this use scenario we need to work around http://task.ms/33900412 (XamlCore leaked)
        // Verify Windows.UI.Xaml.dll!DirectUI::WindowsXamlManager instance counters are 
        // zero or that Windows.UI.Xaml.dll!DirectUI::WindowsXamlManager::XamlCore::Close is called.
        m_xamlSource.Close();
    }

    template <typename Lambda>
    static void StartAppThread(Lambda&& fn)
    {
        std::unique_lock<std::mutex> holdLock(m_lock);
        m_threads.emplace_back([fn = std::forward<Lambda>(fn), threadRef = m_appThreadsWaiter.take_reference()]() mutable
        {
            std::forward<Lambda>(fn)();
        });
    }

    inline static reference_waiter m_appThreadsWaiter;
    inline static std::mutex m_lock;
    inline static std::vector<std::thread> m_threads;

    const PCWSTR WindowClassName = L"Win32XamlAppWindow";
    bool m_rightClickLaunch{};
    wil::unique_hwnd m_window;
    HWND m_xamlSourceWindow{}; // This is owned by m_xamlSource, destroyed when Close() is called.

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };

    winrt::Windows::UI::Xaml::Controls::TextBlock m_status{ nullptr };

    winrt::Windows::UI::Xaml::UIElement::PointerPressed_revoker m_pointerPressedRevoker;
    winrt::Windows::UI::Xaml::XamlRoot::Changed_revoker m_rootChangedRevoker;
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    auto coInit = wil::CoInitializeEx();

    AppWindow::StartAppThread([nCmdShow]()
    {
        auto coInit = wil::CoInitializeEx(COINIT_APARTMENTTHREADED);
        std::make_unique<AppWindow>()->Show(nCmdShow);
    });

    AppWindow::m_appThreadsWaiter.wait_until_zero();

    // now we are safe to iterate over m_threads as it can't change any more
    for (auto& thread : AppWindow::m_threads)
    {
        thread.join();
    }

    return 0;
}
