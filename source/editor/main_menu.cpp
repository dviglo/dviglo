// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "main_menu.h"

using namespace dviglo;

#ifndef NDEBUG
// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool main_menu_destructed = false;
#endif

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
MainMenu& MainMenu::get_instance()
{
    assert(!main_menu_destructed);
    static MainMenu instance;
    return instance;
}

Menu* MainMenu::create_menu_item(const String& text)
{
    Menu* menu_item = new Menu();
    menu_item->SetStyleAuto();

    Text* menu_item_text = menu_item->create_child<Text>();
    menu_item_text->SetStyle("EditorMenuText");
    menu_item_text->SetPosition(8, 2);
    menu_item_text->SetText(text);

    return menu_item;
}

MainMenu::MainMenu()
{
    menu_bar_ = DV_UI.GetRoot()->create_child<BorderImage>();
    menu_bar_->SetStyle("EditorMenuBar");
    menu_bar_->SetFixedHeight(20);
    menu_bar_->SetFixedWidth(DV_GRAPHICS.GetWidth());
    menu_bar_->SetLayout(LayoutMode::LM_HORIZONTAL);

    {
        Menu* menu = create_menu_item("Файл");
        menu_bar_->AddChild(menu);
        menu->SetFixedWidth(menu->GetChildStaticCast<Text>(0)->GetWidth() + 20);
        menu->SetPopupOffset(0, menu->GetHeight());

        Window* popup = new Window();
        popup->SetStyleAuto(DV_UI.GetRoot()->GetDefaultStyle());
        popup->SetLayout(LM_VERTICAL, 1, IntRect(2, 6, 2, 6));
        menu->SetPopup(popup);
        popup->SetFixedWidth(200);

        {
            Menu* item = create_menu_item("Загрузить");
            popup->AddChild(item);
            item->SetLayout(LM_HORIZONTAL, 0, IntRect(8, 2, 8, 2));
        }

        {
            Menu* item = create_menu_item("Сохранить");
            popup->AddChild(item);
            item->SetLayout(LM_HORIZONTAL, 0, IntRect(8, 2, 8, 2));
        }

        {
            Menu* item = create_menu_item("Сохранить как…");
            popup->AddChild(item);
            item->SetLayout(LM_HORIZONTAL, 0, IntRect(8, 2, 8, 2));
        }
    }

    DV_LOGDEBUG("Singleton MainMenu constructed");
}

MainMenu::~MainMenu()
{
    DV_LOGDEBUG("Singleton MainMenu destructed");

#ifndef NDEBUG
    main_menu_destructed = true;
#endif
}
