// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../audio/audio.h"
#include "../core/context.h"
#include "../core/core_events.h"
#include "../core/event_profiler.h"
#include "../core/process_utils.h"
#include "../core/work_queue.h"
#include "console.h"
#include "debug_hud.h"
#include "engine.h"
#include "engine_defs.h"
#include "../graphics/graphics.h"
#include "../graphics/renderer.h"
#include "../input/input.h"
#include "../io/file_system.h"
#include "../io/fs_base.h"
#include "../io/log.h"
#include "../io/package_file.h"
#ifdef DV_NAVIGATION
#include "../navigation/navigation_mesh.h"
#endif
#ifdef DV_NETWORK
#include "../network/network.h"
#endif
#ifdef DV_DATABASE
#include "../database/database.h"
#endif
#ifdef DV_BULLET
#include "../physics/physics_world.h"
#include "../physics/raycast_vehicle.h"
#endif
#ifdef DV_BOX2D
#include "../physics_2d/physics_2d.h"
#endif
#include "../resource/resource_cache.h"
#include "../resource/localization.h"
#include "../scene/scene.h"
#include "../scene/scene_events.h"
#include "../ui/ui.h"
#ifdef DV_URHO2D
#include "../urho_2d/urho_2d.h"
#endif

#if defined(__EMSCRIPTEN__) && defined(DV_TESTING)
#include <emscripten/emscripten.h>
#endif

#include "../common/debug_new.h"


#if defined(_MSC_VER) && defined(_DEBUG)
// From dbgint.h
#define nNoMansLandSize 4

typedef struct _CrtMemBlockHeader
{
    struct _CrtMemBlockHeader* pBlockHeaderNext;
    struct _CrtMemBlockHeader* pBlockHeaderPrev;
    char* szFileName;
    int nLine;
    size_t nDataSize;
    int nBlockUse;
    long lRequest;
    unsigned char gap[nNoMansLandSize];
} _CrtMemBlockHeader;
#endif

namespace dviglo
{

extern const char* logLevelPrefixes[];

Engine::Engine() :
    timeStep_(0.0f),
    timeStepSmoothing_(2),
    minFps_(10),
#if defined(IOS) || defined(TVOS) || defined(__ANDROID__) || defined(__arm__) || defined(__aarch64__)
    maxFps_(60),
    maxInactiveFps_(10),
    pauseMinimized_(true),
#else
    maxFps_(200),
    maxInactiveFps_(60),
    pauseMinimized_(false),
#endif
#ifdef DV_TESTING
    timeOut_(0),
#endif
    autoExit_(true),
    initialized_(false),
    exiting_(false),
    headless_(false),
    audioPaused_(false)
{
    // Создаём синглтоны, которые не зависят от параметров движка
    Time::get_instance();

    // Register self as a subsystem
    DV_CONTEXT.RegisterSubsystem(this);

    // Create subsystems which do not depend on engine initialization or startup parameters
    DV_CONTEXT.RegisterSubsystem(new WorkQueue());
#ifdef DV_PROFILING
    DV_CONTEXT.RegisterSubsystem(new Profiler());
#endif
    DV_CONTEXT.RegisterSubsystem(new FileSystem());
    DV_CONTEXT.RegisterSubsystem(new ResourceCache());
    DV_CONTEXT.RegisterSubsystem(new Localization());
#ifdef DV_NETWORK
    DV_CONTEXT.RegisterSubsystem(new Network());
#endif
#ifdef DV_DATABASE
    DV_CONTEXT.RegisterSubsystem(new Database());
#endif
    DV_CONTEXT.RegisterSubsystem(new Input());
    DV_CONTEXT.RegisterSubsystem(new Audio());
    DV_CONTEXT.RegisterSubsystem(new UI());

    // Register object factories for libraries which are not automatically registered along with subsystem creation
    RegisterSceneLibrary();

#ifdef DV_BULLET
    RegisterPhysicsLibrary();
#endif

#ifdef DV_BOX2D
    RegisterPhysics2DLibrary();
#endif

#ifdef DV_NAVIGATION
    RegisterNavigationLibrary();
#endif

    SubscribeToEvent(E_EXITREQUESTED, DV_HANDLER(Engine, HandleExitRequested));
}

Engine::~Engine() = default;

bool Engine::Initialize(const VariantMap& parameters)
{
    if (initialized_)
        return true;

    DV_PROFILE(InitEngine);

    // Set headless mode
    headless_ = GetParameter(parameters, EP_HEADLESS, false).GetBool();

    // Detect GAPI even in headless mode
    // https://github.com/urho3d/Urho3D/issues/3040
    GAPI gapi = GAPI_NONE;

    // Try to set any possible graphics API as default

#ifdef DV_OPENGL
    gapi = GAPI_OPENGL;
#endif

#ifdef DV_D3D11
    gapi = GAPI_D3D11;
#endif

    // Use command line parameters

#ifdef DV_OPENGL
    bool gapi_gl = GetParameter(parameters, EP_OPENGL, false).GetBool();
    if (gapi_gl)
        gapi = GAPI_OPENGL;
#endif

#ifdef DV_D3D11
    bool gapi_d3d11 = GetParameter(parameters, EP_DIRECT3D11, false).GetBool();
    if (gapi_d3d11)
        gapi = GAPI_D3D11;
#endif

    if (gapi == GAPI_NONE)
    {
        DV_LOGERROR("Graphics API not selected");
        return false;
    }

    // Register the rest of the subsystems
    if (!headless_)
    {
        DV_CONTEXT.RegisterSubsystem(new Graphics(gapi));
        DV_CONTEXT.RegisterSubsystem(new Renderer());
    }
    else
    {
        Graphics::SetGAPI(gapi); // https://github.com/urho3d/Urho3D/issues/3040

        // Register graphics library objects explicitly in headless mode to allow them to work without using actual GPU resources
        RegisterGraphicsLibrary();
    }

#ifdef DV_URHO2D
    // 2D graphics library is dependent on 3D graphics library
    RegisterUrho2DLibrary();
#endif

    // Начинаем запись лога в файл
    if (HasParameter(parameters, EP_LOG_LEVEL))
        Log::get_instance().SetLevel(GetParameter(parameters, EP_LOG_LEVEL).GetI32());
    Log::get_instance().SetQuiet(GetParameter(parameters, EP_LOG_QUIET, false).GetBool());
    Log::get_instance().Open(GetParameter(parameters, EP_LOG_NAME, "dviglo.log").GetString());

    // Set maximally accurate low res timer
    DV_TIME.SetTimerPeriod(1);

    // Configure max FPS
    if (GetParameter(parameters, EP_FRAME_LIMITER, true) == false)
        SetMaxFps(0);

    // Set amount of worker threads according to the available physical CPU cores. Using also hyperthreaded cores results in
    // unpredictable extra synchronization overhead. Also reserve one core for the main thread
#ifdef DV_THREADING
    unsigned numThreads = GetParameter(parameters, EP_WORKER_THREADS, true).GetBool() ? GetNumPhysicalCPUs() - 1 : 0;
    if (numThreads)
    {
        GetSubsystem<WorkQueue>()->CreateThreads(numThreads);

        DV_LOGINFOF("Created %u worker thread%s", numThreads, numThreads > 1 ? "s" : "");
    }
#endif

    // Add resource paths
    if (!InitializeResourceCache(parameters, false))
        return false;

    auto* cache = GetSubsystem<ResourceCache>();
    auto* fileSystem = GetSubsystem<FileSystem>();

    // Initialize graphics & audio output
    if (!headless_)
    {
        auto* graphics = GetSubsystem<Graphics>();
        auto* renderer = GetSubsystem<Renderer>();

        if (HasParameter(parameters, EP_EXTERNAL_WINDOW))
            graphics->SetExternalWindow(GetParameter(parameters, EP_EXTERNAL_WINDOW).GetVoidPtr());
        graphics->SetWindowTitle(GetParameter(parameters, EP_WINDOW_TITLE, "Urho3D").GetString());
        graphics->SetWindowIcon(cache->GetResource<Image>(GetParameter(parameters, EP_WINDOW_ICON, String::EMPTY).GetString()));
        graphics->SetFlushGPU(GetParameter(parameters, EP_FLUSH_GPU, false).GetBool());
        graphics->SetOrientations(GetParameter(parameters, EP_ORIENTATIONS, "LandscapeLeft LandscapeRight").GetString());

        if (HasParameter(parameters, EP_WINDOW_POSITION_X) && HasParameter(parameters, EP_WINDOW_POSITION_Y))
            graphics->SetWindowPosition(GetParameter(parameters, EP_WINDOW_POSITION_X).GetI32(),
                GetParameter(parameters, EP_WINDOW_POSITION_Y).GetI32());

        if (Graphics::GetGAPI() == GAPI_OPENGL)
        {
            if (HasParameter(parameters, EP_FORCE_GL2))
                graphics->SetForceGL2(GetParameter(parameters, EP_FORCE_GL2).GetBool());
        }

        if (!graphics->SetMode(
            GetParameter(parameters, EP_WINDOW_WIDTH, 0).GetI32(),
            GetParameter(parameters, EP_WINDOW_HEIGHT, 0).GetI32(),
            GetParameter(parameters, EP_FULL_SCREEN, true).GetBool(),
            GetParameter(parameters, EP_BORDERLESS, false).GetBool(),
            GetParameter(parameters, EP_WINDOW_RESIZABLE, false).GetBool(),
            GetParameter(parameters, EP_HIGH_DPI, true).GetBool(),
            GetParameter(parameters, EP_VSYNC, false).GetBool(),
            GetParameter(parameters, EP_TRIPLE_BUFFER, false).GetBool(),
            GetParameter(parameters, EP_MULTI_SAMPLE, 1).GetI32(),
            GetParameter(parameters, EP_MONITOR, 0).GetI32(),
            GetParameter(parameters, EP_REFRESH_RATE, 0).GetI32()
        ))
            return false;

        graphics->SetShaderCacheDir(GetParameter(parameters, EP_SHADER_CACHE_DIR, get_pref_path("urho3d", "shadercache")).GetString());

        if (HasParameter(parameters, EP_DUMP_SHADERS))
            graphics->BeginDumpShaders(GetParameter(parameters, EP_DUMP_SHADERS, String::EMPTY).GetString());
        if (HasParameter(parameters, EP_RENDER_PATH))
            renderer->SetDefaultRenderPath(cache->GetResource<XMLFile>(GetParameter(parameters, EP_RENDER_PATH).GetString()));

        renderer->SetDrawShadows(GetParameter(parameters, EP_SHADOWS, true).GetBool());
        if (renderer->GetDrawShadows() && GetParameter(parameters, EP_LOW_QUALITY_SHADOWS, false).GetBool())
            renderer->SetShadowQuality(SHADOWQUALITY_SIMPLE_16BIT);
        renderer->SetMaterialQuality((MaterialQuality)GetParameter(parameters, EP_MATERIAL_QUALITY, QUALITY_HIGH).GetI32());
        renderer->SetTextureQuality((MaterialQuality)GetParameter(parameters, EP_TEXTURE_QUALITY, QUALITY_HIGH).GetI32());
        renderer->SetTextureFilterMode((TextureFilterMode)GetParameter(parameters, EP_TEXTURE_FILTER_MODE, FILTER_TRILINEAR).GetI32());
        renderer->SetTextureAnisotropy(GetParameter(parameters, EP_TEXTURE_ANISOTROPY, 4).GetI32());

        if (GetParameter(parameters, EP_SOUND, true).GetBool())
        {
            GetSubsystem<Audio>()->SetMode(
                GetParameter(parameters, EP_SOUND_BUFFER, 100).GetI32(),
                GetParameter(parameters, EP_SOUND_MIX_RATE, 44100).GetI32(),
                GetParameter(parameters, EP_SOUND_STEREO, true).GetBool(),
                GetParameter(parameters, EP_SOUND_INTERPOLATION, true).GetBool()
            );
        }
    }

    // Init FPU state of main thread
    InitFPU();

    // Initialize input
    if (HasParameter(parameters, EP_TOUCH_EMULATION))
        GetSubsystem<Input>()->SetTouchEmulation(GetParameter(parameters, EP_TOUCH_EMULATION).GetBool());

    // Initialize network
#ifdef DV_NETWORK
    if (HasParameter(parameters, EP_PACKAGE_CACHE_DIR))
        GetSubsystem<Network>()->SetPackageCacheDir(GetParameter(parameters, EP_PACKAGE_CACHE_DIR).GetString());
#endif

#ifdef DV_TESTING
    if (HasParameter(parameters, EP_TIME_OUT))
        timeOut_ = GetParameter(parameters, EP_TIME_OUT, 0).GetI32() * 1000000LL;
#endif

#ifdef DV_PROFILING
    if (GetParameter(parameters, EP_EVENT_PROFILER, true).GetBool())
    {
        DV_CONTEXT.RegisterSubsystem(new EventProfiler());
        EventProfiler::SetActive(true);
    }
#endif
    frameTimer_.Reset();

    DV_LOGINFO("Initialized engine");
    initialized_ = true;
    return true;
}

bool Engine::InitializeResourceCache(const VariantMap& parameters, bool removeOld /*= true*/)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* fileSystem = GetSubsystem<FileSystem>();

    // Remove all resource paths and packages
    if (removeOld)
    {
        Vector<String> resourceDirs = cache->GetResourceDirs();
        Vector<SharedPtr<PackageFile>> packageFiles = cache->GetPackageFiles();
        for (unsigned i = 0; i < resourceDirs.Size(); ++i)
            cache->RemoveResourceDir(resourceDirs[i]);
        for (unsigned i = 0; i < packageFiles.Size(); ++i)
            cache->RemovePackageFile(packageFiles[i]);
    }

    // Add resource paths
    Vector<String> resourcePrefixPaths = GetParameter(parameters, EP_RESOURCE_PREFIX_PATHS, String::EMPTY).GetString().Split(';', true);
    for (unsigned i = 0; i < resourcePrefixPaths.Size(); ++i)
        resourcePrefixPaths[i] = AddTrailingSlash(
            IsAbsolutePath(resourcePrefixPaths[i]) ? resourcePrefixPaths[i] : fileSystem->GetProgramDir() + resourcePrefixPaths[i]);
    Vector<String> resourcePaths = GetParameter(parameters, EP_RESOURCE_PATHS, "Data;CoreData").GetString().Split(';');
    Vector<String> resourcePackages = GetParameter(parameters, EP_RESOURCE_PACKAGES).GetString().Split(';');
    Vector<String> autoLoadPaths = GetParameter(parameters, EP_AUTOLOAD_PATHS, "Autoload").GetString().Split(';');

    for (unsigned i = 0; i < resourcePaths.Size(); ++i)
    {
        // If path is not absolute, prefer to add it as a package if possible
        if (!IsAbsolutePath(resourcePaths[i]))
        {
            unsigned j = 0;
            for (; j < resourcePrefixPaths.Size(); ++j)
            {
                String packageName = resourcePrefixPaths[j] + resourcePaths[i] + ".pak";
                if (fileSystem->FileExists(packageName))
                {
                    if (cache->AddPackageFile(packageName))
                        break;
                    else
                        return false;   // The root cause of the error should have already been logged
                }
                String pathName = resourcePrefixPaths[j] + resourcePaths[i];
                if (dir_exists(pathName))
                {
                    if (cache->AddResourceDir(pathName))
                        break;
                    else
                        return false;
                }
            }
            if (j == resourcePrefixPaths.Size())
            {
                DV_LOGERRORF(
                    "Failed to add resource path '%s', check the documentation on how to set the 'resource prefix path'",
                    resourcePaths[i].c_str());
                return false;
            }
        }
        else
        {
            String pathName = resourcePaths[i];
            if (dir_exists(pathName))
                if (!cache->AddResourceDir(pathName))
                    return false;
        }
    }

    // Then add specified packages
    for (unsigned i = 0; i < resourcePackages.Size(); ++i)
    {
        unsigned j = 0;
        for (; j < resourcePrefixPaths.Size(); ++j)
        {
            String packageName = resourcePrefixPaths[j] + resourcePackages[i];
            if (fileSystem->FileExists(packageName))
            {
                if (cache->AddPackageFile(packageName))
                    break;
                else
                    return false;
            }
        }
        if (j == resourcePrefixPaths.Size())
        {
            DV_LOGERRORF(
                "Failed to add resource package '%s', check the documentation on how to set the 'resource prefix path'",
                resourcePackages[i].c_str());
            return false;
        }
    }

    // Add auto load folders. Prioritize these (if exist) before the default folders
    for (unsigned i = 0; i < autoLoadPaths.Size(); ++i)
    {
        bool autoLoadPathExist = false;

        for (unsigned j = 0; j < resourcePrefixPaths.Size(); ++j)
        {
            String autoLoadPath(autoLoadPaths[i]);
            if (!IsAbsolutePath(autoLoadPath))
                autoLoadPath = resourcePrefixPaths[j] + autoLoadPath;

            if (dir_exists(autoLoadPath))
            {
                autoLoadPathExist = true;

                // Add all the subdirs (non-recursive) as resource directory
                Vector<String> subdirs;
                fileSystem->ScanDir(subdirs, autoLoadPath, "*", SCAN_DIRS, false);
                for (unsigned y = 0; y < subdirs.Size(); ++y)
                {
                    String dir = subdirs[y];
                    if (dir.StartsWith("."))
                        continue;

                    String autoResourceDir = autoLoadPath + "/" + dir;
                    if (!cache->AddResourceDir(autoResourceDir, 0))
                        return false;
                }

                // Add all the found package files (non-recursive)
                Vector<String> paks;
                fileSystem->ScanDir(paks, autoLoadPath, "*.pak", SCAN_FILES, false);
                for (unsigned y = 0; y < paks.Size(); ++y)
                {
                    String pak = paks[y];
                    if (pak.StartsWith("."))
                        continue;

                    String autoPackageName = autoLoadPath + "/" + pak;
                    if (!cache->AddPackageFile(autoPackageName, 0))
                        return false;
                }
            }
        }

        // The following debug message is confusing when user is not aware of the autoload feature
        // Especially because the autoload feature is enabled by default without user intervention
        // The following extra conditional check below is to suppress unnecessary debug log entry under such default situation
        // The cleaner approach is to not enable the autoload by default, i.e. do not use 'Autoload' as default value for 'AutoloadPaths' engine parameter
        // However, doing so will break the existing applications that rely on this
        if (!autoLoadPathExist && (autoLoadPaths.Size() > 1 || autoLoadPaths[0] != "Autoload"))
            DV_LOGDEBUGF(
                "Skipped autoload path '%s' as it does not exist, check the documentation on how to set the 'resource prefix path'",
                autoLoadPaths[i].c_str());
    }

    return true;
}

void Engine::RunFrame()
{
    assert(initialized_);

    // If not headless, and the graphics subsystem no longer has a window open, assume we should exit
    if (!headless_ && !GetSubsystem<Graphics>()->IsInitialized())
        exiting_ = true;

    if (exiting_)
        return;

    // Note: there is a minimal performance cost to looking up subsystems (uses a hashmap); if they would be looked up several
    // times per frame it would be better to cache the pointers
    Time& time = DV_TIME;
    auto* input = GetSubsystem<Input>();
    auto* audio = GetSubsystem<Audio>();

#ifdef DV_PROFILING
    if (EventProfiler::IsActive())
    {
        auto* eventProfiler = GetSubsystem<EventProfiler>();
        if (eventProfiler)
            eventProfiler->BeginFrame();
    }
#endif

    time.BeginFrame(timeStep_);

    // If pause when minimized -mode is in use, stop updates and audio as necessary
    if (pauseMinimized_ && input->IsMinimized())
    {
        if (audio->IsPlaying())
        {
            audio->Stop();
            audioPaused_ = true;
        }
    }
    else
    {
        // Only unpause when it was paused by the engine
        if (audioPaused_)
        {
            audio->Play();
            audioPaused_ = false;
        }

        Update();
    }

    Render();
    ApplyFrameLimit();

    time.EndFrame();

    // Mark a frame for profiling
    DV_PROFILE_FRAME();
}

Console* Engine::CreateConsole()
{
    if (headless_ || !initialized_)
        return nullptr;

    // Return existing console if possible
    auto* console = GetSubsystem<Console>();
    if (!console)
    {
        console = new Console();
        DV_CONTEXT.RegisterSubsystem(console);
    }

    return console;
}

DebugHud* Engine::CreateDebugHud()
{
    if (headless_ || !initialized_)
        return nullptr;

    // Return existing debug HUD if possible
    auto* debugHud = GetSubsystem<DebugHud>();
    if (!debugHud)
    {
        debugHud = new DebugHud();
        DV_CONTEXT.RegisterSubsystem(debugHud);
    }

    return debugHud;
}

void Engine::SetTimeStepSmoothing(int frames)
{
    timeStepSmoothing_ = (unsigned)Clamp(frames, 1, 20);
}

void Engine::SetMinFps(int fps)
{
    minFps_ = (unsigned)Max(fps, 0);
}

void Engine::SetMaxFps(int fps)
{
    maxFps_ = (unsigned)Max(fps, 0);
}

void Engine::SetMaxInactiveFps(int fps)
{
    maxInactiveFps_ = (unsigned)Max(fps, 0);
}

void Engine::SetPauseMinimized(bool enable)
{
    pauseMinimized_ = enable;
}

void Engine::SetAutoExit(bool enable)
{
    // On mobile platforms exit is mandatory if requested by the platform itself and should not be attempted to be disabled
#if defined(__ANDROID__) || defined(IOS) || defined(TVOS)
    enable = true;
#endif
    autoExit_ = enable;
}

void Engine::SetNextTimeStep(float seconds)
{
    timeStep_ = Max(seconds, 0.0f);
}

void Engine::Exit()
{
#if defined(IOS) || defined(TVOS)
    // On iOS/tvOS it's not legal for the application to exit on its own, instead it will be minimized with the home key
#else
    DoExit();
#endif
}

void Engine::DumpProfiler()
{
#ifdef DV_LOGGING
    if (!Thread::IsMainThread())
        return;

    auto* profiler = GetSubsystem<Profiler>();
    if (profiler)
        DV_LOGRAW(profiler->PrintData(true, true) + "\n");
#endif
}

void Engine::DumpResources(bool dumpFileName)
{
#ifdef DV_LOGGING
    if (!Thread::IsMainThread())
        return;

    auto* cache = GetSubsystem<ResourceCache>();
    const HashMap<StringHash, ResourceGroup>& resourceGroups = cache->GetAllResources();
    if (dumpFileName)
    {
        DV_LOGRAW("Used resources:\n");
        for (HashMap<StringHash, ResourceGroup>::ConstIterator i = resourceGroups.Begin(); i != resourceGroups.End(); ++i)
        {
            const HashMap<StringHash, SharedPtr<Resource>>& resources = i->second_.resources_;
            if (dumpFileName)
            {
                for (HashMap<StringHash, SharedPtr<Resource>>::ConstIterator j = resources.Begin(); j != resources.End(); ++j)
                    DV_LOGRAW(j->second_->GetName() + "\n");
            }
        }
    }
    else
        DV_LOGRAW(cache->PrintMemoryUsage() + "\n");
#endif
}

void Engine::DumpMemory()
{
#ifdef DV_LOGGING
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState state;
    _CrtMemCheckpoint(&state);
    _CrtMemBlockHeader* block = state.pBlockHeader;
    unsigned total = 0;
    unsigned blocks = 0;

    for (;;)
    {
        if (block && block->pBlockHeaderNext)
            block = block->pBlockHeaderNext;
        else
            break;
    }

    while (block)
    {
        if (block->nBlockUse > 0)
        {
            if (block->szFileName)
                DV_LOGRAW("Block " + String((int)block->lRequest) + ": " + String(block->nDataSize) + " bytes, file " + String(block->szFileName) + " line " + String(block->nLine) + "\n");
            else
                DV_LOGRAW("Block " + String((int)block->lRequest) + ": " + String(block->nDataSize) + " bytes\n");

            total += block->nDataSize;
            ++blocks;
        }
        block = block->pBlockHeaderPrev;
    }

    DV_LOGRAW("Total allocated memory " + String(total) + " bytes in " + String(blocks) + " blocks\n\n");
#else
    DV_LOGRAW("DumpMemory() supported on MSVC debug mode only\n\n");
#endif
#endif
}

void Engine::Update()
{
    DV_PROFILE(Update);

    // Logic update event
    using namespace Update;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_TIMESTEP] = timeStep_;
    SendEvent(E_UPDATE, eventData);

    // Logic post-update event
    SendEvent(E_POSTUPDATE, eventData);

    // Rendering update event
    SendEvent(E_RENDERUPDATE, eventData);

    // Post-render update event
    SendEvent(E_POSTRENDERUPDATE, eventData);
}

void Engine::Render()
{
    if (headless_)
        return;

    DV_PROFILE(Render);

    // If device is lost, BeginFrame will fail and we skip rendering
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics->BeginFrame())
        return;

    GetSubsystem<Renderer>()->Render();
    GetSubsystem<UI>()->Render();
    graphics->EndFrame();
}

void Engine::ApplyFrameLimit()
{
    if (!initialized_)
        return;

    unsigned maxFps = maxFps_;
    auto* input = GetSubsystem<Input>();
    if (input && !input->HasFocus())
        maxFps = Min(maxInactiveFps_, maxFps);

    long long elapsed = 0;

#ifndef __EMSCRIPTEN__
    // Perform waiting loop if maximum FPS set
#if !defined(IOS) && !defined(TVOS)
    if (maxFps)
#else
    // If on iOS/tvOS and target framerate is 60 or above, just let the animation callback handle frame timing
    // instead of waiting ourselves
    if (maxFps < 60)
#endif
    {
        DV_PROFILE(ApplyFrameLimit);

        long long targetMax = 1000000LL / maxFps;

        for (;;)
        {
            elapsed = frameTimer_.GetUSec(false);
            if (elapsed >= targetMax)
                break;

            // Sleep if 1 ms or more off the frame limiting goal
            if (targetMax - elapsed >= 1000LL)
            {
                auto sleepTime = (unsigned)((targetMax - elapsed) / 1000LL);
                Time::Sleep(sleepTime);
            }
        }
    }
#endif

    elapsed = frameTimer_.GetUSec(true);
#ifdef DV_TESTING
    if (timeOut_ > 0)
    {
        timeOut_ -= elapsed;
        if (timeOut_ <= 0)
            Exit();
    }
#endif

    // If FPS lower than minimum, clamp elapsed time
    if (minFps_)
    {
        long long targetMin = 1000000LL / minFps_;
        if (elapsed > targetMin)
            elapsed = targetMin;
    }

    // Perform timestep smoothing
    timeStep_ = 0.0f;
    lastTimeSteps_.Push(elapsed / 1000000.0f);
    if (lastTimeSteps_.Size() > timeStepSmoothing_)
    {
        // If the smoothing configuration was changed, ensure correct amount of samples
        lastTimeSteps_.Erase(0, lastTimeSteps_.Size() - timeStepSmoothing_);
        for (unsigned i = 0; i < lastTimeSteps_.Size(); ++i)
            timeStep_ += lastTimeSteps_[i];
        timeStep_ /= lastTimeSteps_.Size();
    }
    else
        timeStep_ = lastTimeSteps_.Back();
}

VariantMap Engine::ParseParameters(const Vector<String>& arguments)
{
    VariantMap ret;

    // Pre-initialize the parameters with environment variable values when they are set
    if (const char* paths = getenv("DV_PREFIX_PATH"))
        ret[EP_RESOURCE_PREFIX_PATHS] = paths;

    for (unsigned i = 0; i < arguments.Size(); ++i)
    {
        if (arguments[i].Length() > 1 && arguments[i][0] == '-')
        {
            String argument = arguments[i].Substring(1).ToLower();
            String value = i + 1 < arguments.Size() ? arguments[i + 1] : String::EMPTY;

            if (argument == "headless")
                ret[EP_HEADLESS] = true;
            else if (argument == "nolimit")
                ret[EP_FRAME_LIMITER] = false;
            else if (argument == "flushgpu")
                ret[EP_FLUSH_GPU] = true;
            else if (argument == "opengl")
                ret[EP_OPENGL] = true;
            else if (argument == "d3d11")
                ret[EP_DIRECT3D11] = true;
            else if (argument == "gl2")
                ret[EP_FORCE_GL2] = true;
            else if (argument == "landscape")
                ret[EP_ORIENTATIONS] = "LandscapeLeft LandscapeRight " + ret[EP_ORIENTATIONS].GetString();
            else if (argument == "portrait")
                ret[EP_ORIENTATIONS] = "Portrait PortraitUpsideDown " + ret[EP_ORIENTATIONS].GetString();
            else if (argument == "nosound")
                ret[EP_SOUND] = false;
            else if (argument == "noip")
                ret[EP_SOUND_INTERPOLATION] = false;
            else if (argument == "mono")
                ret[EP_SOUND_STEREO] = false;
            else if (argument == "prepass")
                ret[EP_RENDER_PATH] = "RenderPaths/Prepass.xml";
            else if (argument == "deferred")
                ret[EP_RENDER_PATH] = "RenderPaths/Deferred.xml";
            else if (argument == "renderpath" && !value.Empty())
            {
                ret[EP_RENDER_PATH] = value;
                ++i;
            }
            else if (argument == "noshadows")
                ret[EP_SHADOWS] = false;
            else if (argument == "lqshadows")
                ret[EP_LOW_QUALITY_SHADOWS] = true;
            else if (argument == "nothreads")
                ret[EP_WORKER_THREADS] = false;
            else if (argument == "v")
                ret[EP_VSYNC] = true;
            else if (argument == "t")
                ret[EP_TRIPLE_BUFFER] = true;
            else if (argument == "w")
                ret[EP_FULL_SCREEN] = false;
            else if (argument == "borderless")
                ret[EP_BORDERLESS] = true;
            else if (argument == "lowdpi")
                ret[EP_HIGH_DPI] = false;
            else if (argument == "s")
                ret[EP_WINDOW_RESIZABLE] = true;
            else if (argument == "q")
                ret[EP_LOG_QUIET] = true;
            else if (argument == "log" && !value.Empty())
            {
                i32 logLevel = GetStringListIndex(value.c_str(), logLevelPrefixes, NINDEX);
                if (logLevel != NINDEX)
                {
                    ret[EP_LOG_LEVEL] = logLevel;
                    ++i;
                }
            }
            else if (argument == "x" && !value.Empty())
            {
                ret[EP_WINDOW_WIDTH] = ToI32(value);
                ++i;
            }
            else if (argument == "y" && !value.Empty())
            {
                ret[EP_WINDOW_HEIGHT] = ToI32(value);
                ++i;
            }
            else if (argument == "monitor" && !value.Empty()) {
                ret[EP_MONITOR] = ToI32(value);
                ++i;
            }
            else if (argument == "hz" && !value.Empty()) {
                ret[EP_REFRESH_RATE] = ToI32(value);
                ++i;
            }
            else if (argument == "m" && !value.Empty())
            {
                ret[EP_MULTI_SAMPLE] = ToI32(value);
                ++i;
            }
            else if (argument == "b" && !value.Empty())
            {
                ret[EP_SOUND_BUFFER] = ToI32(value);
                ++i;
            }
            else if (argument == "r" && !value.Empty())
            {
                ret[EP_SOUND_MIX_RATE] = ToI32(value);
                ++i;
            }
            else if (argument == "pp" && !value.Empty())
            {
                ret[EP_RESOURCE_PREFIX_PATHS] = value;
                ++i;
            }
            else if (argument == "p" && !value.Empty())
            {
                ret[EP_RESOURCE_PATHS] = value;
                ++i;
            }
            else if (argument == "pf" && !value.Empty())
            {
                ret[EP_RESOURCE_PACKAGES] = value;
                ++i;
            }
            else if (argument == "ap" && !value.Empty())
            {
                ret[EP_AUTOLOAD_PATHS] = value;
                ++i;
            }
            else if (argument == "ds" && !value.Empty())
            {
                ret[EP_DUMP_SHADERS] = value;
                ++i;
            }
            else if (argument == "mq" && !value.Empty())
            {
                ret[EP_MATERIAL_QUALITY] = ToI32(value);
                ++i;
            }
            else if (argument == "tq" && !value.Empty())
            {
                ret[EP_TEXTURE_QUALITY] = ToI32(value);
                ++i;
            }
            else if (argument == "tf" && !value.Empty())
            {
                ret[EP_TEXTURE_FILTER_MODE] = ToI32(value);
                ++i;
            }
            else if (argument == "af" && !value.Empty())
            {
                ret[EP_TEXTURE_FILTER_MODE] = FILTER_ANISOTROPIC;
                ret[EP_TEXTURE_ANISOTROPY] = ToI32(value);
                ++i;
            }
            else if (argument == "touch")
                ret[EP_TOUCH_EMULATION] = true;
#ifdef DV_TESTING
            else if (argument == "timeout" && !value.Empty())
            {
                ret[EP_TIME_OUT] = ToI32(value);
                ++i;
            }
#endif
        }
    }

    return ret;
}

bool Engine::HasParameter(const VariantMap& parameters, const String& parameter)
{
    StringHash nameHash(parameter);
    return parameters.Find(nameHash) != parameters.End();
}

const Variant& Engine::GetParameter(const VariantMap& parameters, const String& parameter, const Variant& defaultValue)
{
    StringHash nameHash(parameter);
    VariantMap::ConstIterator i = parameters.Find(nameHash);
    return i != parameters.End() ? i->second_ : defaultValue;
}

void Engine::HandleExitRequested(StringHash eventType, VariantMap& eventData)
{
    if (autoExit_)
    {
        // Do not call Exit() here, as it contains mobile platform -specific tests to not exit.
        // If we do receive an exit request from the system on those platforms, we must comply
        DoExit();
    }
}

void Engine::DoExit()
{
    auto* graphics = GetSubsystem<Graphics>();
    if (graphics)
        graphics->Close();

    exiting_ = true;
#if defined(__EMSCRIPTEN__) && defined(DV_TESTING)
    emscripten_force_exit(EXIT_SUCCESS);    // Some how this is required to signal emrun to stop
#endif
}

}
