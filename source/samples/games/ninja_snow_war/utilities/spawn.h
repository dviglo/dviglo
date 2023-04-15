// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

using namespace dviglo;

Node* SpawnObject(Scene* scene, const Vector3& position, const Quaternion& rotation, const String& className);
Node* SpawnParticleEffect(Scene* scene, const Vector3& position, const String& effectName, float duration, CreateMode mode = REPLICATED);
Node* SpawnSound(Scene* scene, const Vector3& position, const String& soundName, float duration);
