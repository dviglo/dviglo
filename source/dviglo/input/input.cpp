// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "input.h"

#include "../core/context.h"
#include "../core/core_events.h"
#include "../core/process_utils.h"
#include "../core/profiler.h"
#include "../core/sdl_helper.h"
#include "../core/string_utils.h"
#include "../graphics/graphics.h"
#include "../graphics/graphics_events.h"
#include "../io/log.h"
#include "../io/path.h"
#include "../resource/resource_cache.h"
#include "../ui/text.h"
#include "../ui/ui.h"

#ifdef _WIN32
#include "../engine/engine.h"
#endif

#include <SDL3/SDL.h>

#include "../common/debug_new.h"

using namespace std;

extern "C" int SDL_AddTouch(SDL_TouchID touchID, SDL_TouchDeviceType type, const char* name);

// Use a "click inside window to focus" mechanism on desktop platforms when the mouse cursor is hidden
#if defined(_WIN32) || (defined(__APPLE__) && !defined(IOS) && !defined(TVOS)) || (defined(__linux__) && !defined(__ANDROID__))
#define REQUIRE_CLICK_TO_FOCUS
#endif

// TODO: После обновления SDL до 3й версии по индексам к джойстикам обращаться нельзя

namespace dviglo
{

static constexpr i32 TOUCHID_MAX = 32;

/// Convert SDL keycode if necessary.
Key ConvertSDLKeyCode(int keySym, int scanCode)
{
    if (scanCode == SCANCODE_AC_BACK)
        return KEY_ESCAPE;
    else
        return (Key)SDL_tolower(keySym);
}

UIElement* TouchState::GetTouchedElement()
{
    return touchedElement_.Get();
}

#ifdef _WIN32
// On Windows repaint while the window is actively being resized.
int Win32_ResizingEventWatcher(void* data, SDL_Event* event)
{
    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
        if (win == (SDL_Window*)data)
        {
            if (!GParams::is_headless())
            {
                if (DV_GRAPHICS.IsInitialized())
                {
                    DV_GRAPHICS.OnWindowResized();
                    DV_ENGINE.RunFrame();
                }
            }
        }
    }
    return 0;
}
#endif

void JoystickState::Initialize(i32 numButtons, i32 numAxes, i32 numHats)
{
    buttons_.Resize(numButtons);
    buttonPress_.Resize(numButtons);
    axes_.Resize(numAxes);
    hats_.Resize(numHats);

    Reset();
}

void JoystickState::Reset()
{
    for (i32 i = 0; i < buttons_.Size(); ++i)
    {
        buttons_[i] = false;
        buttonPress_[i] = false;
    }

    for (i32 i = 0; i < axes_.Size(); ++i)
        axes_[i] = 0.0f;

    for (i32 i = 0; i < hats_.Size(); ++i)
        hats_[i] = HAT_CENTER;
}

#ifdef _DEBUG
// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool input_destructed = false;
#endif

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
Input& Input::get_instance()
{
    assert(!input_destructed);
    static Input instance;
    return instance;
}

Input::Input() :
    mouseButtonDown_(0),
    mouseButtonPress_(0),
    lastVisibleMousePosition_(MOUSE_POSITION_OFFSCREEN),
    mouseMoveWheel_(0),
    inputScale_(Vector2::ONE),
    windowID_(0),
    toggleFullscreen_(true),
    mouseVisible_(false),
    lastMouseVisible_(false),
    mouseGrabbed_(false),
    lastMouseGrabbed_(false),
    mouseMode_(MM_ABSOLUTE),
    lastMouseMode_(MM_ABSOLUTE),
    sdlMouseRelative_(false),
    touchEmulation_(false),
    inputFocus_(false),
    minimized_(false),
    focusedThisFrame_(false),
    suppressNextMouseMove_(false),
    mouseMoveScaled_(false),
    initialized_(false)
{
    DV_SDL_HELPER.require(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD);

    for (int i = 0; i < TOUCHID_MAX; i++)
        availableTouchIDs_.Push(i);

    SubscribeToEvent(E_SCREENMODE, DV_HANDLER(Input, HandleScreenMode));

    // Try to initialize right now, but skip if screen mode is not yet set
    Initialize();

    DV_LOGDEBUG("Singleton Input constructed");
}

Input::~Input()
{
    DV_LOGDEBUG("Singleton Input destructed");
#ifdef _DEBUG
    input_destructed = true;
#endif
}

void Input::Update()
{
    assert(initialized_);

    DV_PROFILE(UpdateInput);

    bool mouseMoved = false;
    if (mouseMove_ != IntVector2::ZERO)
        mouseMoved = true;

    ResetInputAccumulation();

    SDL_Event evt;
    while (SDL_PollEvent(&evt))
        HandleSDLEvent(&evt);

    if (suppressNextMouseMove_ && (mouseMove_ != IntVector2::ZERO || mouseMoved))
        UnsuppressMouseMove();

    Graphics& graphics = DV_GRAPHICS;

    // Check for focus change this frame
    SDL_Window* window = graphics.GetWindow();
    unsigned flags = window ? SDL_GetWindowFlags(window) & (SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS) : 0;
    if (window)
    {
#ifdef REQUIRE_CLICK_TO_FOCUS
        // When using the "click to focus" mechanism, only focus automatically in fullscreen or non-hidden mouse mode
        if (!inputFocus_ && ((mouseVisible_ || mouseMode_ == MM_FREE) || graphics.GetFullscreen()) && (flags & SDL_WINDOW_INPUT_FOCUS))
#else
        if (!inputFocus_ && (flags & SDL_WINDOW_INPUT_FOCUS))
#endif
            focusedThisFrame_ = true;

        if (focusedThisFrame_)
            GainFocus();

        // Check for losing focus. The window flags are not reliable when using an external window, so prevent losing focus in that case
        if (inputFocus_ && (flags & SDL_WINDOW_INPUT_FOCUS) == 0)
            LoseFocus();
    }
    else
        return;

    // Handle mouse mode MM_WRAP
    if (mouseVisible_ && mouseMode_ == MM_WRAP)
    {
        IntVector2 windowPos = graphics.GetWindowPosition();
        Vector2 mpos;
        SDL_GetGlobalMouseState(&mpos.x_, &mpos.y_);
        mpos -= Vector2(windowPos);

        const int buffer = 5;
        const int width = graphics.GetWidth() - buffer * 2;
        const int height = graphics.GetHeight() - buffer * 2;

        // SetMousePosition utilizes backbuffer coordinate system, scale now from window coordinates
        mpos.x_ = (int)(mpos.x_ * inputScale_.x_);
        mpos.y_ = (int)(mpos.y_ * inputScale_.y_);

        bool warp = false;
        if (mpos.x_ < buffer)
        {
            warp = true;
            mpos.x_ += width;
        }

        if (mpos.x_ > buffer + width)
        {
            warp = true;
            mpos.x_ -= width;
        }

        if (mpos.y_ < buffer)
        {
            warp = true;
            mpos.y_ += height;
        }

        if (mpos.y_ > buffer + height)
        {
            warp = true;
            mpos.y_ -= height;
        }

        if (warp)
        {
            SetMousePosition({(int)mpos.x_, (int)mpos.y_});
            SuppressNextMouseMove();
        }
    }

    if (!touchEmulation_ && !sdlMouseRelative_ && !mouseVisible_ && mouseMode_ != MM_FREE && inputFocus_ && (flags & SDL_WINDOW_MOUSE_FOCUS))
    {
        const IntVector2 mousePosition = GetMousePosition();
        mouseMove_ = mousePosition - lastMousePosition_;
        mouseMoveScaled_ = true; // Already in backbuffer scale, since GetMousePosition() operates in that

        // Recenter the mouse cursor manually after move
        CenterMousePosition();

        // Send mouse move event if necessary
        if (mouseMove_ != IntVector2::ZERO)
        {
            if (!suppressNextMouseMove_)
            {
                using namespace MouseMove;

                VariantMap& eventData = GetEventDataMap();

                eventData[P_X] = mousePosition.x_;
                eventData[P_Y] = mousePosition.y_;
                eventData[P_DX] = mouseMove_.x_;
                eventData[P_DY] = mouseMove_.y_;
                eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
                eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
                SendEvent(E_MOUSEMOVE, eventData);
            }
        }
    }
    else if (!touchEmulation_ && !mouseVisible_ && sdlMouseRelative_ && inputFocus_ && (flags & SDL_WINDOW_MOUSE_FOCUS))
    {
        // Keep the cursor trapped in window.
        CenterMousePosition();
    }
}

void Input::SetMouseVisible(bool enable, bool suppressEvent)
{
    const bool startMouseVisible = mouseVisible_;

    // In touch emulation mode only enabled mouse is allowed
    if (touchEmulation_)
        enable = true;

    // In mouse mode relative, the mouse should be invisible
    if (mouseMode_ == MM_RELATIVE)
    {
        if (!suppressEvent)
            lastMouseVisible_ = enable;

        enable = false;
    }

    // SDL Raspberry Pi "video driver" does not have proper OS mouse support yet, so no-op for now
#ifndef RPI
    if (enable != mouseVisible_)
    {
        if (initialized_)
        {
            if (!enable && inputFocus_)
            {
                if (mouseVisible_)
                    lastVisibleMousePosition_ = GetMousePosition();

                if (mouseMode_ == MM_ABSOLUTE)
                    SetMouseModeAbsolute(SDL_TRUE);

                SDL_HideCursor();
                mouseVisible_ = false;
            }
            else if (mouseMode_ != MM_RELATIVE)
            {
                SetMouseGrabbed(false, suppressEvent);

                SDL_ShowCursor();
                mouseVisible_ = true;

                if (mouseMode_ == MM_ABSOLUTE)
                    SetMouseModeAbsolute(SDL_FALSE);

                // Update cursor position
                Cursor* cursor = DV_UI.GetCursor();
                // If the UI Cursor was visible, use that position instead of last visible OS cursor position
                if (cursor && cursor->IsVisible())
                {
                    IntVector2 pos = cursor->GetScreenPosition();
                    if (pos != MOUSE_POSITION_OFFSCREEN)
                    {
                        SetMousePosition(pos);
                        lastMousePosition_ = pos;
                    }
                }
                else
                {
                    if (lastVisibleMousePosition_ != MOUSE_POSITION_OFFSCREEN)
                    {
                        SetMousePosition(lastVisibleMousePosition_);
                        lastMousePosition_ = lastVisibleMousePosition_;
                    }
                }
            }
        }
        else
        {
            // Allow to set desired mouse visibility before initialization
            mouseVisible_ = enable;
        }

        if (mouseVisible_ != startMouseVisible)
        {
            SuppressNextMouseMove();
            if (!suppressEvent)
            {
                lastMouseVisible_ = mouseVisible_;
                using namespace MouseVisibleChanged;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_VISIBLE] = mouseVisible_;
                SendEvent(E_MOUSEVISIBLECHANGED, eventData);
            }
        }
    }
#endif
}

void Input::ResetMouseVisible()
{
    SetMouseVisible(lastMouseVisible_, false);
}

void Input::SetMouseGrabbed(bool grab, bool suppressEvent)
{
    mouseGrabbed_ = grab;

    if (!suppressEvent)
        lastMouseGrabbed_ = grab;
}

void Input::ResetMouseGrabbed()
{
    SetMouseGrabbed(lastMouseGrabbed_, true);
}

void Input::SetMouseModeAbsolute(SDL_bool enable)
{
    SDL_Window* const window = DV_GRAPHICS.GetWindow();

    SDL_SetWindowGrab(window, enable);
}

void Input::SetMouseModeRelative(SDL_bool enable)
{
    SDL_Window* const window = DV_GRAPHICS.GetWindow();

    int result = SDL_SetRelativeMouseMode(enable);
    sdlMouseRelative_ = enable && (result == 0);

    if (result == -1)
        SDL_SetWindowGrab(window, enable);
}

void Input::SetMouseMode(MouseMode mode, bool suppressEvent)
{
    const MouseMode previousMode = mouseMode_;

    if (mode != mouseMode_)
    {
        if (initialized_)
        {
            SuppressNextMouseMove();

            mouseMode_ = mode;
            SDL_Window* const window = DV_GRAPHICS.GetWindow();

            Cursor* const cursor = DV_UI.GetCursor();

            // Handle changing from previous mode
            if (previousMode == MM_ABSOLUTE)
            {
                if (!mouseVisible_)
                    SetMouseModeAbsolute(SDL_FALSE);
            }
            if (previousMode == MM_RELATIVE)
            {
                SetMouseModeRelative(SDL_FALSE);
                ResetMouseVisible();
            }
            else if (previousMode == MM_WRAP)
                SDL_SetWindowGrab(window, SDL_FALSE);

            // Handle changing to new mode
            if (mode == MM_ABSOLUTE)
            {
                if (!mouseVisible_)
                    SetMouseModeAbsolute(SDL_TRUE);
            }
            else if (mode == MM_RELATIVE)
            {
                SetMouseVisible(false, true);
                SetMouseModeRelative(SDL_TRUE);
            }
            else if (mode == MM_WRAP)
            {
                SetMouseGrabbed(true, suppressEvent);
                SDL_SetWindowGrab(window, SDL_TRUE);
            }

            if (mode != MM_WRAP)
                SetMouseGrabbed(!(mouseVisible_ || (cursor && cursor->IsVisible())), suppressEvent);
        }
        else
        {
            // Allow to set desired mouse mode before initialization
            mouseMode_ = mode;
        }
    }

    if (!suppressEvent)
    {
        lastMouseMode_ = mode;

        if (mouseMode_ != previousMode)
        {
            VariantMap& eventData = GetEventDataMap();
            eventData[MouseModeChanged::P_MODE] = mode;
            eventData[MouseModeChanged::P_MOUSELOCKED] = IsMouseLocked();
            SendEvent(E_MOUSEMODECHANGED, eventData);
        }
    }
}

void Input::ResetMouseMode()
{
    SetMouseMode(lastMouseMode_, false);
}

void Input::SetToggleFullscreen(bool enable)
{
    toggleFullscreen_ = enable;
}

void Input::SetTouchEmulation(bool enable)
{
#if !defined(__ANDROID__) && !defined(IOS)
    if (enable != touchEmulation_)
    {
        if (enable)
        {
            // Touch emulation needs the mouse visible
            if (!mouseVisible_)
                SetMouseVisible(true);

            // Add a virtual touch device the first time we are enabling emulated touch
            if (!SDL_GetNumTouchDevices())
                SDL_AddTouch(0, SDL_TOUCH_DEVICE_INDIRECT_RELATIVE, "Emulated Touch");
        }
        else
            ResetTouches();

        touchEmulation_ = enable;
    }
#endif
}

SDL_JoystickID Input::OpenJoystick(SDL_JoystickID id)
{
    assert(id > 0);

    SDL_Joystick* joystick = SDL_OpenJoystick(id);
    if (!joystick)
    {
        DV_LOGERRORF("Cannot open joystick #%d", id);
        return 0;
    }

    // Create joystick state for the new joystick
    int joystickID = SDL_GetJoystickInstanceID(joystick);
    JoystickState& state = joysticks_[joystickID];
    state.joystick_ = joystick;
    state.joystickID_ = joystickID;
    state.name_ = SDL_GetJoystickName(joystick);
    if (SDL_IsGamepad(id))
        state.gamepad_ = SDL_OpenGamepad(id);

    i32 numButtons = SDL_GetNumJoystickButtons(joystick);
    i32 numAxes = SDL_GetNumJoystickAxes(joystick);
    i32 numHats = SDL_GetNumJoystickHats(joystick);

    // When the joystick is a gamepad, make sure there's enough axes & buttons for the standard gamepad mappings
    if (state.gamepad_)
    {
        if (numButtons < SDL_GAMEPAD_BUTTON_MAX)
            numButtons = SDL_GAMEPAD_BUTTON_MAX;
        if (numAxes < SDL_GAMEPAD_AXIS_MAX)
            numAxes = SDL_GAMEPAD_AXIS_MAX;
    }

    state.Initialize(numButtons, numAxes, numHats);

    return joystickID;
}

Key Input::GetKeyFromName(const String& name) const
{
    return (Key)SDL_GetKeyFromName(name.c_str());
}

Key Input::GetKeyFromScancode(Scancode scancode) const
{
    return (Key)SDL_GetKeyFromScancode((SDL_Scancode)scancode);
}

String Input::GetKeyName(Key key) const
{
    return String(SDL_GetKeyName(key));
}

Scancode Input::GetScancodeFromKey(Key key) const
{
    return (Scancode)SDL_GetScancodeFromKey(key);
}

Scancode Input::GetScancodeFromName(const String& name) const
{
    return (Scancode)SDL_GetScancodeFromName(name.c_str());
}

String Input::GetScancodeName(Scancode scancode) const
{
    return SDL_GetScancodeName((SDL_Scancode)scancode);
}

bool Input::GetKeyDown(Key key) const
{
    return keyDown_.Contains(SDL_tolower(key));
}

bool Input::GetKeyPress(Key key) const
{
    return keyPress_.Contains(SDL_tolower(key));
}

bool Input::GetScancodeDown(Scancode scancode) const
{
    return scancodeDown_.Contains(scancode);
}

bool Input::GetScancodePress(Scancode scancode) const
{
    return scancodePress_.Contains(scancode);
}

bool Input::GetMouseButtonDown(MouseButtonFlags button) const
{
    return mouseButtonDown_ & button;
}

bool Input::GetMouseButtonPress(MouseButtonFlags button) const
{
    return mouseButtonPress_ & button;
}

bool Input::GetQualifierDown(Qualifier qualifier) const
{
    if (qualifier == QUAL_SHIFT)
        return GetKeyDown(KEY_LSHIFT) || GetKeyDown(KEY_RSHIFT);
    if (qualifier == QUAL_CTRL)
        return GetKeyDown(KEY_LCTRL) || GetKeyDown(KEY_RCTRL);
    if (qualifier == QUAL_ALT)
        return GetKeyDown(KEY_LALT) || GetKeyDown(KEY_RALT);

    return false;
}

bool Input::GetQualifierPress(Qualifier qualifier) const
{
    if (qualifier == QUAL_SHIFT)
        return GetKeyPress(KEY_LSHIFT) || GetKeyPress(KEY_RSHIFT);
    if (qualifier == QUAL_CTRL)
        return GetKeyPress(KEY_LCTRL) || GetKeyPress(KEY_RCTRL);
    if (qualifier == QUAL_ALT)
        return GetKeyPress(KEY_LALT) || GetKeyPress(KEY_RALT);

    return false;
}

QualifierFlags Input::GetQualifiers() const
{
    QualifierFlags ret;
    if (GetQualifierDown(QUAL_SHIFT))
        ret |= QUAL_SHIFT;
    if (GetQualifierDown(QUAL_CTRL))
        ret |= QUAL_CTRL;
    if (GetQualifierDown(QUAL_ALT))
        ret |= QUAL_ALT;

    return ret;
}

IntVector2 Input::GetMousePosition() const
{
    IntVector2 ret = IntVector2::ZERO;

    if (!initialized_)
        return ret;

    float x, y;
    SDL_GetMouseState(&x, &y);
    ret.x_ = (int)(x * inputScale_.x_);
    ret.y_ = (int)(y * inputScale_.y_);

    return ret;
}

IntVector2 Input::GetMouseMove() const
{
    if (!suppressNextMouseMove_)
        return mouseMoveScaled_ ? mouseMove_ : IntVector2((int)(mouseMove_.x_ * inputScale_.x_), (int)(mouseMove_.y_ * inputScale_.y_));
    else
        return IntVector2::ZERO;
}

int Input::GetMouseMoveX() const
{
    if (!suppressNextMouseMove_)
        return mouseMoveScaled_ ? mouseMove_.x_ : (int)(mouseMove_.x_ * inputScale_.x_);
    else
        return 0;
}

int Input::GetMouseMoveY() const
{
    if (!suppressNextMouseMove_)
        return mouseMoveScaled_ ? mouseMove_.y_ : mouseMove_.y_ * inputScale_.y_;
    else
        return 0;
}

TouchState* Input::GetTouch(i32 index) const
{
    assert(index >= 0);

    if (index >= touches_.Size())
        return nullptr;

    HashMap<int, TouchState>::ConstIterator i = touches_.Begin();
    while (index--)
        ++i;

    return const_cast<TouchState*>(&i->second_);
}

JoystickState* Input::GetJoystickByIndex(i32 index)
{
    assert(index >= 0);

    i32 compare = 0;
    for (HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Begin(); i != joysticks_.End(); ++i)
    {
        if (compare++ == index)
            return &(i->second_);
    }

    return nullptr;
}

JoystickState* Input::GetJoystickByName(const String& name)
{
    for (HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Begin(); i != joysticks_.End(); ++i)
    {
        if (i->second_.name_ == name)
            return &(i->second_);
    }

    return nullptr;
}

JoystickState* Input::GetJoystick(SDL_JoystickID id)
{
    HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Find(id);
    return i != joysticks_.End() ? &(i->second_) : nullptr;
}

bool Input::IsMouseLocked() const
{
    return !((mouseMode_ == MM_ABSOLUTE && mouseVisible_) || mouseMode_ == MM_FREE);
}

bool Input::IsMinimized() const
{
    // Return minimized state also when unfocused in fullscreen
    if (!inputFocus_ && !GParams::is_headless() && DV_GRAPHICS.GetFullscreen())
        return true;
    else
        return minimized_;
}

void Input::Initialize()
{
    if (GParams::is_headless() || !DV_GRAPHICS.IsInitialized())
        return;

    // Set the initial activation
    initialized_ = true;
    GainFocus();

    ResetJoysticks();
    ResetState();

    SubscribeToEvent(E_BEGINFRAME, DV_HANDLER(Input, HandleBeginFrame));

#ifdef _WIN32
    // Register callback for resizing in order to repaint.
    if (SDL_Window* window = DV_GRAPHICS.GetWindow())
        SDL_AddEventWatch(Win32_ResizingEventWatcher, window);
#endif

    DV_LOGINFO("Initialized input");
}

void Input::ResetJoysticks()
{
    joysticks_.Clear();

    i32 num_joysticks;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&num_joysticks);

    if (joysticks)
    {
        for (i32 i = 0; i < num_joysticks; ++i)
        {
            SDL_JoystickID instance_id = joysticks[i];
            OpenJoystick(instance_id);
        }
        SDL_free(joysticks);
    }
}

void Input::ResetInputAccumulation()
{
    // Reset input accumulation for this frame
    keyPress_.Clear();
    scancodePress_.Clear();
    mouseButtonPress_ = MOUSEB_NONE;
    mouseMove_ = IntVector2::ZERO;
    mouseMoveWheel_ = 0;
    for (HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Begin(); i != joysticks_.End(); ++i)
    {
        for (i32 j = 0; j < i->second_.buttonPress_.Size(); ++j)
            i->second_.buttonPress_[j] = false;
    }

    // Reset touch delta movement
    for (HashMap<int, TouchState>::Iterator i = touches_.Begin(); i != touches_.End(); ++i)
    {
        TouchState& state = i->second_;
        state.lastPosition_ = state.position_;
        state.delta_ = IntVector2::ZERO;
    }
}

void Input::GainFocus()
{
    ResetState();

    inputFocus_ = true;
    focusedThisFrame_ = false;

    // Restore mouse mode
    const MouseMode mm = mouseMode_;
    mouseMode_ = MM_FREE;
    SetMouseMode(mm, true);

    SuppressNextMouseMove();

    // Re-establish mouse cursor hiding as necessary
    if (!mouseVisible_)
        SDL_HideCursor();

    SendInputFocusEvent();
}

void Input::LoseFocus()
{
    ResetState();

    inputFocus_ = false;
    focusedThisFrame_ = false;

    // Show the mouse cursor when inactive
    SDL_ShowCursor();

    // Change mouse mode -- removing any cursor grabs, etc.
    const MouseMode mm = mouseMode_;
    SetMouseMode(MM_FREE, true);
    // Restore flags to reflect correct mouse state.
    mouseMode_ = mm;

    SendInputFocusEvent();
}

void Input::ResetState()
{
    keyDown_.Clear();
    keyPress_.Clear();
    scancodeDown_.Clear();
    scancodePress_.Clear();

    /// \todo Check if resetting joystick state on input focus loss is even necessary
    for (HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Begin(); i != joysticks_.End(); ++i)
        i->second_.Reset();

    ResetTouches();

    // Use SetMouseButton() to reset the state so that mouse events will be sent properly
    SetMouseButton(MOUSEB_LEFT, false, 0);
    SetMouseButton(MOUSEB_RIGHT, false, 0);
    SetMouseButton(MOUSEB_MIDDLE, false, 0);

    mouseMove_ = IntVector2::ZERO;
    mouseMoveWheel_ = 0;
    mouseButtonPress_ = MOUSEB_NONE;
}

void Input::ResetTouches()
{
    for (HashMap<int, TouchState>::Iterator i = touches_.Begin(); i != touches_.End(); ++i)
    {
        TouchState& state = i->second_;

        using namespace TouchEnd;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_TOUCHID] = state.touchID_;
        eventData[P_X] = state.position_.x_;
        eventData[P_Y] = state.position_.y_;
        SendEvent(E_TOUCHEND, eventData);
    }

    touches_.Clear();
    touchIDMap_.Clear();
    availableTouchIDs_.Clear();
    for (int i = 0; i < TOUCHID_MAX; i++)
        availableTouchIDs_.Push(i);

}

i32 Input::GetTouchIndexFromID(int touchID)
{
    HashMap<int, int>::ConstIterator i = touchIDMap_.Find(touchID);
    if (i != touchIDMap_.End())
    {
        return i->second_;
    }

    i32 index = PopTouchIndex();
    touchIDMap_[touchID] = index;
    return index;
}

i32 Input::PopTouchIndex()
{
    if (availableTouchIDs_.Empty())
        return 0;

    i32 index = availableTouchIDs_.Front();
    availableTouchIDs_.PopFront();
    return index;
}

void Input::PushTouchIndex(int touchID)
{
    HashMap<int, int>::ConstIterator ci = touchIDMap_.Find(touchID);
    if (ci == touchIDMap_.End())
        return;

    int index = touchIDMap_[touchID];
    touchIDMap_.Erase(touchID);

    // Sorted insertion
    bool inserted = false;
    for (List<int>::Iterator i = availableTouchIDs_.Begin(); i != availableTouchIDs_.End(); ++i)
    {
        if (*i == index)
        {
            // This condition can occur when TOUCHID_MAX is reached.
            inserted = true;
            break;
        }

        if (*i > index)
        {
            availableTouchIDs_.Insert(i, index);
            inserted = true;
            break;
        }
    }

    // If empty, or the lowest value then insert at end.
    if (!inserted)
        availableTouchIDs_.Push(index);
}

void Input::SendInputFocusEvent()
{
    using namespace InputFocus;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_FOCUS] = HasFocus();
    eventData[P_MINIMIZED] = IsMinimized();
    SendEvent(E_INPUTFOCUS, eventData);
}

void Input::SetMouseButton(MouseButton button, bool newState, int clicks)
{
    if (newState)
    {
        if (!(mouseButtonDown_ & button))
            mouseButtonPress_ |= button;

        mouseButtonDown_ |= button;
    }
    else
    {
        if (!(mouseButtonDown_ & button))
            return;

        mouseButtonDown_ &= ~button;
    }

    using namespace MouseButtonDown;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_BUTTON] = button;
    eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
    eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
    eventData[P_CLICKS] = clicks;
    SendEvent(newState ? E_MOUSEBUTTONDOWN : E_MOUSEBUTTONUP, eventData);
}

void Input::SetKey(Key key, Scancode scancode, bool newState)
{
    bool repeat = false;

    if (newState)
    {
        scancodeDown_.Insert(scancode);
        scancodePress_.Insert(scancode);

        if (!keyDown_.Contains(key))
        {
            keyDown_.Insert(key);
            keyPress_.Insert(key);
        }
        else
            repeat = true;
    }
    else
    {
        scancodeDown_.Erase(scancode);

        if (!keyDown_.Erase(key))
            return;
    }

    using namespace KeyDown;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_KEY] = key;
    eventData[P_SCANCODE] = scancode;
    eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
    eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
    if (newState)
        eventData[P_REPEAT] = repeat;
    SendEvent(newState ? E_KEYDOWN : E_KEYUP, eventData);

    if ((key == KEY_RETURN || key == KEY_RETURN2 || key == KEY_KP_ENTER) && newState && !repeat && toggleFullscreen_ &&
        (GetKeyDown(KEY_LALT) || GetKeyDown(KEY_RALT)))
        DV_GRAPHICS.ToggleFullscreen();
}

void Input::SetMouseWheel(int delta)
{
    if (delta)
    {
        mouseMoveWheel_ += delta;

        using namespace MouseWheel;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_WHEEL] = delta;
        eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
        eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
        SendEvent(E_MOUSEWHEEL, eventData);
    }
}

void Input::SetMousePosition(const IntVector2& position)
{
    if (GParams::is_headless())
        return;

    SDL_WarpMouseInWindow(DV_GRAPHICS.GetWindow(), (int)(position.x_ / inputScale_.x_), (int)(position.y_ / inputScale_.y_));
}

void Input::CenterMousePosition()
{
    const IntVector2 center(DV_GRAPHICS.GetWidth() / 2, DV_GRAPHICS.GetHeight() / 2);
    if (GetMousePosition() != center)
    {
        SetMousePosition(center);
        lastMousePosition_ = center;
    }
}

void Input::SuppressNextMouseMove()
{
    suppressNextMouseMove_ = true;
    mouseMove_ = IntVector2::ZERO;
}

void Input::UnsuppressMouseMove()
{
    suppressNextMouseMove_ = false;
    mouseMove_ = IntVector2::ZERO;
    lastMousePosition_ = GetMousePosition();
}

void Input::HandleSDLEvent(void* sdlEvent)
{
    Graphics& graphics = DV_GRAPHICS;

    SDL_Event& evt = *static_cast<SDL_Event*>(sdlEvent);

    // While not having input focus, skip key/mouse/touch/joystick events, except for the "click to focus" mechanism
    if (!inputFocus_ && evt.type >= SDL_EVENT_KEY_DOWN && evt.type <= SDL_EVENT_FINGER_MOTION)
    {
#ifdef REQUIRE_CLICK_TO_FOCUS
        // Require the click to be at least 1 pixel inside the window to disregard clicks in the title bar
        if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.x > 0 && evt.button.y > 0 && evt.button.x < graphics.GetWidth() - 1 &&
            evt.button.y < graphics.GetHeight() - 1)
        {
            focusedThisFrame_ = true;
            // Do not cause the click to actually go throughfin
            return;
        }
        else if (evt.type == SDL_EVENT_FINGER_DOWN)
        {
            // When focusing by touch, call GainFocus() immediately as it resets the state; a touch has sustained state
            // which should be kept
            GainFocus();
        }
        else
#endif
            return;
    }

    // Possibility for custom handling or suppression of default handling for the SDL event
    {
        using namespace SDLRawInput;

        VariantMap eventData = GetEventDataMap();
        eventData[P_SDLEVENT] = &evt;
        eventData[P_CONSUMED] = false;
        SendEvent(E_SDLRAWINPUT, eventData);

        if (eventData[P_CONSUMED].GetBool())
            return;
    }

    switch (evt.type)
    {
    case SDL_EVENT_KEY_DOWN:
        SetKey(ConvertSDLKeyCode(evt.key.keysym.sym, evt.key.keysym.scancode), (Scancode)evt.key.keysym.scancode, true);
        break;

    case SDL_EVENT_KEY_UP:
        SetKey(ConvertSDLKeyCode(evt.key.keysym.sym, evt.key.keysym.scancode), (Scancode)evt.key.keysym.scancode, false);
        break;

    case SDL_EVENT_TEXT_INPUT:
        {
            using namespace TextInput;

            VariantMap textInputEventData;
            textInputEventData[P_TEXT] = textInput_ = &evt.text.text[0];
            SendEvent(E_TEXTINPUT, textInputEventData);
        }
        break;

    case SDL_EVENT_TEXT_EDITING:
        {
            using namespace TextEditing;

            VariantMap textEditingEventData;
            textEditingEventData[P_COMPOSITION] = &evt.edit.text[0];
            textEditingEventData[P_CURSOR] = evt.edit.start;
            textEditingEventData[P_SELECTION_LENGTH] = evt.edit.length;
            SendEvent(E_TEXTEDITING, textEditingEventData);
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (!touchEmulation_)
        {
            const auto mouseButton = static_cast<MouseButton>(1u << (evt.button.button - 1u));  // NOLINT(misc-misplaced-widening-cast)
            SetMouseButton(mouseButton, true, evt.button.clicks);
        }
        else
        {
            float x, y;
            SDL_GetMouseState(&x, &y);
            x = (int)(x * inputScale_.x_);
            y = (int)(y * inputScale_.y_);

            SDL_Event event;
            event.type = SDL_EVENT_FINGER_DOWN;
            event.tfinger.touchId = 0;
            event.tfinger.fingerId = evt.button.button - 1;
            event.tfinger.pressure = 1.0f;
            event.tfinger.x = (float)x / (float)graphics.GetWidth();
            event.tfinger.y = (float)y / (float)graphics.GetHeight();
            event.tfinger.dx = 0;
            event.tfinger.dy = 0;
            SDL_PushEvent(&event);
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (!touchEmulation_)
        {
            const auto mouseButton = static_cast<MouseButton>(1u << (evt.button.button - 1u));  // NOLINT(misc-misplaced-widening-cast)
            SetMouseButton(mouseButton, false, evt.button.clicks);
        }
        else
        {
            float x, y;
            SDL_GetMouseState(&x, &y);
            x = (int)(x * inputScale_.x_);
            y = (int)(y * inputScale_.y_);

            SDL_Event event;
            event.type = SDL_EVENT_FINGER_UP;
            event.tfinger.touchId = 0;
            event.tfinger.fingerId = evt.button.button - 1;
            event.tfinger.pressure = 0.0f;
            event.tfinger.x = (float)x / (float)graphics.GetWidth();
            event.tfinger.y = (float)y / (float)graphics.GetHeight();
            event.tfinger.dx = 0;
            event.tfinger.dy = 0;
            SDL_PushEvent(&event);
        }
        break;

    case SDL_EVENT_MOUSE_MOTION:
        if ((sdlMouseRelative_ || mouseVisible_ || mouseMode_ == MM_FREE) && !touchEmulation_)
        {
            // Accumulate without scaling for accuracy, needs to be scaled to backbuffer coordinates when asked
            mouseMove_.x_ += evt.motion.xrel;
            mouseMove_.y_ += evt.motion.yrel;
            mouseMoveScaled_ = false;

            if (!suppressNextMouseMove_)
            {
                using namespace MouseMove;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_X] = (int)(evt.motion.x * inputScale_.x_);
                eventData[P_Y] = (int)(evt.motion.y * inputScale_.y_);
                // The "on-the-fly" motion data needs to be scaled now, though this may reduce accuracy
                eventData[P_DX] = (int)(evt.motion.xrel * inputScale_.x_);
                eventData[P_DY] = (int)(evt.motion.yrel * inputScale_.y_);
                eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
                eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
                SendEvent(E_MOUSEMOVE, eventData);
            }
        }
        // Only the left mouse button "finger" moves along with the mouse movement
        else if (touchEmulation_ && touches_.Contains(0))
        {
            float x, y;
            SDL_GetMouseState(&x, &y);
            x = (int)(x * inputScale_.x_);
            y = (int)(y * inputScale_.y_);

            SDL_Event event;
            event.type = SDL_EVENT_FINGER_MOTION;
            event.tfinger.touchId = 0;
            event.tfinger.fingerId = 0;
            event.tfinger.pressure = 1.0f;
            event.tfinger.x = (float)x / (float)graphics.GetWidth();
            event.tfinger.y = (float)y / (float)graphics.GetHeight();
            event.tfinger.dx = (float)evt.motion.xrel * inputScale_.x_ / (float)graphics.GetWidth();
            event.tfinger.dy = (float)evt.motion.yrel * inputScale_.y_ / (float)graphics.GetHeight();
            SDL_PushEvent(&event);
        }
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        if (!touchEmulation_)
            SetMouseWheel(evt.wheel.y);
        break;

    case SDL_EVENT_FINGER_DOWN:
        if (evt.tfinger.touchId != SDL_TOUCH_MOUSEID)
        {
            int touchID = GetTouchIndexFromID(evt.tfinger.fingerId & 0x7ffffffu);
            TouchState& state = touches_[touchID];
            state.touchID_ = touchID;
            state.lastPosition_ = state.position_ = IntVector2((int)(evt.tfinger.x * graphics.GetWidth()),
                (int)(evt.tfinger.y * graphics.GetHeight()));
            state.delta_ = IntVector2::ZERO;
            state.pressure_ = evt.tfinger.pressure;

            using namespace TouchBegin;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_TOUCHID] = touchID;
            eventData[P_X] = state.position_.x_;
            eventData[P_Y] = state.position_.y_;
            eventData[P_PRESSURE] = state.pressure_;
            SendEvent(E_TOUCHBEGIN, eventData);

            // Finger touch may move the mouse cursor. Suppress next mouse move when cursor hidden to prevent jumps
            if (!mouseVisible_)
                SuppressNextMouseMove();
        }
        break;

    case SDL_EVENT_FINGER_UP:
        if (evt.tfinger.touchId != SDL_TOUCH_MOUSEID)
        {
            int touchID = GetTouchIndexFromID(evt.tfinger.fingerId & 0x7ffffffu);
            TouchState& state = touches_[touchID];

            using namespace TouchEnd;

            VariantMap& eventData = GetEventDataMap();
            // Do not trust the position in the finger up event. Instead use the last position stored in the
            // touch structure
            eventData[P_TOUCHID] = touchID;
            eventData[P_X] = state.position_.x_;
            eventData[P_Y] = state.position_.y_;
            SendEvent(E_TOUCHEND, eventData);

            // Add touch index back to list of available touch Ids
            PushTouchIndex(evt.tfinger.fingerId & 0x7ffffffu);

            touches_.Erase(touchID);
        }
        break;

    case SDL_EVENT_FINGER_MOTION:
        if (evt.tfinger.touchId != SDL_TOUCH_MOUSEID)
        {
            int touchID = GetTouchIndexFromID(evt.tfinger.fingerId & 0x7ffffffu);
            // We don't want this event to create a new touches_ event if it doesn't exist (touchEmulation)
            if (touchEmulation_ && !touches_.Contains(touchID))
                break;
            TouchState& state = touches_[touchID];
            state.touchID_ = touchID;
            state.position_ = IntVector2((int)(evt.tfinger.x * graphics.GetWidth()),
                (int)(evt.tfinger.y * graphics.GetHeight()));
            state.delta_ = state.position_ - state.lastPosition_;
            state.pressure_ = evt.tfinger.pressure;

            using namespace TouchMove;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_TOUCHID] = touchID;
            eventData[P_X] = state.position_.x_;
            eventData[P_Y] = state.position_.y_;
            eventData[P_DX] = (int)(evt.tfinger.dx * graphics.GetWidth());
            eventData[P_DY] = (int)(evt.tfinger.dy * graphics.GetHeight());
            eventData[P_PRESSURE] = state.pressure_;
            SendEvent(E_TOUCHMOVE, eventData);

            // Finger touch may move the mouse cursor. Suppress next mouse move when cursor hidden to prevent jumps
            if (!mouseVisible_)
                SuppressNextMouseMove();
        }
        break;

    case SDL_EVENT_JOYSTICK_ADDED:
        {
            using namespace JoystickConnected;

            SDL_JoystickID joystickID = OpenJoystick(evt.jdevice.which);

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            SendEvent(E_JOYSTICKCONNECTED, eventData);
        }
        break;

    case SDL_EVENT_JOYSTICK_REMOVED:
        {
            using namespace JoystickDisconnected;

            joysticks_.Erase(evt.jdevice.which);

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = evt.jdevice.which;
            SendEvent(E_JOYSTICKDISCONNECTED, eventData);
        }
        break;

    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        {
            using namespace JoystickButtonDown;

            unsigned button = evt.jbutton.button;
            SDL_JoystickID joystickID = evt.jbutton.which;
            JoystickState& state = joysticks_[joystickID];

            // Skip ordinary joystick event for a gamepad
            if (!state.gamepad_)
            {
                VariantMap& eventData = GetEventDataMap();
                eventData[P_JOYSTICKID] = joystickID;
                eventData[P_BUTTON] = button;

                if (button < state.buttons_.Size())
                {
                    state.buttons_[button] = true;
                    state.buttonPress_[button] = true;
                    SendEvent(E_JOYSTICKBUTTONDOWN, eventData);
                }
            }
        }
        break;

    case SDL_EVENT_JOYSTICK_BUTTON_UP:
        {
            using namespace JoystickButtonUp;

            unsigned button = evt.jbutton.button;
            SDL_JoystickID joystickID = evt.jbutton.which;
            JoystickState& state = joysticks_[joystickID];

            if (!state.gamepad_)
            {
                VariantMap& eventData = GetEventDataMap();
                eventData[P_JOYSTICKID] = joystickID;
                eventData[P_BUTTON] = button;

                if (button < state.buttons_.Size())
                {
                    if (!state.gamepad_)
                        state.buttons_[button] = false;
                    SendEvent(E_JOYSTICKBUTTONUP, eventData);
                }
            }
        }
        break;

    case SDL_EVENT_JOYSTICK_AXIS_MOTION:
        {
            using namespace JoystickAxisMove;

            SDL_JoystickID joystickID = evt.jaxis.which;
            JoystickState& state = joysticks_[joystickID];

            if (!state.gamepad_)
            {
                VariantMap& eventData = GetEventDataMap();
                eventData[P_JOYSTICKID] = joystickID;
                eventData[P_AXIS] = evt.jaxis.axis;
                eventData[P_POSITION] = Clamp((float)evt.jaxis.value / 32767.0f, -1.0f, 1.0f);

                if (evt.jaxis.axis < state.axes_.Size())
                {
                    // If the joystick is a gamepad, only use the gamepad axis mappings
                    // (we'll also get the gamepad event)
                    if (!state.gamepad_)
                        state.axes_[evt.jaxis.axis] = eventData[P_POSITION].GetFloat();
                    SendEvent(E_JOYSTICKAXISMOVE, eventData);
                }
            }
        }
        break;

    case SDL_EVENT_JOYSTICK_HAT_MOTION:
        {
            using namespace JoystickHatMove;

            SDL_JoystickID joystickID = evt.jaxis.which;
            JoystickState& state = joysticks_[joystickID];

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            eventData[P_HAT] = evt.jhat.hat;
            eventData[P_POSITION] = evt.jhat.value;

            if (evt.jhat.hat < state.hats_.Size())
            {
                state.hats_[evt.jhat.hat] = evt.jhat.value;
                SendEvent(E_JOYSTICKHATMOVE, eventData);
            }
        }
        break;

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        {
            using namespace JoystickButtonDown;

            unsigned button = evt.gbutton.button;
            SDL_JoystickID joystickID = evt.gbutton.which;
            JoystickState& state = joysticks_[joystickID];

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            eventData[P_BUTTON] = button;

            if (button < state.buttons_.Size())
            {
                state.buttons_[button] = true;
                state.buttonPress_[button] = true;
                SendEvent(E_JOYSTICKBUTTONDOWN, eventData);
            }
        }
        break;

    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        {
            using namespace JoystickButtonUp;

            unsigned button = evt.gbutton.button;
            SDL_JoystickID joystickID = evt.gbutton.which;
            JoystickState& state = joysticks_[joystickID];

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            eventData[P_BUTTON] = button;

            if (button < state.buttons_.Size())
            {
                state.buttons_[button] = false;
                SendEvent(E_JOYSTICKBUTTONUP, eventData);
            }
        }
        break;

    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            using namespace JoystickAxisMove;

            SDL_JoystickID joystickID = evt.gaxis.which;
            JoystickState& state = joysticks_[joystickID];

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            eventData[P_AXIS] = evt.gaxis.axis;
            eventData[P_POSITION] = Clamp((float)evt.gaxis.value / 32767.0f, -1.0f, 1.0f);

            if (evt.gaxis.axis < state.axes_.Size())
            {
                state.axes_[evt.gaxis.axis] = eventData[P_POSITION].GetFloat();
                SendEvent(E_JOYSTICKAXISMOVE, eventData);
            }
        }
        break;

    case SDL_EVENT_WINDOW_MINIMIZED:
        minimized_ = true;
        SendInputFocusEvent();
        break;

    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_RESTORED:
#if defined(IOS) || defined(TVOS) || defined (__ANDROID__)
        // On iOS/tvOS we never lose the GL context, but may have done GPU object changes that could not be applied yet. Apply them now
        // On Android the old GL context may be lost already, restore GPU objects to the new GL context
        graphics.Restore_OGL();
#endif
        minimized_ = false;
        SendInputFocusEvent();
        break;

    case SDL_EVENT_WINDOW_RESIZED:
        graphics.OnWindowResized();
        break;

    case SDL_EVENT_WINDOW_MOVED:
        graphics.OnWindowMoved();
        break;

    case SDL_EVENT_DROP_FILE:
        {
            using namespace DropFile;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_FILENAME] = to_internal(String(evt.drop.file));
            SDL_free(evt.drop.file);

            SendEvent(E_DROPFILE, eventData);
        }
        break;

    case SDL_EVENT_QUIT:
        SendEvent(E_EXITREQUESTED);
        break;

    default: break;
    }
}

void Input::HandleScreenMode(StringHash eventType, VariantMap& eventData)
{
    if (!initialized_)
        Initialize();

    Graphics& graphics = DV_GRAPHICS;

    // Re-enable cursor clipping, and re-center the cursor (if needed) to the new screen size, so that there is no erroneous
    // mouse move event. Also get new window ID if it changed
    SDL_Window* window = graphics.GetWindow();
    windowID_ = SDL_GetWindowID(window);

    if (graphics.GetFullscreen() || !mouseVisible_)
        focusedThisFrame_ = true;

    // After setting a new screen mode we should not be minimized
    minimized_ = (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0;

    // Calculate input coordinate scaling from SDL window to backbuffer ratio
    int winWidth, winHeight;
    int gfxWidth = graphics.GetWidth();
    int gfxHeight = graphics.GetHeight();
    SDL_GetWindowSize(window, &winWidth, &winHeight);
    if (winWidth > 0 && winHeight > 0 && gfxWidth > 0 && gfxHeight > 0)
    {
        inputScale_.x_ = (float)gfxWidth / (float)winWidth;
        inputScale_.y_ = (float)gfxHeight / (float)winHeight;
    }
    else
        inputScale_ = Vector2::ONE;
}

void Input::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    // Update input right at the beginning of the frame
    SendEvent(E_INPUTBEGIN);
    Update();
    SendEvent(E_INPUTEND);
}

}
