// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

using namespace dviglo;

// Класс создаёт главную сцену редактора и управляет ей
class MainScene : public Object
{
    DV_OBJECT(MainScene);

public:
    static MainScene& get_instance();
    static bool is_destructed();

private:
    SharedPtr<Scene> scene_;
    Camera* camera_;

    MainScene();
    ~MainScene();

public:
    Scene* scene() const { return scene_; }
    Camera* camera() const { return camera_; }
};

#define MAIN_SCENE (MainScene::get_instance())
