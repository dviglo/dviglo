// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/audio/sound.h>
#include <dviglo/audio/sound_source.h>
#include <dviglo/core/context.h>
#include <dviglo/core/core_events.h>
#include <dviglo/core/string_utils.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics_api/texture_2d.h>
#include <dviglo/input/input.h>
#include <dviglo/io/file.h>
#include <dviglo/io/file_system.h>
#include <dviglo/physics_2d/collision_box_2d.h>
#include <dviglo/physics_2d/collision_chain_2d.h>
#include <dviglo/physics_2d/collision_circle_2d.h>
#include <dviglo/physics_2d/collision_edge_2d.h>
#include <dviglo/physics_2d/collision_polygon_2d.h>
#include <dviglo/physics_2d/rigid_body_2d.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/scene/value_animation.h>
#include <dviglo/ui/border_image.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>
#include <dviglo/ui/window.h>
#include <dviglo/urho_2d/animated_sprite_2d.h>
#include <dviglo/urho_2d/animation_set_2d.h>
#include <dviglo/urho_2d/particle_effect_2d.h>
#include <dviglo/urho_2d/particle_emitter_2d.h>
#include <dviglo/urho_2d/tilemap_2d.h>
#include <dviglo/urho_2d/tilemap_layer_2d.h>
#include <dviglo/urho_2d/tmx_file_2d.h>

#include "mover.h"
#include "sample2d.h"


Sample2D::Sample2D()
{
}

void Sample2D::CreateCollisionShapesFromTMXObjects(Node* tileMapNode, TileMapLayer2D* tileMapLayer, const TileMapInfo2D& info)
{
    // Create rigid body to the root node
    auto* body = tileMapNode->create_component<RigidBody2D>();
    body->SetBodyType(BT_STATIC);

    // Generate physics collision shapes and rigid bodies from the tmx file's objects located in "Physics" layer
    for (unsigned i = 0; i < tileMapLayer->GetNumObjects(); ++i)
    {
        TileMapObject2D* tileMapObject = tileMapLayer->GetObject(i); // Get physics objects

        // Create collision shape from tmx object
        switch (tileMapObject->GetObjectType())
        {
            case OT_RECTANGLE:
            {
                CreateRectangleShape(tileMapNode, tileMapObject, tileMapObject->GetSize(), info);
            }
            break;

            case OT_ELLIPSE:
            {
                CreateCircleShape(tileMapNode, tileMapObject, tileMapObject->GetSize().x / 2, info); // Ellipse is built as a Circle shape as it doesn't exist in Box2D
            }
            break;

            case OT_POLYGON:
            {
                CreatePolygonShape(tileMapNode, tileMapObject);
            }
            break;

            case OT_POLYLINE:
            {
                CreatePolyLineShape(tileMapNode, tileMapObject);
            }
            break;
        }
    }
}

CollisionBox2D* Sample2D::CreateRectangleShape(Node* node, TileMapObject2D* object, const Vector2& size, const TileMapInfo2D& info)
{
    auto* shape = node->create_component<CollisionBox2D>();
    shape->SetSize(size);
    if (info.orientation_ == O_ORTHOGONAL)
        shape->SetCenter(object->GetPosition() + size / 2);
    else
    {
        shape->SetCenter(object->GetPosition() + Vector2(info.tileWidth_ / 2, 0.0f));
        shape->SetAngle(45.0f); // If our tile map is isometric then shape is losange
    }
    shape->SetFriction(0.8f);
    if (object->HasProperty("Friction"))
        shape->SetFriction(ToFloat(object->GetProperty("Friction")));
    return shape;
}

CollisionCircle2D* Sample2D::CreateCircleShape(Node* node, TileMapObject2D* object, float radius, const TileMapInfo2D& info)
{
    auto* shape = node->create_component<CollisionCircle2D>();
    Vector2 size = object->GetSize();
    if (info.orientation_ == O_ORTHOGONAL)
        shape->SetCenter(object->GetPosition() + size / 2);
    else
    {
        shape->SetCenter(object->GetPosition() + Vector2(info.tileWidth_ / 2, 0.0f));
    }

    shape->SetRadius(radius);
    shape->SetFriction(0.8f);
    if (object->HasProperty("Friction"))
        shape->SetFriction(ToFloat(object->GetProperty("Friction")));
    return shape;
}

CollisionPolygon2D* Sample2D::CreatePolygonShape(Node* node, TileMapObject2D* object)
{
    auto* shape = node->create_component<CollisionPolygon2D>();
    int numVertices = object->GetNumPoints();
    shape->SetVertexCount(numVertices);
    for (int i = 0; i < numVertices; ++i)
        shape->SetVertex(i, object->GetPoint(i));
    shape->SetFriction(0.8f);
    if (object->HasProperty("Friction"))
        shape->SetFriction(ToFloat(object->GetProperty("Friction")));
    return shape;
}

void Sample2D::CreatePolyLineShape(Node* node, TileMapObject2D* object)
{
    /*
    auto* shape = node->create_component<CollisionChain2D>();
    int numVertices = object->GetNumPoints();
    shape->SetVertexCount(numVertices);
    for (int i = 0; i < numVertices; ++i)
        shape->SetVertex(i, object->GetPoint(i));
    shape->SetFriction(0.8f);
    if (object->HasProperty("Friction"))
        shape->SetFriction(ToFloat(object->GetProperty("Friction")));
    return shape;
    */

    // Latest Box2D supports only one sided chains with ghost vertices, use two sided edges instead.
    // But this can cause stuck at the edges ends https://box2d.org/posts/2020/06/ghost-collisions/

    u32 numVertices = object->GetNumPoints();

    for (u32 i = 1; i < numVertices; ++i)
    {
        CollisionEdge2D* shape = node->create_component<CollisionEdge2D>();
        shape->SetVertices(object->GetPoint(i - 1), object->GetPoint(i));
        shape->SetFriction(0.8f);
        if (object->HasProperty("Friction"))
            shape->SetFriction(ToFloat(object->GetProperty("Friction")));
    }
}

Node* Sample2D::CreateCharacter(const TileMapInfo2D& info, float friction, const Vector3& position, float scale)
{
    Node* spriteNode = scene_->create_child("Imp");
    spriteNode->SetPosition(position);
    spriteNode->SetScale(scale);
    auto* animatedSprite = spriteNode->create_component<AnimatedSprite2D>();
    // Get scml file and Play "idle" anim
    auto* animationSet = DV_RES_CACHE.GetResource<AnimationSet2D>("Urho2D/imp/imp.scml");
    animatedSprite->SetAnimationSet(animationSet);
    animatedSprite->SetAnimation("idle");
    animatedSprite->SetLayer(3); // Put character over tile map (which is on layer 0) and over Orcs (which are on layer 2)
    auto* impBody = spriteNode->create_component<RigidBody2D>();
    impBody->SetBodyType(BT_DYNAMIC);
    impBody->SetAllowSleep(false);
    impBody->SetFixedRotation(true);
    auto* shape = spriteNode->create_component<CollisionCircle2D>();
    shape->SetRadius(1.1f); // Set shape size
    shape->SetFriction(friction); // Set friction
    shape->SetRestitution(0.1f); // Bounce
    shape->SetDensity(6.6f);

    return spriteNode;
}

Node* Sample2D::CreateTrigger()
{
    Node* node = scene_->create_child(); // Clones will be renamed according to object type
    auto* body = node->create_component<RigidBody2D>();
    body->SetBodyType(BT_STATIC);
    auto* shape = node->create_component<CollisionBox2D>(); // Create box shape
    shape->SetTrigger(true);
    return node;
}

Node* Sample2D::CreateEnemy()
{
    Node* node = scene_->create_child("Enemy");
    auto* staticSprite = node->create_component<StaticSprite2D>();
    staticSprite->SetSprite(DV_RES_CACHE.GetResource<Sprite2D>("Urho2D/Aster.png"));
    auto* body = node->create_component<RigidBody2D>();
    body->SetBodyType(BT_STATIC);
    auto* shape = node->create_component<CollisionCircle2D>(); // Create circle shape
    shape->SetRadius(0.25f); // Set radius
    return node;
}

Node* Sample2D::CreateOrc()
{
    Node* node = scene_->create_child("Orc");
    node->SetScale(scene_->GetChild("Imp", true)->GetScale());
    auto* animatedSprite = node->create_component<AnimatedSprite2D>();
    auto* animationSet = DV_RES_CACHE.GetResource<AnimationSet2D>("Urho2D/Orc/Orc.scml");
    animatedSprite->SetAnimationSet(animationSet);
    animatedSprite->SetAnimation("run"); // Get scml file and Play "run" anim
    animatedSprite->SetLayer(2); // Make orc always visible
    auto* body = node->create_component<RigidBody2D>();
    auto* shape = node->create_component<CollisionCircle2D>();
    shape->SetRadius(1.3f); // Set shape size
    shape->SetTrigger(true);
    return node;
}

Node* Sample2D::CreateCoin()
{
    Node* node = scene_->create_child("Coin");
    node->SetScale(0.5);
    auto* animatedSprite = node->create_component<AnimatedSprite2D>();
    auto* animationSet = DV_RES_CACHE.GetResource<AnimationSet2D>("Urho2D/GoldIcon.scml");
    animatedSprite->SetAnimationSet(animationSet); // Get scml file and Play "idle" anim
    animatedSprite->SetAnimation("idle");
    animatedSprite->SetLayer(4);
    auto* body = node->create_component<RigidBody2D>();
    body->SetBodyType(BT_STATIC);
    auto* shape = node->create_component<CollisionCircle2D>(); // Create circle shape
    shape->SetRadius(0.32f); // Set radius
    shape->SetTrigger(true);
    return node;
}

Node* Sample2D::CreateMovingPlatform()
{
    Node* node = scene_->create_child("MovingPlatform");
    node->SetScale(Vector3(3.0f, 1.0f, 0.0f));
    auto* staticSprite = node->create_component<StaticSprite2D>();
    staticSprite->SetSprite(DV_RES_CACHE.GetResource<Sprite2D>("Urho2D/Box.png"));
    auto* body = node->create_component<RigidBody2D>();
    body->SetBodyType(BT_STATIC);
    auto* shape = node->create_component<CollisionBox2D>(); // Create box shape
    shape->SetSize(Vector2(0.32f, 0.32f)); // Set box size
    shape->SetFriction(0.8f); // Set friction
    return node;
}

void Sample2D::PopulateMovingEntities(TileMapLayer2D* movingEntitiesLayer)
{
    // Create enemy (will be cloned at each placeholder)
    Node* enemyNode = CreateEnemy();
    Node* orcNode = CreateOrc();
    Node* platformNode = CreateMovingPlatform();

    // Instantiate enemies and moving platforms at each placeholder (placeholders are Poly Line objects defining a path from points)
    for (unsigned i=0; i < movingEntitiesLayer->GetNumObjects(); ++i)
    {
        // Get placeholder object
        TileMapObject2D* movingObject = movingEntitiesLayer->GetObject(i); // Get placeholder object
        if (movingObject->GetObjectType() == OT_POLYLINE)
        {
            // Clone the enemy and position it at placeholder point
            Node* movingClone;
            Vector2 offset = Vector2(0.0f, 0.0f);
            if (movingObject->GetType() == "Enemy")
            {
                movingClone = enemyNode->Clone();
                offset = Vector2(0.0f, -0.32f);
            }
            else if (movingObject->GetType() == "Orc")
                movingClone = orcNode->Clone();
            else if (movingObject->GetType() == "MovingPlatform")
                movingClone = platformNode->Clone();
            else
                continue;
            movingClone->SetPosition2D(movingObject->GetPoint(0) + offset);

            // Create script object that handles entity translation along its path
            auto* mover = movingClone->create_component<Mover>();

            // Set path from points
            Vector<Vector2> path = CreatePathFromPoints(movingObject, offset);
            mover->path_ = path;

            // Override default speed
            if (movingObject->HasProperty("Speed"))
                mover->speed_ = ToFloat(movingObject->GetProperty("Speed"));
        }
    }

    // Remove nodes used for cloning purpose
    enemyNode->Remove();
    orcNode->Remove();
    platformNode->Remove();
}

void Sample2D::PopulateCoins(TileMapLayer2D* coinsLayer)
{
    // Create coin (will be cloned at each placeholder)
    Node* coinNode = CreateCoin();

    // Instantiate coins to pick at each placeholder
    for (unsigned i=0; i < coinsLayer->GetNumObjects(); ++i)
    {
        TileMapObject2D* coinObject = coinsLayer->GetObject(i); // Get placeholder object
        Node* coinClone = coinNode->Clone();
        coinClone->SetPosition2D(coinObject->GetPosition() + coinObject->GetSize() / 2 + Vector2(0.0f, 0.16f));

    }

    // Remove node used for cloning purpose
    coinNode->Remove();
}

void Sample2D::PopulateTriggers(TileMapLayer2D* triggersLayer)
{
    // Create trigger node (will be cloned at each placeholder)
    Node* triggerNode = CreateTrigger();

    // Instantiate triggers at each placeholder (Rectangle objects)
    for (unsigned i=0; i < triggersLayer->GetNumObjects(); ++i)
    {
        TileMapObject2D* triggerObject = triggersLayer->GetObject(i); // Get placeholder object
        if (triggerObject->GetObjectType() == OT_RECTANGLE)
        {
            Node* triggerClone = triggerNode->Clone();
            triggerClone->SetName(triggerObject->GetType());
            auto* shape = triggerClone->GetComponent<CollisionBox2D>();
            shape->SetSize(triggerObject->GetSize());
            triggerClone->SetPosition2D(triggerObject->GetPosition() + triggerObject->GetSize() / 2);
        }
    }
}

float Sample2D::Zoom(Camera* camera)
{
    float zoom_ = camera->GetZoom();

    if (DV_INPUT.GetMouseMoveWheel() != 0)
    {
        zoom_ = Clamp(zoom_ + DV_INPUT.GetMouseMoveWheel() * 0.1f, CAMERA_MIN_DIST, CAMERA_MAX_DIST);
        camera->SetZoom(zoom_);
    }

    if (DV_INPUT.GetKeyDown(KEY_PAGEUP))
    {
        zoom_ = Clamp(zoom_ * 1.01f, CAMERA_MIN_DIST, CAMERA_MAX_DIST);
        camera->SetZoom(zoom_);
    }

    if (DV_INPUT.GetKeyDown(KEY_PAGEDOWN))
    {
        zoom_ = Clamp(zoom_ * 0.99f, CAMERA_MIN_DIST, CAMERA_MAX_DIST);
        camera->SetZoom(zoom_);
    }

    return zoom_;
}

Vector<Vector2> Sample2D::CreatePathFromPoints(TileMapObject2D* object, const Vector2& offset)
{
    Vector<Vector2> path;
    for (unsigned i=0; i < object->GetNumPoints(); ++i)
        path.Push(object->GetPoint(i) + offset);
    return path;
}

void Sample2D::CreateUIContent(const String& demoTitle, int remainingLifes, int remainingCoins)
{
    ResourceCache& cache = DV_RES_CACHE;
    UI& ui = DV_UI;

    // Set the default UI style and font
    ui.GetRoot()->SetDefaultStyle(cache.GetResource<XmlFile>("UI/DefaultStyle.xml"));
    auto* font = cache.GetResource<Font>("fonts/anonymous pro.ttf");

    // We create in-game UIs (coins and lifes) first so that they are hidden by the fullscreen UI (we could also temporary hide them using SetVisible)

    // Create the UI for displaying the remaining coins
    auto* coinsUI = ui.GetRoot()->create_child<BorderImage>("Coins");
    coinsUI->SetTexture(cache.GetResource<Texture2D>("Urho2D/GoldIcon.png"));
    coinsUI->SetSize(50, 50);
    coinsUI->SetImageRect(IntRect(0, 64, 60, 128));
    coinsUI->SetAlignment(HA_LEFT, VA_TOP);
    coinsUI->SetPosition(5, 5);
    auto* coinsText = coinsUI->create_child<Text>("CoinsText");
    coinsText->SetAlignment(HA_CENTER, VA_CENTER);
    coinsText->SetFont(font, 24);
    coinsText->SetTextEffect(TE_SHADOW);
    coinsText->SetText(String(remainingCoins));

    // Create the UI for displaying the remaining lifes
    auto* lifeUI = ui.GetRoot()->create_child<BorderImage>("Life");
    lifeUI->SetTexture(cache.GetResource<Texture2D>("Urho2D/imp/imp_all.png"));
    lifeUI->SetSize(70, 80);
    lifeUI->SetAlignment(HA_RIGHT, VA_TOP);
    lifeUI->SetPosition(-5, 5);
    auto* lifeText = lifeUI->create_child<Text>("LifeText");
    lifeText->SetAlignment(HA_CENTER, VA_CENTER);
    lifeText->SetFont(font, 24);
    lifeText->SetTextEffect(TE_SHADOW);
    lifeText->SetText(String(remainingLifes));

    // Create the fullscreen UI for start/end
    auto* fullUI = ui.GetRoot()->create_child<Window>("FullUI");
    fullUI->SetStyleAuto();
    fullUI->SetSize(ui.GetRoot()->GetWidth(), ui.GetRoot()->GetHeight());
    fullUI->SetEnabled(false); // Do not react to input, only the 'Exit' and 'Play' buttons will

    // Create the title
    auto* title = fullUI->create_child<BorderImage>("Title");
    title->SetMinSize(fullUI->GetWidth(), 50);
    title->SetTexture(cache.GetResource<Texture2D>("Textures/HeightMap.png"));
    title->SetFullImageRect();
    title->SetAlignment(HA_CENTER, VA_TOP);
    auto* titleText = title->create_child<Text>("TitleText");
    titleText->SetAlignment(HA_CENTER, VA_CENTER);
    titleText->SetFont(font, 24);
    titleText->SetText(demoTitle);

    // Create the image
    auto* spriteUI = fullUI->create_child<BorderImage>("Sprite");
    spriteUI->SetTexture(cache.GetResource<Texture2D>("Urho2D/imp/imp_all.png"));
    spriteUI->SetSize(238, 271);
    spriteUI->SetAlignment(HA_CENTER, VA_CENTER);
    spriteUI->SetPosition(0, - ui.GetRoot()->GetHeight() / 4);

    // Create the 'EXIT' button
    auto* exitButton = ui.GetRoot()->create_child<Button>("ExitButton");
    exitButton->SetStyleAuto();
    exitButton->SetFocusMode(FM_RESETFOCUS);
    exitButton->SetSize(100, 50);
    exitButton->SetAlignment(HA_CENTER, VA_CENTER);
    exitButton->SetPosition(-100, 0);
    auto* exitText = exitButton->create_child<Text>("ExitText");
    exitText->SetAlignment(HA_CENTER, VA_CENTER);
    exitText->SetFont(font, 24);
    exitText->SetText("EXIT");
    subscribe_to_event(exitButton, E_RELEASED, DV_HANDLER(Sample2D, HandleExitButton));

    // Create the 'PLAY' button
    auto* playButton = ui.GetRoot()->create_child<Button>("PlayButton");
    playButton->SetStyleAuto();
    playButton->SetFocusMode(FM_RESETFOCUS);
    playButton->SetSize(100, 50);
    playButton->SetAlignment(HA_CENTER, VA_CENTER);
    playButton->SetPosition(100, 0);
    auto* playText = playButton->create_child<Text>("PlayText");
    playText->SetAlignment(HA_CENTER, VA_CENTER);
    playText->SetFont(font, 24);
    playText->SetText("PLAY");
//  subscribe_to_event(playButton, E_RELEASED, HANDLER(Urho2DPlatformer, HandlePlayButton));

    // Create the instructions
    auto* instructionText = ui.GetRoot()->create_child<Text>("Instructions");
    instructionText->SetText("Use WASD keys or Arrows to move\nPageUp/PageDown/MouseWheel to zoom\nF5/F7 to save/reload scene\n'Z' to toggle debug geometry\nSpace to fight");
    instructionText->SetFont(cache.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    instructionText->SetTextAlignment(HA_CENTER); // Center rows in relation to each other
    instructionText->SetAlignment(HA_CENTER, VA_CENTER);
    instructionText->SetPosition(0, ui.GetRoot()->GetHeight() / 4);

    // Show mouse cursor
    DV_INPUT.SetMouseVisible(true);
}

void Sample2D::HandleExitButton(StringHash eventType, VariantMap& eventData)
{
    DV_ENGINE.Exit();
}

void Sample2D::SaveScene(bool initial)
{
    String filename = demoFilename_;
    if (!initial)
        filename += "_in_game";
    File saveFile(DV_FILE_SYSTEM.GetProgramDir() + "data/scenes/" + filename + ".xml", FILE_WRITE);
    scene_->save_xml(saveFile);
}

void Sample2D::CreateBackgroundSprite(const TileMapInfo2D& info, float scale, const String& texture, bool animate)
{
    Node* node = scene_->create_child("Background");
    node->SetPosition(Vector3(info.GetMapWidth(), info.GetMapHeight(), 0) / 2);
    node->SetScale(scale);
    auto* sprite = node->create_component<StaticSprite2D>();
    sprite->SetSprite(DV_RES_CACHE.GetResource<Sprite2D>(texture));
    set_random_seed(Time::GetSystemTime()); // Randomize from system clock
    sprite->SetColor(Color(Random(0.0f, 1.0f), Random(0.0f, 1.0f), Random(0.0f, 1.0f), 1.0f));
    sprite->SetLayer(-99);

    // Create rotation animation
    if (animate)
    {
        SharedPtr<ValueAnimation> animation(new ValueAnimation());
        animation->SetKeyFrame(0, Variant(Quaternion(0.0f, 0.0f, 0.0f)));
        animation->SetKeyFrame(1, Variant(Quaternion(0.0f, 0.0f, 180.0f)));
        animation->SetKeyFrame(2, Variant(Quaternion(0.0f, 0.0f, 0.0f)));
        node->SetAttributeAnimation("Rotation", animation, WM_LOOP, 0.05f);
    }
}

void Sample2D::SpawnEffect(Node* node)
{
    Node* particleNode = node->create_child("Emitter");
    particleNode->SetScale(0.5f / node->GetScale().x);
    auto* particleEmitter = particleNode->create_component<ParticleEmitter2D>();
    particleEmitter->SetLayer(2);
    particleEmitter->SetEffect(DV_RES_CACHE.GetResource<ParticleEffect2D>("Urho2D/sun.pex"));
}

void Sample2D::PlaySoundEffect(const String& soundName)
{
    auto* source = scene_->create_component<SoundSource>();
    auto* sound = DV_RES_CACHE.GetResource<Sound>("Sounds/" + soundName);

    if (sound)
    {
        source->SetAutoRemoveMode(REMOVE_COMPONENT);
        source->Play(sound);
    }
}
