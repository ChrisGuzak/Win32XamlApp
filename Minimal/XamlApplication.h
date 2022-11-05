#include "pch.h"

#pragma once
#include "XamlApplication.g.h"

#include <winrt/Windows.UI.Xaml.Hosting.h>

namespace winrt::Win32XamlApp::implementation
{
    struct XamlApplication : XamlApplicationT<XamlApplication>
    {
        XamlApplication()
        {
            Initialize();
        }

        ~XamlApplication()
        {
            Close();
        }

        XamlApplication(winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> const& providers);
        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> MetadataProviders();
        winrt::Windows::Foundation::IClosable WindowsXamlManager();
        void Initialize();
        void Close();

    private:
        winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_windowsXamlManager{ nullptr };
        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> m_providers
            = winrt::single_threaded_vector<Windows::UI::Xaml::Markup::IXamlMetadataProvider>();
        bool b_closed{};
    };
}

namespace winrt::Win32XamlApp::factory_implementation
{
    struct XamlApplication : XamlApplicationT<XamlApplication, implementation::XamlApplication>
    {
    };
}
