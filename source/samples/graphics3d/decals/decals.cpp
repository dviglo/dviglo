// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/animated_model.h>
#include <dviglo/graphics/animation_controller.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/decal_set.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "decals.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Decals)

Decals::Decals() :
    draw_debug_(false)
{
}

void Decals::Start()
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

void Decals::create_scene()
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
    planeObject->SetModel(cache.GetResource<Model>("models/plane.mdl"));
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
    const unsigned NUM_MUSHROOMS = 240;
    for (unsigned i = 0; i < NUM_MUSHROOMS; ++i)
    {
        Node* mushroomNode = scene_->create_child("Mushroom");
        mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
        mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        mushroomNode->SetScale(0.5f + Random(2.0f));
        auto* mushroomObject = mushroomNode->create_component<StaticModel>();
        mushroomObject->SetModel(cache.GetResource<Model>("models/mushroom.mdl"));
        mushroomObject->SetMaterial(cache.GetResource<Material>("materials/mushroom.xml"));
        mushroomObject->SetCastShadows(true);
    }

    // Create randomly sized boxes. If boxes are big enough, make them occluders. Occluders will be software rasterized before
    // rendering to a low-resolution depth-only buffer to test the objects in the view frustum for visibility
    const unsigned NUM_BOXES = 20;
    for (unsigned i = 0; i < NUM_BOXES; ++i)
    {
        Node* boxNode = scene_->create_child("Box");
        float size = 1.0f + Random(10.0f);
        boxNode->SetPosition(Vector3(Random(80.0f) - 40.0f, size * 0.5f, Random(80.0f) - 40.0f));
        boxNode->SetScale(size);
        auto* boxObject = boxNode->create_component<StaticModel>();
        boxObject->SetModel(cache.GetResource<Model>("models/box.mdl"));
        boxObject->SetMaterial(cache.GetResource<Material>("materials/stone.xml"));
        boxObject->SetCastShadows(true);
        if (size >= 3.0f)
            boxObject->SetOccluder(true);
    }

    // Create some animated models
    const i32 NUM_MUTANTS = 20;
    for (i32 i = 0; i < NUM_MUTANTS; ++i)
    {
        Node* mutantNode = scene_->create_child("Mutant");
        mutantNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
        mutantNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        mutantNode->SetScale(0.5f + Random(2.0f));
        AnimatedModel* mutantObject = mutantNode->create_component<AnimatedModel>();
        mutantObject->SetModel(cache.GetResource<Model>("models/Mutant/Mutant.mdl"));
        mutantObject->SetMaterial(cache.GetResource<Material>("models/Mutant/materials/mutant_M.xml"));
        mutantObject->SetCastShadows(true);
        AnimationController* animCtrl = mutantNode->create_component<AnimationController>();
        animCtrl->PlayExclusive("models/Mutant/Mutant_Idle0.ani", 0, true, 0.f);
        animCtrl->SetTime("models/Mutant/Mutant_Idle0.ani", Random(animCtrl->GetLength("models/Mutant/Mutant_Idle0.ani")));
    }

    // Create the camera. Limit far clip distance to match the fog
    cameraNode_ = scene_->create_child("Camera");
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void Decals::create_ui()
{
    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will point the raycast target
    auto* style = DV_RES_CACHE.GetResource<XmlFile>("ui/default_style.xml");
    SharedPtr<Cursor> cursor(new Cursor());
    cursor->SetStyleAuto(style);
    DV_UI.SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    cursor->SetPosition(DV_GRAPHICS.GetWidth() / 2, DV_GRAPHICS.GetHeight() / 2);

    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText(
        "Use WASD keys to move\n"
        "LMB to paint decals, RMB to rotate view\n"
        "Space to toggle debug geometry\n"
        "7 to toggle occlusion culling"
    );
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void Decals::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void Decals::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(Decals, handle_update));

    // Subscribe handle_post_render_update() function for processing the post-render update event, during which we request
    // debug geometry
    subscribe_to_event(E_POSTRENDERUPDATE, DV_HANDLER(Decals, handle_post_render_update));
}

void Decals::move_camera(float timeStep)
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

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input.GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // Toggle debug geometry with space
    if (input.GetKeyPress(KEY_SPACE))
        draw_debug_ = !draw_debug_;

    // Paint decal with the left mousebutton; cursor must be visible
    if (DV_UI.GetCursor()->IsVisible() && input.GetMouseButtonPress(MOUSEB_LEFT))
        PaintDecal();
}

void Decals::PaintDecal()
{
    Vector3 hitPos;
    Drawable* hitDrawable;

    if (Raycast(250.0f, hitPos, hitDrawable))
    {
        // Check if target scene node already has a DecalSet component. If not, create now
        Node* targetNode = hitDrawable->GetNode();
        auto* decal = targetNode->GetComponent<DecalSet>();
        if (!decal)
        {
            decal = targetNode->create_component<DecalSet>();
            decal->SetMaterial(DV_RES_CACHE.GetResource<Material>("materials/urho_decal.xml"));
        }
        // Add a square decal to the decal set using the geometry of the drawable that was hit, orient it to face the camera,
        // use full texture UV's (0,0) to (1,1). Note that if we create several decals to a large object (such as the ground
        // plane) over a large area using just one DecalSet component, the decals will all be culled as one unit. If that is
        // undesirable, it may be necessary to create more than one DecalSet based on the distance
        decal->AddDecal(hitDrawable, hitPos, cameraNode_->GetRotation(), 0.5f, 1.0f, 1.0f, Vector2::ZERO,
            Vector2::ONE);
    }
}

bool Decals::Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable)
{
    hitDrawable = nullptr;

    IntVector2 pos = DV_UI.GetCursorPosition();
    // Check the cursor is visible and there is no UI element in front of the cursor
    if (!DV_UI.GetCursor()->IsVisible() || DV_UI.GetElementAt(pos, true))
        return false;

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

void Decals::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);
}

void Decals::handle_post_render_update(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw viewport debug geometry. Disable depth test so that we can see the effect of occlusion
    if (draw_debug_)
        DV_RENDERER.draw_debug_geometry(false);
}
