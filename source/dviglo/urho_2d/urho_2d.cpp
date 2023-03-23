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

void register_urho_2d_library()
{
    Renderer2D::register_object();

    Sprite2D::register_object();
    SpriteSheet2D::register_object();

    // Must register objects from base to derived order
    Drawable2D::register_object();
    StaticSprite2D::register_object();

    StretchableSprite2D::register_object();

    AnimationSet2D::register_object();
    AnimatedSprite2D::register_object();

    ParticleEffect2D::register_object();
    ParticleEmitter2D::register_object();

    TmxFile2D::register_object();
    TileMap2D::register_object();
    TileMapLayer2D::register_object();
}

}
