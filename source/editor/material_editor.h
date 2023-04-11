// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

// Класс создаёт окно редактора материалов и обрабатывает его события

#pragma once

#include <dviglo/dviglo_all.h>

namespace dv = dviglo;

class MaterialEditor : public dv::Object
{
    DV_OBJECT(MaterialEditor, Object);

public:
    static MaterialEditor& get_instance();

private:
    dv::Window* window_ = nullptr;
    dv::SharedPtr<dv::FileSelector> file_selector_;

    MaterialEditor();
    ~MaterialEditor();

    // Создаёт кнопку с текстом
    dv::Button* create_button(dv::UiElement* parent, const dv::String& name, const dv::String& text);

    void handle_file_selected(dv::StringHash event_type, dv::VariantMap& event_data);

    // Обрабатывает нажатие любой кнопки в окне редактора
    void handle_button_pressed(dv::StringHash event_type, dv::VariantMap& event_data);

public:
};
