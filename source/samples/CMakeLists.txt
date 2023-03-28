# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

if(DV_IS_SUBPROJECT)
    return()
endif()

if(NOT DV_SAMPLES)
    return()
endif()

# Помещаем примеры в отдельный проект
project("dv samples")

# В IDE таргеты будут отображаться в папке примеры
set(CMAKE_FOLDER примеры)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Список папок для копирования в bin
set(dir_names Autoload CoreData Data)

# Копируем папки в bin, если нужно
foreach(dir_name ${dir_names})
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/${dir_name})
        dv_create_symlink(${CMAKE_SOURCE_DIR}/bin/${dir_name} ${CMAKE_BINARY_DIR}/bin/${dir_name})
    endif()
endforeach()

# Список файлов для копирования в bin
if(WIN32)
    set(file_names convert_models.bat pbr_demo_deferred.bat pbr_demo_deferred_hw_depth.bat)
else() # Linux
    set(file_names pbr_demo.sh)
endif()

# Копируем файлы в bin, если нужно
foreach(file_name ${file_names})
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/file_name)
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/bin/${file_name} ${CMAKE_BINARY_DIR}/bin/${file_name})
    endif()
endforeach()

# Добавляем все папки
dv_add_all_subdirs()
