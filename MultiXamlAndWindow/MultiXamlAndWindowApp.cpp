#include "pch.h"
#include <win32app/XamlWin32Helpers.h>
#include <win32app/reference_waiter.h>
#include <win32app/win32_app_helpers.h>

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
    LRESULT Create()
    {
        using namespace winrt::Windows::UI::Xaml;
        using namespace winrt::Windows::UI::Xaml::Controls;
        using namespace winrt::Windows::UI::Xaml::Input;

        m_xamlSource1 = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();

        auto interop = m_xamlSource1.as<IDesktopWindowXamlSourceNative>();
        THROW_IF_FAILED(interop->AttachToWindow(m_window.get()));
        THROW_IF_FAILED(interop->get_WindowHandle(&m_xamlSourceWindow1));

        auto content = winrt::Windows::UI::Xaml::Markup::XamlReader::Load(contentText).as<winrt::Windows::UI::Xaml::UIElement>();

        m_xamlSource1.Content(content);

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
                std::make_unique<AppWindow>()->Show(SW_SHOWNORMAL);
            });
        });

        m_xamlSource2 = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();
        interop = m_xamlSource2.as<IDesktopWindowXamlSourceNative>();
        THROW_IF_FAILED(interop->AttachToWindow(m_window.get()));
        THROW_IF_FAILED(interop->get_WindowHandle(&m_xamlSourceWindow2));

        m_xamlSource2.Content(winrt::Windows::UI::Xaml::Markup::XamlReader::Load(contentText).as<UIElement>());

        return 0;
    }

    LRESULT Size(WORD dx, WORD dy)
    {
        SetWindowPos(m_xamlSourceWindow1, nullptr, 0, 0, dx / 2, dy, SWP_SHOWWINDOW);
        SetWindowPos(m_xamlSourceWindow2, nullptr, dx / 2, 0, dx / 2, dy, SWP_SHOWWINDOW);
        return 0;
    }

    LRESULT Destroy()
    {
        // Since the xaml rundown is async and requires message dispatching,
        // run it down here while the message loop is still running.
        // Work around http://task.ms/33900412, to be fixed

        // In this use scenario we need to work around http://task.ms/33900412 (XamlCore leaked)
        // Verify Windows.UI.Xaml.dll!DirectUI::WindowsXamlManager::XamlCore::Close is called
        // in the debugger.
        m_xamlSource1.Close(); // Either order here works, but there is an order related bug
        m_xamlSource2.Close();

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

    wil::unique_hwnd m_window;
    HWND m_xamlSourceWindow1{}; // This is owned by m_xamlSource, destroyed when Close() is called.
    HWND m_xamlSourceWindow2{}; // This is owned by m_xamlSource, destroyed when Close() is called.

    // This is needed to coordinate the use of Xaml from multiple threads.
    winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_xamlManager = winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource1{ nullptr };
    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource2{ nullptr };

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
