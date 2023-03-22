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

AppState_Benchmark03::AppState_Benchmark03()
{
    name_ = "Molecules";

    // This constructor is called once when the application runs, so we can register here
    DV_CONTEXT.RegisterFactory<Benchmark03_MoleculeLogic>();

    SharedPtr<Material> material0 = MakeShared<Material>();
    material0->SetName("Molecule0");
    material0->SetShaderParameter("MatDiffColor", Vector4(0, 1, 1, 1));
    material0->SetShaderParameter("MatSpecColor", Vector4(1, 1, 1, 100));
    DV_RES_CACHE.AddManualResource(material0);

    SharedPtr<Material> material1 = material0->Clone("Molecule1");
    material1->SetShaderParameter("MatDiffColor", Vector4(0, 1, 0, 1));
    DV_RES_CACHE.AddManualResource(material1);

    SharedPtr<Material> material2 = material0->Clone("Molecule2");
    material2->SetShaderParameter("MatDiffColor", Vector4(0, 0, 1, 1));
    DV_RES_CACHE.AddManualResource(material2);

    SharedPtr<Material> material3 = material0->Clone("Molecule3");
    material3->SetShaderParameter("MatDiffColor", Vector4(1, 0.5f, 0, 1));
    DV_RES_CACHE.AddManualResource(material3);

    SharedPtr<Material> material4 = material0->Clone("Molecule4");
    material4->SetShaderParameter("MatDiffColor", Vector4(1, 0, 0, 1));
    DV_RES_CACHE.AddManualResource(material4);

    SharedPtr<Material> material5 = material0->Clone("Molecule5");
    material5->SetShaderParameter("MatDiffColor", Vector4(1, 0, 1, 1));
    DV_RES_CACHE.AddManualResource(material5);

    SharedPtr<Material> material6 = material0->Clone("Molecule6");
    material6->SetShaderParameter("MatDiffColor", Vector4(1, 1, 0, 1));
    DV_RES_CACHE.AddManualResource(material6);
}

void AppState_Benchmark03::OnEnter()
{
    assert(!scene_);
    scene_ = new Scene();
    scene_->create_component<Octree>();

    Node* zoneNode = scene_->create_child();
    Zone* zone = zoneNode->create_component<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.f, 1000.f));
    zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
    zone->SetFogColor(Color(0.3f, 0.6f, 0.9f));
    zone->SetFogStart(10000.f);
    zone->SetFogEnd(10000.f);

    Node* lightNode = scene_->create_child();
    lightNode->SetRotation(Quaternion(45.f, 45.f, 0.f));
    Light* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);

    Node* cameraNode = scene_->create_child("Camera");
    cameraNode->SetPosition(Vector3(0.f, 0.f, -10.f));
    Camera* camera = cameraNode->create_component<Camera>();
    camera->SetOrthographic(true);

    constexpr int NUM_MOLECULES = 280;

    for (int i = 0; i < NUM_MOLECULES; ++i)
        CreateMolecule({Random(-10.f, 10.f), Random(-6.f, 6.f)}, Random(7));

    DV_INPUT.SetMouseVisible(false);
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

    Node* node = scene_->create_child();
    node->SetPosition2D(pos);
    
    StaticModel* obj = node->create_component<StaticModel>();
    obj->SetModel(DV_RES_CACHE.GetResource<Model>("Models/Sphere.mdl"));
    obj->SetMaterial(DV_RES_CACHE.GetResource<Material>("Molecule" + String(type)));

    Benchmark03_MoleculeLogic* logic = node->create_component<Benchmark03_MoleculeLogic>();
    logic->SetParameters(type);
}

void AppState_Benchmark03::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    fpsCounter_.Update(timeStep);
    UpdateCurrentFpsElement();

    if (DV_INPUT.GetKeyDown(KEY_ESCAPE))
    {
        APP_STATE_MANAGER.SetRequiredAppStateId(APPSTATEID_MAINSCREEN);
        return;
    }

    if (fpsCounter_.GetTotalTime() >= 30.f)
        APP_STATE_MANAGER.SetRequiredAppStateId(APPSTATEID_RESULTSCREEN);
}
