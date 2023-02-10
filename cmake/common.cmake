# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Этот файл подключается как самим движком, так и приложением, которое использует движок

# Аналог #pragma once
include_guard(GLOBAL)

# Получаем доступ к макросу cmake_dependent_option
include(CMakeDependentOption)

# Опции движка
cmake_dependent_option(dv_static_runtime "Линковать MSVC runtime статически" FALSE "MSVC" FALSE)

if(dv_static_runtime)
    # Статически линковуем MSVC runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    # Используем динамическую версию MSVC runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
