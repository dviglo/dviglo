// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics_api/texture_2d.h>
#include <dviglo/ui/sprite.h>
#include <dviglo/ui/ui.h>

#include "sprites.h"

#include <dviglo/common/debug_new.h>

// Number of sprites to draw
static const unsigned NUM_SPRITES = 100;

// Custom variable identifier for storing sprite velocity within the UI element
static constexpr StringHash VAR_VELOCITY = "Velocity"_hash;

DV_DEFINE_APPLICATION_MAIN(Sprites)

Sprites::Sprites()
{
}

void Sprites::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the sprites to the user interface
    CreateSprites();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Sprites::CreateSprites()
{
    // Get rendering window size as floats
    auto width = (float)DV_GRAPHICS.GetWidth();
    auto height = (float)DV_GRAPHICS.GetHeight();

    // Get the Urho3D fish texture
    auto* decalTex = DV_RES_CACHE.GetResource<Texture2D>("Textures/UrhoDecal.dds");

    for (unsigned i = 0; i < NUM_SPRITES; ++i)
    {
        // Create a new sprite, set it to use the texture
        SharedPtr<Sprite> sprite(new Sprite());
        sprite->SetTexture(decalTex);

        // The UI root element is as big as the rendering window, set random position within it
        sprite->SetPosition(Vector2(Random() * width, Random() * height));

        // Set sprite size & hotspot in its center
        sprite->SetSize(IntVector2(128, 128));
        sprite->SetHotSpot(IntVector2(64, 64));

        // Set random rotation in degrees and random scale
        sprite->SetRotation(Random() * 360.0f);
        sprite->SetScale(Random(1.0f) + 0.5f);

        // Set random color and additive blending mode
        sprite->SetColor(Color(Random(0.5f) + 0.5f, Random(0.5f) + 0.5f, Random(0.5f) + 0.5f));
        sprite->SetBlendMode(BLEND_ADD);

        // Add as a child of the root UI element
        DV_UI.GetRoot()->AddChild(sprite);

        // Store sprite's velocity as a custom variable
        sprite->SetVar(VAR_VELOCITY, Vector2(Random(200.0f) - 100.0f, Random(200.0f) - 100.0f));

        // Store sprites to our own container for easy movement update iteration
        sprites_.Push(sprite);
    }
}

void Sprites::MoveSprites(float timeStep)
{
    auto width = (float)DV_GRAPHICS.GetWidth();
    auto height = (float)DV_GRAPHICS.GetHeight();

    // Go through all sprites
    for (const SharedPtr<Sprite>& sprite : sprites_)
    {
        // Rotate
        float newRot = sprite->GetRotation() + timeStep * 30.0f;
        sprite->SetRotation(newRot);

        // Move, wrap around rendering window edges
        Vector2 newPos = sprite->GetPosition() + sprite->GetVar(VAR_VELOCITY).GetVector2() * timeStep;
        if (newPos.x < 0.0f)
            newPos.x += width;
        if (newPos.x >= width)
            newPos.x -= width;
        if (newPos.y < 0.0f)
            newPos.y += height;
        if (newPos.y >= height)
            newPos.y -= height;
        sprite->SetPosition(newPos);
    }
}

void Sprites::SubscribeToEvents()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(Sprites, handle_update));
}

void Sprites::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move sprites, scale movement with time step
    MoveSprites(timeStep);
}
