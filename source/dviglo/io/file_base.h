// Copyright (c) the Dviglo project
// License: MIT

// Функции, которые можно использовать до инициализации любых подсистем

#pragma once

#include "path.h"

#include <cstdio>

namespace dviglo
{

// ============= Обёртки над 64-битными файловыми функциями =============

/// https://en.cppreference.com/w/c/io/fopen
inline FILE* file_open(const String& filename, const String& mode)
{
#ifdef _WIN32
    return _wfopen(to_win_native(filename).c_str(), WString(mode).c_str());
#else
    return fopen64(filename.c_str(), mode.c_str());
#endif
}

/// https://en.cppreference.com/w/c/io/fseek
inline i32 file_seek(FILE* stream, i64 offset, i32 origin)
{
#ifdef _MSC_VER
    return _fseeki64(stream, offset, origin);
#else
    return fseeko64(stream, offset, origin);
#endif
}

/// https://en.cppreference.com/w/c/io/ftell
inline i64 file_tell(FILE* stream)
{
#ifdef _MSC_VER
    return _ftelli64(stream);
#else
    return ftello64(stream);
#endif
}

/// https://en.cppreference.com/w/c/io/fwrite
inline i32 file_write(const void* buffer, i32 size, i32 count, FILE* stream)
{
    return fwrite(buffer, size, count, stream);
}

/// https://en.cppreference.com/w/c/io/fread
inline i32 file_read(void* buffer, i32 size, i32 count, FILE* stream)
{
    return fread(buffer, size, count, stream);
}

/// https://en.cppreference.com/w/c/io/fflush
inline i32 file_flush(FILE* stream)
{
    return fflush(stream);
}

/// https://en.cppreference.com/w/c/io/fclose
inline i32 file_close(FILE* stream)
{
    return fclose(stream);
}

} // namespace dviglo
