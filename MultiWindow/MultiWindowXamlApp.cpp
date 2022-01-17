#include "pch.h"
#include "resource.h"
#include <win32app/XamlWin32Helpers.h>
#include <win32app/win32_app_helpers.h>
#include <win32app/reference_waiter.h>

struct AppWindow
{
    AppWindow(bool rightClickLaunch = false) : m_rightClickLaunch(rightClickLaunch)
    {
    }

    LRESULT Create()
    {
        using namespace winrt::Windows::UI::Xaml;
        using namespace winrt::Windows::UI::Xaml::Controls;
        using namespace winrt::Windows::UI::Xaml::Input;

        // WindowsXamlManager must be used if multiple islands are created on the thread or in the process.
        // It must be constructed before the first DesktopWindowXamlSource.
        m_xamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();

        auto interop = m_xamlSource.as<IDesktopWindowXamlSourceNative>();
        THROW_IF_FAILED(interop->AttachToWindow(m_window.get()));
        THROW_IF_FAILED(interop->get_WindowHandle(&m_xamlSourceWindow));

        auto content = LoadXamlResource(nullptr, IDR_APP_XAML);

        m_xamlSource.Content(content);

        m_status = content.as<FrameworkElement>().FindName(L"Status").as<TextBlock>();

        m_rootChangedRevoker = content.XamlRoot().Changed(winrt::auto_revoke, [this](auto&& sender, auto&& args)
        {
            auto scale = sender.RasterizationScale();
            auto visible = sender.IsHostVisible();

            m_status.Text(std::to_wstring(scale));
        });

        m_pointerPressedRevoker = content.PointerPressed(winrt::auto_revoke, [](auto&& sender, auto&& args)
        {
            const bool isRightClick = args.GetCurrentPoint(sender.as<UIElement>()).Properties().IsRightButtonPressed();

            StartThread([isRightClick]()
            {
                auto coInit = wil::CoInitializeEx(COINIT_APARTMENTTHREADED);
                std::make_unique<AppWindow>(isRightClick)->Show(SW_SHOWNORMAL);
            });
        });

        return 0;
    }

    LRESULT Size(WORD dx, WORD dy)
    {
        SetWindowPos(m_xamlSourceWindow, nullptr, 0, 0, dx, dy, SWP_SHOWWINDOW);
        return 0;
    }

    LRESULT Destroy()
    {
        // Since the xaml rundown is async and requires message dispatching,
        // run it down here while the message loop is still running.
        // Work around http://task.ms/33900412, to be fixed
        m_xamlSource.Close();
        PostQuitMessage(0);
        return 0;
    }

    void Show(int nCmdShow)
    {
        win32app::create_top_level_window_for_xaml(*this, L"Win32XamlAppWindow", L"Win32 Xaml App");
        win32app::enter_simple_message_loop(*this, nCmdShow);
    }

    template <typename Lambda>
    static void StartThread(Lambda&& fn)
    {
        std::unique_lock<std::mutex> holdLock(m_lock);
        m_threads.emplace_back([fn = std::forward<Lambda>(fn), threadRef = m_appThreadsWaiter.take_reference()]() mutable
        {
            std::forward<Lambda>(fn)();
        });
    }

    static void WaitUntilAllWindowsAreClosed()
    {
        m_appThreadsWaiter.wait_until_zero();

        // now we are safe to iterate over m_threads as it can't change any more
        for (auto& thread : AppWindow::m_threads)
        {
            thread.join();
        }
    }

    inline static reference_waiter m_appThreadsWaiter;
    inline static std::mutex m_lock;
    inline static std::vector<std::thread> m_threads;

    bool m_rightClickLaunch{};
    wil::unique_hwnd m_window;
    HWND m_xamlSourceWindow{}; // This is owned by m_xamlSource, destroyed when Close() is called.

    // This is needed to coordinate the use of Xaml from multiple threads.
    winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_xamlManager = winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };

    winrt::Windows::UI::Xaml::Controls::TextBlock m_status{ nullptr };

    winrt::Windows::UI::Xaml::UIElement::PointerPressed_revoker m_pointerPressedRevoker;
    winrt::Windows::UI::Xaml::XamlRoot::Changed_revoker m_rootChangedRevoker;
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    auto coInit = wil::CoInitializeEx();

    AppWindow::StartThread([nCmdShow]()
    {
        auto coInit = wil::CoInitializeEx(COINIT_APARTMENTTHREADED);
        std::make_unique<AppWindow>()->Show(nCmdShow);
    });

    AppWindow::WaitUntilAllWindowsAreClosed();
    return 0;
}
