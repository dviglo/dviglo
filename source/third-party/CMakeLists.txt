# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Помещаем сторонние библиотеки в отдельный проект
project("dv 3rd-party libs")

# В IDE таргеты будут отображаться в папке "сторонние библиотеки"
set(CMAKE_FOLDER "сторонние библиотеки")

# Добавляем подпапки.
# EXCLUDE_FROM_ALL означает, что библиотеки будут компилироваться,
# только если они подключаются другими таргетами
dv_add_all_subdirs(EXCLUDE_FROM_ALL)
