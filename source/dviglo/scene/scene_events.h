// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Variable timestep scene update.
DV_EVENT(E_SCENEUPDATE, SceneUpdate)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Scene subsystem update.
DV_EVENT(E_SCENESUBSYSTEMUPDATE, SceneSubsystemUpdate)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Scene transform smoothing update.
DV_EVENT(E_UPDATESMOOTHING, UpdateSmoothing)
{
    DV_PARAM(P_CONSTANT, Constant);            // float
    DV_PARAM(P_SQUAREDSNAPTHRESHOLD, SquaredSnapThreshold);  // float
}

/// Scene drawable update finished. Custom animation (eg. IK) can be done at this point.
DV_EVENT(E_SCENEDRAWABLEUPDATEFINISHED, SceneDrawableUpdateFinished)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// SmoothedTransform target position changed.
DV_EVENT(E_TARGETPOSITION, TargetPositionChanged)
{
}

/// SmoothedTransform target position changed.
DV_EVENT(E_TARGETROTATION, TargetRotationChanged)
{
}

/// Scene attribute animation update.
DV_EVENT(E_ATTRIBUTEANIMATIONUPDATE, AttributeAnimationUpdate)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Attribute animation added to object animation.
DV_EVENT(E_ATTRIBUTEANIMATIONADDED, AttributeAnimationAdded)
{
    DV_PARAM(P_OBJECTANIMATION, ObjectAnimation);               // Object animation pointer
    DV_PARAM(P_ATTRIBUTEANIMATIONNAME, AttributeAnimationName); // String
}

/// Attribute animation removed from object animation.
DV_EVENT(E_ATTRIBUTEANIMATIONREMOVED, AttributeAnimationRemoved)
{
    DV_PARAM(P_OBJECTANIMATION, ObjectAnimation);               // Object animation pointer
    DV_PARAM(P_ATTRIBUTEANIMATIONNAME, AttributeAnimationName); // String
}

/// Variable timestep scene post-update.
DV_EVENT(E_SCENEPOSTUPDATE, ScenePostUpdate)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Asynchronous scene loading progress.
DV_EVENT(E_ASYNCLOADPROGRESS, AsyncLoadProgress)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_PROGRESS, Progress);            // float
    DV_PARAM(P_LOADEDNODES, LoadedNodes);      // int
    DV_PARAM(P_TOTALNODES, TotalNodes);        // int
    DV_PARAM(P_LOADEDRESOURCES, LoadedResources); // int
    DV_PARAM(P_TOTALRESOURCES, TotalResources);   // int
}

/// Asynchronous scene loading finished.
DV_EVENT(E_ASYNCLOADFINISHED, AsyncLoadFinished)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
}

/// A child node has been added to a parent node.
DV_EVENT(E_NODEADDED, NodeAdded)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_PARENT, Parent);                // Node pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
}

/// A child node is about to be removed from a parent node. Note that individual component removal events will not be sent.
DV_EVENT(E_NODEREMOVED, NodeRemoved)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_PARENT, Parent);                // Node pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
}

/// A component has been created to a node.
DV_EVENT(E_COMPONENTADDED, ComponentAdded)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_COMPONENT, Component);          // Component pointer
}

/// A component is about to be removed from a node.
DV_EVENT(E_COMPONENTREMOVED, ComponentRemoved)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_COMPONENT, Component);          // Component pointer
}

/// A node's name has changed.
DV_EVENT(E_NODENAMECHANGED, NodeNameChanged)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
}

/// A node's enabled state has changed.
DV_EVENT(E_NODEENABLEDCHANGED, NodeEnabledChanged)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
}

/// A node's tag has been added.
DV_EVENT(E_NODETAGADDED, NodeTagAdded)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_TAG, Tag);                      // String tag
}

/// A node's tag has been removed.
DV_EVENT(E_NODETAGREMOVED, NodeTagRemoved)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_TAG, Tag);                      // String tag
}

/// A component's enabled state has changed.
DV_EVENT(E_COMPONENTENABLEDCHANGED, ComponentEnabledChanged)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_COMPONENT, Component);          // Component pointer
}

/// A serializable's temporary state has changed.
DV_EVENT(E_TEMPORARYCHANGED, TemporaryChanged)
{
    DV_PARAM(P_SERIALIZABLE, Serializable);    // Serializable pointer
}

/// A node (and its children and components) has been cloned.
DV_EVENT(E_NODECLONED, NodeCloned)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_CLONENODE, CloneNode);          // Node pointer
}

/// A component has been cloned.
DV_EVENT(E_COMPONENTCLONED, ComponentCloned)
{
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_COMPONENT, Component);          // Component pointer
    DV_PARAM(P_CLONECOMPONENT, CloneComponent); // Component pointer
}

/// A network attribute update from the server has been intercepted.
DV_EVENT(E_INTERCEPTNETWORKUPDATE, InterceptNetworkUpdate)
{
    DV_PARAM(P_SERIALIZABLE, Serializable);    // Serializable pointer
    DV_PARAM(P_TIMESTAMP, TimeStamp);          // unsigned (0-255)
    DV_PARAM(P_INDEX, Index);                  // unsigned
    DV_PARAM(P_NAME, Name);                    // String
    DV_PARAM(P_VALUE, Value);                  // Variant
}

}
