// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "random.h"

#include "../common/debug_new.h"

namespace dviglo
{

static unsigned randomSeed = 1;

void set_random_seed(unsigned seed)
{
    randomSeed = seed;
}

unsigned GetRandomSeed()
{
    return randomSeed;
}

int Rand()
{
    randomSeed = randomSeed * 214013 + 2531011;
    return (randomSeed >> 16u) & 32767u;
}

float rand_standard_normal()
{
    float val = 0.0f;
    for (int i = 0; i < 12; i++)
        val += Rand() / 32768.0f;
    val -= 6.0f;

    // Now val is approximatly standard normal distributed
    return val;
}

}
