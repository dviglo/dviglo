// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once


#ifdef DV_SHARED // Динамическая версия движка

    #ifdef _WIN32 // Visual Studio или MinGW

        #ifdef DV_IS_BUILDING // Компиляция движка
            #define DV_API __declspec(dllexport)
        #else // Использование движка
            #define DV_API __declspec(dllimport)
        #endif

    #else // Linux

        #define DV_API __attribute__((visibility("default")))

    #endif

#else // Статическая версия движка

    #define DV_API

#endif // def DV_SHARED
