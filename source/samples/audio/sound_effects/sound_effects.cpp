// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/audio/audio.h>
#include <dviglo/audio/audio_events.h>
#include <dviglo/audio/sound.h>
#include <dviglo/audio/sound_source.h>
#include <dviglo/engine/engine.h>
#include <dviglo/input/input.h>
#include <dviglo/io/log.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/slider.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>

#include "sound_effects.h"

#include <dviglo/common/debug_new.h>

// Custom variable identifier for storing sound effect name within the UI element
static const StringHash VAR_SOUNDRESOURCE("SoundResource");
static const unsigned NUM_SOUNDS = 3;

static const char* soundNames[] = {
    "Fist",
    "Explosion",
    "Power-up"
};

static const char* soundResourceNames[] = {
    "sounds/player_fist_hit.wav",
    "sounds/big_explosion.wav",
    "sounds/powerup.wav"
};

DV_DEFINE_APPLICATION_MAIN(SoundEffects)

SoundEffects::SoundEffects() :
    musicSource_(nullptr)
{
}

void SoundEffects::Setup()
{
    // Modify engine startup parameters
    Sample::Setup();
    engineParameters_[EP_SOUND] = true;
}

void SoundEffects::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create a scene which will not be actually rendered, but is used to hold SoundSource components while they play sounds
    scene_ = new Scene();

    // Create music sound source
    musicSource_ = scene_->create_component<SoundSource>();
    // Set the sound type to music so that master volume control works correctly
    musicSource_->SetSoundType(SOUND_MUSIC);

    // Enable OS cursor
    DV_INPUT.SetMouseVisible(true);

    // Create the user interface
    create_ui();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void SoundEffects::create_ui()
{
    UiElement* root = DV_UI.GetRoot();
    auto* uiStyle = DV_RES_CACHE.GetResource<XmlFile>("ui/default_style.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    // Create buttons for playing back sounds
    for (unsigned i = 0; i < NUM_SOUNDS; ++i)
    {
        Button* button = CreateButton(i * 140 + 20, 20, 120, 40, soundNames[i]);
        // Store the sound effect resource name as a custom variable into the button
        button->SetVar(VAR_SOUNDRESOURCE, soundResourceNames[i]);
        subscribe_to_event(button, E_PRESSED, DV_HANDLER(SoundEffects, HandlePlaySound));
    }

    // Create buttons for playing/stopping music
    Button* button = CreateButton(20, 80, 120, 40, "Play Music");
    subscribe_to_event(button, E_RELEASED, DV_HANDLER(SoundEffects, HandlePlayMusic));

    button = CreateButton(160, 80, 120, 40, "Stop Music");
    subscribe_to_event(button, E_RELEASED, DV_HANDLER(SoundEffects, HandleStopMusic));

    // Create sliders for controlling sound and music master volume
    Slider* slider = CreateSlider(20, 140, 200, 20, "Sound Volume");
    slider->SetValue(DV_AUDIO.GetMasterGain(SOUND_EFFECT));
    subscribe_to_event(slider, E_SLIDERCHANGED, DV_HANDLER(SoundEffects, HandleSoundVolume));

    slider = CreateSlider(20, 200, 200, 20, "Music Volume");
    slider->SetValue(DV_AUDIO.GetMasterGain(SOUND_MUSIC));
    subscribe_to_event(slider, E_SLIDERCHANGED, DV_HANDLER(SoundEffects, HandleMusicVolume));
}

Button* SoundEffects::CreateButton(int x, int y, int xSize, int ySize, const String& text)
{
    UiElement* root = DV_UI.GetRoot();
    auto* font = DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf");

    // Create the button and center the text onto it
    auto* button = root->create_child<Button>();
    button->SetStyleAuto();
    button->SetPosition(x, y);
    button->SetSize(xSize, ySize);

    auto* buttonText = button->create_child<Text>();
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetFont(font, 12);
    buttonText->SetText(text);

    return button;
}

Slider* SoundEffects::CreateSlider(int x, int y, int xSize, int ySize, const String& text)
{
    UiElement* root = DV_UI.GetRoot();
    auto* font = DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf");

    // Create text and slider below it
    auto* sliderText = root->create_child<Text>();
    sliderText->SetPosition(x, y);
    sliderText->SetFont(font, 12);
    sliderText->SetText(text);

    auto* slider = root->create_child<Slider>();
    slider->SetStyleAuto();
    slider->SetPosition(x, y + 20);
    slider->SetSize(xSize, ySize);
    // Use 0-1 range for controlling sound/music master volume
    slider->SetRange(1.0f);

    return slider;
}

void SoundEffects::HandlePlaySound(StringHash eventType, VariantMap& eventData)
{
    auto* button = static_cast<Button*>(GetEventSender());
    const String& soundResourceName = button->GetVar(VAR_SOUNDRESOURCE).GetString();

    // Get the sound resource
    auto* sound = DV_RES_CACHE.GetResource<Sound>(soundResourceName);

    if (sound)
    {
        // Create a SoundSource component for playing the sound. The SoundSource component plays
        // non-positional audio, so its 3D position in the scene does not matter. For positional sounds the
        // SoundSource3D component would be used instead
        auto* soundSource = scene_->create_component<SoundSource>();
        // Component will automatically remove itself when the sound finished playing
        soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
        soundSource->Play(sound);
        // In case we also play music, set the sound volume below maximum so that we don't clip the output
        soundSource->SetGain(0.75f);
    }
}

void SoundEffects::HandlePlayMusic(StringHash eventType, VariantMap& eventData)
{
    auto* music = DV_RES_CACHE.GetResource<Sound>("music/ninja gods.ogg");
    // Set the song to loop
    music->SetLooped(true);

    musicSource_->Play(music);
}

void SoundEffects::HandleStopMusic(StringHash eventType, VariantMap& eventData)
{
    // Remove the music player node from the scene
    musicSource_->Stop();
}

void SoundEffects::HandleSoundVolume(StringHash eventType, VariantMap& eventData)
{
    using namespace SliderChanged;

    float newVolume = eventData[P_VALUE].GetFloat();
    DV_AUDIO.SetMasterGain(SOUND_EFFECT, newVolume);
}

void SoundEffects::HandleMusicVolume(StringHash eventType, VariantMap& eventData)
{
    using namespace SliderChanged;

    float newVolume = eventData[P_VALUE].GetFloat();
    DV_AUDIO.SetMasterGain(SOUND_MUSIC, newVolume);
}
