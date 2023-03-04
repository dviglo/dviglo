// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

// Функции, которые можно использовать до инициализации любых подсистем

#include "time_base.h"

#include <ctime>

namespace dviglo
{

String time_to_str()
{
    time_t current_time = time(nullptr);
    char tmp_buffer[sizeof "yyyy-mm-dd hh:mm:ss"];
    strftime(tmp_buffer, sizeof tmp_buffer, "%F %T", localtime(&current_time));
    return String(tmp_buffer);
}

} // namespace dviglo
