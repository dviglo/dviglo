// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/time_base.h>
#include <dviglo/engine/application.h>
#include <dviglo/engine/engine.h>
#include <dviglo/engine/engine_defs.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics_api/texture_2d.h>
#include <dviglo/input/input.h>
#include <dviglo/input/input_events.h>
#include <dviglo/io/file_system.h>
#include <dviglo/io/fs_base.h>
#include <dviglo/io/log.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/resource/xml_file.h>
#include <dviglo/scene/scene.h>
#include <dviglo/scene/scene_events.h>
#include <dviglo/ui/console.h>
#include <dviglo/ui/cursor.h>
#include <dviglo/ui/debug_hud.h>
#include <dviglo/ui/sprite.h>
#include <dviglo/ui/ui.h>

Sample::Sample() :
    yaw_(0.0f),
    pitch_(0.0f),
    useMouseMode_(MM_ABSOLUTE),
    paused_(false)
{
}

void Sample::Setup()
{
    // Modify engine startup parameters
    engineParameters_[EP_WINDOW_TITLE] = GetTypeName();
    engineParameters_[EP_LOG_NAME]     = get_pref_path("dviglo", "logs") + GetTypeName() + ".log";
    engineParameters_[EP_FULL_SCREEN]  = false;
    engineParameters_[EP_HEADLESS]     = false;
    engineParameters_[EP_SOUND]        = false;

    // Construct a search path to find the resource prefix with two entries:
    // The first entry is an empty path which will be substituted with program/bin directory -- this entry is for binary when it is still in build tree
    // The second and third entries are possible relative paths from the installed program/bin directory to the asset directory -- these entries are for binary when it is in the Urho3D SDK installation location
    if (!engineParameters_.Contains(EP_RESOURCE_PREFIX_PATHS))
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";../share/Resources;../share/Urho3D/Resources";
}

void Sample::Start()
{
    // Create logo
    CreateLogo();

    // Set custom window Title & Icon
    SetWindowTitleAndIcon();

    // Create console and debug HUD
    InitConsoleAndDebugHud();

    // Subscribe key down event
    subscribe_to_event(E_KEYDOWN, DV_HANDLER(Sample, HandleKeyDown));
    // Subscribe key up event
    subscribe_to_event(E_KEYUP, DV_HANDLER(Sample, HandleKeyUp));
    // Subscribe scene update event
    subscribe_to_event(E_SCENEUPDATE, DV_HANDLER(Sample, HandleSceneUpdate));
}

void Sample::Stop()
{
    DV_ENGINE.DumpResources(true);
}

void Sample::InitMouseMode(MouseMode mode)
{
    useMouseMode_ = mode;

    Input* input = DV_INPUT;

    if (GetPlatform() != "Web")
    {
        if (useMouseMode_ == MM_FREE)
            input->SetMouseVisible(true);

        if (useMouseMode_ != MM_ABSOLUTE)
        {
            input->SetMouseMode(useMouseMode_);
            if (DV_CONSOLE->IsVisible())
                input->SetMouseMode(MM_ABSOLUTE, true);
        }
    }
    else
    {
        input->SetMouseVisible(true);
        subscribe_to_event(E_MOUSEBUTTONDOWN, DV_HANDLER(Sample, HandleMouseModeRequest));
        subscribe_to_event(E_MOUSEMODECHANGED, DV_HANDLER(Sample, HandleMouseModeChange));
    }
}

void Sample::SetLogoVisible(bool enable)
{
    if (logoSprite_)
        logoSprite_->SetVisible(enable);
}

void Sample::CreateLogo()
{
    // Get logo texture
    Texture2D* logoTexture = DV_RES_CACHE.GetResource<Texture2D>("textures/fish_bone_logo.png");
    if (!logoTexture)
        return;

    // Create logo sprite and add to the UI layout
    logoSprite_ = DV_UI->GetRoot()->create_child<Sprite>();

    // Set logo sprite texture
    logoSprite_->SetTexture(logoTexture);

    int textureWidth = logoTexture->GetWidth();
    int textureHeight = logoTexture->GetHeight();

    // Set logo sprite scale
    logoSprite_->SetScale(256.0f / textureWidth);

    // Set logo sprite size
    logoSprite_->SetSize(textureWidth, textureHeight);

    // Set logo sprite hot spot
    logoSprite_->SetHotSpot(textureWidth, textureHeight);

    // Set logo sprite alignment
    logoSprite_->SetAlignment(HA_RIGHT, VA_BOTTOM);

    // Make logo not fully opaque to show the scene underneath
    logoSprite_->SetOpacity(0.9f);

    // Set a low priority for the logo so that other UI elements can be drawn on top
    logoSprite_->SetPriority(-100);
}

void Sample::SetWindowTitleAndIcon()
{
    Graphics* graphics = DV_GRAPHICS;
    Image* icon = DV_RES_CACHE.GetResource<Image>("textures/urho_icon.png");
    graphics->SetWindowIcon(icon);
    graphics->SetWindowTitle("Urho3D Sample");
}

void Sample::InitConsoleAndDebugHud()
{
    // Get default style
    XmlFile* xmlFile = DV_RES_CACHE.GetResource<XmlFile>("ui/default_style.xml");

    // Create console
    DV_CONSOLE->SetDefaultStyle(xmlFile);
    DV_CONSOLE->GetBackground()->SetOpacity(0.8f);

    // Init debug HUD
    DV_DEBUG_HUD->SetDefaultStyle(xmlFile);
}


void Sample::HandleKeyUp(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyUp;

    int key = eventData[P_KEY].GetI32();

    // Close console (if open) or exit when ESC is pressed
    if (key == KEY_ESCAPE)
    {
        if (DV_CONSOLE->IsVisible())
            DV_CONSOLE->SetVisible(false);
        else
            DV_ENGINE.Exit();
    }
}

void Sample::HandleKeyDown(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyDown;

    int key = eventData[P_KEY].GetI32();

    // Toggle console with F1
    if (key == KEY_F1)
        DV_CONSOLE->Toggle();

    // Toggle debug HUD with F2
    else if (key == KEY_F2)
        DV_DEBUG_HUD->ToggleAll();

    // Common rendering quality controls, only when UI has no focused element
    else if (!DV_UI->GetFocusElement())
    {
        Renderer* renderer = DV_RENDERER;

        // Texture quality
        if (key == '1')
        {
            auto quality = (unsigned)renderer->GetTextureQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetTextureQuality((MaterialQuality)quality);
        }

        // Material quality
        else if (key == '2')
        {
            auto quality = (unsigned)renderer->GetMaterialQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetMaterialQuality((MaterialQuality)quality);
        }

        // Specular lighting
        else if (key == '3')
            renderer->SetSpecularLighting(!renderer->GetSpecularLighting());

        // Shadow rendering
        else if (key == '4')
            renderer->SetDrawShadows(!renderer->GetDrawShadows());

        // Shadow map resolution
        else if (key == '5')
        {
            int shadowMapSize = renderer->GetShadowMapSize();
            shadowMapSize *= 2;
            if (shadowMapSize > 2048)
                shadowMapSize = 512;
            renderer->SetShadowMapSize(shadowMapSize);
        }

        // Shadow depth and filtering quality
        else if (key == '6')
        {
            ShadowQuality quality = renderer->GetShadowQuality();
            quality = (ShadowQuality)(quality + 1);
            if (quality > SHADOWQUALITY_BLUR_VSM)
                quality = SHADOWQUALITY_SIMPLE_16BIT;
            renderer->SetShadowQuality(quality);
        }

        // Occlusion culling
        else if (key == '7')
        {
            bool occlusion = renderer->GetMaxOccluderTriangles() > 0;
            occlusion = !occlusion;
            renderer->SetMaxOccluderTriangles(occlusion ? 5000 : 0);
        }

        // Instancing
        else if (key == '8')
            renderer->SetDynamicInstancing(!renderer->GetDynamicInstancing());

        // Take screenshot
        else if (key == '9')
        {
            Image screenshot;
            DV_GRAPHICS->TakeScreenShot(screenshot);
            // Here we save in the Data folder with date and time appended
            screenshot.SavePNG(DV_FILE_SYSTEM.GetProgramDir() + "Data/Screenshot_" +
                time_to_str().Replaced(':', '_').Replaced('-', '_').Replaced(' ', '_') + ".png");
        }
    }
}

void Sample::HandleSceneUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
}

// If the user clicks the canvas, attempt to switch to relative mouse mode on web platform
void Sample::HandleMouseModeRequest(StringHash /*eventType*/, VariantMap& eventData)
{
    if (DV_CONSOLE->IsVisible())
        return;

    if (useMouseMode_ == MM_ABSOLUTE)
        DV_INPUT->SetMouseVisible(false);
    else if (useMouseMode_ == MM_FREE)
        DV_INPUT->SetMouseVisible(true);

    DV_INPUT->SetMouseMode(useMouseMode_);
}

void Sample::HandleMouseModeChange(StringHash /*eventType*/, VariantMap& eventData)
{
    bool mouseLocked = eventData[MouseModeChanged::P_MOUSELOCKED].GetBool();
    DV_INPUT->SetMouseVisible(!mouseLocked);
}
