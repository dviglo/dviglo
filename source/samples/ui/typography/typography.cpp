// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/core/process_utils.h>
#include <dviglo/graphics/render_path.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/ui/check_box.h>
#include <dviglo/ui/dropdown_list.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>

#include "typography.h"

#include <dviglo/common/debug_new.h>

// Expands to this example's entry-point
DV_DEFINE_APPLICATION_MAIN(Typography)

namespace
{
    // Tag used to find all Text elements
    const char* TEXT_TAG = "Typography_text_tag";
}

Typography::Typography()
{
}

void Typography::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    DV_INPUT.SetMouseVisible(true);

    // Load XML file containing default UI style sheet
    auto* style = DV_RES_CACHE.GetResource<XMLFile>("UI/DefaultStyle.xml");

    // Set the loaded style as default style
    UIElement* root = DV_UI.GetRoot();
    root->SetDefaultStyle(style);

    // Create a UIElement to hold all our content
    // (Don't modify the root directly, as the base Sample class uses it)
    uielement_ = new UIElement();
    uielement_->SetAlignment(HA_CENTER, VA_CENTER);
    uielement_->SetLayout(LM_VERTICAL, 10, IntRect(20, 40, 20, 40));
    root->AddChild(uielement_);

    // Add some sample text.
    CreateText();

    // Add a checkbox to toggle the background color.
    CreateCheckbox("White background", DV_HANDLER(Typography, HandleWhiteBackground))
        ->SetChecked(false);

    // Add a checkbox to toggle SRGB output conversion (if available).
    // This will give more correct text output for FreeType fonts, as the FreeType rasterizer
    // outputs linear coverage values rather than SRGB values. However, this feature isn't
    // available on all platforms.
    CreateCheckbox("Graphics::SetSRGB", DV_HANDLER(Typography, HandleSRGB))
        ->SetChecked(DV_GRAPHICS.GetSRGB());

    // Add a checkbox for the global ForceAutoHint setting. This affects character spacing.
    CreateCheckbox("UI::SetForceAutoHint", DV_HANDLER(Typography, HandleForceAutoHint))
        ->SetChecked(DV_UI.GetForceAutoHint());

    // Add a drop-down menu to control the font hinting level.
    const char* levels[] = {
        "FONT_HINT_LEVEL_NONE",
        "FONT_HINT_LEVEL_LIGHT",
        "FONT_HINT_LEVEL_NORMAL",
        nullptr
    };
    CreateMenu("UI::SetFontHintLevel", levels, DV_HANDLER(Typography, HandleFontHintLevel))
        ->SetSelection(DV_UI.GetFontHintLevel());

    // Add a drop-down menu to control the subpixel threshold.
    const char* thresholds[] = {
        "0",
        "3",
        "6",
        "9",
        "12",
        "15",
        "18",
        "21",
        nullptr
    };
    CreateMenu("UI::SetFontSubpixelThreshold", thresholds, DV_HANDLER(Typography, HandleFontSubpixel))
        ->SetSelection(DV_UI.GetFontSubpixelThreshold() / 3);

    // Add a drop-down menu to control oversampling.
    const char* limits[] = {
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        nullptr
    };
    CreateMenu("UI::SetFontOversampling", limits, DV_HANDLER(Typography, HandleFontOversampling))
        ->SetSelection(DV_UI.GetFontOversampling() - 1);

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Typography::CreateText()
{
    SharedPtr<UIElement> container(new UIElement());
    container->SetAlignment(HA_LEFT, VA_TOP);
    container->SetLayout(LM_VERTICAL);
    uielement_->AddChild(container);

    auto* font = DV_RES_CACHE.GetResource<Font>("Fonts/BlueHighway.ttf");

    for (auto size2x = 2; size2x <= 36; ++size2x)
    {
        auto size = size2x / 2.f;
        SharedPtr<Text> text(new Text());
        text->SetText(String("The quick brown fox jumps over the lazy dog (") + String(size) + String("pt)"));
        text->SetFont(font, size);
        text->AddTag(TEXT_TAG);
        container->AddChild(text);
    }
}

SharedPtr<CheckBox> Typography::CreateCheckbox(const String& label, EventHandler* handler)
{
    SharedPtr<UIElement> container(new UIElement());
    container->SetAlignment(HA_LEFT, VA_TOP);
    container->SetLayout(LM_HORIZONTAL, 8);
    uielement_->AddChild(container);

    SharedPtr<CheckBox> box(new CheckBox());
    container->AddChild(box);
    box->SetStyleAuto();

    SharedPtr<Text> text(new Text());
    container->AddChild(text);
    text->SetText(label);
    text->SetStyleAuto();
    text->AddTag(TEXT_TAG);

    SubscribeToEvent(box, E_TOGGLED, handler);
    return box;
}

SharedPtr<DropDownList> Typography::CreateMenu(const String& label, const char** items, EventHandler* handler)
{
    SharedPtr<UIElement> container(new UIElement());
    container->SetAlignment(HA_LEFT, VA_TOP);
    container->SetLayout(LM_HORIZONTAL, 8);
    uielement_->AddChild(container);

    SharedPtr<Text> text(new Text());
    container->AddChild(text);
    text->SetText(label);
    text->SetStyleAuto();
    text->AddTag(TEXT_TAG);

    SharedPtr<DropDownList> list(new DropDownList());
    container->AddChild(list);
    list->SetStyleAuto();

    for (int i = 0; items[i]; ++i)
    {
        SharedPtr<Text> item(new Text());
        list->AddItem(item);
        item->SetText(items[i]);
        item->SetStyleAuto();
        item->SetMinWidth(item->GetRowWidth(0) + 10);
        item->AddTag(TEXT_TAG);
    }

    text->SetMaxWidth(text->GetRowWidth(0));

    SubscribeToEvent(list, E_ITEMSELECTED, handler);
    return list;
}

void Typography::HandleWhiteBackground(StringHash eventType, VariantMap& eventData)
{
    auto* box = static_cast<CheckBox*>(eventData[Toggled::P_ELEMENT].GetPtr());
    bool checked = box->IsChecked();

    Color fg = checked ? Color::BLACK : Color::WHITE;
    Color bg = checked ? Color::WHITE : Color::BLACK;

    Zone* zone = DV_RENDERER.GetDefaultZone();
    zone->SetFogColor(bg);

    Vector<UIElement*> text = uielement_->GetChildrenWithTag(TEXT_TAG, true);
    for (int i = 0; i < text.Size(); i++)
        text[i]->SetColor(fg);
}

void Typography::HandleForceAutoHint(StringHash eventType, VariantMap& eventData)
{
    auto* box = static_cast<CheckBox*>(eventData[Toggled::P_ELEMENT].GetPtr());
    bool checked = box->IsChecked();
    DV_UI.SetForceAutoHint(checked);
}

void Typography::HandleSRGB(StringHash eventType, VariantMap& eventData)
{
    auto* box = static_cast<CheckBox*>(eventData[Toggled::P_ELEMENT].GetPtr());
    bool checked = box->IsChecked();

    if (DV_GRAPHICS.GetSRGBWriteSupport())
    {
        DV_GRAPHICS.SetSRGB(checked);
    }
    else
    {
        DV_LOGWARNING("Graphics::GetSRGBWriteSupport returned false");

        // Note: PostProcess/GammaCorrection.xml implements SRGB conversion.
        // However, post-processing filters don't affect the UI layer.
    }
}

void Typography::HandleFontHintLevel(StringHash eventType, VariantMap& eventData)
{
    auto* list = static_cast<DropDownList*>(eventData[Toggled::P_ELEMENT].GetPtr());
    unsigned i = list->GetSelection();
    DV_UI.SetFontHintLevel((FontHintLevel)i);
}

void Typography::HandleFontSubpixel(StringHash eventType, VariantMap& eventData)
{
    auto* list = static_cast<DropDownList*>(eventData[Toggled::P_ELEMENT].GetPtr());
    unsigned i = list->GetSelection();
    DV_UI.SetFontSubpixelThreshold(i * 3);
}

void Typography::HandleFontOversampling(StringHash eventType, VariantMap& eventData)
{
    auto* list = static_cast<DropDownList*>(eventData[Toggled::P_ELEMENT].GetPtr());
    unsigned i = list->GetSelection();
    DV_UI.SetFontOversampling(i + 1);
}
