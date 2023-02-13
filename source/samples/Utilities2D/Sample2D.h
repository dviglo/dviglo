// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/core/object.h>

namespace dviglo
{

class Node;
class Scene;

}

class Character2D;

// All Urho3D classes reside in namespace dviglo
using namespace dviglo;

const float CAMERA_MIN_DIST = 0.1f;
const float CAMERA_MAX_DIST = 6.0f;

/// Convenient functions for Urho2D and Physics2D samples:
///    - Generate collision shapes from a tmx file objects
///    - Create Spriter Imp character
///    - Create enemies, coins and platforms to tile map placeholders
///    - Handle camera zoom using PageUp, PageDown and MouseWheel
///    - Create UI instructions
///    - Create a particle emitter attached to a given node
///    - Play a non-looping sound effect
///    - Load/Save the scene
///    - Create XML patch instructions for screen joystick layout
class Sample2D : public Object
{
    URHO3D_OBJECT(Sample2D, Object);

public:
    /// Construct.
    explicit Sample2D(Context* context);
    /// Destruct.
    ~Sample2D() override = default;

    /// Generate physics collision shapes from the tmx file's objects located in tileMapLayer.
    void CreateCollisionShapesFromTMXObjects(Node* tileMapNode, TileMapLayer2D* tileMapLayer, const TileMapInfo2D& info);
    /// Build collision shape from Tiled 'Rectangle' objects.
    CollisionBox2D* CreateRectangleShape(Node* node, TileMapObject2D* object, const Vector2& size, const TileMapInfo2D& info);
    /// Build collision shape from Tiled 'Ellipse' objects.
    CollisionCircle2D* CreateCircleShape(Node* node, TileMapObject2D* object, float radius, const TileMapInfo2D& info);
    /// Build collision shape from Tiled 'Polygon' objects.
    CollisionPolygon2D* CreatePolygonShape(Node* node, TileMapObject2D* object);
    /// Build collision shape from Tiled 'Poly Line' objects.
    void CreatePolyLineShape(Node* node, TileMapObject2D* object);
    /// Create Imp Spriter character.
    Node* CreateCharacter(const TileMapInfo2D& info, float friction, const Vector3& position, float scale);
    /// Create a trigger (will be cloned at each tmx placeholder).
    Node* CreateTrigger();
    /// Create an enemy (will be cloned at each tmx placeholder).
    Node* CreateEnemy();
    /// Create an Orc (will be cloned at each tmx placeholder).
    Node* CreateOrc();
    /// Create a coin (will be cloned at each tmx placeholder).
    Node* CreateCoin();
    /// Create a moving platform (will be cloned at each tmx placeholder).
    Node* CreateMovingPlatform();
    /// Instantiate enemies and moving platforms at each placeholder (placeholders are Poly Line objects defining a path from points).
    void PopulateMovingEntities(TileMapLayer2D* movingEntitiesLayer);
    /// Instantiate coins to pick at each placeholder.
    void PopulateCoins(TileMapLayer2D* coinsLayer);
    /// Instantiate triggers at each placeholder (Rectangle objects).
    void PopulateTriggers(TileMapLayer2D* triggersLayer);
    /// Read input and zoom the camera.
    float Zoom(Camera* camera);
    /// Create path from tmx object's points.
    Vector<Vector2> CreatePathFromPoints(TileMapObject2D* object, const Vector2& offset);
    /// Create the UI content.
    void CreateUIContent(const String& demoTitle, int remainingLifes, int remainingCoins);
    /// Handle 'EXIT' button released event.
    void HandleExitButton(StringHash eventType, VariantMap& eventData);
    /// Save the scene.
    void SaveScene(bool initial);
    /// Create a background 2D sprite, optionally rotated by a ValueAnimation object.
    void CreateBackgroundSprite(const TileMapInfo2D& info, float scale, const String& texture, bool animate);
    /// Create a particle emitter attached to the given node.
    void SpawnEffect(Node* node);
    /// Play a non-looping sound effect.
    void PlaySoundEffect(const String& soundName);

    /// Filename used in load/save functions.
    String demoFilename_;
    /// The scene.
    Scene* scene_{};

protected:
    /// Return XML patch instructions for screen joystick layout.
    virtual String GetScreenJoystickPatchString() const { return
        "<patch>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Fight</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"SPACE\" />"
        "        </element>"
        "    </add>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Jump</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"UP\" />"
        "        </element>"
        "    </add>"
        "</patch>";
    }
};
