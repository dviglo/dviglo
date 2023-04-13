// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "main_scene.h"

using namespace dviglo;

// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool main_scene_destructed = false;

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
MainScene& MainScene::get_instance()
{
    assert(!main_scene_destructed);
    static MainScene instance;
    return instance;
}

bool MainScene::is_destructed()
{
    return main_scene_destructed;
}

MainScene::MainScene()
{
    scene_ = new Scene();
    scene_->create_component<Octree>();

    Node* camera_node = scene_->create_child();
    camera_ = camera_node->create_component<Camera>();

    DV_LOGDEBUG("Singleton MainScene constructed");
}

MainScene::~MainScene()
{
    DV_LOGDEBUG("Singleton MainScene destructed");
    main_scene_destructed = true;
}

#define MAIN_SCENE (MainScene::get_instance())
