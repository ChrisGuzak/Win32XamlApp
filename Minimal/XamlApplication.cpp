#include "pch.h"
#include "XamlApplication.h"

namespace xaml
{
    using namespace winrt::Windows::UI::Xaml;
}

namespace winrt::Win32XamlApp::implementation
{
    XamlApplication::XamlApplication(winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> const& providers)
    {
        for (auto&& provider : providers)
        {
            m_providers.Append(provider);
        }

        Initialize();
    }

    winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> XamlApplication::MetadataProviders()
    {
        return m_providers;
    }

    winrt::Windows::Foundation::IClosable XamlApplication::WindowsXamlManager()
    {
        return m_windowsXamlManager;
    }

    void XamlApplication::Initialize()
    {
        const auto out = outer();

        winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider provider(nullptr);
        winrt::check_hresult(out->QueryInterface(
            winrt::guid_of<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider>(),
            winrt::put_abi(provider)));
        m_providers.Append(provider);

        m_windowsXamlManager = xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
    }

    void XamlApplication::Close()
    {
        if (b_closed) return;

        b_closed = true;

        m_windowsXamlManager.Close();
        m_providers.Clear();
        m_windowsXamlManager = nullptr;

        // Toolkit has a message loop here.
    }
}
