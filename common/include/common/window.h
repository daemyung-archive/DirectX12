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
    //! Retrieve a window.
    //! \return A window.
    [[nodiscard]]
    static Window* GetInstance();

    //! Destructor.
    ~Window();

    //! Start the main loop.
    //! \param example An example will be shown on a window.
    void MainLoop(Example *example);

    //! Retrieve a window resolution.
    //! \return A resolution of window.
    [[nodiscard]]
    Resolution GetResolution() const;

    //! Retrieve a window handle.
    //! \return A window handle.
    [[nodiscard]]
    inline auto GetWindow() const {
        return _window;
    }

private:
    //! Constructor.
    Window();

    //! Initialize an atom handle.
    void InitAtom();

    //! Terminate an atom handle.
    void TermAtom();

    //! Initialize a window handle.
    void InitWindow(const Resolution &resolution);

    //! Terminate a window handle.
    void TermWindow();

private:
    ATOM _atom = 0;
    HWND _window = nullptr;
};

//----------------------------------------------------------------------------------------------------------------------

#endif
