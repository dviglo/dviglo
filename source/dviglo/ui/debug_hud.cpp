// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "debug_hud.h"

#include "../core/context.h"
#include "../core/core_events.h"
#include "../engine/engine.h"
#include "../graphics/graphics.h"
#include "../graphics/renderer.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "font.h"
#include "text.h"
#include "ui.h"

#include "../common/debug_new.h"

namespace dviglo
{

static const char* qualityTexts[] =
{
    "Low",
    "Med",
    "High",
    "High+"
};

static const char* shadowQualityTexts[] =
{
    "16bit Simple",
    "24bit Simple",
    "16bit PCF",
    "24bit PCF",
    "VSM",
    "Blurred VSM"
};


#ifndef NDEBUG
// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool debug_hud_destructed = false;
#endif

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
DebugHud& DebugHud::get_instance()
{
    assert(!debug_hud_destructed);
    static DebugHud instance;
    return instance;
}

DebugHud::DebugHud() :
    useRendererStats_(false),
    mode_(DebugHudElements::None)
{
    assert(!GParams::is_headless());

    UiElement* uiRoot = DV_UI->GetRoot();

    statsText_ = new Text();
    statsText_->SetAlignment(HA_LEFT, VA_TOP);
    statsText_->SetPriority(100);
    statsText_->SetVisible(false);
    uiRoot->AddChild(statsText_);

    modeText_ = new Text();
    modeText_->SetAlignment(HA_LEFT, VA_BOTTOM);
    modeText_->SetPriority(100);
    modeText_->SetVisible(false);
    uiRoot->AddChild(modeText_);

    memoryText_ = new Text();
    memoryText_->SetAlignment(HA_LEFT, VA_BOTTOM);
    memoryText_->SetPriority(100);
    memoryText_->SetVisible(false);
    uiRoot->AddChild(memoryText_);

    subscribe_to_event(E_POSTUPDATE, DV_HANDLER(DebugHud, HandlePostUpdate));

    DV_LOGDEBUG("Singleton DebugHud constructed");
}

DebugHud::~DebugHud()
{
    statsText_->Remove();
    modeText_->Remove();
    memoryText_->Remove();

    DV_LOGDEBUG("Singleton DebugHud destructed");

#ifndef NDEBUG
    debug_hud_destructed = true;
#endif
}

void DebugHud::Update()
{
    if (GParams::is_headless())
        return;

    Graphics* graphics = DV_GRAPHICS;
    Renderer* renderer = DV_RENDERER;

    // Ensure UI-elements are not detached
    if (!statsText_->GetParent())
    {
        UiElement* uiRoot = DV_UI->GetRoot();
        uiRoot->AddChild(statsText_);
        uiRoot->AddChild(modeText_);
    }

    if (statsText_->IsVisible())
    {
        unsigned primitives, batches;
        if (!useRendererStats_)
        {
            primitives = graphics->GetNumPrimitives();
            batches = graphics->GetNumBatches();
        }
        else
        {
            primitives = renderer->GetNumPrimitives();
            batches = renderer->GetNumBatches();
        }

        if (fps_timer_.GetMSec(false) >= 500)
        {
            fps_timer_.Reset();
            fps_ = (u32)Round(DV_TIME.GetFramesPerSecond());
        }

        String stats;
        stats.AppendWithFormat("FPS %u\nTriangles %u\nBatches %u\nViews %u\nLights %u\nShadowmaps %u\nOccluders %u",
            fps_,
            primitives,
            batches,
            renderer->GetNumViews(),
            renderer->GetNumLights(true),
            renderer->GetNumShadowMaps(true),
            renderer->GetNumOccluders(true));

        if (!appStats_.Empty())
        {
            stats.Append("\n");
            for (HashMap<String, String>::ConstIterator i = appStats_.Begin(); i != appStats_.End(); ++i)
                stats.AppendWithFormat("\n%s %s", i->first_.c_str(), i->second_.c_str());
        }

        statsText_->SetText(stats);
    }

    if (modeText_->IsVisible())
    {
        String mode;
        mode.AppendWithFormat("Tex:%s Mat:%s Spec:%s Shadows:%s Size:%i Quality:%s Occlusion:%s Instancing:%s",
            qualityTexts[renderer->GetTextureQuality()],
            qualityTexts[Min((i32)renderer->GetMaterialQuality(), 3)],
            renderer->GetSpecularLighting() ? "On" : "Off",
            renderer->GetDrawShadows() ? "On" : "Off",
            renderer->GetShadowMapSize(),
            shadowQualityTexts[renderer->GetShadowQuality()],
            renderer->GetMaxOccluderTriangles() > 0 ? "On" : "Off",
            renderer->GetDynamicInstancing() ? "On" : "Off");
    #ifdef DV_OPENGL
        mode.AppendWithFormat(" Renderer:%s Version:%s", graphics->GetRendererName().c_str(),
            graphics->GetVersionString().c_str());
    #endif


        modeText_->SetText(mode);
    }

    if (memoryText_->IsVisible())
        memoryText_->SetText(DV_RES_CACHE.print_memory_usage());
}

void DebugHud::SetDefaultStyle(XmlFile* style)
{
    if (!style)
        return;

    statsText_->SetDefaultStyle(style);
    statsText_->SetStyle("DebugHudText");
    modeText_->SetDefaultStyle(style);
    modeText_->SetStyle("DebugHudText");
    memoryText_->SetDefaultStyle(style);
    memoryText_->SetStyle("DebugHudText");
}

void DebugHud::SetMode(DebugHudElements mode)
{
    statsText_->SetVisible(!!(mode & DebugHudElements::Stats));
    modeText_->SetVisible(!!(mode & DebugHudElements::Mode));
    memoryText_->SetVisible(!!(mode & DebugHudElements::Memory));

    memoryText_->SetPosition(0, modeText_->IsVisible() ? modeText_->GetHeight() * -2 : 0);

    mode_ = mode;
}

void DebugHud::SetUseRendererStats(bool enable)
{
    useRendererStats_ = enable;
}

void DebugHud::Toggle(DebugHudElements mode)
{
    SetMode(GetMode() ^ mode);
}

void DebugHud::ToggleAll()
{
    Toggle(DebugHudElements::All);
}

XmlFile* DebugHud::GetDefaultStyle() const
{
    return statsText_->GetDefaultStyle(false);
}

void DebugHud::SetAppStats(const String& label, const Variant& stats)
{
    SetAppStats(label, stats.ToString());
}

void DebugHud::SetAppStats(const String& label, const String& stats)
{
    bool newLabel = !appStats_.Contains(label);
    appStats_[label] = stats;
    if (newLabel)
        appStats_.Sort();
}

bool DebugHud::ResetAppStats(const String& label)
{
    return appStats_.Erase(label);
}

void DebugHud::ClearAppStats()
{
    appStats_.Clear();
}

void DebugHud::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace PostUpdate;

    Update();
}

} // namespace dviglo
