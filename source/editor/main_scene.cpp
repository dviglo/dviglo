// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "main_scene.h"

MainScene::MainScene()
{
    scene_ = new Scene();
    scene_->create_component<Octree>();

    Node* camera_node = scene_->create_child();
    camera_ = camera_node->create_component<Camera>();

    instance_ = this;

    DV_LOGDEBUG("MainScene constructed");
}

MainScene::~MainScene()
{
    instance_ = nullptr;
    DV_LOGDEBUG("MainScene destructed");
}

#define MAIN_SCENE (MainScene::instance())
