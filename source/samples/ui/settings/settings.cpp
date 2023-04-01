// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/graphics_events.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/graphics_api/texture_2d.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/check_box.h>
#include <dviglo/ui/dropdown_list.h>
#include <dviglo/ui/line_edit.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/tooltip.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>
#include <dviglo/ui/window.h>

#include "settings.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(WindowSettingsDemo)

WindowSettingsDemo::WindowSettingsDemo()
{
}

void WindowSettingsDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    DV_INPUT.SetMouseVisible(true);

    // Load XML file containing default UI style sheet
    auto* style = DV_RES_CACHE.GetResource<XmlFile>("ui/DefaultStyle.xml");

    uiRoot_ = DV_UI.GetRoot();

    // Set the loaded style as default style
    uiRoot_->SetDefaultStyle(style);

    // Create window with settings.
    InitSettings();
    SynchronizeSettings();
    subscribe_to_event(E_SCREENMODE,
        [this](StringHash /*eventType*/, const VariantMap& /*eventData*/)
    {
        SynchronizeSettings();
    });

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);

    // Create scene
    create_scene();

    // Setup viewport
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void WindowSettingsDemo::create_scene()
{
    scene_ = new Scene();
    scene_->create_component<Octree>();

    auto* zone = scene_->create_component<Zone>();
    zone->SetAmbientColor(Color::WHITE);

    // Create 3D object
    Node* objectNode = scene_->create_child("Object");
    objectNode->SetRotation(Quaternion(45.0f, 45.0f, 45.0f));
    auto* objectModel = objectNode->create_component<StaticModel>();
    objectModel->SetModel(DV_RES_CACHE.GetResource<Model>("models/Box.mdl"));
    objectModel->SetMaterial(DV_RES_CACHE.GetResource<Material>("materials/Stone.xml"));

    // Create camera
    cameraNode_ = scene_->create_child("Camera");
    cameraNode_->create_component<Camera>();
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -4.0f));

    // Rotate object
    subscribe_to_event(scene_, E_SCENEUPDATE,
        [objectNode](StringHash /*eventType*/, VariantMap& eventData)
    {
        const float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();
        objectNode->Rotate(Quaternion(0.0f, 20.0f * timeStep, 0.0f), TransformSpace::World);
    });
}

void WindowSettingsDemo::InitSettings()
{
    // Create the Window and add it to the UI's root node
    window_ = uiRoot_->create_child<Window>("Window");

    // Set Window size and layout settings
    window_->SetPosition(128, 128);
    window_->SetMinWidth(300);
    window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window_->SetMovable(true);
    window_->SetStyleAuto();

    // Create the Window title Text
    auto* windowTitle = window_->create_child<Text>("WindowTitle");
    windowTitle->SetText("Window Settings");
    windowTitle->SetStyleAuto();

    // Create monitor selector
    monitorControl_ = window_->create_child<DropDownList>("Monitor");
    monitorControl_->SetMinHeight(24);
    monitorControl_->SetStyleAuto();
    Vector<SDL_DisplayID> displays = DV_GRAPHICS.get_displays();

    for (SDL_DisplayID display : displays)
    {
        SharedPtr<Text> text = MakeShared<Text>();
        text->SetText(ToString("Display %u", display));
        text->SetMinWidth(CeilToInt(text->GetRowWidth(0) + 10));
        monitorControl_->AddItem(text);
        text->SetStyleAuto();
        text->SetVar("display", display);
    }

    // Create resolution selector
    resolutionControl_ = window_->create_child<DropDownList>("Resolution");
    resolutionControl_->SetMinHeight(24);
    resolutionControl_->SetStyleAuto();

    auto resolutionPlaceholder = MakeShared<Text>();
    resolutionPlaceholder->SetText("[Cannot fill list of resolutions]");
    resolutionPlaceholder->SetMinWidth(CeilToInt(resolutionPlaceholder->GetRowWidth(0) + 10));
    resolutionControl_->AddItem(resolutionPlaceholder);
    resolutionPlaceholder->SetStyleAuto();

    // Create fullscreen controller
    auto* fullscreenFrame = window_->create_child<UiElement>("Fullscreen Frame");
    fullscreenFrame->SetMinHeight(24);
    fullscreenFrame->SetLayout(LM_HORIZONTAL, 6);

    fullscreenControl_ = fullscreenFrame->create_child<CheckBox>("Fullscreen Control");
    fullscreenControl_->SetStyleAuto();

    auto* fullscreenText = fullscreenFrame->create_child<Text>("Fullscreen Label");
    fullscreenText->SetText("Fullscreen");
    fullscreenText->SetMinWidth(CeilToInt(fullscreenText->GetRowWidth(0) + 10));
    fullscreenText->SetStyleAuto();

    // Create borderless controller
    auto* borderlessFrame = window_->create_child<UiElement>("Borderless Frame");
    borderlessFrame->SetMinHeight(24);
    borderlessFrame->SetLayout(LM_HORIZONTAL, 6);

    borderlessControl_ = borderlessFrame->create_child<CheckBox>("Borderless Control");
    borderlessControl_->SetStyleAuto();

    auto* borderlessText = borderlessFrame->create_child<Text>("Borderless Label");
    borderlessText->SetText("Borderless");
    borderlessText->SetMinWidth(CeilToInt(borderlessText->GetRowWidth(0) + 10));
    borderlessText->SetStyleAuto();

    // Create resizable controller
    auto* resizableFrame = window_->create_child<UiElement>("Resizable Frame");
    resizableFrame->SetMinHeight(24);
    resizableFrame->SetLayout(LM_HORIZONTAL, 6);

    resizableControl_ = resizableFrame->create_child<CheckBox>("Resizable Control");
    resizableControl_->SetStyleAuto();

    auto* resizableText = resizableFrame->create_child<Text>("Resizable Label");
    resizableText->SetText("Resizable");
    resizableText->SetMinWidth(CeilToInt(resizableText->GetRowWidth(0) + 10));
    resizableText->SetStyleAuto();

    // Create resizable controller
    auto* vsyncFrame = window_->create_child<UiElement>("V-Sync Frame");
    vsyncFrame->SetMinHeight(24);
    vsyncFrame->SetLayout(LM_HORIZONTAL, 6);

    vsyncControl_ = vsyncFrame->create_child<CheckBox>("V-Sync Control");
    vsyncControl_->SetStyleAuto();

    auto* vsyncText = vsyncFrame->create_child<Text>("V-Sync Label");
    vsyncText->SetText("V-Sync");
    vsyncText->SetMinWidth(CeilToInt(vsyncText->GetRowWidth(0) + 10));
    vsyncText->SetStyleAuto();

    // Create multi-sample controller from 1 (= 2^0) to 16 (= 2^4)
    multiSampleControl_ = window_->create_child<DropDownList>("Multi-Sample Control");
    multiSampleControl_->SetMinHeight(24);
    multiSampleControl_->SetStyleAuto();
    for (int i = 0; i <= 4; ++i)
    {
        auto text = MakeShared<Text>();
        text->SetText(i == 0 ? "No MSAA" : ToString("MSAA x%d", 1 << i));
        text->SetMinWidth(CeilToInt(text->GetRowWidth(0) + 10));
        multiSampleControl_->AddItem(text);
        text->SetStyleAuto();
    }

    // Create "Apply" button
    auto* applyButton = window_->create_child<Button>("Apply");
    applyButton->SetLayout(LM_HORIZONTAL, 6, IntRect(6, 6, 6, 6));
    applyButton->SetStyleAuto();

    auto* applyButtonText = applyButton->create_child<Text>("Apply Text");
    applyButtonText->SetAlignment(HA_CENTER, VA_CENTER);
    applyButtonText->SetText("Apply");
    applyButtonText->SetStyleAuto();

    applyButton->SetFixedWidth(CeilToInt(applyButtonText->GetRowWidth(0) + 20));
    applyButton->SetFixedHeight(30);

    // Apply settings when "Apply" button is clicked
    subscribe_to_event(applyButton, E_RELEASED,
        [this](StringHash /*eventType*/, const VariantMap& /*eventData*/)
    {
        const i32 index = monitorControl_->GetSelection();
        if (index == NINDEX)
            return;

        SDL_DisplayID display = monitorControl_->GetSelectedItem()->GetVar("display").GetU32();
        const auto& resolutions = DV_GRAPHICS.GetResolutions(display);
        const i32 selectedResolution = resolutionControl_->GetSelection();
        if (selectedResolution >= resolutions.Size())
            return;

        const bool fullscreen = fullscreenControl_->IsChecked();
        const bool borderless = borderlessControl_->IsChecked();
        const bool resizable = resizableControl_->IsChecked();
        const bool vsync = vsyncControl_->IsChecked();

        const unsigned multiSampleSelection = multiSampleControl_->GetSelection();
        const int multiSample = multiSampleSelection == M_MAX_UNSIGNED ? 1 : static_cast<int>(1 << multiSampleSelection);

        // TODO: Expose these options too?
        const bool highDPI = DV_GRAPHICS.GetHighDPI();
        const bool tripleBuffer = DV_GRAPHICS.GetTripleBuffer();

        const int width = resolutions[selectedResolution].x;
        const int height = resolutions[selectedResolution].y;
        const int refreshRate = resolutions[selectedResolution].z;
        DV_GRAPHICS.SetMode(width, height, fullscreen, borderless, resizable, highDPI, vsync, tripleBuffer, multiSample, display, refreshRate);
    });
}

void WindowSettingsDemo::SynchronizeSettings()
{
    Graphics& graphics = DV_GRAPHICS;

    // Synchronize monitor
    const SDL_DisplayID current_display = graphics.GetDisplay();

    for (int i = 0; i < monitorControl_->GetNumItems(); ++i)
    {
        UiElement* item = monitorControl_->GetItem(i);

        if (item->GetVar("display").GetU32() == current_display)
        {
            monitorControl_->SetSelection(i);
            break;
        }
    }

    // Synchronize resolution list
    resolutionControl_->RemoveAllItems();
    const auto& resolutions = graphics.GetResolutions(current_display);
    for (const IntVector3& resolution : resolutions)
    {
        auto resolutionEntry = MakeShared<Text>();
        resolutionEntry->SetText(ToString("%dx%d, %d Hz", resolution.x, resolution.y, resolution.z));
        resolutionEntry->SetMinWidth(CeilToInt(resolutionEntry->GetRowWidth(0) + 10));
        resolutionControl_->AddItem(resolutionEntry);
        resolutionEntry->SetStyleAuto();
    }

    // Synchronize selected resolution
    const i32 currentResolution = graphics.FindBestResolutionIndex(current_display,
        graphics.GetWidth(), graphics.GetHeight(), graphics.GetRefreshRate());
    resolutionControl_->SetSelection(currentResolution);

    // Synchronize fullscreen and borderless flags
    fullscreenControl_->SetChecked(graphics.GetFullscreen());
    borderlessControl_->SetChecked(graphics.GetBorderless());
    resizableControl_->SetChecked(graphics.GetResizable());
    vsyncControl_->SetChecked(graphics.GetVSync());

    // Synchronize MSAA
    for (unsigned i = 0; i <= 4; ++i)
    {
        if (graphics.GetMultiSample() == static_cast<int>(1 << i))
            multiSampleControl_->SetSelection(i);
    }
}
