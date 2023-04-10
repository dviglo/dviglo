// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

// Класс создаёт окно редактора материалов и обрабатывает его события

#pragma once

#include <dviglo/dviglo_all.h>

namespace dv = dviglo;

class MaterialEditor : public dv::Object
{
    DV_OBJECT(MaterialEditor, Object);

private:
    dv::Window* window_ = nullptr;

    // Создаёт кнопку с текстом
    dv::Button* create_button(dv::UiElement* parent, const dv::String& name, const dv::String& text);

    // Обрабатывает нажатие любой кнопки в окне редактора
    void handle_button_pressed(dv::StringHash eventType, dv::VariantMap& eventData);

public:
    MaterialEditor()
    {
    }

    void init();
};
