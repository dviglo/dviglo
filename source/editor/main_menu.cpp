// Copyright (c) the Dviglo project
// License: MIT

#include "main_menu.h"

const String str_file("file");
const String str_load_scene("load scene");
const String str_save_scene("save scene");
const String str_save_scene_as("save scene as");

Menu* MainMenu::create_menu_item(const String& name, const String& text)
{
    Menu* menu_item = new Menu();
    menu_item->SetStyleAuto();
    menu_item->SetName(name);

    Text* menu_item_text = menu_item->create_child<Text>();
    menu_item_text->SetStyle("EditorMenuText");
    menu_item_text->SetPosition(8, 2);
    menu_item_text->SetText(text);

    return menu_item;
}

MainMenu::MainMenu()
{
    menu_bar_ = DV_UI->GetRoot()->create_child<BorderImage>();
    menu_bar_->SetStyle("EditorMenuBar");
    menu_bar_->SetFixedHeight(20);
    menu_bar_->SetFixedWidth(DV_GRAPHICS->GetWidth());
    menu_bar_->SetLayout(LayoutMode::LM_HORIZONTAL);

    {
        Menu* menu = create_menu_item(str_file, "Файл");
        menu_bar_->AddChild(menu);
        menu->SetFixedWidth(menu->GetChildStaticCast<Text>(0)->GetWidth() + 20);
        menu->SetPopupOffset(0, menu->GetHeight());

        Window* popup = new Window();
        popup->SetStyleAuto(DV_UI->GetRoot()->GetDefaultStyle());
        popup->SetLayout(LM_VERTICAL, 1, IntRect(2, 6, 2, 6));
        menu->SetPopup(popup);
        popup->SetFixedWidth(200);

        {
            Menu* item = create_menu_item(str_load_scene, "Загрузить сцену");
            popup->AddChild(item);
            item->SetLayout(LM_HORIZONTAL, 0, IntRect(8, 2, 8, 2));
        }

        {
            Menu* item = create_menu_item(str_save_scene, "Сохранить сцену");
            popup->AddChild(item);
            item->SetLayout(LM_HORIZONTAL, 0, IntRect(8, 2, 8, 2));
        }

        {
            Menu* item = create_menu_item(str_save_scene_as, "Сохранить сцену как…");
            popup->AddChild(item);
            item->SetLayout(LM_HORIZONTAL, 0, IntRect(8, 2, 8, 2));
        }
    }

    subscribe_to_event(E_MENUSELECTED, DV_HANDLER(MainMenu, handle_menu_selected));

    instance_ = this;

    DV_LOGDEBUG("MainMenu constructed");
}

MainMenu::~MainMenu()
{
    instance_ = nullptr;
    DV_LOGDEBUG("MainMenu destructed");
}

void MainMenu::handle_menu_selected(StringHash event_type, VariantMap& event_data)
{
    using namespace MenuSelected;
    Menu* menu = static_cast<Menu*>(event_data[P_ELEMENT].GetPtr());

    // После клика по пункту подменю прячем подменю
    if (menu->GetParent() != menu_bar_)
        menu_bar_->GetChildStaticCast<Menu>(str_file, false)->ShowPopup(false);
}
