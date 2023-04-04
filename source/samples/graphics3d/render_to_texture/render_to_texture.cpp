// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/technique.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/graphics_api/render_surface.h>
#include <dviglo/graphics_api/texture_2d.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "render_to_texture.h"
#include "rotator.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(RenderToTexture)

RenderToTexture::RenderToTexture()
{
    // Register an object factory for our custom Rotator component so that we can create them to scene nodes
    DV_CONTEXT.RegisterFactory<Rotator>();
}

void RenderToTexture::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Create the UI content
    create_instructions();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Hook up to the frame update events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void RenderToTexture::create_scene()
{
    ResourceCache& cache = DV_RES_CACHE;

    {
        // Create the scene which will be rendered to a texture
        rttScene_ = new Scene();

        // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
        rttScene_->create_component<Octree>();

        // Create a Zone for ambient light & fog control
        Node* zoneNode = rttScene_->create_child("Zone");
        auto* zone = zoneNode->create_component<Zone>();
        // Set same volume as the Octree, set a close bluish fog and some ambient light
        zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
        zone->SetAmbientColor(Color(0.05f, 0.1f, 0.15f));
        zone->SetFogColor(Color(0.1f, 0.2f, 0.3f));
        zone->SetFogStart(10.0f);
        zone->SetFogEnd(100.0f);

        // Create randomly positioned and oriented box StaticModels in the scene
        const unsigned NUM_OBJECTS = 2000;
        for (unsigned i = 0; i < NUM_OBJECTS; ++i)
        {
            Node* boxNode = rttScene_->create_child("Box");
            boxNode->SetPosition(Vector3(Random(200.0f) - 100.0f, Random(200.0f) - 100.0f, Random(200.0f) - 100.0f));
            // Orient using random pitch, yaw and roll Euler angles
            boxNode->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
            auto* boxObject = boxNode->create_component<StaticModel>();
            boxObject->SetModel(cache.GetResource<Model>("models/box.mdl"));
            boxObject->SetMaterial(cache.GetResource<Material>("materials/stone.xml"));

            // Add our custom Rotator component which will rotate the scene node each frame, when the scene sends its update event.
            // Simply set same rotation speed for all objects
            auto* rotator = boxNode->create_component<Rotator>();
            rotator->SetRotationSpeed(Vector3(10.0f, 20.0f, 30.0f));
        }

        // Create a camera for the render-to-texture scene. Simply leave it at the world origin and let it observe the scene
        rttCameraNode_ = rttScene_->create_child("Camera");
        auto* camera = rttCameraNode_->create_component<Camera>();
        camera->SetFarClip(100.0f);

        // Create a point light to the camera scene node
        auto* light = rttCameraNode_->create_component<Light>();
        light->SetLightType(LIGHT_POINT);
        light->SetRange(30.0f);
    }

    {
        // Create the scene in which we move around
        scene_ = new Scene();

        // Create octree, use also default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
        scene_->create_component<Octree>();

        // Create a Zone component for ambient lighting & fog control
        Node* zoneNode = scene_->create_child("Zone");
        auto* zone = zoneNode->create_component<Zone>();
        zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
        zone->SetAmbientColor(Color(0.1f, 0.1f, 0.1f));
        zone->SetFogStart(100.0f);
        zone->SetFogEnd(300.0f);

        // Create a directional light without shadows
        Node* lightNode = scene_->create_child("DirectionalLight");
        lightNode->SetDirection(Vector3(0.5f, -1.0f, 0.5f));
        auto* light = lightNode->create_component<Light>();
        light->SetLightType(LIGHT_DIRECTIONAL);
        light->SetColor(Color(0.2f, 0.2f, 0.2f));
        light->SetSpecularIntensity(1.0f);

        // Create a "floor" consisting of several tiles
        for (int y = -5; y <= 5; ++y)
        {
            for (int x = -5; x <= 5; ++x)
            {
                Node* floorNode = scene_->create_child("FloorTile");
                floorNode->SetPosition(Vector3(x * 20.5f, -0.5f, y * 20.5f));
                floorNode->SetScale(Vector3(20.0f, 1.0f, 20.f));
                auto* floorObject = floorNode->create_component<StaticModel>();
                floorObject->SetModel(cache.GetResource<Model>("models/box.mdl"));
                floorObject->SetMaterial(cache.GetResource<Material>("materials/stone.xml"));
            }
        }

        // Create a "screen" like object for viewing the second scene. Construct it from two StaticModels, a box for the frame
        // and a plane for the actual view
        {
            Node* boxNode = scene_->create_child("ScreenBox");
            boxNode->SetPosition(Vector3(0.0f, 10.0f, 0.0f));
            boxNode->SetScale(Vector3(21.0f, 16.0f, 0.5f));
            auto* boxObject = boxNode->create_component<StaticModel>();
            boxObject->SetModel(cache.GetResource<Model>("models/box.mdl"));
            boxObject->SetMaterial(cache.GetResource<Material>("materials/stone.xml"));

            Node* screenNode = scene_->create_child("Screen");
            screenNode->SetPosition(Vector3(0.0f, 10.0f, -0.27f));
            screenNode->SetRotation(Quaternion(-90.0f, 0.0f, 0.0f));
            screenNode->SetScale(Vector3(20.0f, 0.0f, 15.0f));
            auto* screenObject = screenNode->create_component<StaticModel>();
            screenObject->SetModel(cache.GetResource<Model>("models/plane.mdl"));

            // Create a renderable texture (1024x768, RGB format), enable bilinear filtering on it
            SharedPtr<Texture2D> renderTexture(new Texture2D());
            renderTexture->SetSize(1024, 768, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
            renderTexture->SetFilterMode(FILTER_BILINEAR);

            // Create a new material from scratch, use the diffuse unlit technique, assign the render texture
            // as its diffuse texture, then assign the material to the screen plane object
            SharedPtr<Material> renderMaterial(new Material());
            renderMaterial->SetTechnique(0, cache.GetResource<Technique>("techniques/diff_unlit.xml"));
            renderMaterial->SetTexture(TU_DIFFUSE, renderTexture);
            // Since the screen material is on top of the box model and may Z-fight, use negative depth bias
            // to push it forward (particularly necessary on mobiles with possibly less Z resolution)
            renderMaterial->SetDepthBias(BiasParameters(-0.001f, 0.0f));
            screenObject->SetMaterial(renderMaterial);

            // Get the texture's RenderSurface object (exists when the texture has been created in rendertarget mode)
            // and define the viewport for rendering the second scene, similarly as how backbuffer viewports are defined
            // to the Renderer subsystem. By default the texture viewport will be updated when the texture is visible
            // in the main view
            RenderSurface* surface = renderTexture->GetRenderSurface();
            SharedPtr<Viewport> rttViewport(new Viewport(rttScene_, rttCameraNode_->GetComponent<Camera>()));
            surface->SetViewport(0, rttViewport);
        }

        // Create the camera which we will move around. Limit far clip distance to match the fog
        cameraNode_ = scene_->create_child("Camera");
        auto* camera = cameraNode_->create_component<Camera>();
        camera->SetFarClip(300.0f);

        // Set an initial position for the camera scene node above the plane
        cameraNode_->SetPosition(Vector3(0.0f, 7.0f, -30.0f));
    }
}

void RenderToTexture::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText("Use WASD keys and mouse to move");
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void RenderToTexture::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void RenderToTexture::move_camera(float timeStep)
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
}

void RenderToTexture::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(RenderToTexture, handle_update));
}

void RenderToTexture::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);
}
