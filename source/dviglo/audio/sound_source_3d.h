// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "sound_source.h"

namespace dviglo
{

class Audio;

/// %Sound source component with three-dimensional position.
class DV_API SoundSource3D : public SoundSource
{
    DV_OBJECT(SoundSource3D);

public:
    /// Construct.
    explicit SoundSource3D();
    /// Register object factory.
    static void register_object();

    /// Visualize the component as debug geometry.
    void draw_debug_geometry(DebugRenderer* debug, bool depthTest) override;
    /// Update sound source.
    void Update(float timeStep) override;

    /// Set attenuation parameters.
    void SetDistanceAttenuation(float nearDistance, float farDistance, float rolloffFactor);
    /// Set angle attenuation parameters.
    void SetAngleAttenuation(float innerAngle, float outerAngle);
    /// Set near distance. Inside this range sound will not be attenuated.
    void SetNearDistance(float distance);
    /// Set far distance. Outside this range sound will be completely attenuated.
    void SetFarDistance(float distance);
    /// Set inner angle in degrees. Inside this angle sound will not be attenuated.By default 360, meaning direction never has an effect.
    void SetInnerAngle(float angle);
    /// Set outer angle in degrees. Outside this angle sound will be completely attenuated. By default 360, meaning direction never has an effect.
    void SetOuterAngle(float angle);
    /// Set rolloff power factor, defines attenuation function shape.
    void SetRolloffFactor(float factor);
    /// Calculate attenuation and panning based on current position and listener position.
    void CalculateAttenuation();

    /// Return near distance.
    float GetNearDistance() const { return nearDistance_; }

    /// Return far distance.
    float GetFarDistance() const { return farDistance_; }

    /// Return inner angle in degrees.
    float GetInnerAngle() const { return innerAngle_; }

    /// Return outer angle in degrees.
    float GetOuterAngle() const { return outerAngle_; }

    /// Return rolloff power factor.
    float RollAngleoffFactor() const { return rolloffFactor_; }

protected:
    /// Near distance.
    float nearDistance_;
    /// Far distance.
    float farDistance_;
    /// Inner angle for directional attenuation.
    float innerAngle_;
    /// Outer angle for directional attenuation.
    float outerAngle_;
    /// Rolloff power factor.
    float rolloffFactor_;
};

}
