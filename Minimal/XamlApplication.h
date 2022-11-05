#pragma once

#include "pch.h"
#include "XamlApplication.g.h"

#include <winrt/Windows.UI.Xaml.Hosting.h>

namespace winrt::Win32XamlApp::implementation
{
    struct XamlApplication : XamlApplicationT<XamlApplication>
    {
        XamlApplication() = default;

        void InitializeComponent(); // C++ WinRT 2 phase construction

        ~XamlApplication()
        {
            Close();
        }

        XamlApplication(winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> const& providers);
        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> MetadataProviders();
        winrt::Windows::Foundation::IClosable WindowsXamlManager();

        // ICloseable
        void Close();

        // IXamlMetadataProvider
        winrt::Windows::UI::Xaml::Markup::IXamlType GetXamlType(winrt::Windows::UI::Xaml::Interop::TypeName const& type);
        winrt::Windows::UI::Xaml::Markup::IXamlType GetXamlType(hstring const& fullName);
        com_array<winrt::Windows::UI::Xaml::Markup::XmlnsDefinition> GetXmlnsDefinitions();

    private:
        winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_windowsXamlManager{ nullptr };
        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> m_providers
            = winrt::single_threaded_vector<Windows::UI::Xaml::Markup::IXamlMetadataProvider>();
        bool b_closed{};
    };
}
