// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

using namespace dviglo;

class Ninja;

void ResetAI();
void MakeAIHarder();

class AIController
{
private:
    // Use a weak handle instead of a normal handle to point to the current target
    // so that we don't mistakenly keep it alive.
    WeakPtr<Node> currentTarget;
    float newTargetTimer = 0;

public:
    void Control(Ninja* ownNinja, Node* ownNode, float timeStep);
    void GetNewTarget(Node* ownNode);
};
