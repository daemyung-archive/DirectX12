//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef WINDOW_H_
#define WINDOW_H_

#include "utility.h"

//----------------------------------------------------------------------------------------------------------------------

constexpr auto kFHDResolution = Resolution(1280, 720);

//----------------------------------------------------------------------------------------------------------------------

class Example;

//----------------------------------------------------------------------------------------------------------------------

enum class MouseButton {
    kLeft = MK_LBUTTON, KMiddle = MK_MBUTTON, kRight = MK_RBUTTON
};

//----------------------------------------------------------------------------------------------------------------------

class Window {
public:
    /// Constructor.
    Window();

    /// Destructor.
    ~Window();

    /// Start the main loop.
    /// \param example An example will be shown on a window.
    void MainLoop(Example *example);

    //! Retrieve a window resolution.
    //! \return A resolution of window.
    Resolution GetResolution() const;

    /// Retrieve a instance handle.
    /// \return A instance handle.
    [[nodiscard]]
    inline auto GetInstance() const {
        return _instance;
    }

    /// Retrieve a window handle.
    /// \return A window handle.
    [[nodiscard]]
    inline auto GetWindow() const {
        return _window;
    }

private:
    /// Initialize an instance handle.
    void InitInstance();

    /// Initialize an atom handle.
    void InitAtom();

    /// Terminate an atom handle.
    void TermAtom();

    /// Initialize a window handle.
    void InitWindow(const Resolution &resolution);

    /// Terminate a window handle.
    void TermWindow();

private:
    HINSTANCE _instance = nullptr;
    ATOM _atom = 0;
    HWND _window = nullptr;
};

//----------------------------------------------------------------------------------------------------------------------

#endif
