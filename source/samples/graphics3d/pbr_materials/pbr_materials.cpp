// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/render_path.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/slider.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>
#include <dviglo/ui/text.h>
#ifdef DV_ANGELSCRIPT
#include <dviglo/AngelScript/Script.h>
#endif

#include "pbr_materials.h"

#include <dviglo/common/debug_new.h>

using namespace std;

DV_DEFINE_APPLICATION_MAIN(PBRMaterials)

PBRMaterials::PBRMaterials() :
    dynamicMaterial_(nullptr),
    roughnessLabel_(nullptr),
    metallicLabel_(nullptr),
    ambientLabel_(nullptr)
{
}

void PBRMaterials::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Create the UI content
    create_ui();
    create_instructions();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Subscribe to global events for camera movement
    subscribe_to_events();
}

void PBRMaterials::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI->GetRoot()->create_child<Text>();
    instructionText->SetText("Use sliders to change Roughness and Metallic\n"
        "Hold RMB and use WASD keys and mouse to move");
    instructionText->SetFont(DV_RES_CACHE->GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI->GetRoot()->GetHeight() / 4);
}

void PBRMaterials::create_scene()
{
    scene_ = new Scene();

    // Load scene content prepared in the editor (XML format). GetFile() returns an open file from the resource system
    // which scene.load_xml() will read
    shared_ptr<File> file = DV_RES_CACHE->GetFile("scenes/pbr_example.xml");
    scene_->load_xml(*file);

    Node* sphereWithDynamicMatNode = scene_->GetChild("SphereWithDynamicMat");
    auto* staticModel = sphereWithDynamicMatNode->GetComponent<StaticModel>();
    dynamicMaterial_ = staticModel->GetMaterial(0);

    Node* zoneNode = scene_->GetChild("Zone");
    zone_ = zoneNode->GetComponent<Zone>();

    // Create the camera (not included in the scene file)
    cameraNode_ = scene_->create_child("Camera");
    cameraNode_->create_component<Camera>();

    cameraNode_->SetPosition(sphereWithDynamicMatNode->GetPosition() + Vector3(2.0f, 2.0f, 2.0f));
    cameraNode_->LookAt(sphereWithDynamicMatNode->GetPosition());
    yaw_ = cameraNode_->GetRotation().YawAngle();
    pitch_ = cameraNode_->GetRotation().PitchAngle();
}

void PBRMaterials::create_ui()
{
    UI* ui = DV_UI;

    // Set up global UI style into the root UI element
    auto* style = DV_RES_CACHE->GetResource<XmlFile>("ui/default_style.xml");
    ui->GetRoot()->SetDefaultStyle(style);

    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will interact with the UI
    SharedPtr<Cursor> cursor(new Cursor());
    cursor->SetStyleAuto();
    ui->SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    cursor->SetPosition(DV_GRAPHICS->GetWidth() / 2, DV_GRAPHICS->GetHeight() / 2);

    roughnessLabel_ = ui->GetRoot()->create_child<Text>();
    roughnessLabel_->SetFont(DV_RES_CACHE->GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    roughnessLabel_->SetPosition(370, 50);
    roughnessLabel_->SetTextEffect(TE_SHADOW);

    metallicLabel_ = ui->GetRoot()->create_child<Text>();
    metallicLabel_->SetFont(DV_RES_CACHE->GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    metallicLabel_->SetPosition(370, 100);
    metallicLabel_->SetTextEffect(TE_SHADOW);

    ambientLabel_ = ui->GetRoot()->create_child<Text>();
    ambientLabel_->SetFont(DV_RES_CACHE->GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    ambientLabel_->SetPosition(370, 150);
    ambientLabel_->SetTextEffect(TE_SHADOW);

    auto* roughnessSlider = ui->GetRoot()->create_child<Slider>();
    roughnessSlider->SetStyleAuto();
    roughnessSlider->SetPosition(50, 50);
    roughnessSlider->SetSize(300, 20);
    roughnessSlider->SetRange(1.0f); // 0 - 1 range
    subscribe_to_event(roughnessSlider, E_SLIDERCHANGED, DV_HANDLER(PBRMaterials, HandleRoughnessSliderChanged));
    roughnessSlider->SetValue(0.5f);

    auto* metallicSlider = ui->GetRoot()->create_child<Slider>();
    metallicSlider->SetStyleAuto();
    metallicSlider->SetPosition(50, 100);
    metallicSlider->SetSize(300, 20);
    metallicSlider->SetRange(1.0f); // 0 - 1 range
    subscribe_to_event(metallicSlider, E_SLIDERCHANGED, DV_HANDLER(PBRMaterials, HandleMetallicSliderChanged));
    metallicSlider->SetValue(0.5f);

    auto* ambientSlider = ui->GetRoot()->create_child<Slider>();
    ambientSlider->SetStyleAuto();
    ambientSlider->SetPosition(50, 150);
    ambientSlider->SetSize(300, 20);
    ambientSlider->SetRange(10.0f); // 0 - 10 range
    subscribe_to_event(ambientSlider, E_SLIDERCHANGED, DV_HANDLER(PBRMaterials, HandleAmbientSliderChanged));
    ambientSlider->SetValue(zone_->GetAmbientColor().a_);
}

void PBRMaterials::HandleRoughnessSliderChanged(StringHash eventType, VariantMap& eventData)
{
    float newValue = eventData[SliderChanged::P_VALUE].GetFloat();
    dynamicMaterial_->SetShaderParameter("Roughness", newValue);
    roughnessLabel_->SetText("Roughness: " + String(newValue));
}

void PBRMaterials::HandleMetallicSliderChanged(StringHash eventType, VariantMap& eventData)
{
    float newValue = eventData[SliderChanged::P_VALUE].GetFloat();
    dynamicMaterial_->SetShaderParameter("Metallic", newValue);
    metallicLabel_->SetText("Metallic: " + String(newValue));
}

void PBRMaterials::HandleAmbientSliderChanged(StringHash eventType, VariantMap& eventData)
{
    float newValue = eventData[SliderChanged::P_VALUE].GetFloat();
    Color col = Color(0.0, 0.0, 0.0, newValue);
    zone_->SetAmbientColor(col);
    ambientLabel_->SetText("Ambient HDR Scale: " + String(zone_->GetAmbientColor().a_));
}

void PBRMaterials::setup_viewport()
{
    DV_RENDERER->SetHDRRendering(true);

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER->SetViewport(0, viewport);

    // Add postprocessing effects appropriate with the example scene
    SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
    effectRenderPath->Append(DV_RES_CACHE->GetResource<XmlFile>("postprocess/fxaa2.xml"));
    effectRenderPath->Append(DV_RES_CACHE->GetResource<XmlFile>("postprocess/gamma_correction.xml"));
    effectRenderPath->Append(DV_RES_CACHE->GetResource<XmlFile>("postprocess/tonemap.xml"));
    effectRenderPath->Append(DV_RES_CACHE->GetResource<XmlFile>("postprocess/autoexposure.xml"));

    viewport->SetRenderPath(effectRenderPath);
}

void PBRMaterials::subscribe_to_events()
{
    // Subscribe handle_update() function for camera motion
    subscribe_to_event(E_UPDATE, DV_HANDLER(PBRMaterials, handle_update));
}

void PBRMaterials::move_camera(float timeStep)
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    Input* input = DV_INPUT;
    DV_UI->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));

    // Do not move if the UI has a focused element
    if (DV_UI->GetFocusElement())
        return;

    // Movement speed as world units per second
    const float MOVE_SPEED = 10.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    // Only move the camera when the cursor is hidden
    if (!DV_UI->GetCursor()->IsVisible())
    {
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void PBRMaterials::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);
}
