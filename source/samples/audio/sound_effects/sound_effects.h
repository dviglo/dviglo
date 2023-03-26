// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{

class Button;
class Scene;
class Slider;

}

/// Sound effects example
/// This sample demonstrates:
///     - Playing sound effects and music
///     - Controlling sound and music master volume
class SoundEffects : public Sample
{
    DV_OBJECT(SoundEffects, Sample);

public:
    /// Construct.
    explicit SoundEffects();

    /// Setup before engine initialization. Modifies the engine parameters.
    void Setup() override;
    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    SoundSource* musicSource_;

private:
    /// Create the UI and subscribes to UI events.
    void create_ui();
    /// Create a button at position with specified text in it.
    Button* CreateButton(int x, int y, int xSize, int ySize, const String& text);
    /// Create a horizontal slider with specified text above it.
    Slider* CreateSlider(int x, int y, int xSize, int ySize, const String& text);
    /// Handle a sound effect button click.
    void HandlePlaySound(StringHash eventType, VariantMap& eventData);
    /// Handle "play music" button click.
    void HandlePlayMusic(StringHash eventType, VariantMap& eventData);
    /// Handle "stop music" button click.
    void HandleStopMusic(StringHash eventType, VariantMap& eventData);
    /// Handle sound effects volume slider change.
    void HandleSoundVolume(StringHash eventType, VariantMap& eventData);
    /// Handle music volume slider change.
    void HandleMusicVolume(StringHash eventType, VariantMap& eventData);
};


