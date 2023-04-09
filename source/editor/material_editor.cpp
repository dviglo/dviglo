// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "material_editor.h"

#include <dviglo/dviglo_all.h>

using namespace dviglo;

SharedPtr<Window> window;

void create_material_editor_window()
{
    // Создаём окно
    window = DV_UI.GetRoot()->create_child<Window>("material editor");
    window->SetStyleAuto();
    window->SetMinSize(400, 400);
    window->SetPosition(40, 40);
    window->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window->SetResizable(true);
    window->SetMovable(true);

    // Создаём область заголовка
    UiElement* title_bar = window->create_child<UiElement>();
    title_bar->SetFixedHeight(24);
    title_bar->SetVerticalAlignment(VA_TOP);
    title_bar->SetLayoutMode(LM_HORIZONTAL);

    // Создаём текст в области заголовка
    Text* title = title_bar->create_child<Text>();
    title->SetStyleAuto();
    title->SetText("Редактор материалов");

    // Создаём кнопку закрытия окна
    Button* close_button = title_bar->create_child<Button>();
    close_button->SetStyle("CloseButton");

    // Создаём сцену для элемента с 3D-моделью
    SharedPtr<Scene> scene = MakeShared<Scene>();
    scene->create_component<Octree>();
    Node* model_node = scene->create_child();
    StaticModel* model_component = model_node->create_component<StaticModel>();
    model_component->SetModel(DV_RES_CACHE.GetResource<Model>("models/sphere.mdl"));
    Node* light_node = scene->create_child();
    Light* light_component = light_node->create_component<Light>();
    light_component->SetLightType(LightType::LIGHT_DIRECTIONAL);
    light_node->SetRotation(Quaternion(45.f, 45.f, 0.f));
    Node* camera_node = scene->create_child();
    camera_node->SetPosition(Vector3(0.f, 0.f, -2.f));
    Camera* camera_component = camera_node->create_component<Camera>();

    // Создаём элемент с 3D-моделью
    View3D* view3d = window->create_child<View3D>();
    view3d->SetView(scene, camera_component);

    // Создаём разделитель между 3D-моделью и настройками материала
    BorderImage* divider = window->create_child<BorderImage>();
    divider->SetStyle("EditorDivider");

    // Настройки материала
    ListView* settings = window->create_child<ListView>();
    settings->SetStyle("PanelView");
}
