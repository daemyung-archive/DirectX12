//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "window.h"

#include <imgui_impl_win32.h>
#include <combaseapi.h>
#include <stdexcept>
#include <array>
#include <tuple>

#include "example.h"

//----------------------------------------------------------------------------------------------------------------------

constexpr size_t kUniqueNameLength = 39;

//----------------------------------------------------------------------------------------------------------------------

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//----------------------------------------------------------------------------------------------------------------------

/// Generate an unique name.
/// \return An unique name.
inline std::array<wchar_t, kUniqueNameLength> GenerateUniqueName() {
    // Create a GUID which is unique.
    GUID guid;
    if (FAILED(CoCreateGuid(&guid))) {
        throw std::runtime_error("Fail to create GUID.");
    }

    // Convert from a GUID to a string.
    std::array<wchar_t, kUniqueNameLength> name = {L'\0'};
    if (StringFromGUID2(guid, name.data(), kUniqueNameLength) != kUniqueNameLength) {
        throw std::runtime_error("Fail to convert from GUID to string.");
    }

    return name;
}

//----------------------------------------------------------------------------------------------------------------------

/// Retrieve a window size.
/// \param width A width of a window.
/// \param height A height of a window.
/// \return A window size.
std::tuple<UINT, UINT> GetWindowSize(const Resolution &resolution) {
    RECT rect = {};
    rect.right = GetWidth(resolution);
    rect.bottom = GetHeight(resolution);

    if (!AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0)) {
        throw std::runtime_error("Fail to adjust window rect.");
    }

    return {rect.right - rect.left, rect.bottom - rect.top};
}

//----------------------------------------------------------------------------------------------------------------------

/// Process messages sent to a window.
/// \param hWnd A window handle.
/// \param uMsg The message.
/// \param wParam Additional message information. It depends on the value of the uMsg parameter.
/// \param lParam Additional message information. It depends on the value of the uMsg parameter.
/// \return The result of the message processing and depends on the message sent.
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
        return 1;
    }

    switch (uMsg) {
        case WM_CREATE: {
            auto data = reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
            return 0;
        }
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_PAINT: {
            auto example = reinterpret_cast<Example *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            example->Render();
            return 0;
        }
        default: {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

Window::Window() {
    InitInstance();
    InitAtom();
}

//----------------------------------------------------------------------------------------------------------------------

Window::~Window() {
    TermAtom();
}

//----------------------------------------------------------------------------------------------------------------------

void Window::MainLoop(Example *example) {
    InitWindow(example->GetResolution(), example);

    example->BindToWindow(this);
    example->Init();

    // Show and update a window.
    ShowWindow(_window, SW_SHOW);
    UpdateWindow(_window);

    // Start to process messages.
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    example->Term();
    TermWindow();
}

//----------------------------------------------------------------------------------------------------------------------

void Window::InitInstance() {
    // Get an instance.
    _instance = GetModuleHandle(nullptr);
}

//----------------------------------------------------------------------------------------------------------------------

void Window::InitAtom() {
    // Generate an unique name.
    auto class_name = GenerateUniqueName();

    // Define a window class.
    WNDCLASSEX window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = _instance;
    window_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.lpszClassName = class_name.data();

    // Register a window class.
    _atom = RegisterClassEx(&window_class);
    if (_atom == 0) {
        throw std::runtime_error("Fail to register the window class.");
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Window::TermAtom() {
    // Unregister a window class.
    UnregisterClass(MAKEINTATOM(_atom), _instance);
    _atom = NULL;
}

//----------------------------------------------------------------------------------------------------------------------

void Window::InitWindow(const Resolution &resolution, void *userData) {
    // Get a window size.
    auto size = GetWindowSize(resolution);

    // Create a window.
    _window = CreateWindow(MAKEINTATOM(_atom), L"DirectX12", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           std::get<0>(size), std::get<1>(size), nullptr, nullptr, _instance, userData);
    if (!_window) {
        throw std::runtime_error("Fail to create a window.");
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Window::TermWindow() {
    // Destroy a window.
    DestroyWindow(_window);
    _window = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

