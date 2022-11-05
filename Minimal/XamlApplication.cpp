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
    }

    winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> XamlApplication::MetadataProviders()
    {
        return m_providers;
    }

    winrt::Windows::Foundation::IClosable XamlApplication::WindowsXamlManager()
    {
        return m_windowsXamlManager;
    }

    void XamlApplication::InitializeComponent()
    {
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

    xaml::Markup::IXamlType XamlApplication::GetXamlType(xaml::Interop::TypeName const& type)
    {
        for (const auto& provider : m_providers)
        {
            const auto xamlType = provider.GetXamlType(type);
            if (xamlType != nullptr)
            {
                return xamlType;
            }
        }

        return nullptr;
    }

    xaml::Markup::IXamlType XamlApplication::GetXamlType(winrt::hstring const& fullName)
    {
        for (const auto& provider : m_providers)
        {
            const auto xamlType = provider.GetXamlType(fullName);
            if (xamlType != nullptr)
            {
                return xamlType;
            }
        }

        return nullptr;
    }

    winrt::com_array<xaml::Markup::XmlnsDefinition> XamlApplication::GetXmlnsDefinitions()
    {
        std::list<xaml::Markup::XmlnsDefinition> definitions;
        for (const auto& provider : m_providers)
        {
            auto defs = provider.GetXmlnsDefinitions();
            for (const auto& def : defs)
            {
                definitions.insert(definitions.begin(), def);
            }
        }

        return winrt::com_array<xaml::Markup::XmlnsDefinition>(definitions.begin(), definitions.end());
    }
}
