# Win32 Xaml App Samples
C++ samples that demonstrate basic Win32 use of Xaml using [DesktopWindowXamlSource](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.hosting.desktopwindowxamlsource?view=winrt-20348), also known
as Xaml Islands. 

## Samples
### Minimal - 200 line .cpp file Xaml App
[Minimal](./Minimal/Win32XamlApp.cpp) - The app client area is all Xaml. The markup is stored as a string and parsed at runtime. 
Less that 150 lines of code with 50 of it being markup (the `NavigationView` stolen from Terminals Settings UI)

![Win32 Xaml App](Win32XamlApp.png)

### Multi Window App - Supports multiple top level windows
[MultiWindow](./MultiWindow/MultiWindowXamlApp.cpp) - This demonstrates a multi-window Win32 app that uses Xaml. This hooks up some 
handlers to the markups contents to demonstrate how to survive without data binding.

![Win32 Xaml App Multi Window](Win32XamlAppMultiWindow.png)

### Multi Xaml Mulit Window App - Each Window hosts 2 Islands
[MultiXamlAndWindow](./MultiXamlAndWindow/MultiXamlAndWindowApp.cpp) - Multi window app that creates 2 islands in each window. Mostly a diagnostic case for testing.

## Limitations
* No Data binding
* Intellisense in the .xaml files is broken, VS is confused seeing a .xaml file in a Win32 app project type.
* Not using WinUI 2.x, using System Xaml instead.
* No Xaml Application object, so no way to provide custom metadata.
* Does not implement accelerator handling or any integration with Win32 UI (who wants to use Win32 UI anyway!).

## Debugging tips
Look in the debugger output window for Xaml parsing errors, it identifies line and offset of the error.

## Implementation notes
* To use Islands Apps must specify `maxversiontested` via a manifest, see docs [here](https://docs.microsoft.com/en-us/windows/apps/desktop/modernize/host-standard-control-with-xaml-islands-cpp#create-a-desktop-application-project),
see app.manifest used in all of the projects.
