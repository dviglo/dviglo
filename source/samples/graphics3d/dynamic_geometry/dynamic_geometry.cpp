// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/core/profiler.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/geometry.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/graphics_api/index_buffer.h>
#include <dviglo/graphics_api/vertex_buffer.h>
#include <dviglo/input/input.h>
#include <dviglo/io/log.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "dynamic_geometry.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(DynamicGeometry)

DynamicGeometry::DynamicGeometry() :
    animate_(true),
    time_(0.0f)
{
}

void DynamicGeometry::Start()
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

void DynamicGeometry::create_scene()
{
    scene_ = new Scene();

    // Create the Octree component to the scene so that drawable objects can be rendered. Use default volume
    // (-1000, -1000, -1000) to (1000, 1000, 1000)
    scene_->create_component<Octree>();

    // Create a Zone for ambient light & fog control
    Node* zoneNode = scene_->create_child("Zone");
    auto* zone = zoneNode->create_component<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetFogColor(Color(0.2f, 0.2f, 0.2f));
    zone->SetFogStart(200.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light
    Node* lightNode = scene_->create_child("DirectionalLight");
    lightNode->SetDirection(Vector3(-0.6f, -1.0f, -0.8f)); // The direction vector does not need to be normalized
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(0.4f, 1.0f, 0.4f));
    light->SetSpecularIntensity(1.5f);

    // Get the original model and its unmodified vertices, which are used as source data for the animation
    auto* originalModel = DV_RES_CACHE.GetResource<Model>("Models/Box.mdl");
    if (!originalModel)
    {
        DV_LOGERROR("Model not found, cannot initialize example scene");
        return;
    }
    // Get the vertex buffer from the first geometry's first LOD level
    VertexBuffer* buffer = originalModel->GetGeometry(0, 0)->GetVertexBuffer(0);
    const auto* vertexData = (const unsigned char*)buffer->Lock(0, buffer->GetVertexCount());
    if (vertexData)
    {
        unsigned numVertices = buffer->GetVertexCount();
        unsigned vertexSize = buffer->GetVertexSize();
        // Copy the original vertex positions
        for (unsigned i = 0; i < numVertices; ++i)
        {
            const Vector3& src = *reinterpret_cast<const Vector3*>(vertexData + i * vertexSize);
            originalVertices_.Push(src);
        }
        buffer->Unlock();

        // Detect duplicate vertices to allow seamless animation
        vertexDuplicates_.Resize(originalVertices_.Size());
        for (i32 i = 0; i < originalVertices_.Size(); ++i)
        {
            vertexDuplicates_[i] = i; // Assume not a duplicate
            for (i32 j = 0; j < i; ++j)
            {
                if (originalVertices_[i].Equals(originalVertices_[j]))
                {
                    vertexDuplicates_[i] = j;
                    break;
                }
            }
        }
    }
    else
    {
        DV_LOGERROR("Failed to lock the model vertex buffer to get original vertices");
        return;
    }

    // Create StaticModels in the scene. Clone the model for each so that we can modify the vertex data individually
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            Node* node = scene_->create_child("Object");
            node->SetPosition(Vector3(x * 2.0f, 0.0f, y * 2.0f));
            auto* object = node->create_component<StaticModel>();
            SharedPtr<Model> cloneModel = originalModel->Clone();
            object->SetModel(cloneModel);
            // Store the cloned vertex buffer that we will modify when animating
            animatingBuffers_.Push(SharedPtr<VertexBuffer>(cloneModel->GetGeometry(0, 0)->GetVertexBuffer(0)));
        }
    }

    // Finally create one model (pyramid shape) and a StaticModel to display it from scratch
    // Note: there are duplicated vertices to enable face normals. We will calculate normals programmatically
    {
        const unsigned numVertices = 18;

        float vertexData[] = {
            // Position             Normal
            0.0f, 0.5f, 0.0f,       0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f,      0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 0.0f,

            0.0f, 0.5f, 0.0f,       0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f,      0.0f, 0.0f, 0.0f,

            0.0f, 0.5f, 0.0f,       0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 0.0f,

            0.0f, 0.5f, 0.0f,       0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, 0.0f,

            0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f,      0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 0.0f,

            0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, 0.0f
        };

        const unsigned short indexData[] = {
            0, 1, 2,
            3, 4, 5,
            6, 7, 8,
            9, 10, 11,
            12, 13, 14,
            15, 16, 17
        };

        // Calculate face normals now
        for (unsigned i = 0; i < numVertices; i += 3)
        {
            Vector3& v1 = *(reinterpret_cast<Vector3*>(&vertexData[6 * i]));
            Vector3& v2 = *(reinterpret_cast<Vector3*>(&vertexData[6 * (i + 1)]));
            Vector3& v3 = *(reinterpret_cast<Vector3*>(&vertexData[6 * (i + 2)]));
            Vector3& n1 = *(reinterpret_cast<Vector3*>(&vertexData[6 * i + 3]));
            Vector3& n2 = *(reinterpret_cast<Vector3*>(&vertexData[6 * (i + 1) + 3]));
            Vector3& n3 = *(reinterpret_cast<Vector3*>(&vertexData[6 * (i + 2) + 3]));

            Vector3 edge1 = v1 - v2;
            Vector3 edge2 = v1 - v3;
            n1 = n2 = n3 = edge1.CrossProduct(edge2).normalized();
        }

        SharedPtr<Model> fromScratchModel(new Model());
        SharedPtr<VertexBuffer> vb(new VertexBuffer());
        SharedPtr<IndexBuffer> ib(new IndexBuffer());
        SharedPtr<Geometry> geom(new Geometry());

        // Shadowed buffer needed for raycasts to work, and so that data can be automatically restored on device loss
        vb->SetShadowed(true);
        // We could use the "legacy" element bitmask to define elements for more compact code, but let's demonstrate
        // defining the vertex elements explicitly to allow any element types and order
        Vector<VertexElement> elements;
        elements.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION));
        elements.Push(VertexElement(TYPE_VECTOR3, SEM_NORMAL));
        vb->SetSize(numVertices, elements);
        vb->SetData(vertexData);

        ib->SetShadowed(true);
        ib->SetSize(numVertices, false);
        ib->SetData(indexData);

        geom->SetVertexBuffer(0, vb);
        geom->SetIndexBuffer(ib);
        geom->SetDrawRange(TRIANGLE_LIST, 0, numVertices);

        fromScratchModel->SetNumGeometries(1);
        fromScratchModel->SetGeometry(0, 0, geom);
        fromScratchModel->SetBoundingBox(BoundingBox(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f)));

        // Though not necessary to render, the vertex & index buffers must be listed in the model so that it can be saved properly
        Vector<SharedPtr<VertexBuffer>> vertexBuffers;
        Vector<SharedPtr<IndexBuffer>> indexBuffers;
        vertexBuffers.Push(vb);
        indexBuffers.Push(ib);
        // Morph ranges could also be not defined. Here we simply define a zero range (no morphing) for the vertex buffer
        Vector<i32> morphRangeStarts;
        Vector<i32> morphRangeCounts;
        morphRangeStarts.Push(0);
        morphRangeCounts.Push(0);
        fromScratchModel->SetVertexBuffers(vertexBuffers, morphRangeStarts, morphRangeCounts);
        fromScratchModel->SetIndexBuffers(indexBuffers);

        Node* node = scene_->create_child("FromScratchObject");
        node->SetPosition(Vector3(0.0f, 3.0f, 0.0f));
        auto* object = node->create_component<StaticModel>();
        object->SetModel(fromScratchModel);
    }

    // Create the camera
    cameraNode_ = new Node();
    cameraNode_->SetPosition(Vector3(0.0f, 2.0f, -20.0f));
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(300.0f);
}

void DynamicGeometry::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText(
        "Use WASD keys and mouse to move\n"
        "Space to toggle animation"
    );
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void DynamicGeometry::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void DynamicGeometry::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(DynamicGeometry, handle_update));
}

void DynamicGeometry::move_camera(float timeStep)
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

void DynamicGeometry::AnimateObjects(float timeStep)
{
    DV_PROFILE(AnimateObjects);

    time_ += timeStep * 100.0f;

    // Repeat for each of the cloned vertex buffers
    for (i32 i = 0; i < animatingBuffers_.Size(); ++i)
    {
        float startPhase = time_ + i * 30.0f;
        VertexBuffer* buffer = animatingBuffers_[i];

        // Lock the vertex buffer for update and rewrite positions with sine wave modulated ones
        // Cannot use discard lock as there is other data (normals, UVs) that we are not overwriting
        auto* vertexData = (unsigned char*)buffer->Lock(0, buffer->GetVertexCount());
        if (vertexData)
        {
            unsigned vertexSize = buffer->GetVertexSize();
            unsigned numVertices = buffer->GetVertexCount();
            for (unsigned j = 0; j < numVertices; ++j)
            {
                // If there are duplicate vertices, animate them in phase of the original
                float phase = startPhase + vertexDuplicates_[j] * 10.0f;
                Vector3& src = originalVertices_[j];
                Vector3& dest = *reinterpret_cast<Vector3*>(vertexData + j * vertexSize);
                dest.x = src.x * (1.0f + 0.1f * Sin(phase));
                dest.y = src.y * (1.0f + 0.1f * Sin(phase + 60.0f));
                dest.z = src.z * (1.0f + 0.1f * Sin(phase + 120.0f));
            }

            buffer->Unlock();
        }
    }
}

void DynamicGeometry::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Toggle animation with space
    if (DV_INPUT.GetKeyPress(KEY_SPACE))
        animate_ = !animate_;

    // Move the camera, scale movement with time step
    move_camera(timeStep);

    // Animate objects' vertex data if enabled
    if (animate_)
        AnimateObjects(timeStep);
}
