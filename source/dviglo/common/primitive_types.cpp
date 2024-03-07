// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include <cstddef> // ptrdiff_t
#include <cstdint> // intptr_t
#include <limits>  // numeric_limits

using namespace std;


// https://en.cppreference.com/w/cpp/language/types
static_assert(numeric_limits<unsigned char>::digits == 8);
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);
static_assert(sizeof(long long) == 8);
static_assert(sizeof(char32_t) == 4);

#ifdef _WIN32 // Windows
static_assert(sizeof(long) == 4);
static_assert(sizeof(wchar_t) == 2);
#else // Unix
static_assert(sizeof(long) == sizeof(void*)); // 4 или 8
static_assert(sizeof(wchar_t) == 4);
#endif

// Арифметика указателей
static_assert(sizeof(void*) == sizeof(ptrdiff_t));
static_assert(sizeof(void*) == sizeof(intptr_t));

// Dviglo поддерживает только 64-битные платформы
static_assert(sizeof(void*) == 8);
