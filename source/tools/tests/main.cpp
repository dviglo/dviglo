// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include <iostream>

using namespace std;


void test_containers_str();
void test_math_big_int();
void test_third_party_sdl();

void run()
{
    test_containers_str();
    test_math_big_int();
    test_third_party_sdl();
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "en_US.UTF-8");

    run();

    cout << "Все тесты пройдены успешно" << endl;

    return 0;
}
