// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../containers/ptr.h"
#include "../containers/ref_counted.h"
#include "../core/variant.h"
#include "../graphics_api/graphics_defs.h"
#include "../math/color.h"
#include "../math/vector4.h"

namespace dviglo
{

class Variant;
class XmlElement;
class XmlFile;

/// Rendering path command types.
enum RenderCommandType
{
    CMD_NONE = 0,
    CMD_CLEAR,
    CMD_SCENEPASS,
    CMD_QUAD,
    CMD_FORWARDLIGHTS,
    CMD_LIGHTVOLUMES,
    CMD_RENDERUI,
    CMD_SENDEVENT
};

/// Rendering path sorting modes.
enum RenderCommandSortMode
{
    SORT_FRONTTOBACK = 0,
    SORT_BACKTOFRONT
};

/// Rendertarget size mode.
enum RenderTargetSizeMode
{
    SIZE_ABSOLUTE = 0,
    SIZE_VIEWPORTDIVISOR,
    SIZE_VIEWPORTMULTIPLIER
};

/// Rendertarget definition.
struct DV_API RenderTargetInfo
{
    /// Read from an XML element.
    void Load(const XmlElement& element);

    /// Name.
    String name_;
    /// Tag name.
    String tag_;
    /// Texture format.
    unsigned format_{};
    /// Absolute size or multiplier.
    Vector2 size_;
    /// Size mode.
    RenderTargetSizeMode sizeMode_{SIZE_ABSOLUTE};
    /// Multisampling level (1 = no multisampling).
    int multiSample_{1};
    /// Multisampling autoresolve flag.
    bool autoResolve_{true};
    /// Enabled flag.
    bool enabled_{true};
    /// Cube map flag.
    bool cubemap_{};
    /// Filtering flag.
    bool filtered_{};
    /// sRGB sampling/writing mode flag.
    bool sRGB_{};
    /// Should be persistent and not shared/reused between other buffers of same size.
    bool persistent_{};
};

/// Rendering path command.
struct DV_API RenderPathCommand
{
    /// Read from an XML element.
    void Load(const XmlElement& element);
    /// Set a texture resource name. Can also refer to a rendertarget defined in the rendering path.
    void SetTextureName(TextureUnit unit, const String& name);
    /// Set a shader parameter.
    void SetShaderParameter(const String& name, const Variant& value);
    /// Remove a shader parameter.
    void RemoveShaderParameter(const String& name);
    /// Set number of output rendertargets.
    void SetNumOutputs(i32 num);
    /// Set output rendertarget name and face index for cube maps.
    void SetOutput(i32 index, const String& name, CubeMapFace face = FACE_POSITIVE_X);
    /// Set output rendertarget name.
    void SetOutputName(i32 index, const String& name);
    /// Set output rendertarget face index for cube maps.
    void SetOutputFace(i32 index, CubeMapFace face);
    /// Set depth-stencil output name. When empty, will assign a depth-stencil buffer automatically.
    void SetDepthStencilName(const String& name);

    /// Return texture resource name.
    const String& GetTextureName(TextureUnit unit) const;
    /// Return shader parameter.
    const Variant& GetShaderParameter(const String& name) const;

    /// Return number of output rendertargets.
    i32 GetNumOutputs() const { return outputs_.Size(); }

    /// Return output rendertarget name.
    const String& GetOutputName(i32 index) const;
    /// Return output rendertarget face index.
    CubeMapFace GetOutputFace(i32 index) const;

    /// Return depth-stencil output name.
    const String& GetDepthStencilName() const { return depthStencilName_; }

    /// Tag name.
    String tag_;
    /// Command type.
    RenderCommandType type_{};
    /// Sorting mode.
    RenderCommandSortMode sortMode_{};
    /// Scene pass name.
    String pass_;
    /// Scene pass index. Filled by View.
    unsigned passIndex_{};
    /// Command/pass metadata.
    String metadata_;
    /// Vertex shader name.
    String vertexShaderName_;
    /// Pixel shader name.
    String pixelShaderName_;
    /// Vertex shader defines.
    String vertexShaderDefines_;
    /// Pixel shader defines.
    String pixelShaderDefines_;
    /// Textures.
    String textureNames_[MAX_TEXTURE_UNITS];
    /// %Shader parameters.
    HashMap<StringHash, Variant> shaderParameters_;
    /// Output rendertarget names and faces.
    Vector<Pair<String, CubeMapFace>> outputs_;
    /// Depth-stencil output name.
    String depthStencilName_;
    /// Clear flags. Affects clear command only.
    ClearTargetFlags clearFlags_{};
    /// Clear color. Affects clear command only.
    Color clearColor_;
    /// Clear depth. Affects clear command only.
    float clearDepth_{};
    /// Clear stencil value. Affects clear command only.
    unsigned clearStencil_{};
    /// Blend mode. Affects quad command only.
    BlendMode blend_mode_{BLEND_REPLACE};
    /// Enabled flag.
    bool enabled_{true};
    /// Use fog color for clearing.
    bool useFogColor_{};
    /// Mark to stencil flag.
    bool markToStencil_{};
    /// Use lit base pass optimization for forward per-pixel lights.
    bool useLitBase_{true};
    /// Vertex lights flag.
    bool vertexLights_{};
    /// Event name.
    String eventName_;
};

/// Rendering path definition. A sequence of commands (e.g. clear screen, draw objects with specific pass) that yields the scene rendering result.
class DV_API RenderPath : public RefCounted
{
public:
    /// Construct.
    RenderPath();
    /// Destruct.
    ~RenderPath() override;

    /// Clone the rendering path.
    SharedPtr<RenderPath> Clone();
    /// Clear existing data and load from an XML file. Return true if successful.
    bool Load(XmlFile* file);
    /// Append data from an XML file. Return true if successful.
    bool Append(XmlFile* file);
    /// Enable/disable commands and rendertargets by tag.
    void SetEnabled(const String& tag, bool active);
    /// Return true of any of render targets or commands with specified tag are enabled.
    bool IsEnabled(const String& tag) const;
    /// Return true if renderpath or command with given tag exists.
    bool IsAdded(const String& tag) const;
    /// Toggle enabled state of commands and rendertargets by tag.
    void ToggleEnabled(const String& tag);
    /// Assign rendertarget at index.
    void SetRenderTarget(unsigned index, const RenderTargetInfo& info);
    /// Add a rendertarget.
    void AddRenderTarget(const RenderTargetInfo& info);
    /// Remove a rendertarget by index.
    void RemoveRenderTarget(unsigned index);
    /// Remove a rendertarget by name.
    void RemoveRenderTarget(const String& name);
    /// Remove rendertargets by tag name.
    void RemoveRenderTargets(const String& tag);
    /// Assign command at index.
    void SetCommand(unsigned index, const RenderPathCommand& command);
    /// Add a command to the end of the list.
    void AddCommand(const RenderPathCommand& command);
    /// Insert a command at a position.
    void InsertCommand(unsigned index, const RenderPathCommand& command);
    /// Remove a command by index.
    void RemoveCommand(unsigned index);
    /// Remove commands by tag name.
    void RemoveCommands(const String& tag);
    /// Set a shader parameter in all commands that define it.
    void SetShaderParameter(const String& name, const Variant& value);

    /// Return number of rendertargets.
    i32 GetNumRenderTargets() const { return renderTargets_.Size(); }

    /// Return number of commands.
    i32 GetNumCommands() const { return commands_.Size(); }

    /// Return command at index, or null if does not exist.
    RenderPathCommand* GetCommand(i32 index)
    {
        assert(index >= 0);
        return index < commands_.Size() ? &commands_[index] : nullptr;
    }

    /// Return a shader parameter (first appearance in any command).
    const Variant& GetShaderParameter(const String& name) const;

    /// Rendertargets.
    Vector<RenderTargetInfo> renderTargets_;
    /// Rendering commands.
    Vector<RenderPathCommand> commands_;
};

}
