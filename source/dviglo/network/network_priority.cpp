// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "network_priority.h"

#include "../common/debug_new.h"

namespace dviglo
{

const char* NETWORK_CATEGORY = "Network";

static const float DEFAULT_BASE_PRIORITY = 100.0f;
static const float DEFAULT_DISTANCE_FACTOR = 0.0f;
static const float DEFAULT_MIN_PRIORITY = 0.0f;
static const float UPDATE_THRESHOLD = 100.0f;

NetworkPriority::NetworkPriority() :
    basePriority_(DEFAULT_BASE_PRIORITY),
    distanceFactor_(DEFAULT_DISTANCE_FACTOR),
    min_priority_(DEFAULT_MIN_PRIORITY),
    alwaysUpdateOwner_(true)
{
}

NetworkPriority::~NetworkPriority() = default;

void NetworkPriority::register_object()
{
    DV_CONTEXT->RegisterFactory<NetworkPriority>(NETWORK_CATEGORY);

    DV_ATTRIBUTE("Base Priority", basePriority_, DEFAULT_BASE_PRIORITY, AM_DEFAULT);
    DV_ATTRIBUTE("Distance Factor", distanceFactor_, DEFAULT_DISTANCE_FACTOR, AM_DEFAULT);
    DV_ATTRIBUTE("Minimum Priority", min_priority_, DEFAULT_MIN_PRIORITY, AM_DEFAULT);
    DV_ATTRIBUTE("Always Update Owner", alwaysUpdateOwner_, true, AM_DEFAULT);
}

void NetworkPriority::SetBasePriority(float priority)
{
    basePriority_ = Max(priority, 0.0f);
    MarkNetworkUpdate();
}

void NetworkPriority::SetDistanceFactor(float factor)
{
    distanceFactor_ = Max(factor, 0.0f);
    MarkNetworkUpdate();
}

void NetworkPriority::SetMinPriority(float priority)
{
    min_priority_ = Max(priority, 0.0f);
    MarkNetworkUpdate();
}

void NetworkPriority::SetAlwaysUpdateOwner(bool enable)
{
    alwaysUpdateOwner_ = enable;
    MarkNetworkUpdate();
}

bool NetworkPriority::CheckUpdate(float distance, float& accumulator)
{
    float currentPriority = Max(basePriority_ - distanceFactor_ * distance, min_priority_);
    accumulator += currentPriority;
    if (accumulator >= UPDATE_THRESHOLD)
    {
        accumulator = fmodf(accumulator, UPDATE_THRESHOLD);
        return true;
    }
    else
        return false;
}

}
