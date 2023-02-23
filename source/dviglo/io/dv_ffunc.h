// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

// Обёртки над 64-битными файловыми функциями

#pragma once

#include "../containers/str.h"

#include <cstdio>

// Так как используются префиксы, то не помещаем функции в namespace dviglo

// https://en.cppreference.com/w/c/io/fopen
inline FILE* dv_fopen(const dviglo::String& filename, const dviglo::String& mode)
{
#ifdef _WIN32
    dviglo::WString w_filename{filename.Replaced('/', '\\')};
    dviglo::WString w_mode{mode};
    return _wfopen(w_filename.CString(), w_mode.CString());
#else
    return fopen64(filename.CString(), mode.CString());
#endif
}

// https://en.cppreference.com/w/c/io/fseek
inline dvt::i32 dv_fseek(FILE* stream, dvt::i64 offset, dvt::i32 origin)
{
#ifdef _MSC_VER
    return _fseeki64(stream, offset, origin);
#else
    return fseeko64(stream, offset, origin);
#endif
}

// https://en.cppreference.com/w/c/io/ftell
inline dvt::i64 dv_ftell(FILE* stream)
{
#ifdef _MSC_VER
    return _ftelli64(stream);
#else
    return ftello64(stream);
#endif
}

// https://en.cppreference.com/w/c/io/fwrite
inline dvt::i32 dv_fwrite(const void* buffer, dvt::i32 size, dvt::i32 count, FILE* stream)
{
    return fwrite(buffer, size, count, stream);
}

// https://en.cppreference.com/w/c/io/fread
inline dvt::i32 dv_fread(void* buffer, dvt::i32 size, dvt::i32 count, FILE* stream)
{
    return fread(buffer, size, count, stream);
}

// https://en.cppreference.com/w/c/io/fflush
inline dvt::i32 dv_fflush(FILE* stream)
{
    return fflush(stream);
}

// https://en.cppreference.com/w/c/io/fclose
inline dvt::i32 dv_fclose(FILE* stream)
{
    return fclose(stream);
}
