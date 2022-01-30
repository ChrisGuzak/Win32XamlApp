#include "pch.h"
#include <win32app/XamlWin32Helpers.h>
#include <win32app/win32_app_helpers.h>

inline constexpr auto contentText = LR"(
<Page
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
    xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006">
    <NavigationView>
        <StackPanel x:Name="StackPanel1"/>

        <NavigationView.MenuItems>
            <NavigationViewItem Content="Startup">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE7B5;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Interaction">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE7C9;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Appearance">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE771;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Color schemes">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE790;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Rendering">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE7F8;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Actions">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE765;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

        </NavigationView.MenuItems>
    </NavigationView>
</Page>
)";


DECLARE_INTERFACE_IID_(IXamlRootInterop, IUnknown, "68601abf-23cc-4017-a946-fb25c574fbab")
{
    virtual HRESULT STDMETHODCALLTYPE GetHostingChidWindow(_Out_ HWND* childHwnd) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTopLevelWindow(_Out_ HWND* topLevelHwnd) = 0;
};

struct AppWindow
{
    LRESULT Create()
    {
        m_xamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();

        auto interop = m_xamlSource.as<IDesktopWindowXamlSourceNative>();
        THROW_IF_FAILED(interop->AttachToWindow(m_window.get()));
        THROW_IF_FAILED(interop->get_WindowHandle(&m_xamlSourceWindow));

        // When this fails look in the debug output window, it shows the line and offset
        // that has the parsing problem.
        auto page = winrt::Windows::UI::Xaml::Markup::XamlReader::Load(contentText).as
            <winrt::Windows::UI::Xaml::Controls::Page>();
        auto navView = page.Content().as<winrt::Windows::UI::Xaml::Controls::NavigationView>();

        m_itemInvokedRevoker = navView.ItemInvoked(winrt::auto_revoke, [this](auto&& sender, auto&& args)
        {
            auto item = args.InvokedItemContainer().as<winrt::Windows::UI::Xaml::Controls::NavigationViewItem>();
            auto text = winrt::unbox_value<winrt::hstring>(item.Content());

            auto tb = winrt::Windows::UI::Xaml::Controls::TextBlock();
            tb.Text(L"Hello " + text);

            auto stackPanel = sender.FindName(L"StackPanel1").as<winrt::Windows::UI::Xaml::Controls::StackPanel>();
            stackPanel.Children().Append(tb);
        });

        m_pointerPressedRevoker = page.PointerPressed(winrt::auto_revoke, [this](auto&& sender, auto&& args)
        {
            auto pointerID = args.Pointer().PointerId();
            auto tb = winrt::Windows::UI::Xaml::Controls::TextBlock();
            tb.Text(L"PointerPressed");

            auto page = sender.as<winrt::Windows::UI::Xaml::Controls::Page>();
            auto stackPanel = page.FindName(L"StackPanel1").as<winrt::Windows::UI::Xaml::Controls::StackPanel>();
            stackPanel.Children().Append(tb);
        });

        m_xamlSource.Content(page);

        /*
        HWND window{};
        page.as<winrt::Windows::UI::Xaml::UIElement>().XamlRoot().as<IXamlRootInterop>()->GetTopLevelWindow(&window);
        MessageBoxW(window, L"Hello from XamlControl!", L"XamlControl", MB_OK);
        */

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
        m_xamlSource.Close();
        PostQuitMessage(0);
        return 0;
    }

    void Show(int nCmdShow)
    {
        win32app::create_top_level_window_for_xaml(*this, L"Win32XamlAppWindow", L"Win32 Xaml App");
        win32app::enter_simple_message_loop(*this, nCmdShow);
    }

    wil::unique_hwnd m_window;
    HWND m_xamlSourceWindow{}; // This is owned by m_xamlSource, destroyed when Close() is called.

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };

    winrt::Windows::UI::Xaml::Controls::NavigationView::ItemInvoked_revoker m_itemInvokedRevoker;
    winrt::Windows::UI::Xaml::UIElement::PointerPressed_revoker m_pointerPressedRevoker;
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    auto coInit = wil::CoInitializeEx(COINIT_APARTMENTTHREADED);
    std::make_unique<AppWindow>()->Show(nCmdShow);
    return 0;
}
