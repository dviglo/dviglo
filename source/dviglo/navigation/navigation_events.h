// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Complete rebuild of navigation mesh.
DV_EVENT(E_NAVIGATION_MESH_REBUILT, NavigationMeshRebuilt)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_MESH, Mesh); // NavigationMesh pointer
}

/// Partial bounding box rebuild of navigation mesh.
DV_EVENT(E_NAVIGATION_AREA_REBUILT, NavigationAreaRebuilt)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_MESH, Mesh); // NavigationMesh pointer
    DV_PARAM(P_BOUNDSMIN, BoundsMin); // Vector3
    DV_PARAM(P_BOUNDSMAX, BoundsMax); // Vector3
}

/// Mesh tile is added to navigation mesh.
DV_EVENT(E_NAVIGATION_TILE_ADDED, NavigationTileAdded)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_MESH, Mesh); // NavigationMesh pointer
    DV_PARAM(P_TILE, Tile); // IntVector2
}

/// Mesh tile is removed from navigation mesh.
DV_EVENT(E_NAVIGATION_TILE_REMOVED, NavigationTileRemoved)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_MESH, Mesh); // NavigationMesh pointer
    DV_PARAM(P_TILE, Tile); // IntVector2
}

/// All mesh tiles are removed from navigation mesh.
DV_EVENT(E_NAVIGATION_ALL_TILES_REMOVED, NavigationAllTilesRemoved)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_MESH, Mesh); // NavigationMesh pointer
}

/// Crowd agent formation.
DV_EVENT(E_CROWD_AGENT_FORMATION, CrowdAgentFormation)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DV_PARAM(P_INDEX, Index); // unsigned
    DV_PARAM(P_SIZE, Size); // unsigned
    DV_PARAM(P_POSITION, Position); // Vector3 [in/out]
}

/// Crowd agent formation specific to a node.
DV_EVENT(E_CROWD_AGENT_NODE_FORMATION, CrowdAgentNodeFormation)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DV_PARAM(P_INDEX, Index); // unsigned
    DV_PARAM(P_SIZE, Size); // unsigned
    DV_PARAM(P_POSITION, Position); // Vector3 [in/out]
}

/// Crowd agent has been repositioned.
DV_EVENT(E_CROWD_AGENT_REPOSITION, CrowdAgentReposition)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DV_PARAM(P_POSITION, Position); // Vector3
    DV_PARAM(P_VELOCITY, Velocity); // Vector3
    DV_PARAM(P_ARRIVED, Arrived); // bool
    DV_PARAM(P_TIMESTEP, TimeStep); // float
}

/// Crowd agent has been repositioned, specific to a node.
DV_EVENT(E_CROWD_AGENT_NODE_REPOSITION, CrowdAgentNodeReposition)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DV_PARAM(P_POSITION, Position); // Vector3
    DV_PARAM(P_VELOCITY, Velocity); // Vector3
    DV_PARAM(P_ARRIVED, Arrived); // bool
    DV_PARAM(P_TIMESTEP, TimeStep); // float
}

/// Crowd agent's internal state has become invalidated. This is a special case of CrowdAgentStateChanged event.
DV_EVENT(E_CROWD_AGENT_FAILURE, CrowdAgentFailure)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DV_PARAM(P_POSITION, Position); // Vector3
    DV_PARAM(P_VELOCITY, Velocity); // Vector3
    DV_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    DV_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Crowd agent's internal state has become invalidated. This is a special case of CrowdAgentStateChanged event.
DV_EVENT(E_CROWD_AGENT_NODE_FAILURE, CrowdAgentNodeFailure)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DV_PARAM(P_POSITION, Position); // Vector3
    DV_PARAM(P_VELOCITY, Velocity); // Vector3
    DV_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    DV_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Crowd agent's state has been changed.
DV_EVENT(E_CROWD_AGENT_STATE_CHANGED, CrowdAgentStateChanged)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DV_PARAM(P_POSITION, Position); // Vector3
    DV_PARAM(P_VELOCITY, Velocity); // Vector3
    DV_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    DV_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Crowd agent's state has been changed.
DV_EVENT(E_CROWD_AGENT_NODE_STATE_CHANGED, CrowdAgentNodeStateChanged)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    DV_PARAM(P_POSITION, Position); // Vector3
    DV_PARAM(P_VELOCITY, Velocity); // Vector3
    DV_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    DV_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Addition of obstacle to dynamic navigation mesh.
DV_EVENT(E_NAVIGATION_OBSTACLE_ADDED, NavigationObstacleAdded)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_OBSTACLE, Obstacle); // Obstacle pointer
    DV_PARAM(P_POSITION, Position); // Vector3
    DV_PARAM(P_RADIUS, Radius); // float
    DV_PARAM(P_HEIGHT, Height); // float
}

/// Removal of obstacle from dynamic navigation mesh.
DV_EVENT(E_NAVIGATION_OBSTACLE_REMOVED, NavigationObstacleRemoved)
{
    DV_PARAM(P_NODE, Node); // Node pointer
    DV_PARAM(P_OBSTACLE, Obstacle); // Obstacle pointer
    DV_PARAM(P_POSITION, Position); // Vector3
    DV_PARAM(P_RADIUS, Radius); // float
    DV_PARAM(P_HEIGHT, Height); // float
}

}
