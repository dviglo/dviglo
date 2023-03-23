// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../containers/flag_set.h"
#include "../core/object.h"
#include "../core/timer.h"

namespace dviglo
{

class Engine;
class Font;
class Text;
class XmlFile;

enum class DebugHudElements
{
    None          = 0,
    Stats         = 1 << 0,
    Mode          = 1 << 1,
    Memory        = 1 << 2,
    All           = Stats | Mode | Memory
};
DV_FLAGS(DebugHudElements);

/// Displays rendering stats and profiling information.
class DV_API DebugHud : public Object
{
    DV_OBJECT(DebugHud, Object);

public:
    static DebugHud& get_instance();

private:
    /// Construct.
    explicit DebugHud();
    /// Destruct.
    ~DebugHud() override;

public:
    // Запрещаем копирование
    DebugHud(const DebugHud&) = delete;
    DebugHud& operator =(const DebugHud&) = delete;

    /// Update. Called by HandlePostUpdate().
    void Update();
    /// Set UI elements' style from an XML file.
    void SetDefaultStyle(XmlFile* style);

    /// Set elements to show.
    void SetMode(DebugHudElements mode);

    /// Set whether to show 3D geometry primitive/batch count only. Default false.
    void SetUseRendererStats(bool enable);

    /// Toggle elements.
    void Toggle(DebugHudElements mode);

    /// Toggle all elements.
    void ToggleAll();

    /// Return the UI style file.
    XmlFile* GetDefaultStyle() const;

    /// Return rendering stats text.
    Text* GetStatsText() const { return statsText_; }

    /// Return rendering mode text.
    Text* GetModeText() const { return modeText_; }

    /// Return memory text.
    Text* GetMemoryText() const { return memoryText_; }

    /// Return currently shown elements.
    DebugHudElements GetMode() const { return mode_; }

    /// Return whether showing 3D geometry primitive/batch count only.
    bool GetUseRendererStats() const { return useRendererStats_; }

    /// Set application-specific stats.
    void SetAppStats(const String& label, const Variant& stats);
    /// Set application-specific stats.
    void SetAppStats(const String& label, const String& stats);
    /// Reset application-specific stats. Return true if it was erased successfully.
    bool ResetAppStats(const String& label);
    /// Clear all application-specific stats.
    void ClearAppStats();

private:
    /// Handle logic post-update event. The HUD texts are updated here.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    /// Rendering stats text.
    SharedPtr<Text> statsText_;
    /// Rendering mode text.
    SharedPtr<Text> modeText_;
    /// Memory stats text.
    SharedPtr<Text> memoryText_;
    /// Hashmap containing application specific stats.
    HashMap<String, String> appStats_;

    /// Отображаемый FPS
    u32 fps_ = 0;

    /// Нужен, чтобы обновлять FPS не каждый кадр
    Timer fps_timer_;

    /// Show 3D geometry primitive/batch count flag.
    bool useRendererStats_;

    /// Current shown-element mode.
    DebugHudElements mode_;
};

#define DV_DEBUG_HUD (dviglo::DebugHud::get_instance())

}
