// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/skybox.h>
#include <dviglo/graphics/terrain.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/graphics_api/render_surface.h>
#include <dviglo/graphics_api/texture_2d.h>
#include <dviglo/input/input.h>
#include <dviglo/io/file.h>
#include <dviglo/io/file_system.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "water.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Water)

Water::Water()
{
}

void Water::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Create the UI content
    create_instructions();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Hook up to the frame update event
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void Water::create_scene()
{
    ResourceCache& cache = DV_RES_CACHE;

    scene_ = new Scene();

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    scene_->create_component<Octree>();

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->create_child("Zone");
    auto* zone = zoneNode->create_component<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(1.0f, 1.0f, 1.0f));
    zone->SetFogStart(500.0f);
    zone->SetFogEnd(750.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->create_child("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    light->SetSpecularIntensity(0.5f);
    // Apply slightly overbright lighting to match the skybox
    light->SetColor(Color(1.2f, 1.2f, 1.2f));

    // Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
    // illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
    // generate the necessary 3D texture coordinates for cube mapping
    Node* skyNode = scene_->create_child("Sky");
    skyNode->SetScale(500.0f); // The scale actually does not matter
    auto* skybox = skyNode->create_component<Skybox>();
    skybox->SetModel(cache.GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache.GetResource<Material>("materials/Skybox.xml"));

    // Create heightmap terrain
    Node* terrainNode = scene_->create_child("Terrain");
    terrainNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    auto* terrain = terrainNode->create_component<Terrain>();
    terrain->SetPatchSize(64);
    terrain->SetSpacing(Vector3(2.0f, 0.5f, 2.0f)); // Spacing between vertices and vertical resolution of the height map
    terrain->SetSmoothing(true);
    terrain->SetHeightMap(cache.GetResource<Image>("Textures/HeightMap.png"));
    terrain->SetMaterial(cache.GetResource<Material>("materials/Terrain.xml"));
    // The terrain consists of large triangles, which fits well for occlusion rendering, as a hill can occlude all
    // terrain patches and other objects behind it
    terrain->SetOccluder(true);

    // Create 1000 boxes in the terrain. Always face outward along the terrain normal
    unsigned NUM_OBJECTS = 1000;
    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* objectNode = scene_->create_child("Box");
        Vector3 position(Random(2000.0f) - 1000.0f, 0.0f, Random(2000.0f) - 1000.0f);
        position.y = terrain->GetHeight(position) + 2.25f;
        objectNode->SetPosition(position);
        // Create a rotation quaternion from up vector to terrain normal
        objectNode->SetRotation(Quaternion(Vector3(0.0f, 1.0f, 0.0f), terrain->GetNormal(position)));
        objectNode->SetScale(5.0f);
        auto* object = objectNode->create_component<StaticModel>();
        object->SetModel(cache.GetResource<Model>("Models/Box.mdl"));
        object->SetMaterial(cache.GetResource<Material>("materials/Stone.xml"));
        object->SetCastShadows(true);
    }

    // Create a water plane object that is as large as the terrain
    waterNode_ = scene_->create_child("Water");
    waterNode_->SetScale(Vector3(2048.0f, 1.0f, 2048.0f));
    waterNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
    auto* water = waterNode_->create_component<StaticModel>();
    water->SetModel(cache.GetResource<Model>("Models/Plane.mdl"));
    water->SetMaterial(cache.GetResource<Material>("materials/Water.xml"));
    // Set a different viewmask on the water plane to be able to hide it from the reflection camera
    water->SetViewMask(0x80000000);

    // Create the camera. Set far clip to match the fog. Note: now we actually create the camera node outside
    // the scene, because we want it to be unaffected by scene load / save
    cameraNode_ = new Node();
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(750.0f);

    // Set an initial position for the camera scene node above the ground
    cameraNode_->SetPosition(Vector3(0.0f, 7.0f, -20.0f));
}

void Water::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText("Use WASD keys and mouse to move");
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void Water::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);

    // Create a mathematical plane to represent the water in calculations
    waterPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition());
    // Create a downward biased plane for reflection view clipping. Biasing is necessary to avoid too aggressive clipping
    waterClipPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition() -
        Vector3(0.0f, 0.1f, 0.0f));

    // Create camera for water reflection
    // It will have the same farclip and position as the main viewport camera, but uses a reflection plane to modify
    // its position when rendering
    reflectionCameraNode_ = cameraNode_->create_child();
    auto* reflectionCamera = reflectionCameraNode_->create_component<Camera>();
    reflectionCamera->SetFarClip(750.0);
    reflectionCamera->SetViewMask(0x7fffffff); // Hide objects with only bit 31 in the viewmask (the water plane)
    reflectionCamera->SetAutoAspectRatio(false);
    reflectionCamera->SetUseReflection(true);
    reflectionCamera->SetReflectionPlane(waterPlane_);
    reflectionCamera->SetUseClipping(true); // Enable clipping of geometry behind water plane
    reflectionCamera->SetClipPlane(waterClipPlane_);
    // The water reflection texture is rectangular. Set reflection camera aspect ratio to match
    reflectionCamera->SetAspectRatio((float)DV_GRAPHICS.GetWidth() / (float)DV_GRAPHICS.GetHeight());
    // View override flags could be used to optimize reflection rendering. For example disable shadows
    //reflectionCamera->SetViewOverrideFlags(VO_DISABLE_SHADOWS);

    // Create a texture and setup viewport for water reflection. Assign the reflection texture to the diffuse
    // texture unit of the water material
    int texSize = 1024;
    SharedPtr<Texture2D> renderTexture(new Texture2D());
    renderTexture->SetSize(texSize, texSize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    renderTexture->SetFilterMode(FILTER_BILINEAR);
    RenderSurface* surface = renderTexture->GetRenderSurface();
    SharedPtr<Viewport> rttViewport(new Viewport(scene_, reflectionCamera));
    surface->SetViewport(0, rttViewport);
    auto* waterMat = DV_RES_CACHE.GetResource<Material>("materials/Water.xml");
    waterMat->SetTexture(TU_DIFFUSE, renderTexture);
}

void Water::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(Water, handle_update));
}

void Water::move_camera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (DV_UI.GetFocusElement())
        return;

    Input& input = DV_INPUT;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input.GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input.GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // In case resolution has changed, adjust the reflection camera aspect ratio
    auto* reflectionCamera = reflectionCameraNode_->GetComponent<Camera>();
    reflectionCamera->SetAspectRatio((float)DV_GRAPHICS.GetWidth() / (float)DV_GRAPHICS.GetHeight());
}

void Water::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);
}
