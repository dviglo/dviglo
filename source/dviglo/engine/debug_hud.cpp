// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "debug_hud.h"

#include "../core/core_events.h"
#include "../core/context.h"
#include "engine.h"
#include "../graphics/graphics.h"
#include "../graphics/renderer.h"
#include "../resource/resource_cache.h"
#include "../io/log.h"
#include "../ui/font.h"
#include "../ui/text.h"
#include "../ui/ui.h"

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


#ifdef _DEBUG
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
    profilerMaxDepth_(M_MAX_UNSIGNED),
    profilerInterval_(1000),
    useRendererStats_(false),
    mode_(DebugHudElements::None)
{
    assert(!GParams::is_headless());

    UIElement* uiRoot = DV_UI.GetRoot();

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

    SubscribeToEvent(E_POSTUPDATE, DV_HANDLER(DebugHud, HandlePostUpdate));

    DV_LOGDEBUG("Singleton DebugHud constructed");
}

DebugHud::~DebugHud()
{
    statsText_->Remove();
    modeText_->Remove();
    memoryText_->Remove();

    DV_LOGDEBUG("Singleton DebugHud destructed");

#ifdef _DEBUG
    debug_hud_destructed = true;
#endif
}

void DebugHud::Update()
{
    if (GParams::is_headless())
        return;

    Graphics& graphics = DV_GRAPHICS;
    Renderer& renderer = DV_RENDERER;

    // Ensure UI-elements are not detached
    if (!statsText_->GetParent())
    {
        UIElement* uiRoot = DV_UI.GetRoot();
        uiRoot->AddChild(statsText_);
        uiRoot->AddChild(modeText_);
    }

    if (statsText_->IsVisible())
    {
        unsigned primitives, batches;
        if (!useRendererStats_)
        {
            primitives = graphics.GetNumPrimitives();
            batches = graphics.GetNumBatches();
        }
        else
        {
            primitives = renderer.GetNumPrimitives();
            batches = renderer.GetNumBatches();
        }

        String stats;
        stats.AppendWithFormat("Triangles %u\nBatches %u\nViews %u\nLights %u\nShadowmaps %u\nOccluders %u",
            primitives,
            batches,
            renderer.GetNumViews(),
            renderer.GetNumLights(true),
            renderer.GetNumShadowMaps(true),
            renderer.GetNumOccluders(true));

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
            qualityTexts[renderer.GetTextureQuality()],
            qualityTexts[Min((i32)renderer.GetMaterialQuality(), 3)],
            renderer.GetSpecularLighting() ? "On" : "Off",
            renderer.GetDrawShadows() ? "On" : "Off",
            renderer.GetShadowMapSize(),
            shadowQualityTexts[renderer.GetShadowQuality()],
            renderer.GetMaxOccluderTriangles() > 0 ? "On" : "Off",
            renderer.GetDynamicInstancing() ? "On" : "Off");
    #ifdef DV_OPENGL
        mode.AppendWithFormat(" Renderer:%s Version:%s", graphics.GetRendererName().c_str(),
            graphics.GetVersionString().c_str());
    #endif


        modeText_->SetText(mode);
    }

    if (memoryText_->IsVisible())
        memoryText_->SetText(DV_RES_CACHE.PrintMemoryUsage());
}

void DebugHud::SetDefaultStyle(XMLFile* style)
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

void DebugHud::SetProfilerMaxDepth(unsigned depth)
{
    profilerMaxDepth_ = depth;
}

void DebugHud::SetProfilerInterval(float interval)
{
    profilerInterval_ = Max((unsigned)(interval * 1000.0f), 0U);
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

XMLFile* DebugHud::GetDefaultStyle() const
{
    return statsText_->GetDefaultStyle(false);
}

float DebugHud::GetProfilerInterval() const
{
    return (float)profilerInterval_ / 1000.0f;
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
