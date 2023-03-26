// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../containers/array_ptr.h"
#include "../resource/resource.h"

#include <memory>

#ifdef DV_SPINE
struct spAtlas;
struct spSkeletonData;
struct spAnimationStateData;
#endif

namespace dviglo
{

namespace Spriter
{
    struct SpriterData;
}

class Sprite2D;
class SpriteSheet2D;

/// Spriter animation set, it includes one or more animations, for more information please refer to http://www.esotericsoftware.com and http://www.brashmonkey.com/spriter.htm.
class DV_API AnimationSet2D : public Resource
{
    DV_OBJECT(AnimationSet2D, Resource);

public:
    /// Construct.
    explicit AnimationSet2D();
    /// Destruct.
    ~AnimationSet2D() override;
    /// Register object factory.
    static void register_object();

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool begin_load(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool EndLoad() override;

    /// Get number of animations.
    unsigned GetNumAnimations() const;
    /// Return animation name.
    String GetAnimation(unsigned index) const;
    /// Check has animation.
    bool HasAnimation(const String& animationName) const;

    /// Return sprite.
    Sprite2D* GetSprite() const;

#ifdef DV_SPINE
    /// Return spine skeleton data.
    spSkeletonData* GetSkeletonData() const { return skeletonData_; }
#endif

    /// Return spriter data.
    Spriter::SpriterData* GetSpriterData() const { return spriterData_.get(); }

    /// Return spriter file sprite.
    Sprite2D* GetSpriterFileSprite(int folderId, int fileId) const;

private:
    /// Return sprite by hash.
    Sprite2D* GetSpriterFileSprite(const StringHash& hash) const;
#ifdef DV_SPINE
    /// Begin load spine.
    bool BeginLoadSpine(Deserializer& source);
    /// Finish load spine.
    bool EndLoadSpine();
#endif
    /// Begin load scml.
    bool BeginLoadSpriter(Deserializer& source);
    /// Finish load scml.
    bool EndLoadSpriter();
    /// Dispose all data.
    void Dispose();

    /// Spine sprite.
    SharedPtr<Sprite2D> sprite_;

#ifdef DV_SPINE
    /// Spine json data.
    SharedArrayPtr<char> jsonData_;
    /// Spine skeleton data.
    spSkeletonData* skeletonData_;
    /// Spine atlas.
    spAtlas* atlas_;
#endif

    /// Spriter data.
    std::unique_ptr<Spriter::SpriterData> spriterData_;

    /// Has sprite sheet.
    bool hasSpriteSheet_;

    /// Sprite sheet file path.
    String spriteSheetFilePath_;

    /// Sprite sheet.
    SharedPtr<SpriteSheet2D> spriteSheet_;

    /// Spriter sprites.
    HashMap<unsigned, SharedPtr<Sprite2D>> spriterFileSprites_;
};

}

