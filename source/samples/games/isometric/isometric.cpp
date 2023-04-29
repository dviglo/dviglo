// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/graphics_events.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/physics_2d/collision_box_2d.h>
#include <dviglo/physics_2d/collision_chain_2d.h>
#include <dviglo/physics_2d/collision_circle_2d.h>
#include <dviglo/physics_2d/collision_polygon_2d.h>
#include <dviglo/physics_2d/physics_events_2d.h>
#include <dviglo/physics_2d/physics_world_2d.h>
#include <dviglo/physics_2d/rigid_body_2d.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui_events.h>
#include <dviglo/urho_2d/animated_sprite_2d.h>
#include <dviglo/urho_2d/animation_set_2d.h>
#include <dviglo/urho_2d/tilemap_2d.h>
#include <dviglo/urho_2d/tilemap_layer_2d.h>
#include <dviglo/urho_2d/tmx_file_2d.h>

#include <dviglo/common/debug_new.h>

#include "character2d.h"
#include "isometric.h"
#include "../../utilities2d/sample2d.h"
#include "../../utilities2d/mover.h"

Urho2DIsometricDemo::Urho2DIsometricDemo() :
    zoom_(2.0f),
    draw_debug_(false)
{
    // Register factory for the Character2D component so it can be created via create_component
    Character2D::register_object();
    // Register factory and attributes for the Mover component so it can be created via create_component, and loaded / saved
    Mover::register_object();
}

void Urho2DIsometricDemo::Setup()
{
    Sample::Setup();
    engineParameters_[EP_SOUND] = true;
}

void Urho2DIsometricDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    sample2D_ = new Sample2D();

    // Set filename for load/save functions
    sample2D_->demoFilename_ = "isometric2d";

    // Create the scene content
    create_scene();

    // Create the UI content
    sample2D_->CreateUIContent("ISOMETRIC 2.5D DEMO", character2D_->remainingLifes_, character2D_->remainingCoins_);
    Button* playButton = static_cast<Button*>(DV_UI->GetRoot()->GetChild("PlayButton", true));
    subscribe_to_event(playButton, E_RELEASED, DV_HANDLER(Urho2DIsometricDemo, HandlePlayButton));

    // Hook up to the frame update events
    subscribe_to_events();
}

void Urho2DIsometricDemo::create_scene()
{
    scene_ = new Scene();
    sample2D_->scene_ = scene_;

    // Create the Octree, DebugRenderer and PhysicsWorld2D components to the scene
    scene_->create_component<Octree>();
    scene_->create_component<DebugRenderer>();
    auto* physicsWorld = scene_->create_component<PhysicsWorld2D>();
    physicsWorld->SetGravity(Vector2(0.0f, 0.0f)); // Neutralize gravity as the character will always be grounded

    // Create camera
    cameraNode_ = scene_->create_child("Camera");
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetOrthographic(true);

    camera->SetOrthoSize((float)DV_GRAPHICS->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(2.0f * Min((float)DV_GRAPHICS->GetWidth() / 1280.0f, (float)DV_GRAPHICS->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (2.0) is set for full visibility at 1280x800 resolution)

    // Setup the viewport for displaying the scene
    SharedPtr<Viewport> viewport(new Viewport(scene_, camera));
    DV_RENDERER->SetViewport(0, viewport);

    // Create tile map from tmx file
    auto* tmxFile = DV_RES_CACHE->GetResource<TmxFile2D>("sprites/tilesets/atrium.tmx");
    SharedPtr<Node> tileMapNode(scene_->create_child("TileMap"));
    auto* tileMap = tileMapNode->create_component<TileMap2D>();
    tileMap->SetTmxFile(tmxFile);
    const TileMapInfo2D& info = tileMap->GetInfo();

    // Create Spriter Imp character (from sample 33_SpriterAnimation)
    Node* spriteNode = sample2D_->CreateCharacter(info, 0.0f, Vector3(-5.0f, 11.0f, 0.0f), 0.15f);
    character2D_ = spriteNode->create_component<Character2D>(); // Create a logic component to handle character behavior
    // Scale character's speed on the Y axis according to tiles' aspect ratio
    character2D_->moveSpeedScale_ = info.tileHeight_ / info.tileWidth_;
    character2D_->zoom_ = camera->GetZoom();

    // Generate physics collision shapes from the tmx file's objects located in "Physics" (top) layer
    TileMapLayer2D* tileMapLayer = tileMap->GetLayer(tileMap->GetNumLayers() - 1);
    sample2D_->CreateCollisionShapesFromTMXObjects(tileMapNode, tileMapLayer, info);

    // Instantiate enemies at each placeholder of "MovingEntities" layer (placeholders are Poly Line objects defining a path from points)
    sample2D_->PopulateMovingEntities(tileMap->GetLayer(tileMap->GetNumLayers() - 2));

    // Instantiate coins to pick at each placeholder of "Coins" layer (placeholders for coins are Rectangle objects)
    TileMapLayer2D* coinsLayer = tileMap->GetLayer(tileMap->GetNumLayers() - 3);
    sample2D_->PopulateCoins(coinsLayer);

    // Init coins counters
    character2D_->remainingCoins_ = coinsLayer->GetNumObjects();
    character2D_->maxCoins_ = coinsLayer->GetNumObjects();

    // Check when scene is rendered
    subscribe_to_event(E_ENDRENDERING, DV_HANDLER(Urho2DIsometricDemo, HandleSceneRendered));
}

void Urho2DIsometricDemo::HandleCollisionBegin(StringHash eventType, VariantMap& eventData)
{
    // Get colliding node
    auto* hitNode = static_cast<Node*>(eventData[PhysicsBeginContact2D::P_NODEA].GetPtr());
    if (hitNode->GetName() == "Imp")
        hitNode = static_cast<Node*>(eventData[PhysicsBeginContact2D::P_NODEB].GetPtr());
    String nodeName = hitNode->GetName();
    Node* character2DNode = scene_->GetChild("Imp", true);

    // Handle coins picking
    if (nodeName == "Coin")
    {
        hitNode->Remove();
        character2D_->remainingCoins_ -= 1;
        if (character2D_->remainingCoins_ == 0)
        {
            Text* instructions = static_cast<Text*>(DV_UI->GetRoot()->GetChild("Instructions", true));
            instructions->SetText("!!! You have all the coins !!!");
        }
        Text* coinsText = static_cast<Text*>(DV_UI->GetRoot()->GetChild("CoinsText", true));
        coinsText->SetText(String(character2D_->remainingCoins_)); // Update coins UI counter
        sample2D_->PlaySoundEffect("powerup.wav");
    }

    // Handle interactions with enemies
    if (nodeName == "Orc")
    {
        auto* animatedSprite = character2DNode->GetComponent<AnimatedSprite2D>();
        float deltaX = character2DNode->GetPosition().x - hitNode->GetPosition().x;

        // Orc killed if character is fighting in its direction when the contact occurs
        if (animatedSprite->GetAnimation() == "attack" && (deltaX < 0 == animatedSprite->GetFlipX()))
        {
            static_cast<Mover*>(hitNode->GetComponent<Mover>())->emitTime_ = 1;
            if (!hitNode->GetChild("Emitter", true))
            {
                hitNode->GetComponent("RigidBody2D")->Remove(); // Remove Orc's body
                sample2D_->SpawnEffect(hitNode);
                sample2D_->PlaySoundEffect("big_explosion.wav");
            }
        }
        // Player killed if not fighting in the direction of the Orc when the contact occurs
        else
        {
            if (!character2DNode->GetChild("Emitter", true))
            {
                character2D_->wounded_ = true;
                if (nodeName == "Orc")
                {
                    auto* orc = static_cast<Mover*>(hitNode->GetComponent<Mover>());
                    orc->fightTimer_ = 1;
                }
                sample2D_->SpawnEffect(character2DNode);
                sample2D_->PlaySoundEffect("big_explosion.wav");
            }
        }
    }
}

void Urho2DIsometricDemo::HandleSceneRendered(StringHash eventType, VariantMap& eventData)
{
    unsubscribe_from_event(E_ENDRENDERING);
    // Save the scene so we can reload it later
    sample2D_->SaveScene(true);
    // Pause the scene as long as the UI is hiding it
    scene_->SetUpdateEnabled(false);
}

void Urho2DIsometricDemo::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(Urho2DIsometricDemo, handle_update));

    // Subscribe HandlePostUpdate() function for processing post update events
    subscribe_to_event(E_POSTUPDATE, DV_HANDLER(Urho2DIsometricDemo, HandlePostUpdate));

    // Subscribe to PostRenderUpdate to draw debug geometry
    subscribe_to_event(E_POSTRENDERUPDATE, DV_HANDLER(Urho2DIsometricDemo, handle_post_render_update));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    unsubscribe_from_event(E_SCENEUPDATE);

    // Subscribe to Box2D contact listeners
    subscribe_to_event(E_PHYSICSBEGINCONTACT2D, DV_HANDLER(Urho2DIsometricDemo, HandleCollisionBegin));
}

void Urho2DIsometricDemo::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Zoom in/out
    if (cameraNode_)
        sample2D_->Zoom(cameraNode_->GetComponent<Camera>());

    // Toggle debug geometry with 'Z' key.
    // Используем скан-код, а не код клавиши, иначе не будет работать в Linux, когда включена русская раскладка клавиатуры
    if (DV_INPUT->GetScancodePress(SCANCODE_Z))
        draw_debug_ = !draw_debug_;

    // Check for loading / saving the scene
    if (DV_INPUT->GetKeyPress(KEY_F5))
        sample2D_->SaveScene(false);

    if (DV_INPUT->GetKeyPress(KEY_F7))
        ReloadScene(false);
}

void Urho2DIsometricDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!character2D_)
        return;

    Node* character2DNode = character2D_->GetNode();
    cameraNode_->SetPosition(Vector3(character2DNode->GetPosition().x, character2DNode->GetPosition().y, -10.0f)); // Camera tracks character
}

void Urho2DIsometricDemo::handle_post_render_update(StringHash eventType, VariantMap& eventData)
{
    if (draw_debug_)
    {
        auto* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
        physicsWorld->draw_debug_geometry();

        Node* tileMapNode = scene_->GetChild("TileMap", true);
        auto* map = tileMapNode->GetComponent<TileMap2D>();
        map->draw_debug_geometry(scene_->GetComponent<DebugRenderer>(), false);
    }
}

void Urho2DIsometricDemo::ReloadScene(bool reInit)
{
    String filename = sample2D_->demoFilename_;
    if (!reInit)
        filename += "_in_game";

    File loadFile(DV_FILE_SYSTEM->GetProgramDir() + "data/scenes/" + filename + ".xml", FILE_READ);
    scene_->load_xml(loadFile);
    // After loading we have to reacquire the weak pointer to the Character2D component, as it has been recreated
    // Simply find the character's scene node by name as there's only one of them
    Node* character2DNode = scene_->GetChild("Imp", true);
    if (character2DNode)
        character2D_ = character2DNode->GetComponent<Character2D>();

    // Set what number to use depending whether reload is requested from 'PLAY' button (reInit=true) or 'F7' key (reInit=false)
    int lifes = character2D_->remainingLifes_;
    int coins = character2D_->remainingCoins_;
    if (reInit)
    {
        lifes = LIFES;
        coins = character2D_->maxCoins_;
    }

    // Update lifes UI
    Text* lifeText = static_cast<Text*>(DV_UI->GetRoot()->GetChild("LifeText", true));
    lifeText->SetText(String(lifes));

    // Update coins UI
    Text* coinsText = static_cast<Text*>(DV_UI->GetRoot()->GetChild("CoinsText", true));
    coinsText->SetText(String(coins));
}

void Urho2DIsometricDemo::HandlePlayButton(StringHash eventType, VariantMap& eventData)
{
    // Remove fullscreen UI and unfreeze the scene
    if (static_cast<Text*>(DV_UI->GetRoot()->GetChild("FullUI", true)))
    {
        DV_UI->GetRoot()->GetChild("FullUI", true)->Remove();
        scene_->SetUpdateEnabled(true);
    }
    else
        // Reload scene
        ReloadScene(true);

    // Hide Instructions and Play/Exit buttons
    Text* instructionText = static_cast<Text*>(DV_UI->GetRoot()->GetChild("Instructions", true));
    instructionText->SetText("");
    Button* exitButton = static_cast<Button*>(DV_UI->GetRoot()->GetChild("ExitButton", true));
    exitButton->SetVisible(false);
    Button* playButton = static_cast<Button*>(DV_UI->GetRoot()->GetChild("PlayButton", true));
    playButton->SetVisible(false);

    // Hide mouse cursor
    DV_INPUT->SetMouseVisible(false);
}

DV_DEFINE_APPLICATION_MAIN(Urho2DIsometricDemo)
