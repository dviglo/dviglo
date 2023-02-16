// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "app_state_manager.h"
#include "appstate_benchmark03.h"
#include "benchmark03_molecule_logic.h"

#include <dviglo/core/context.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene_events.h>

#include <dviglo/common/debug_new.h>

using namespace dviglo;

AppState_Benchmark03::AppState_Benchmark03(Context* context)
    : AppState_Base(context)
{
    name_ = "Molecules";

    // This constructor is called once when the application runs, so we can register here
    context->RegisterFactory<Benchmark03_MoleculeLogic>();

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    SharedPtr<Material> material0 = MakeShared<Material>(context_);
    material0->SetName("Molecule0");
    material0->SetShaderParameter("MatDiffColor", Vector4(0, 1, 1, 1));
    material0->SetShaderParameter("MatSpecColor", Vector4(1, 1, 1, 100));
    cache->AddManualResource(material0);

    SharedPtr<Material> material1 = material0->Clone("Molecule1");
    material1->SetShaderParameter("MatDiffColor", Vector4(0, 1, 0, 1));
    cache->AddManualResource(material1);

    SharedPtr<Material> material2 = material0->Clone("Molecule2");
    material2->SetShaderParameter("MatDiffColor", Vector4(0, 0, 1, 1));
    cache->AddManualResource(material2);

    SharedPtr<Material> material3 = material0->Clone("Molecule3");
    material3->SetShaderParameter("MatDiffColor", Vector4(1, 0.5f, 0, 1));
    cache->AddManualResource(material3);

    SharedPtr<Material> material4 = material0->Clone("Molecule4");
    material4->SetShaderParameter("MatDiffColor", Vector4(1, 0, 0, 1));
    cache->AddManualResource(material4);

    SharedPtr<Material> material5 = material0->Clone("Molecule5");
    material5->SetShaderParameter("MatDiffColor", Vector4(1, 0, 1, 1));
    cache->AddManualResource(material5);

    SharedPtr<Material> material6 = material0->Clone("Molecule6");
    material6->SetShaderParameter("MatDiffColor", Vector4(1, 1, 0, 1));
    cache->AddManualResource(material6);
}

void AppState_Benchmark03::OnEnter()
{
    assert(!scene_);
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();

    Node* zoneNode = scene_->CreateChild();
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.f, 1000.f));
    zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
    zone->SetFogColor(Color(0.3f, 0.6f, 0.9f));
    zone->SetFogStart(10000.f);
    zone->SetFogEnd(10000.f);

    Node* lightNode = scene_->CreateChild();
    lightNode->SetRotation(Quaternion(45.f, 45.f, 0.f));
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);

    Node* cameraNode = scene_->CreateChild("Camera");
    cameraNode->SetPosition(Vector3(0.f, 0.f, -10.f));
    Camera* camera = cameraNode->CreateComponent<Camera>();
    camera->SetOrthographic(true);

    constexpr int NUM_MOLECULES = 280;

    for (int i = 0; i < NUM_MOLECULES; ++i)
        CreateMolecule({Random(-10.f, 10.f), Random(-6.f, 6.f)}, Random(7));

    GetSubsystem<Input>()->SetMouseVisible(false);
    SetupViewport();
    SubscribeToEvent(scene_, E_SCENEUPDATE, DV_HANDLER(AppState_Benchmark03, HandleSceneUpdate));
    fpsCounter_.Clear();
}

void AppState_Benchmark03::OnLeave()
{
    UnsubscribeFromAllEvents();
    DestroyViewport();
    scene_ = nullptr;
}

void AppState_Benchmark03::CreateMolecule(const Vector2& pos, i32 type)
{
    assert(type >= 0 && type <= 6);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    Node* node = scene_->CreateChild();
    node->SetPosition2D(pos);
    
    StaticModel* obj = node->CreateComponent<StaticModel>();
    obj->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
    obj->SetMaterial(cache->GetResource<Material>("Molecule" + String(type)));

    Benchmark03_MoleculeLogic* logic = node->CreateComponent<Benchmark03_MoleculeLogic>();
    logic->SetParameters(type);
}

void AppState_Benchmark03::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    fpsCounter_.Update(timeStep);
    UpdateCurrentFpsElement();

    if (GetSubsystem<Input>()->GetKeyDown(KEY_ESCAPE))
    {
        GetSubsystem<AppStateManager>()->SetRequiredAppStateId(APPSTATEID_MAINSCREEN);
        return;
    }

    if (fpsCounter_.GetTotalTime() >= 30.f)
        GetSubsystem<AppStateManager>()->SetRequiredAppStateId(APPSTATEID_RESULTSCREEN);
}
