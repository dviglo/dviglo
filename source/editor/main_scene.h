// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

namespace dv = dviglo;

// Класс создаёт главную сцену редактора и управляет ей
class MainScene : public dv::Object
{
    DV_OBJECT(MainScene);

public:
    static MainScene& get_instance();
    static bool is_destructed();

private:
    dv::SharedPtr<dv::Scene> scene_;
    dv::Camera* camera_;

    MainScene();
    ~MainScene();

public:
    dv::Scene* scene() const { return scene_; }
    dv::Camera* camera() const { return camera_; }
};

#define MAIN_SCENE (MainScene::get_instance())
