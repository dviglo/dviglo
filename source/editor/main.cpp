// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include <dviglo/dviglo_all.h>

#include "main_menu.h"
#include "main_scene.h"
#include "material_editor.h"

// Главный класс приложения
class App : public Application
{
    DV_OBJECT(App);

public:
    App()
    {
    }

    ~App()
    {
        delete MaterialEditor::instance_;
        delete MainScene::instance_;
        delete MainMenu::instance_;
    }

    void Setup() override
    {
        engineParameters_[EP_FULL_SCREEN] = false;
        engineParameters_[EP_WINDOW_RESIZABLE] = true;
        engineParameters_[EP_LOG_NAME] = "editor.log";
        engineParameters_[EP_RESOURCE_PATHS] = "data;core_data";
    }

    void Start() override
    {
        XmlFile* style = DV_RES_CACHE->GetResource<XmlFile>("ui/default_style.xml");
        DV_UI->GetRoot()->SetDefaultStyle(style);

        SharedPtr<Cursor> cursor(new Cursor());
        cursor->SetStyleAuto();
        DV_UI->SetCursor(cursor);
        DV_INPUT->SetMouseVisible(true);

        // Указатели на объекты хранятся в самих классах
        new MainMenu();
        new MainScene();
        new MaterialEditor();

        SharedPtr<Viewport> viewport(new Viewport(MAIN_SCENE->scene(), MAIN_SCENE->camera()));
        DV_RENDERER->SetViewport(0, viewport);
    }
};

DV_DEFINE_APPLICATION_MAIN(App);
