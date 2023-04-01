// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/animated_model.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/navigation/navigable.h>
#include <dviglo/navigation/navigation_mesh.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "navigation.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Navigation)

Navigation::Navigation() :
    draw_debug_(false),
    useStreaming_(false),
    streamingDistance_(2)
{
}

void Navigation::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Create the UI content
    create_ui();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Hook up to the frame update and render post-update events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void Navigation::create_scene()
{
    ResourceCache& cache = DV_RES_CACHE;

    scene_ = new Scene();

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->create_component<Octree>();
    scene_->create_component<DebugRenderer>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode = scene_->create_child("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    auto* planeObject = planeNode->create_component<StaticModel>();
    planeObject->SetModel(cache.GetResource<Model>("models/Plane.mdl"));
    planeObject->SetMaterial(cache.GetResource<Material>("materials/StoneTiled.xml"));

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->create_child("Zone");
    auto* zone = zoneNode->create_component<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->create_child("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create some mushrooms
    const unsigned NUM_MUSHROOMS = 100;
    for (unsigned i = 0; i < NUM_MUSHROOMS; ++i)
        create_mushroom(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));

    // Create randomly sized boxes. If boxes are big enough, make them occluders
    const unsigned NUM_BOXES = 20;
    for (unsigned i = 0; i < NUM_BOXES; ++i)
    {
        Node* boxNode = scene_->create_child("Box");
        float size = 1.0f + Random(10.0f);
        boxNode->SetPosition(Vector3(Random(80.0f) - 40.0f, size * 0.5f, Random(80.0f) - 40.0f));
        boxNode->SetScale(size);
        auto* boxObject = boxNode->create_component<StaticModel>();
        boxObject->SetModel(cache.GetResource<Model>("models/Box.mdl"));
        boxObject->SetMaterial(cache.GetResource<Material>("materials/Stone.xml"));
        boxObject->SetCastShadows(true);
        if (size >= 3.0f)
            boxObject->SetOccluder(true);
    }

    // Create Jack node that will follow the path
    jackNode_ = scene_->create_child("Jack");
    jackNode_->SetPosition(Vector3(-5.0f, 0.0f, 20.0f));
    auto* modelObject = jackNode_->create_component<AnimatedModel>();
    modelObject->SetModel(cache.GetResource<Model>("models/Jack.mdl"));
    modelObject->SetMaterial(cache.GetResource<Material>("materials/Jack.xml"));
    modelObject->SetCastShadows(true);

    // Create a NavigationMesh component to the scene root
    auto* navMesh = scene_->create_component<NavigationMesh>();
    // Set small tiles to show navigation mesh streaming
    navMesh->SetTileSize(32);
    // Create a Navigable component to the scene root. This tags all of the geometry in the scene as being part of the
    // navigation mesh. By default this is recursive, but the recursion could be turned off from Navigable
    scene_->create_component<Navigable>();
    // Add padding to the navigation mesh in Y-direction so that we can add objects on top of the tallest boxes
    // in the scene and still update the mesh correctly
    navMesh->SetPadding(Vector3(0.0f, 10.0f, 0.0f));
    // Now build the navigation geometry. This will take some time. Note that the navigation mesh will prefer to use
    // physics geometry from the scene nodes, as it often is simpler, but if it can not find any (like in this example)
    // it will use renderable geometry instead
    navMesh->Build();

    // Create the camera. Limit far clip distance to match the fog
    cameraNode_ = scene_->create_child("Camera");
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane and looking down
    cameraNode_->SetPosition(Vector3(0.0f, 50.0f, 0.0f));
    pitch_ = 80.0f;
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
}

void Navigation::create_ui()
{
    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will point the raycast target
    auto* style = DV_RES_CACHE.GetResource<XmlFile>("UI/DefaultStyle.xml");
    SharedPtr<Cursor> cursor(new Cursor());
    cursor->SetStyleAuto(style);
    DV_UI.SetCursor(cursor);

    // Set starting position of the cursor at the rendering window center
    cursor->SetPosition(DV_GRAPHICS.GetWidth() / 2, DV_GRAPHICS.GetHeight() / 2);

    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText(
        "Use WASD keys to move, RMB to rotate view\n"
        "LMB to set destination, SHIFT+LMB to teleport\n"
        "MMB or O key to add or remove obstacles\n"
        "Tab to toggle navigation mesh streaming\n"
        "Space to toggle debug geometry"
    );
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void Navigation::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void Navigation::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(Navigation, handle_update));

    // Subscribe handle_post_render_update() function for processing the post-render update event, during which we request
    // debug geometry
    subscribe_to_event(E_POSTRENDERUPDATE, DV_HANDLER(Navigation, handle_post_render_update));
}

void Navigation::move_camera(float timeStep)
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    Input& input = DV_INPUT;
    DV_UI.GetCursor()->SetVisible(!input.GetMouseButtonDown(MOUSEB_RIGHT));

    // Do not move if the UI has a focused element (the console)
    if (DV_UI.GetFocusElement())
        return;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    // Only move the camera when the cursor is hidden
    if (!DV_UI.GetCursor()->IsVisible())
    {
        IntVector2 mouseMove = input.GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed.
    // Используем скан-коды, а не коды клавиш, иначе не будет работать в Linux, когда включена русская раскладка клавиатуры
    if (input.GetScancodeDown(SCANCODE_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input.GetScancodeDown(SCANCODE_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input.GetScancodeDown(SCANCODE_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input.GetScancodeDown(SCANCODE_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // Set destination or teleport with left mouse button
    if (input.GetMouseButtonPress(MOUSEB_LEFT))
        SetPathPoint();
    // Add or remove objects with middle mouse button, then rebuild navigation mesh partially
    if (input.GetMouseButtonPress(MOUSEB_MIDDLE) || input.GetScancodePress(SCANCODE_O))
        AddOrRemoveObject();

    // Toggle debug geometry with space
    if (input.GetKeyPress(KEY_SPACE))
        draw_debug_ = !draw_debug_;
}

void Navigation::SetPathPoint()
{
    Vector3 hitPos;
    Drawable* hitDrawable;
    auto* navMesh = scene_->GetComponent<NavigationMesh>();

    if (Raycast(250.0f, hitPos, hitDrawable))
    {
        Vector3 pathPos = navMesh->FindNearestPoint(hitPos, Vector3(1.0f, 1.0f, 1.0f));

        if (DV_INPUT.GetQualifierDown(QUAL_SHIFT))
        {
            // Teleport
            currentPath_.Clear();
            jackNode_->LookAt(Vector3(pathPos.x, jackNode_->GetPosition().y, pathPos.z), Vector3::UP);
            jackNode_->SetPosition(pathPos);
        }
        else
        {
            // Calculate path from Jack's current position to the end point
            endPos_ = pathPos;
            navMesh->find_path(currentPath_, jackNode_->GetPosition(), endPos_);
        }
    }
}

void Navigation::AddOrRemoveObject()
{
    // Raycast and check if we hit a mushroom node. If yes, remove it, if no, create a new one
    Vector3 hitPos;
    Drawable* hitDrawable;

    if (!useStreaming_ && Raycast(250.0f, hitPos, hitDrawable))
    {
        // The part of the navigation mesh we must update, which is the world bounding box of the associated
        // drawable component
        BoundingBox updateBox;

        Node* hitNode = hitDrawable->GetNode();
        if (hitNode->GetName() == "Mushroom")
        {
            updateBox = hitDrawable->GetWorldBoundingBox();
            hitNode->Remove();
        }
        else
        {
            Node* newNode = create_mushroom(hitPos);
            updateBox = newNode->GetComponent<StaticModel>()->GetWorldBoundingBox();
        }

        // Rebuild part of the navigation mesh, then recalculate path if applicable
        auto* navMesh = scene_->GetComponent<NavigationMesh>();
        navMesh->Build(updateBox);
        if (currentPath_.Size())
            navMesh->find_path(currentPath_, jackNode_->GetPosition(), endPos_);
    }
}

Node* Navigation::create_mushroom(const Vector3& pos)
{
    Node* mushroomNode = scene_->create_child("Mushroom");
    mushroomNode->SetPosition(pos);
    mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
    mushroomNode->SetScale(2.0f + Random(0.5f));
    auto* mushroomObject = mushroomNode->create_component<StaticModel>();
    mushroomObject->SetModel(DV_RES_CACHE.GetResource<Model>("models/Mushroom.mdl"));
    mushroomObject->SetMaterial(DV_RES_CACHE.GetResource<Material>("materials/Mushroom.xml"));
    mushroomObject->SetCastShadows(true);

    return mushroomNode;
}

bool Navigation::Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable)
{
    hitDrawable = nullptr;

    IntVector2 pos = DV_UI.GetCursorPosition();
    // Check the cursor is visible and there is no UI element in front of the cursor
    if (!DV_UI.GetCursor()->IsVisible() || DV_UI.GetElementAt(pos, true))
        return false;

    pos = DV_UI.ConvertUIToSystem(pos);

    auto* camera = cameraNode_->GetComponent<Camera>();
    Ray cameraRay = camera->GetScreenRay((float)pos.x / DV_GRAPHICS.GetWidth(), (float)pos.y / DV_GRAPHICS.GetHeight());
    // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
    Vector<RayQueryResult> results;
    RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, maxDistance, DrawableTypes::Geometry);
    scene_->GetComponent<Octree>()->RaycastSingle(query);
    if (results.Size())
    {
        RayQueryResult& result = results[0];
        hitPos = result.position_;
        hitDrawable = result.drawable_;
        return true;
    }

    return false;
}

void Navigation::FollowPath(float timeStep)
{
    if (currentPath_.Size())
    {
        Vector3 nextWaypoint = currentPath_[0]; // NB: currentPath[0] is the next waypoint in order

        // Rotate Jack toward next waypoint to reach and move. Check for not overshooting the target
        float move = 5.0f * timeStep;
        float distance = (jackNode_->GetPosition() - nextWaypoint).Length();
        if (move > distance)
            move = distance;

        jackNode_->LookAt(nextWaypoint, Vector3::UP);
        jackNode_->Translate(Vector3::FORWARD * move);

        // Remove waypoint if reached it
        if (distance < 0.1f)
            currentPath_.Erase(0);
    }
}

void Navigation::toggle_streaming(bool enabled)
{
    auto* navMesh = scene_->GetComponent<NavigationMesh>();
    if (enabled)
    {
        int maxTiles = (2 * streamingDistance_ + 1) * (2 * streamingDistance_ + 1);
        BoundingBox boundingBox = navMesh->GetBoundingBox();
        save_navigation_data();
        navMesh->Allocate(boundingBox, maxTiles);
    }
    else
        navMesh->Build();
}

void Navigation::update_streaming()
{
    // Center the navigation mesh at the jack
    auto* navMesh = scene_->GetComponent<NavigationMesh>();
    const IntVector2 jackTile = navMesh->GetTileIndex(jackNode_->GetWorldPosition());
    const IntVector2 numTiles = navMesh->GetNumTiles();
    const IntVector2 beginTile = VectorMax(IntVector2::ZERO, jackTile - IntVector2::ONE * streamingDistance_);
    const IntVector2 endTile = VectorMin(jackTile + IntVector2::ONE * streamingDistance_, numTiles - IntVector2::ONE);

    // Remove tiles
    for (HashSet<IntVector2>::Iterator i = addedTiles_.Begin(); i != addedTiles_.End();)
    {
        const IntVector2 tileIdx = *i;
        if (beginTile.x <= tileIdx.x && tileIdx.x <= endTile.x && beginTile.y <= tileIdx.y && tileIdx.y <= endTile.y)
            ++i;
        else
        {
            navMesh->RemoveTile(tileIdx);
            i = addedTiles_.Erase(i);
        }
    }

    // Add tiles
    for (int z = beginTile.y; z <= endTile.y; ++z)
        for (int x = beginTile.x; x <= endTile.x; ++x)
        {
            const IntVector2 tileIdx(x, z);
            if (!navMesh->HasTile(tileIdx) && tileData_.Contains(tileIdx))
            {
                addedTiles_.Insert(tileIdx);
                navMesh->AddTile(tileData_[tileIdx]);
            }
        }
}

void Navigation::save_navigation_data()
{
    auto* navMesh = scene_->GetComponent<NavigationMesh>();
    tileData_.Clear();
    addedTiles_.Clear();
    const IntVector2 numTiles = navMesh->GetNumTiles();
    for (int z = 0; z < numTiles.y; ++z)
        for (int x = 0; x <= numTiles.x; ++x)
        {
            const IntVector2 tileIdx = IntVector2(x, z);
            tileData_[tileIdx] = navMesh->GetTileData(tileIdx);
        }
}

void Navigation::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);

    // Make Jack follow the Detour path
    FollowPath(timeStep);

    // Update streaming
    if (DV_INPUT.GetKeyPress(KEY_TAB))
    {
        useStreaming_ = !useStreaming_;
        toggle_streaming(useStreaming_);
    }

    if (useStreaming_)
        update_streaming();
}

void Navigation::handle_post_render_update(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw navigation mesh debug geometry
    if (draw_debug_)
        scene_->GetComponent<NavigationMesh>()->draw_debug_geometry(true);

    if (currentPath_.Size())
    {
        // Visualize the current calculated path
        auto* debug = scene_->GetComponent<DebugRenderer>();
        debug->AddBoundingBox(BoundingBox(endPos_ - Vector3(0.1f, 0.1f, 0.1f), endPos_ + Vector3(0.1f, 0.1f, 0.1f)),
            Color(1.0f, 1.0f, 1.0f));

        // Draw the path with a small upward bias so that it does not clip into the surfaces
        Vector3 bias(0.0f, 0.05f, 0.0f);
        debug->AddLine(jackNode_->GetPosition() + bias, currentPath_[0] + bias, Color(1.0f, 1.0f, 1.0f));

        if (currentPath_.Size() > 1)
        {
            for (i32 i = 0; i < currentPath_.Size() - 1; ++i)
                debug->AddLine(currentPath_[i] + bias, currentPath_[i + 1] + bias, Color(1.0f, 1.0f, 1.0f));
        }
    }
}
