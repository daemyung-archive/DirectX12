//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "window.h"

#include <imgui_impl_win32.h>
#include <combaseapi.h>
#include <windowsx.h>
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

    auto example = reinterpret_cast<Example *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (uMsg) {
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_SIZE: {
            example->Resize({LOWORD(lParam), HIWORD(lParam)});
            return 0;
        }
        case WM_PAINT: {
            example->Update();
            example->Render();
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN: {
            if (!ImGui::GetIO().WantCaptureMouse) {
                example->OnMouseButtonDown(static_cast<MouseButton>(wParam),
                                           {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
                SetCapture(hWnd);
            }
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            if (!ImGui::GetIO().WantCaptureMouse) {
                ReleaseCapture();
                example->OnMouseButtonUp(static_cast<MouseButton>(wParam),
                                         {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (!ImGui::GetIO().WantCaptureMouse) {
                example->OnMouseMove(static_cast<MouseButton>(wParam),
                                     {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            if (!ImGui::GetIO().WantCaptureMouse) {
                example->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA));
            }
            return 0;
        }
        default: {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

Window *Window::GetInstance() {
    static std::unique_ptr<Window> window(new Window());
    return window.get();
}

//----------------------------------------------------------------------------------------------------------------------

Window::~Window() {
    TermWindow();
    TermAtom();
}

//----------------------------------------------------------------------------------------------------------------------

void Window::MainLoop(Example *example) {
    SetWindowLongPtr(_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(example));

    example->BindToWindow(this);
    example->Init();
    example->Resize(GetResolution());

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
}

//----------------------------------------------------------------------------------------------------------------------

Resolution Window::GetResolution() const {
    RECT rect;
    if (!GetClientRect(_window, &rect)) {
        throw std::runtime_error("Fail to get window resolution.");
    }

    return {rect.right - rect.left, rect.bottom - rect.top};
}

//----------------------------------------------------------------------------------------------------------------------

Window::Window() {
    InitAtom();
    InitWindow(kFHDResolution);
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
    window_class.hInstance = GetModuleHandle(nullptr);
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
    UnregisterClass(MAKEINTATOM(_atom), GetModuleHandle(nullptr));
    _atom = NULL;
}

//----------------------------------------------------------------------------------------------------------------------

void Window::InitWindow(const Resolution &resolution) {
    // Get a window size.
    auto size = GetWindowSize(resolution);

    // Create a window.
    _window = CreateWindow(MAKEINTATOM(_atom), L"DirectX12", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           std::get<0>(size), std::get<1>(size), nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
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
