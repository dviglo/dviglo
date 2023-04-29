// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

using namespace dviglo;

// Класс создаёт главное меню и обрабатывает его события
class MainMenu : public Object
{
    DV_OBJECT(MainMenu);

    /// Только App может создать и уничтожить объект
    friend class App;

private:
    /// Инициализируется в конструкторе
    inline static MainMenu* instance_ = nullptr;

public:
    static MainMenu* instance() { return instance_; }

private:
    BorderImage* menu_bar_;

    MainMenu();
    ~MainMenu();

    // Создаёт пункт меню с текстом
    Menu* create_menu_item(const String& name, const String& text);

    // Обрабатывает нажатие любого пункта меню в программе
    void handle_menu_selected(StringHash event_type, VariantMap& event_data);
};

#define MAIN_MENU (MainMenu::instance())
