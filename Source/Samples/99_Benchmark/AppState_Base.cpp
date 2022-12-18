// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#include "AppState_Base.h"

#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include <dviglo/debug_new.h>

using namespace Urho3D;

void AppState_Base::LoadSceneXml(const String& path)
{
    assert(!scene_);
    scene_ = MakeShared<Scene>(context_);
    SharedPtr<File> file = GetSubsystem<ResourceCache>()->GetFile(path);
    scene_->LoadXML(*file);

#ifndef NDEBUG
    Node* cameraNode = scene_->GetChild("Camera");
    assert(cameraNode);
    Camera* camera = cameraNode->GetComponent<Camera>();
    assert(camera);
#endif
}

void AppState_Base::UpdateCurrentFpsElement()
{
    String fpsStr = fpsCounter_.GetCurrentFps() == -1 ? "?" : String(fpsCounter_.GetCurrentFps());
    Text* fpsElement = GetSubsystem<UI>()->GetRoot()->GetChildStaticCast<Text>(CURRENT_FPS_STR);
    fpsElement->SetText("FPS: " + fpsStr);
}

void AppState_Base::SetupViewport()
{
    Node* cameraNode = scene_->GetChild("Camera");
    Camera* camera = cameraNode->GetComponent<Camera>();
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, camera));

    Renderer* renderer = GetSubsystem<Renderer>();
    renderer->SetViewport(0, viewport);
}

void AppState_Base::DestroyViewport()
{
    GetSubsystem<Renderer>()->SetViewport(0, nullptr);
}
