#include "pch.h"
#include "resource.h"
#include <win32app/XamlWin32Helpers.h>
#include <win32app/win32_app_helpers.h>

struct AppWindow : public std::enable_shared_from_this<AppWindow>
{
    AppWindow(winrt::Windows::System::DispatcherQueueController queueController, bool rightClickLaunch = false) :
        m_queueController(std::move(queueController)),
        m_rightClickLaunch(rightClickLaunch),
        m_xamlManager(winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread())
    {
        m_appWindowCount++;
    }

    ~AppWindow()
    {
        if (--m_appWindowCount == 0)
        {
            m_appDoneSignal.SetEvent();
        }
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

            StartThread([isRightClick](auto&& queueController)
            {
                auto appWindow = std::make_shared<AppWindow>(std::move(queueController), isRightClick);
                appWindow->Show(SW_SHOWNORMAL);
            });
        });

        return 0;
    }

    LRESULT Size(WORD dx, WORD dy)
    {
        SetWindowPos(m_xamlSourceWindow, nullptr, 0, 0, dx, dy, SWP_SHOWWINDOW);
        return 0;
    }

    winrt::fire_and_forget ShutdownAsync()
    {
        auto strongThis = std::move(m_selfRef);
        co_await m_queueController.ShutdownQueueAsync();
    }

    LRESULT Destroy()
    {
        // Since the xaml rundown is async and requires message dispatching,
        // start its run down here while the message loop is still running.
        m_xamlSource.Close();
        ShutdownAsync();
        return 0;
    }

    void Show(int nCmdShow)
    {
        win32app::create_top_level_window_for_xaml(*this, L"Win32XamlAppWindow", L"Win32 Xaml App");
        ShowWindow(m_window.get(), nCmdShow);
        m_selfRef = shared_from_this();
    }

    winrt::Windows::System::DispatcherQueue DispatcherQueue() const
    {
        return m_queueController.DispatcherQueue();
    }

    template <typename Lambda>
    static void StartThread(Lambda&& fn)
    {
        auto queueController = winrt::Windows::System::DispatcherQueueController::CreateOnDedicatedThread();

        queueController.DispatcherQueue().TryEnqueue(
            [fn = std::forward<Lambda>(fn), queueController = std::move(queueController)]() mutable
        {
            std::forward<Lambda>(fn)(std::move(queueController));
        });
    }

    inline static std::atomic<int> m_appWindowCount;
    inline static wil::unique_event m_appDoneSignal{ wil::EventOptions::None };

    bool m_rightClickLaunch{};
    wil::unique_hwnd m_window;
    HWND m_xamlSourceWindow{}; // This is owned by m_xamlSource, destroyed when Close() is called.

    std::shared_ptr<AppWindow> m_selfRef;

    // This is needed to coordinate the use of Xaml from multiple threads.
    winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_xamlManager{nullptr};

    winrt::Windows::System::DispatcherQueueController m_queueController{ nullptr };

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };

    winrt::Windows::UI::Xaml::Controls::TextBlock m_status{ nullptr };

    winrt::Windows::UI::Xaml::UIElement::PointerPressed_revoker m_pointerPressedRevoker;
    winrt::Windows::UI::Xaml::XamlRoot::Changed_revoker m_rootChangedRevoker;
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    auto coInit = wil::CoInitializeEx();

    AppWindow::m_appWindowCount++;
    AppWindow::StartThread([nCmdShow](const auto&& queueController)
    {
        auto appWindow = std::make_shared<AppWindow>(std::move(queueController));
        appWindow->Show(nCmdShow);
        --AppWindow::m_appWindowCount;
    });

    AppWindow::m_appDoneSignal.wait();
    return 0;
}
