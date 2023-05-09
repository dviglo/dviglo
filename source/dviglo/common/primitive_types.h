// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include <cstddef> // std::byte


// Пользователь может инжектировать dvt (dviglo types) в любое пространство имён.
// Для namespace используется короткое имя вместо dviglo::primitive_types, так как в
// подсказах VS тип пишется вместе с пространством имён
namespace dvt
{

// https://en.cppreference.com/w/cpp/language/types
using i8 = signed char;
using u8 = unsigned char;
using i16 = short;
using u16 = unsigned short;
using i32 = int;
using u32 = unsigned;
using i64 = long long;
using u64 = unsigned long long;

// Unicode code point (UTF-32 code unit)
using c32 = char32_t;

// Для сырых данных
using std::byte;

// Некий хеш (например контрольная сумма)
using hash16 = u16;
using hash32 = u32;
using hash64 = u64;

// Некий идентификатор
using id32 = u32;

// Некая маска
using mask32 = u32;

// Некий набор битовых флагов
using flagset32 = u32;

} // namespace dvt

namespace dviglo
{

using namespace dvt;

} // namespace dviglo

/*

# Мотивация

У большинства современных компиляторов размеры примитивных типов совпадают (за исключением long).
Но стандарт C++ этого не требует: <https://en.cppreference.com/w/cpp/language/types>.
Поэтому распространённой практикой является использование типов с фиксированным размером (например Uint32 в SDL).

# Чем плохи [стандартные типы с фиксированным размером](https://en.cppreference.com/w/cpp/types/integer)?

Тип int64_t зависит от компилятора:
* GCC 32, Clang 32, VS 32, VS 64 - long long
* GCC 64, Clang 64 - long

Следующий код успешно скомпилируют VS 32, VS 64 и GCC 32, но не скомпилирует GCC 64:

```
#include <cstdint>

struct S
{
    S(int i32) {}
    S(long long i64) {}
};

int main()
{
    int64_t x;
    S s(x);
}
```

GCC 64 просто не сможет выбрать, какой из двух конструкторов использовать.

Проверить можно [тут](https://godbolt.org).

Кроме того имена стандартных типов излишне длинные: имя uint32_t такое же длинное, как unsigned.

Поэтому используем имена [как в Rust](https://doc.rust-lang.org/book/ch03-02-data-types.html).

В Common Intermediate Language используются еще более короткие имена (i8 вместо i64),
однако такой вариант встречается редко и может сбивать с толку: u8 - это байт или 64 бита?

*/
