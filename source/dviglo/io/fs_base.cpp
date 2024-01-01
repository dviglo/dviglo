// Copyright (c) 2022-2024 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

// Функции, которые можно использовать до инициализации любых подсистем

#include "fs_base.h"
#include "path.h"

#ifdef _WIN32
    #include <shlobj.h> // SHGetKnownFolderPath()
#else // Linux
    #include <sys/stat.h> // mkdir(), stat()
#endif

namespace dviglo
{

bool dir_exists(const String& path)
{
#ifndef _WIN32
    // Возвращаем true для корневой папки
    if (path == "/")
        return true;
#endif

#ifdef _WIN32
    DWORD attributes = GetFileAttributesW(to_win_native(path).c_str());

    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY))
        return false;
#else
    struct stat st{};

    if (stat(path.c_str(), &st) || !(st.st_mode & S_IFDIR))
        return false;
#endif

    return true;
}

bool create_dir_silent(const String& path)
{
    // Рекурсивно создаём родительские папки
    String parent_path = get_parent(path);

    if (parent_path.Length() > 1 && !dir_exists(parent_path))
    {
        if (!create_dir_silent(parent_path))
            return false;
    }

#ifdef _WIN32
    bool success = CreateDirectoryW(to_win_native(path).c_str(), NULL)
                   || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    // S_IRWXU == 0700
    bool success = mkdir(path.c_str(), S_IRWXU) == 0
                   || errno == EEXIST;
#endif

    return success;
}

String get_pref_path(const String& org, const String& app)
{
    if (app.Empty())
        return "";

#if _WIN32
    // %APPDATA% == %USERPROFILE%\AppData\Roaming
    wchar_t* w_path = nullptr;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &w_path);
    String ret(w_path);
    CoTaskMemFree(w_path);
    ret = to_internal(ret) + "/";
#else
    char* home = getenv("XDG_DATA_HOME");

    if (!home)
        home = getenv("HOME");

    String ret(home);
    ret += "/.local/share/";
#endif

    if (!org.Empty())
        ret += org + "/";

    ret += app + "/";

    if (!create_dir_silent(ret))
        return "";

    return ret;
}

} // namespace dviglo
