// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../scene/component.h"

namespace dviglo
{

/// A link between otherwise unconnected regions of the navigation mesh.
class DV_API OffMeshConnection : public Component
{
    DV_OBJECT(OffMeshConnection, Component);

public:
    /// Construct.
    explicit OffMeshConnection(Context* context);
    /// Destruct.
    ~OffMeshConnection() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    void ApplyAttributes() override;
    /// Visualize the component as debug geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Set endpoint node.
    void SetEndPoint(Node* node);
    /// Set radius.
    void SetRadius(float radius);
    /// Set bidirectional flag. Default true.
    void SetBidirectional(bool enabled);
    /// Set a user assigned mask.
    void SetMask(unsigned newMask);
    /// Sets the assigned area Id for the connection.
    void SetAreaID(unsigned newAreaID);

    /// Return endpoint node.
    Node* GetEndPoint() const;

    /// Return radius.
    float GetRadius() const { return radius_; }

    /// Return whether is bidirectional.
    bool IsBidirectional() const { return bidirectional_; }

    /// Return the user assigned mask.
    unsigned GetMask() const { return mask_; }

    /// Return the user assigned area ID.
    unsigned GetAreaID() const { return areaId_; }

private:
    /// Mark end point dirty.
    void MarkEndPointDirty() { endPointDirty_ = true; }
    /// Endpoint node.
    WeakPtr<Node> endPoint_;
    /// Endpoint node ID.
    unsigned endPointID_;
    /// Radius.
    float radius_;
    /// Bidirectional flag.
    bool bidirectional_;
    /// Endpoint changed flag.
    bool endPointDirty_;
    /// Flags mask to represent properties of this mesh.
    unsigned mask_;
    /// Area id to be used for this off mesh connection's internal poly.
    unsigned areaId_;
};

}
