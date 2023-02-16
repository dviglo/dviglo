// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "stretchable_sprite_2d.h"
#include "animated_sprite_2d.h"
#include "animation_set_2d.h"
#include "particle_effect_2d.h"
#include "particle_emitter_2d.h"
#include "renderer_2d.h"
#include "sprite_2d.h"
#include "sprite_sheet_2d.h"
#include "tilemap_2d.h"
#include "tilemap_layer_2d.h"
#include "tmx_file_2d.h"
#include "urho_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

const char* URHO2D_CATEGORY = "Urho2D";

void RegisterUrho2DLibrary(Context* context)
{
    Renderer2D::RegisterObject(context);

    Sprite2D::RegisterObject(context);
    SpriteSheet2D::RegisterObject(context);

    // Must register objects from base to derived order
    Drawable2D::RegisterObject(context);
    StaticSprite2D::RegisterObject(context);

    StretchableSprite2D::RegisterObject(context);

    AnimationSet2D::RegisterObject(context);
    AnimatedSprite2D::RegisterObject(context);

    ParticleEffect2D::RegisterObject(context);
    ParticleEmitter2D::RegisterObject(context);

    TmxFile2D::RegisterObject(context);
    TileMap2D::RegisterObject(context);
    TileMapLayer2D::RegisterObject(context);
}

}
