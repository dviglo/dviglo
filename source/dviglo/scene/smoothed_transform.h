// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

/// \file

#pragma once

#include "component.h"

namespace dviglo
{

enum SmoothingType : unsigned
{
    /// No ongoing smoothing.
    SMOOTH_NONE = 0,
    /// Ongoing position smoothing.
    SMOOTH_POSITION = 1,
    /// Ongoing rotation smoothing.
    SMOOTH_ROTATION = 2,
};
DV_FLAGSET(SmoothingType, SmoothingTypeFlags);

/// Transform smoothing component for network updates.
class DV_API SmoothedTransform : public Component
{
    DV_OBJECT(SmoothedTransform);

public:
    /// Construct.
    explicit SmoothedTransform();
    /// Destruct.
    ~SmoothedTransform() override;
    /// Register object factory.
    static void register_object();

    /// Update smoothing.
    void Update(float constant, float squaredSnapThreshold);
    /// Set target position in parent space.
    void SetTargetPosition(const Vector3& position);
    /// Set target rotation in parent space.
    void SetTargetRotation(const Quaternion& rotation);
    /// Set target position in world space.
    void SetTargetWorldPosition(const Vector3& position);
    /// Set target rotation in world space.
    void SetTargetWorldRotation(const Quaternion& rotation);

    /// Return target position in parent space.
    const Vector3& GetTargetPosition() const { return targetPosition_; }

    /// Return target rotation in parent space.
    const Quaternion& GetTargetRotation() const { return targetRotation_; }

    /// Return target position in world space.
    Vector3 GetTargetWorldPosition() const;
    /// Return target rotation in world space.
    Quaternion GetTargetWorldRotation() const;

    /// Return whether smoothing is in progress.
    bool IsInProgress() const { return smoothingMask_ != SMOOTH_NONE; }

protected:
    /// Handle scene node being assigned at creation.
    void OnNodeSet(Node* node) override;

private:
    /// Handle smoothing update event.
    void HandleUpdateSmoothing(StringHash eventType, VariantMap& eventData);

    /// Target position.
    Vector3 targetPosition_;
    /// Target rotation.
    Quaternion targetRotation_;
    /// Active smoothing operations bitmask.
    SmoothingTypeFlags smoothingMask_;
    /// Subscribed to smoothing update event flag.
    bool subscribed_;
};

}
