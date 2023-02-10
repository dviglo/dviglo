# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Помещаем сторонние библиотеки в отдельный проект
project(dviglo-third-party)

# В IDE таргеты будут отображаться в папке third-party libs
set(CMAKE_FOLDER "third-party libs")

# Добавляем все папки
dv_add_all_subdirs()
