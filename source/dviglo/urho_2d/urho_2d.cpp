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

void RegisterUrho2DLibrary()
{
    Renderer2D::RegisterObject();

    Sprite2D::RegisterObject();
    SpriteSheet2D::RegisterObject();

    // Must register objects from base to derived order
    Drawable2D::RegisterObject();
    StaticSprite2D::RegisterObject();

    StretchableSprite2D::RegisterObject();

    AnimationSet2D::RegisterObject();
    AnimatedSprite2D::RegisterObject();

    ParticleEffect2D::RegisterObject();
    ParticleEmitter2D::RegisterObject();

    TmxFile2D::RegisterObject();
    TileMap2D::RegisterObject();
    TileMapLayer2D::RegisterObject();
}

}
