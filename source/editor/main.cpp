// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

// Главный класс приложения

#include <dviglo/dviglo_all.h>

using namespace dviglo;

class App : public Application
{
    DV_OBJECT(App, Application);

public:
    App()
    {
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
        Input& input = DV_INPUT;
        input.SetMouseVisible(true);
    }
};

DV_DEFINE_APPLICATION_MAIN(App);
