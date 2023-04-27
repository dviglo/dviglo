// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include "../force_assert.h"

#include <dviglo/containers/str.h>

#include <dviglo/common/debug_new.h>

using namespace dviglo;


void test_containers_str()
{
    {
        String str;
        assert(str.Length() == 0);
        assert(str.IsShort());

        str.Resize(9);
        assert(str.Length() == 9);
        assert(str.IsShort());

        str.Resize(1024);
        assert(str.Length() == 1024);
        assert(!str.IsShort());

        str = "0123456789";
        assert(str.Length() == 10);
        assert(!str.IsShort());

        str.Resize(5);
        assert(!str.IsShort());
        assert(str == "01234");

        str.Compact();
        assert(str.IsShort());
        assert(str == "01234");
        assert(str.Length() == 5);
    }

    {
        String str = String::Joined({"aa", "bb", "CC"}, "!");
        assert(str == "aa!bb!CC");
        assert(str.IsShort());
    }

    {
        String str = "bool Swap(T[]&)";
        str.Replace(10, 3, "Array<T>");
        assert(str == "bool Swap(Array<T>&)");
    }

    {
        const char* str = "aa bb CC";
        Vector<String> substrings = String::Split(str, ' ');
        assert(substrings.Size() == 3);
        assert(substrings[0] == "aa");
        assert(substrings[1] == "bb");
        assert(substrings[2] == "CC");
    }

    {
        String uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ@*?4点";
        String lowercase = "abcdefghijklmnopqrstuvwxyzабвгдеёжзийклмнопрстуфхцчшщъыьэюя@*?4点";
        assert(uppercase.ToLower() == lowercase);
        assert(lowercase.ToUpper() == uppercase);
    }
}
