// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include "spawn.h"

#include "../game_object.h"

Node* SpawnObject(Scene* scene, const Vector3& position, const Quaternion& rotation, const String& className)
{
    XmlFile* xml = DV_RES_CACHE.GetResource<XmlFile>("ninja_objects/" + className + ".xml");
    return scene->InstantiateXML(xml->GetRoot(), position, rotation);
}

Node* SpawnParticleEffect(Scene* scene, const Vector3& position, const String& effectName, float duration, CreateMode mode)
{
    Node* newNode = scene->create_child("Effect", mode);
    newNode->SetPosition(position);

    // Create the particle emitter
    ParticleEmitter* emitter = newNode->create_component<ParticleEmitter>();
    emitter->SetEffect(DV_RES_CACHE.GetResource<ParticleEffect>(effectName));

    // Create a GameObject for managing the effect lifetime. This is always local, so for server-controlled effects it
    // exists only on the server
    GameObject* object = newNode->create_component<GameObject>(LOCAL);
    object->duration = duration;

    return newNode;
}

Node* SpawnSound(Scene* scene, const Vector3& position, const String& soundName, float duration)
{
    Node* newNode = scene->create_child();
    newNode->SetPosition(position);

    // Create the sound source
    SoundSource3D* source = newNode->create_component<SoundSource3D>();
    Sound* sound = DV_RES_CACHE.GetResource<Sound>(soundName);
    source->SetDistanceAttenuation(200.0f, 5000.0f, 1.0f);
    source->Play(sound);

    // Create a GameObject for managing the sound lifetime
    GameObject* object = newNode->create_component<GameObject>(LOCAL);
    object->duration = duration;

    return newNode;
}
