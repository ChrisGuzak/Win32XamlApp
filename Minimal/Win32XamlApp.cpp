#include "pch.h"
#include <win32app/XamlWin32Helpers.h>
#include <win32app/win32_app_helpers.h>
#include "XamlApplication.h"

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

MIDL_INTERFACE("37642806-F421-4FD0-9F82-23AE7C776182")
IDesktopWindowContentBridgeInterop : public IUnknown
{
    IFACEMETHOD(Initialize)(IUnknown* compositor, HWND parentHwnd) = 0;
    IFACEMETHOD(get_Hwnd)(_Outptr_ HWND * value) = 0;
    IFACEMETHOD(get_AppliedScaleFactor)(_Out_ float* value) = 0;
};

MIDL_INTERFACE("1DFCBAC6-B36B-5CB9-9BC5-2B7A0EDDC378")
IUIContentRoot : public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE get_UIContext(_COM_Outptr_result_maybenull_ IInspectable** value) = 0;
};

MIDL_INTERFACE("603381CB-2327-5454-919D-A61C5DC4A7D9")
IUIContentRootPartner: public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE get_InputSite(_COM_Outptr_result_maybenull_ IInspectable** value) = 0;
    // Composition Island, not what you are looking for.
    virtual HRESULT STDMETHODCALLTYPE get_Island(_COM_Outptr_result_maybenull_ IInspectable** value) = 0;
    // This object implements IDesktopWindowContentBridgeInterop
    virtual HRESULT STDMETHODCALLTYPE get_WindowContext(_COM_Outptr_ IInspectable** value) = 0;
};

MIDL_INTERFACE("F21E14C1-E669-52AF-99CD-171EEE8E940D")
IUIContextPartner : public IInspectable
{
    // This object implements IUIContentRootPartner
    virtual HRESULT STDMETHODCALLTYPE get_UIContentRoot(_COM_Outptr_ IUIContentRoot** value) = 0;
    // This object implements IDesktopWindowContentBridgeInterop
    virtual HRESULT STDMETHODCALLTYPE get_WindowContext(_COM_Outptr_ IInspectable** value) = 0;
};

struct AppWindow
{
    LRESULT Create()
    {
        // TODO: Proof of concept test of XamlApplicaiton, work in progress.
        // auto winuiIXMP = winrt::Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider();
        // auto markupIXMP = winrt::AppMarkup::XamlMetaDataProvider();
        // auto app = winrt::make_xaml_application(winuiIXMP, markupIXMP);

        auto app = winrt::make_xaml_application();

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

        auto contextPartner = page.UIContext().as<IUIContextPartner>();
        winrt::com_ptr<IInspectable> windowContextUnk;
        auto hr = contextPartner->get_WindowContext(windowContextUnk.put());
        auto bridgeInterop = windowContextUnk.as<IDesktopWindowContentBridgeInterop>();
        HWND hwnd{};
        hr = bridgeInterop->get_Hwnd(&hwnd);

        return 0;
    }

    LRESULT Size(WORD dx, WORD dy)
    {
        SetWindowPos(m_xamlSourceWindow, nullptr, 0, 0, dx, dy, SWP_SHOWWINDOW);
        return 0;
    }

    LRESULT Destroy()
    {
        winrt::Windows::UI::Xaml::Application::Current().as<winrt::WinUIHelpers::XamlApplication>().Close();

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
