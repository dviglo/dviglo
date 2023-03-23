// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/profiler.h"
#include "animated_model.h"
#include "animation.h"
#include "animation_controller.h"
#include "camera.h"
#include "custom_geometry.h"
#include "debug_renderer.h"
#include "decal_set.h"
#include "graphics.h"
#include "graphics_events.h"
#include "material.h"
#include "octree.h"
#include "particle_effect.h"
#include "particle_emitter.h"
#include "ribbon_trail.h"
#include "skybox.h"
#include "static_model_group.h"
#include "technique.h"
#include "terrain.h"
#include "terrain_patch.h"
#include "zone.h"
#include "../graphics_api/graphics_impl.h"
#include "../graphics_api/shader.h"
#include "../graphics_api/shader_precache.h"
#include "../graphics_api/texture_2d.h"
#include "../graphics_api/texture_2d_array.h"
#include "../graphics_api/texture_3d.h"
#include "../graphics_api/texture_cube.h"
#include "../io/file_system.h"
#include "../io/log.h"

#include <SDL3/SDL.h>

#include "../common/debug_new.h"

namespace dviglo
{

// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool graphics_destructed = false;

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
Graphics& Graphics::get_instance()
{
    assert(!graphics_destructed);
    static Graphics instance;
    return instance;
}

bool Graphics::is_destructed()
{
    return graphics_destructed;
}

void Graphics::SetWindowTitle(const String& windowTitle)
{
    windowTitle_ = windowTitle;
    if (window_)
        SDL_SetWindowTitle(window_, windowTitle_.c_str());
}

void Graphics::SetWindowIcon(Image* windowIcon)
{
    windowIcon_ = windowIcon;
    if (window_)
        CreateWindowIcon();
}

void Graphics::SetWindowPosition(const IntVector2& position)
{
    if (window_)
        SDL_SetWindowPosition(window_, position.x_, position.y_);
    else
        position_ = position; // Sets as initial position for SDL_CreateWindow()
}

void Graphics::SetWindowPosition(int x, int y)
{
    SetWindowPosition(IntVector2(x, y));
}

void Graphics::SetOrientations(const String& orientations)
{
    orientations_ = orientations.Trimmed();
    SDL_SetHint(SDL_HINT_ORIENTATIONS, orientations_.c_str());
}

bool Graphics::SetScreenMode(int width, int height)
{
    return SetScreenMode(width, height, screenParams_);
}

bool Graphics::SetWindowModes(const WindowModeParams& windowMode, const WindowModeParams& secondaryWindowMode, bool maximize)
{
    primaryWindowMode_ = windowMode;
    secondaryWindowMode_ = secondaryWindowMode;
    return SetScreenMode(primaryWindowMode_.width_, primaryWindowMode_.height_, primaryWindowMode_.screenParams_, maximize);
}

bool Graphics::SetDefaultWindowModes(int width, int height, const ScreenModeParams& params)
{
    // Fill window mode to be applied now
    WindowModeParams primaryWindowMode;
    primaryWindowMode.width_ = width;
    primaryWindowMode.height_ = height;
    primaryWindowMode.screenParams_ = params;

    // Fill window mode to be applied on Graphics::ToggleFullscreen
    WindowModeParams secondaryWindowMode = primaryWindowMode;

    // Pick resolution automatically
    secondaryWindowMode.width_ = 0;
    secondaryWindowMode.height_ = 0;

    if (params.fullscreen_ || params.borderless_)
    {
        secondaryWindowMode.screenParams_.fullscreen_ = false;
        secondaryWindowMode.screenParams_.borderless_ = false;
    }
    else
    {
        secondaryWindowMode.screenParams_.borderless_ = true;
    }

    const bool maximize = (!width || !height) && !params.fullscreen_ && !params.borderless_ && params.resizable_;
    return SetWindowModes(primaryWindowMode, secondaryWindowMode, maximize);
}

bool Graphics::SetMode(int width, int height, bool fullscreen, bool borderless, bool resizable,
    bool highDPI, bool vsync, bool tripleBuffer, int multiSample, SDL_DisplayID display, int refreshRate)
{
    ScreenModeParams params;
    params.fullscreen_ = fullscreen;
    params.borderless_ = borderless;
    params.resizable_ = resizable;
    params.highDPI_ = highDPI;
    params.vsync_ = vsync;
    params.tripleBuffer_ = tripleBuffer;
    params.multiSample_ = multiSample;
    params.display_ = display;
    params.refreshRate_ = refreshRate;

    return SetDefaultWindowModes(width, height, params);
}

bool Graphics::SetMode(int width, int height)
{
    return SetDefaultWindowModes(width, height, screenParams_);
}

bool Graphics::ToggleFullscreen()
{
    std::swap(primaryWindowMode_, secondaryWindowMode_);
    return SetScreenMode(primaryWindowMode_.width_, primaryWindowMode_.height_, primaryWindowMode_.screenParams_);
}

void Graphics::SetShaderParameter(StringHash param, const Variant& value)
{
    switch (value.GetType())
    {
    case VAR_BOOL:
        SetShaderParameter(param, value.GetBool());
        break;

    case VAR_INT:
        SetShaderParameter(param, value.GetI32());
        break;

    case VAR_FLOAT:
    case VAR_DOUBLE:
        SetShaderParameter(param, value.GetFloat());
        break;

    case VAR_VECTOR2:
        SetShaderParameter(param, value.GetVector2());
        break;

    case VAR_VECTOR3:
        SetShaderParameter(param, value.GetVector3());
        break;

    case VAR_VECTOR4:
        SetShaderParameter(param, value.GetVector4());
        break;

    case VAR_COLOR:
        SetShaderParameter(param, value.GetColor());
        break;

    case VAR_MATRIX3:
        SetShaderParameter(param, value.GetMatrix3());
        break;

    case VAR_MATRIX3X4:
        SetShaderParameter(param, value.GetMatrix3x4());
        break;

    case VAR_MATRIX4:
        SetShaderParameter(param, value.GetMatrix4());
        break;

    case VAR_BUFFER:
        {
            const Vector<byte>& buffer = value.GetBuffer();
            if (buffer.Size() >= sizeof(float))
                SetShaderParameter(param, reinterpret_cast<const float*>(&buffer[0]), buffer.Size() / sizeof(float));
        }
        break;

    default:
        // Unsupported parameter type, do nothing
        break;
    }
}

IntVector2 Graphics::GetWindowPosition() const
{
    if (window_)
    {
        IntVector2 position;
        SDL_GetWindowPosition(window_, &position.x_, &position.y_);
        return position;
    }
    return position_;
}

Vector<IntVector3> Graphics::GetResolutions(SDL_DisplayID display) const
{
    Vector<IntVector3> ret;

    int num_modes = 0;
    const SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display, &num_modes);

    if (modes)
    {
        for (i32 i = 0; i < num_modes; ++i)
        {
            const SDL_DisplayMode* mode = modes[i];

            i32 width = mode->pixel_w;
            i32 height = mode->pixel_h;
            i32 rate = mode->refresh_rate;

            // Store mode if unique
            bool unique = true;
            for (const IntVector3& resolution : ret)
            {
                if (resolution.x_ == width && resolution.y_ == height && resolution.z_ == rate)
                {
                    unique = false;
                    break;
                }
            }

            if (unique)
                ret.Push(IntVector3(width, height, rate));
        }

        SDL_free(modes);
    }

    return ret;
}

i32 Graphics::FindBestResolutionIndex(SDL_DisplayID display, int width, int height, int refreshRate) const
{
    const Vector<IntVector3> resolutions = GetResolutions(display);
    if (resolutions.Empty())
        return NINDEX;

    i32 best = 0;
    i32 bestError = M_MAX_INT;

    for (i32 i = 0; i < resolutions.Size(); ++i)
    {
        i32 error = Abs(resolutions[i].x_ - width) + Abs(resolutions[i].y_ - height);
        if (refreshRate != 0)
            error += Abs(resolutions[i].z_ - refreshRate);
        if (error < bestError)
        {
            best = i;
            bestError = error;
        }
    }

    return best;
}

IntVector2 Graphics::GetDesktopResolution(SDL_DisplayID display) const
{
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display);
    return IntVector2(mode->pixel_w, mode->pixel_h);
}

Vector<SDL_DisplayID> Graphics::get_displays() const
{
    i32 count = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&count);

    if (!ids)
    {
        DV_LOGERRORF("Graphics::get_displays(): \"%s\"", SDL_GetError());
        return {};
    }

    Vector<SDL_DisplayID> ret(ids, count);
    SDL_free(ids);

    return ret;
}

SDL_DisplayID Graphics::get_current_display() const
{
    if (!window_)
    {
        DV_LOGERROR("Graphics::get_current_display(): !window_");
        return 0;
    }

    SDL_DisplayID ret = SDL_GetDisplayForWindow(window_);

    if (!ret)
        DV_LOGERRORF("Graphics::get_current_display(): \"%s\"", SDL_GetError());

    return ret;
}

bool Graphics::GetMaximized() const
{
    return window_? static_cast<bool>(SDL_GetWindowFlags(window_) & SDL_WINDOW_MAXIMIZED) : false;
}

void Graphics::Maximize()
{
    if (!window_)
        return;

    SDL_MaximizeWindow(window_);
}

void Graphics::Minimize()
{
    if (!window_)
        return;

    SDL_MinimizeWindow(window_);
}

void Graphics::Raise() const
{
    if (!window_)
        return;

    SDL_RaiseWindow(window_);
}

void Graphics::BeginDumpShaders(const String& fileName)
{
    shaderPrecache_ = new ShaderPrecache(fileName);
}

void Graphics::EndDumpShaders()
{
    shaderPrecache_.Reset();
}

void Graphics::PrecacheShaders(Deserializer& source)
{
    DV_PROFILE(PrecacheShaders);

    ShaderPrecache::LoadShaders(this, source);
}

void Graphics::SetShaderCacheDir(const String& path)
{
    String trimmedPath = path.Trimmed();
    if (trimmedPath.Length())
        shaderCacheDir_ = AddTrailingSlash(trimmedPath);
}

void Graphics::AddGPUObject(GpuObject* object)
{
    std::scoped_lock lock(gpuObjectMutex_);

    gpuObjects_.Push(object);
}

void Graphics::RemoveGPUObject(GpuObject* object)
{
    std::scoped_lock lock(gpuObjectMutex_);

    gpuObjects_.Remove(object);
}

void* Graphics::ReserveScratchBuffer(i32 size)
{
    assert(size >= 0);

    if (!size)
        return nullptr;

    if (size > maxScratchBufferRequest_)
        maxScratchBufferRequest_ = size;

    // First check for a free buffer that is large enough
    for (ScratchBuffer& scratchBuffer : scratchBuffers_)
    {
        if (!scratchBuffer.reserved_ && scratchBuffer.size_ >= size)
        {
            scratchBuffer.reserved_ = true;
            return scratchBuffer.data_.Get();
        }
    }

    // Then check if a free buffer can be resized
    for (ScratchBuffer& scratchBuffer : scratchBuffers_)
    {
        if (!scratchBuffer.reserved_)
        {
            scratchBuffer.data_ = new u8[size];
            scratchBuffer.size_ = size;
            scratchBuffer.reserved_ = true;

            DV_LOGDEBUG("Resized scratch buffer to size " + String(size));

            return scratchBuffer.data_.Get();
        }
    }

    // Finally allocate a new buffer
    ScratchBuffer newBuffer;
    newBuffer.data_ = new u8[size];
    newBuffer.size_ = size;
    newBuffer.reserved_ = true;
    scratchBuffers_.Push(newBuffer);

    DV_LOGDEBUG("Allocated scratch buffer with size " + String(size));

    return newBuffer.data_.Get();
}

void Graphics::FreeScratchBuffer(void* buffer)
{
    if (!buffer)
        return;

    for (ScratchBuffer& scratchBuffer : scratchBuffers_)
    {
        if (scratchBuffer.reserved_ && scratchBuffer.data_.Get() == buffer)
        {
            scratchBuffer.reserved_ = false;
            return;
        }
    }

    DV_LOGWARNING("Reserved scratch buffer " + ToStringHex((unsigned)(size_t)buffer) + " not found");
}

void Graphics::CleanupScratchBuffers()
{
    for (ScratchBuffer& scratchBuffer : scratchBuffers_)
    {
        if (!scratchBuffer.reserved_ && scratchBuffer.size_ > maxScratchBufferRequest_ * 2 && scratchBuffer.size_ >= 1024 * 1024)
        {
            scratchBuffer.data_ = maxScratchBufferRequest_ > 0 ? (new u8[maxScratchBufferRequest_]) : nullptr;
            scratchBuffer.size_ = maxScratchBufferRequest_;

            DV_LOGDEBUG("Resized scratch buffer to size " + String(maxScratchBufferRequest_));
        }
    }

    maxScratchBufferRequest_ = 0;
}

void Graphics::CreateWindowIcon()
{
    if (windowIcon_)
    {
        SDL_Surface* surface = windowIcon_->GetSDLSurface();
        if (surface)
        {
            SDL_SetWindowIcon(window_, surface);
            SDL_DestroySurface(surface);
        }
    }
}

void Graphics::AdjustScreenMode(int& newWidth, int& newHeight, ScreenModeParams& params, bool& maximize) const
{
    // High DPI is supported only for OpenGL backend
    if (GParams::get_gapi() != GAPI_OPENGL)
        params.highDPI_ = false;

#if defined(IOS) || defined(TVOS)
    // iOS and tvOS app always take the fullscreen (and with status bar hidden)
    params.fullscreen_ = true;
#endif

    // Make sure monitor index is not bigger than the currently detected monitors
    /*const int numMonitors = SDL_GetNumVideoDisplays();
    if (params.monitor_ >= numMonitors || params.monitor_ < 0)
        params.monitor_ = 0; // this monitor is not present, use first monitor*/

    // Fullscreen or Borderless can not be resizable and cannot be maximized
    if (params.fullscreen_ || params.borderless_)
    {
        params.resizable_ = false;
        maximize = false;
    }

    // Borderless cannot be fullscreen, they are mutually exclusive
    if (params.borderless_)
        params.fullscreen_ = false;

    // Ensure that multisample factor is in valid range
    params.multiSample_ = NextPowerOfTwo(Clamp(params.multiSample_, 1, 16));

    // If zero dimensions in windowed mode, set windowed mode to maximize and set a predefined default restored window size.
    // If zero in fullscreen, use desktop mode
    if (!newWidth || !newHeight)
    {
        if (params.fullscreen_ || params.borderless_)
        {
            const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(params.display_);
            newWidth = mode->pixel_w;
            newHeight = mode->pixel_h;
        }
        else
        {
            newWidth = 1024;
            newHeight = 768;
        }
    }

    // Check fullscreen mode validity (desktop only). Use a closest match if not found
#ifdef DESKTOP_GRAPHICS
    if (params.fullscreen_)
    {
        const Vector<IntVector3> resolutions = GetResolutions(params.display_);
        if (!resolutions.Empty())
        {
            const i32 bestResolution = FindBestResolutionIndex(params.display_,
                newWidth, newHeight, params.refreshRate_);
            newWidth = resolutions[bestResolution].x_;
            newHeight = resolutions[bestResolution].y_;
            params.refreshRate_ = resolutions[bestResolution].z_;
        }
    }
    else
    {
        // If windowed, use the same refresh rate as desktop
        const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(params.display_);
        params.refreshRate_ = mode->refresh_rate;
    }
#endif
}

void Graphics::OnScreenModeChanged()
{
#ifdef DV_LOGGING
    String msg;
    msg.AppendWithFormat("Set screen mode %dx%d rate %d Hz %s monitor %d", width_, height_, screenParams_.refreshRate_,
        (screenParams_.fullscreen_ ? "fullscreen" : "windowed"), screenParams_.display_);
    if (screenParams_.borderless_)
        msg.Append(" borderless");
    if (screenParams_.resizable_)
        msg.Append(" resizable");
    if (screenParams_.highDPI_)
        msg.Append(" highDPI");
    if (screenParams_.multiSample_ > 1)
        msg.AppendWithFormat(" multisample %d", screenParams_.multiSample_);
    DV_LOGINFO(msg);
#endif

    using namespace ScreenMode;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_WIDTH] = width_;
    eventData[P_HEIGHT] = height_;
    eventData[P_FULLSCREEN] = screenParams_.fullscreen_;
    eventData[P_BORDERLESS] = screenParams_.borderless_;
    eventData[P_RESIZABLE] = screenParams_.resizable_;
    eventData[P_HIGHDPI] = screenParams_.highDPI_;
    eventData[P_MONITOR] = screenParams_.display_;
    eventData[P_REFRESHRATE] = screenParams_.refreshRate_;
    SendEvent(E_SCREENMODE, eventData);
}

Graphics::Graphics()
{
    assert(!GParams::is_headless());

    GAPI gapi = GParams::get_gapi();

    // Проверяем, что gapi установлен перед вызовом конструктора
    assert(gapi != GAPI_NONE);

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
    {
        Constructor_OGL();
        goto end;
    }
#endif

end:
    DV_LOGDEBUG("Singleton Graphics constructed");
}

Graphics::~Graphics()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
    {
        Destructor_OGL();
        goto end;
    }
#endif

end:
    DV_LOGDEBUG("Singleton Graphics destructed");

    graphics_destructed = true;
}

bool Graphics::SetScreenMode(int width, int height, const ScreenModeParams& params, bool maximize)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetScreenMode_OGL(width, height, params, maximize);
#endif

    return {}; // Prevent warning
}

void Graphics::SetSRGB(bool enable)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetSRGB_OGL(enable);
#endif
}

void Graphics::SetDither(bool enable)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDither_OGL(enable);
#endif
}

void Graphics::SetFlushGPU(bool enable)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetFlushGPU_OGL(enable);
#endif
}

void Graphics::Close()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Close_OGL();
#endif
}

bool Graphics::TakeScreenShot(Image& destImage)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return TakeScreenShot_OGL(destImage);
#endif

    return {}; // Prevent warning
}

bool Graphics::BeginFrame()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return BeginFrame_OGL();
#endif

    return {}; // Prevent warning
}

void Graphics::EndFrame()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return EndFrame_OGL();
#endif
}

void Graphics::Clear(ClearTargetFlags flags, const Color& color, float depth, unsigned stencil)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Clear_OGL(flags, color, depth, stencil);
#endif
}

bool Graphics::ResolveToTexture(Texture2D* destination, const IntRect& viewport)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return ResolveToTexture_OGL(destination, viewport);
#endif

    return {}; // Prevent warning
}

bool Graphics::ResolveToTexture(Texture2D* texture)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return ResolveToTexture_OGL(texture);
#endif

    return {}; // Prevent warning
}

bool Graphics::ResolveToTexture(TextureCube* texture)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return ResolveToTexture_OGL(texture);
#endif

    return {}; // Prevent warning
}

void Graphics::Draw(PrimitiveType type, unsigned vertexStart, unsigned vertexCount)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Draw_OGL(type, vertexStart, vertexCount);
#endif
}

void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Draw_OGL(type, indexStart, indexCount, minVertex, vertexCount);
#endif
}

void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex, unsigned vertexCount)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Draw_OGL(type, indexStart, indexCount, baseVertexIndex, minVertex, vertexCount);
#endif
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount, unsigned instanceCount)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return DrawInstanced_OGL(type, indexStart, indexCount, minVertex, vertexCount, instanceCount);
#endif
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex,
    unsigned vertexCount, unsigned instanceCount)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return DrawInstanced_OGL(type, indexStart, indexCount, baseVertexIndex, minVertex, vertexCount, instanceCount);
#endif
}

void Graphics::SetVertexBuffer(VertexBuffer* buffer)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetVertexBuffer_OGL(buffer);
#endif
}

bool Graphics::SetVertexBuffers(const Vector<VertexBuffer*>& buffers, unsigned instanceOffset)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetVertexBuffers_OGL(buffers, instanceOffset);
#endif

    return {}; // Prevent warning
}

bool Graphics::SetVertexBuffers(const Vector<SharedPtr<VertexBuffer>>& buffers, unsigned instanceOffset)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetVertexBuffers_OGL(buffers, instanceOffset);
#endif

    return {}; // Prevent warning
}

void Graphics::SetIndexBuffer(IndexBuffer* buffer)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetIndexBuffer_OGL(buffer);
#endif
}

void Graphics::SetShaders(ShaderVariation* vs, ShaderVariation* ps)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaders_OGL(vs, ps);
#endif
}

void Graphics::SetShaderParameter(StringHash param, const float* data, unsigned count)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, data, count);
#endif
}

void Graphics::SetShaderParameter(StringHash param, float value)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, value);
#endif
}

void Graphics::SetShaderParameter(StringHash param, int value)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, value);
#endif
}

void Graphics::SetShaderParameter(StringHash param, bool value)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, value);
#endif
}

void Graphics::SetShaderParameter(StringHash param, const Color& color)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, color);
#endif
}

void Graphics::SetShaderParameter(StringHash param, const Vector2& vector)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, vector);
#endif
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3& matrix)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, matrix);
#endif
}

void Graphics::SetShaderParameter(StringHash param, const Vector3& vector)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, vector);
#endif
}

void Graphics::SetShaderParameter(StringHash param, const Matrix4& matrix)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, matrix);
#endif
}

void Graphics::SetShaderParameter(StringHash param, const Vector4& vector)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, vector);
#endif
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3x4& matrix)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetShaderParameter_OGL(param, matrix);
#endif
}

bool Graphics::NeedParameterUpdate(ShaderParameterGroup group, const void* source)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return NeedParameterUpdate_OGL(group, source);
#endif

    return {}; // Prevent warning
}

bool Graphics::HasShaderParameter(StringHash param)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return HasShaderParameter_OGL(param);
#endif

    return {}; // Prevent warning
}

bool Graphics::HasTextureUnit(TextureUnit unit)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return HasTextureUnit_OGL(unit);
#endif

    return {}; // Prevent warning
}

void Graphics::ClearParameterSource(ShaderParameterGroup group)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return ClearParameterSource_OGL(group);
#endif
}

void Graphics::ClearParameterSources()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return ClearParameterSources_OGL();
#endif
}

void Graphics::ClearTransformSources()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return ClearTransformSources_OGL();
#endif
}

void Graphics::SetTexture(unsigned index, Texture* texture)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetTexture_OGL(index, texture);
#endif
}

void Graphics::SetDefaultTextureFilterMode(TextureFilterMode mode)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDefaultTextureFilterMode_OGL(mode);
#endif
}

void Graphics::SetDefaultTextureAnisotropy(unsigned level)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDefaultTextureAnisotropy_OGL(level);
#endif
}

void Graphics::ResetRenderTargets()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return ResetRenderTargets_OGL();
#endif
}

void Graphics::ResetRenderTarget(unsigned index)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return ResetRenderTarget_OGL(index);
#endif
}

void Graphics::ResetDepthStencil()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return ResetDepthStencil_OGL();
#endif
}

void Graphics::SetRenderTarget(unsigned index, RenderSurface* renderTarget)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetRenderTarget_OGL(index, renderTarget);
#endif
}

void Graphics::SetRenderTarget(unsigned index, Texture2D* texture)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetRenderTarget_OGL(index, texture);
#endif
}

void Graphics::SetDepthStencil(RenderSurface* depthStencil)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDepthStencil_OGL(depthStencil);
#endif
}

void Graphics::SetDepthStencil(Texture2D* texture)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDepthStencil_OGL(texture);
#endif
}

void Graphics::SetViewport(const IntRect& rect)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetViewport_OGL(rect);
#endif
}

void Graphics::SetBlendMode(BlendMode mode, bool alphaToCoverage)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetBlendMode_OGL(mode, alphaToCoverage);
#endif
}

void Graphics::SetColorWrite(bool enable)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetColorWrite_OGL(enable);
#endif
}

void Graphics::SetCullMode(CullMode mode)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetCullMode_OGL(mode);
#endif
}

void Graphics::SetDepthBias(float constantBias, float slopeScaledBias)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDepthBias_OGL(constantBias, slopeScaledBias);
#endif
}

void Graphics::SetDepthTest(CompareMode mode)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDepthTest_OGL(mode);
#endif
}

void Graphics::SetDepthWrite(bool enable)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDepthWrite_OGL(enable);
#endif
}

void Graphics::SetFillMode(FillMode mode)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetFillMode_OGL(mode);
#endif
}

void Graphics::SetLineAntiAlias(bool enable)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetLineAntiAlias_OGL(enable);
#endif
}

void Graphics::SetScissorTest(bool enable, const Rect& rect, bool borderInclusive)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetScissorTest_OGL(enable, rect, borderInclusive);
#endif
}

void Graphics::SetScissorTest(bool enable, const IntRect& rect)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetScissorTest_OGL(enable, rect);
#endif
}

void Graphics::SetClipPlane(bool enable, const Plane& clipPlane, const Matrix3x4& view, const Matrix4& projection)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetClipPlane_OGL(enable, clipPlane, view, projection);
#endif
}

void Graphics::SetStencilTest(bool enable, CompareMode mode, StencilOp pass, StencilOp fail, StencilOp zFail, u32 stencilRef,
    u32 compareMask, u32 writeMask)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetStencilTest_OGL(enable, mode, pass, fail, zFail, stencilRef, compareMask, writeMask);
#endif
}

bool Graphics::IsInitialized() const
{
    return window_ != nullptr;
}

bool Graphics::GetDither() const
{
    return glIsEnabled(GL_DITHER) ? true : false;
}

bool Graphics::IsDeviceLost() const
{
    return GetImpl_OGL()->context_ == nullptr;
}

Vector<int> Graphics::GetMultiSampleLevels() const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetMultiSampleLevels_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetFormat(CompressedFormat format) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetFormat_OGL(format);
#endif

    return {}; // Prevent warning
}

ShaderVariation* Graphics::GetShader(ShaderType type, const String& name, const String& defines) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetShader_OGL(type, name, defines);
#endif

    return {}; // Prevent warning
}

ShaderVariation* Graphics::GetShader(ShaderType type, const char* name, const char* defines) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetShader_OGL(type, name, defines);
#endif

    return {}; // Prevent warning
}

VertexBuffer* Graphics::GetVertexBuffer(unsigned index) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetVertexBuffer_OGL(index);
#endif

    return {}; // Prevent warning
}

TextureUnit Graphics::GetTextureUnit(const String& name)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetTextureUnit_OGL(name);
#endif

    return {}; // Prevent warning
}

const String& Graphics::GetTextureUnitName(TextureUnit unit)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetTextureUnitName_OGL(unit);
#endif

    return String::EMPTY; // Prevent warning
}

Texture* Graphics::GetTexture(unsigned index) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetTexture_OGL(index);
#endif

    return {}; // Prevent warning
}

RenderSurface* Graphics::GetRenderTarget(unsigned index) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRenderTarget_OGL(index);
#endif

    return {}; // Prevent warning
}

IntVector2 Graphics::GetRenderTargetDimensions() const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRenderTargetDimensions_OGL();
#endif

    return {}; // Prevent warning
}

void Graphics::OnWindowResized()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnWindowResized_OGL();
#endif
}

void Graphics::OnWindowMoved()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnWindowMoved_OGL();
#endif
}

ConstantBuffer* Graphics::GetOrCreateConstantBuffer(ShaderType type, unsigned index, unsigned size)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetOrCreateConstantBuffer_OGL(type, index, size);
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetMaxBones()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetMaxBones_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetAlphaFormat()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetAlphaFormat_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetLuminanceFormat()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetLuminanceFormat_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetLuminanceAlphaFormat()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetLuminanceAlphaFormat_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetRGBFormat()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRGBFormat_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetRGBAFormat()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRGBAFormat_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetRGBA16Format()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRGBA16Format_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetRGBAFloat16Format()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRGBAFloat16Format_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetRGBAFloat32Format()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRGBAFloat32Format_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetRG16Format()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRG16Format_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetRGFloat16Format()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRGFloat16Format_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetRGFloat32Format()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRGFloat32Format_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetFloat16Format()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetFloat16Format_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetFloat32Format()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetFloat32Format_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetLinearDepthFormat()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetLinearDepthFormat_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetDepthStencilFormat()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetDepthStencilFormat_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetReadableDepthFormat()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetReadableDepthFormat_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Graphics::GetFormat(const String& formatName)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetFormat_OGL(formatName);
#endif

    return {}; // Prevent warning
}

void RegisterGraphicsLibrary()
{
    Animation::RegisterObject();
    Material::RegisterObject();
    Model::RegisterObject();
    Shader::RegisterObject();
    Technique::RegisterObject();
    Texture2D::RegisterObject();
    Texture2DArray::RegisterObject();
    Texture3D::RegisterObject();
    TextureCube::RegisterObject();
    Camera::RegisterObject();
    Drawable::RegisterObject();
    Light::RegisterObject();
    StaticModel::RegisterObject();
    StaticModelGroup::RegisterObject();
    Skybox::RegisterObject();
    AnimatedModel::RegisterObject();
    AnimationController::RegisterObject();
    BillboardSet::RegisterObject();
    ParticleEffect::RegisterObject();
    ParticleEmitter::RegisterObject();
    RibbonTrail::RegisterObject();
    CustomGeometry::RegisterObject();
    DecalSet::RegisterObject();
    Terrain::RegisterObject();
    TerrainPatch::RegisterObject();
    DebugRenderer::RegisterObject();
    Octree::RegisterObject();
    Zone::RegisterObject();
}

}
