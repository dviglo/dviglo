// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"
#include "input_constants.h"


namespace dviglo
{

/// Mouse button pressed.
DV_EVENT(E_MOUSEBUTTONDOWN, MouseButtonDown)
{
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
    DV_PARAM(P_CLICKS, Clicks);                // int
}

/// Mouse button released.
DV_EVENT(E_MOUSEBUTTONUP, MouseButtonUp)
{
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse moved.
DV_EVENT(E_MOUSEMOVE, MouseMove)
{
    DV_PARAM(P_X, X);                          // int (only when mouse visible)
    DV_PARAM(P_Y, Y);                          // int (only when mouse visible)
    DV_PARAM(P_DX, DX);                        // int
    DV_PARAM(P_DY, DY);                        // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse wheel moved.
DV_EVENT(E_MOUSEWHEEL, MouseWheel)
{
    DV_PARAM(P_WHEEL, Wheel);                  // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Key pressed.
DV_EVENT(E_KEYDOWN, KeyDown)
{
    DV_PARAM(P_KEY, Key);                      // int
    DV_PARAM(P_SCANCODE, Scancode);            // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
    DV_PARAM(P_REPEAT, Repeat);                // bool
}

/// Key released.
DV_EVENT(E_KEYUP, KeyUp)
{
    DV_PARAM(P_KEY, Key);                      // int
    DV_PARAM(P_SCANCODE, Scancode);            // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Text input event.
DV_EVENT(E_TEXTINPUT, TextInput)
{
    DV_PARAM(P_TEXT, Text);                    // String
}

/// Text editing event.
DV_EVENT(E_TEXTEDITING, TextEditing)
{
    DV_PARAM(P_COMPOSITION, Composition);      // String
    DV_PARAM(P_CURSOR, Cursor);                // int
    DV_PARAM(P_SELECTION_LENGTH, SelectionLength);  // int
}

/// Joystick connected.
DV_EVENT(E_JOYSTICKCONNECTED, JoystickConnected)
{
    DV_PARAM(P_JOYSTICKID, JoystickID);        // int
}

/// Joystick disconnected.
DV_EVENT(E_JOYSTICKDISCONNECTED, JoystickDisconnected)
{
    DV_PARAM(P_JOYSTICKID, JoystickID);        // int
}

/// Joystick button pressed.
DV_EVENT(E_JOYSTICKBUTTONDOWN, JoystickButtonDown)
{
    DV_PARAM(P_JOYSTICKID, JoystickID);        // int
    DV_PARAM(P_BUTTON, Button);                // int
}

/// Joystick button released.
DV_EVENT(E_JOYSTICKBUTTONUP, JoystickButtonUp)
{
    DV_PARAM(P_JOYSTICKID, JoystickID);        // int
    DV_PARAM(P_BUTTON, Button);                // int
}

/// Joystick axis moved.
DV_EVENT(E_JOYSTICKAXISMOVE, JoystickAxisMove)
{
    DV_PARAM(P_JOYSTICKID, JoystickID);        // int
    DV_PARAM(P_AXIS, Button);                  // int
    DV_PARAM(P_POSITION, Position);            // float
}

/// Joystick POV hat moved.
DV_EVENT(E_JOYSTICKHATMOVE, JoystickHatMove)
{
    DV_PARAM(P_JOYSTICKID, JoystickID);        // int
    DV_PARAM(P_HAT, Button);                   // int
    DV_PARAM(P_POSITION, Position);            // int
}

/// Finger pressed on the screen.
DV_EVENT(E_TOUCHBEGIN, TouchBegin)
{
    DV_PARAM(P_TOUCHID, TouchID);              // int
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_PRESSURE, Pressure);            // float
}

/// Finger released from the screen.
DV_EVENT(E_TOUCHEND, TouchEnd)
{
    DV_PARAM(P_TOUCHID, TouchID);              // int
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
}

/// Finger moved on the screen.
DV_EVENT(E_TOUCHMOVE, TouchMove)
{
    DV_PARAM(P_TOUCHID, TouchID);              // int
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_DX, DX);                        // int
    DV_PARAM(P_DY, DY);                        // int
    DV_PARAM(P_PRESSURE, Pressure);            // float
}

/// A file was drag-dropped into the application window.
DV_EVENT(E_DROPFILE, DropFile)
{
    DV_PARAM(P_FILENAME, FileName);            // String
}

/// Application input focus or minimization changed.
DV_EVENT(E_INPUTFOCUS, InputFocus)
{
    DV_PARAM(P_FOCUS, Focus);                  // bool
    DV_PARAM(P_MINIMIZED, Minimized);          // bool
}

/// OS mouse cursor visibility changed.
DV_EVENT(E_MOUSEVISIBLECHANGED, MouseVisibleChanged)
{
    DV_PARAM(P_VISIBLE, Visible);              // bool
}

/// Mouse mode changed.
DV_EVENT(E_MOUSEMODECHANGED, MouseModeChanged)
{
    DV_PARAM(P_MODE, Mode);                    // MouseMode
    DV_PARAM(P_MOUSELOCKED, MouseLocked);      // bool
}

/// Application exit requested.
DV_EVENT(E_EXITREQUESTED, ExitRequested)
{
}

/// Raw SDL input event.
DV_EVENT(E_SDLRAWINPUT, SDLRawInput)
{
    DV_PARAM(P_SDLEVENT, SDLEvent);           // SDL_Event*
    DV_PARAM(P_CONSUMED, Consumed);           // bool
}

/// Input handling begins.
DV_EVENT(E_INPUTBEGIN, InputBegin)
{
}

/// Input handling ends.
DV_EVENT(E_INPUTEND, InputEnd)
{
}

}
