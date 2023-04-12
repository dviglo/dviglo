// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

namespace dv = dviglo;

// Класс создаёт главное меню и обрабатывает его события
class MainMenu : public dv::Object
{
    DV_OBJECT(MainMenu, Object);

public:
    static MainMenu& get_instance();

private:
    dv::BorderImage* menu_bar_;

    MainMenu();
    ~MainMenu();

    // Создаёт пункт меню с текстом
    dv::Menu* create_menu_item(const dv::String& text);
};

#define MAIN_MENU (MainMenu::get_instance())
