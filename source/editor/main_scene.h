// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

using namespace dviglo;

// Класс создаёт главную сцену редактора и управляет ей
class MainScene : public Object
{
    DV_OBJECT(MainScene);

    /// Только App может создать и уничтожить объект
    friend class App;

private:
    /// Инициализируется в конструкторе
    inline static MainScene* instance_ = nullptr;

public:
    static MainScene* instance() { return instance_; }

private:
    SharedPtr<Scene> scene_;
    Camera* camera_;

    MainScene();
    ~MainScene();

public:
    Scene* scene() const { return scene_; }
    Camera* camera() const { return camera_; }
};

#define MAIN_SCENE (MainScene::instance())
