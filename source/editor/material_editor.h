// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

using namespace dviglo;


// Класс создаёт окно редактора материалов и обрабатывает его события
class MaterialEditor : public Object
{
    DV_OBJECT(MaterialEditor);

    /// Только App может создать и уничтожить объект
    friend class App;

private:
    /// Инициализируется в конструкторе
    inline static MaterialEditor* instance_ = nullptr;

public:
    static MaterialEditor* instance() { return instance_; }

private:
    Window* window_ = nullptr;
    SharedPtr<FileSelector> file_selector_;
    SharedPtr<Material> material_;

    MaterialEditor();
    ~MaterialEditor();

    // Создаёт кнопку с текстом
    Button* create_button(UiElement* parent, const String& name, const String& text);

    void handle_pick_file_selected(StringHash event_type, VariantMap& event_data);
    void handle_save_file_as_selected(StringHash event_type, VariantMap& event_data);

    // Обрабатывает нажатие любой кнопки в окне редактора
    void handle_button_pressed(StringHash event_type, VariantMap& event_data);

public:
};
