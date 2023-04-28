// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "material_editor.h"

#include <dviglo/dviglo_all.h>

const String str_pick_material("pick material");
const String str_new_material("new material");
const String str_reload_material("reload material");
const String str_save_material("save material");
const String str_save_material_as("save material as");
const String str_material_file_path("material file path");

#ifndef NDEBUG
// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool material_editor_destructed = false;
#endif

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
MaterialEditor& MaterialEditor::get_instance()
{
    assert(!material_editor_destructed);
    static MaterialEditor instance;
    return instance;
}

MaterialEditor::MaterialEditor()
{
    // Создаём окно
    window_ = DV_UI->GetRoot()->create_child<Window>("material editor");
    window_->SetStyleAuto();
    window_->SetMinSize(400, 400);
    window_->SetPosition(40, 40);
    window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window_->SetResizable(true);
    window_->SetMovable(true);

    // Создаём область заголовка
    UiElement* title_bar = window_->create_child<UiElement>();
    title_bar->SetFixedHeight(24);
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
    Node* model_node = scene->create_child("model");
    StaticModel* model_component = model_node->create_component<StaticModel>();
    model_component->SetModel(DV_RES_CACHE->GetResource<Model>("models/sphere.mdl"));
    material_ = new Material();
    model_component->SetMaterial(material_);
    Node* light_node = scene->create_child();
    Light* light_component = light_node->create_component<Light>();
    light_component->SetLightType(LightType::LIGHT_DIRECTIONAL);
    light_node->SetRotation(Quaternion(45.f, 45.f, 0.f));
    Node* camera_node = scene->create_child();
    camera_node->SetPosition(Vector3(0.f, 0.f, -2.f));
    Camera* camera_component = camera_node->create_component<Camera>();

    // Создаём элемент с 3D-моделью
    View3D* view3d = window_->create_child<View3D>("view3d");
    view3d->SetView(scene, camera_component);
    view3d->SetResizable(true);
    view3d->SetResizeBorder(IntRect(0, 6, 0, 6));
    view3d->SetFixedHeightResizing(true);

    // Создаём скроллируемый элемент
    ListView* scrollable = window_->create_child<ListView>();
    scrollable->SetStyleAuto();

    // Файл материала
    UiElement* material_file = new UiElement();
    material_file->SetStyleAuto();
    material_file->SetFixedHeight(24);
    material_file->SetLayoutMode(LM_HORIZONTAL);
    LineEdit* material_file_path = material_file->create_child<LineEdit>(str_material_file_path);
    material_file_path->SetStyleAuto();
    Button* pick_material_button = create_button(material_file, str_pick_material, "Выбрать");
    pick_material_button->SetFixedWidth(70);
    scrollable->AddItem(material_file);

    // Кнопки создания/загрузки/сохранения файла
    UiElement* material_file_io = new UiElement();
    material_file_io->SetStyleAuto();
    material_file_io->SetFixedHeight(24);
    material_file_io->SetLayoutMode(LM_HORIZONTAL);
    Button* new_button = create_button(material_file_io, str_new_material, "Новый");
    Button* reload_button = create_button(material_file_io, str_reload_material, "Перезагр.");
    Button* save_button = create_button(material_file_io, str_save_material, "Сохранить");
    Button* save_as_button = create_button(material_file_io, str_save_material_as, "Сохр. как…");
    scrollable->AddItem(material_file_io);

    DV_LOGDEBUG("Singleton MaterialEditor constructed");
}

MaterialEditor::~MaterialEditor()
{
    DV_LOGDEBUG("Singleton MaterialEditor destructed");

#ifndef NDEBUG
    material_editor_destructed = true;
#endif
}

Button* MaterialEditor::create_button(UiElement* parent, const String& name, const String& text)
{
    Button* button = parent->create_child<Button>(name);
    button->SetStyleAuto();

    Text* text_element = button->create_child<Text>();
    text_element->SetStyleAuto();
    text_element->SetText(text);
    text_element->SetAlignment(HorizontalAlignment::HA_CENTER, VerticalAlignment::VA_CENTER);

    subscribe_to_event(button, E_RELEASED, DV_HANDLER(MaterialEditor, handle_button_pressed));

    return button;
}

void MaterialEditor::handle_pick_file_selected(StringHash event_type, VariantMap& event_data)
{
    using namespace FileSelected;
    String file_name = event_data[P_FILENAME].GetString();
    bool ok = event_data[P_OK].GetBool();

    if (ok)
    {
        LineEdit* material_file_path = window_->GetChildStaticCast<LineEdit>(str_material_file_path, true);
        material_file_path->SetText(file_name);
        material_ = DV_RES_CACHE->GetResource<Material>(file_name);

        if (material_)
        {
            View3D* view3d = window_->GetChildStaticCast<View3D>("view3d", false);
            StaticModel* model = view3d->GetScene()->GetChild("model")->GetComponent<StaticModel>();
            model->SetMaterial(material_);
        }
    }

    file_selector_ = nullptr;
}

void MaterialEditor::handle_save_file_as_selected(StringHash event_type, VariantMap& event_data)
{
    using namespace FileSelected;
    String file_name = event_data[P_FILENAME].GetString();
    bool ok = event_data[P_OK].GetBool();

    if (ok)
    {
        if (GetExtension(file_name) == ".json")
        {
            JSONFile json;
            material_->Save(json.GetRoot());
            File file(file_name, FILE_WRITE);
            json.Save(file);
        }
        else
        {
            material_->SaveFile(file_name);
        }
    }

    file_selector_ = nullptr;
}

void MaterialEditor::handle_button_pressed(StringHash event_type, VariantMap& event_data)
{
    using namespace Released;
    Button* pressed_button = static_cast<Button*>(event_data[P_ELEMENT].GetPtr());

    if (pressed_button->GetName() == str_pick_material)
    {
        if (!file_selector_)
        {
            file_selector_ = new FileSelector();
            file_selector_->SetDefaultStyle(DV_UI->GetRoot()->GetDefaultStyle());
            file_selector_->SetTitle("Выберите материал");
            file_selector_->SetButtonTexts("Выбрать", "Отмена");
            file_selector_->SetFilters({"*.xml", "*.mater", "*.json", "*.*"}, 0);
            subscribe_to_event(file_selector_, E_FILESELECTED, DV_HANDLER(MaterialEditor, handle_pick_file_selected));
        }
    }

    else if (pressed_button->GetName() == str_new_material)
    {
        material_ = new Material();

        View3D* view3d = window_->GetChildStaticCast<View3D>("view3d", false);
        StaticModel* model = view3d->GetScene()->GetChild("model")->GetComponent<StaticModel>();
        model->SetMaterial(material_);

        LineEdit* material_file_path = window_->GetChildStaticCast<LineEdit>(str_material_file_path, true);
        material_file_path->SetText("");
    }

    else if (pressed_button->GetName() == str_reload_material)
    {
        DV_RES_CACHE->reload_resource(material_);
    }

    else if (pressed_button->GetName() == str_save_material)
    {
        String full_name = DV_RES_CACHE->GetResourceFileName(material_->GetName());

        if (full_name.Empty())
            return;

        if (GetExtension(full_name) == ".json")
        {
            JSONFile json;
            material_->Save(json.GetRoot());
            File file(full_name, FILE_WRITE);
            json.Save(file);
        }
        else
        {
            material_->SaveFile(full_name);
        }
    }

    else if (pressed_button->GetName() == str_save_material_as)
    {
        if (!file_selector_)
        {
            file_selector_ = new FileSelector();
            file_selector_->SetDefaultStyle(DV_UI->GetRoot()->GetDefaultStyle());
            file_selector_->SetTitle("Сохранить материал как…");
            file_selector_->SetButtonTexts("Сохранить", "Отмена");
            file_selector_->SetFilters({"*.xml", "*.mater", "*.json", "*.*"}, 0);
            subscribe_to_event(file_selector_, E_FILESELECTED, DV_HANDLER(MaterialEditor, handle_save_file_as_selected));
        }
    }
}

#define MATERIAL_EDITOR (MaterialEditor::get_instance())
