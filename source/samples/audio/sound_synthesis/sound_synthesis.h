// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{

class Node;
class BufferedSoundStream;

}

/// Sound synthesis example.
/// This sample demonstrates:
///     - Playing back a sound stream produced on-the-fly by a simple CPU synthesis algorithm
class SoundSynthesis : public Sample
{
    DV_OBJECT(SoundSynthesis, Sample);

public:
    /// Construct.
    explicit SoundSynthesis();

    /// Setup before engine initialization. Modifies the engine parameters.
    void Setup() override;
    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the sound stream and start playback.
    void CreateSound();
    /// Buffer more sound data.
    void UpdateSound();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    /// Scene node for the sound component.
    SharedPtr<Node> node_;
    /// Sound stream that we update.
    SharedPtr<BufferedSoundStream> soundStream_;
    /// Instruction text.
    SharedPtr<Text> instructionText_;
    /// Filter coefficient for the sound.
    float filter_;
    /// Synthesis accumulator.
    float accumulator_;
    /// First oscillator.
    float osc1_;
    /// Second oscillator.
    float osc2_;
};
