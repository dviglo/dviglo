// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/billboard_set.h"
#include "../graphics/camera.h"
#include "../graphics/graphics.h"
#include "../graphics/material.h"
#include "../graphics/octree.h"
#include "../graphics/renderer.h"
#include "../graphics/static_model.h"
#include "../graphics/technique.h"
#include "../graphics_api/texture_2d.h"
#include "../graphics_api/vertex_buffer.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "../scene/scene.h"
#include "../scene/scene_events.h"
#include "ui.h"
#include "ui_component.h"
#include "ui_events.h"

#include "../common/debug_new.h"

namespace dviglo
{

static int const UICOMPONENT_DEFAULT_TEXTURE_SIZE = 512;
static int const UICOMPONENT_MIN_TEXTURE_SIZE = 64;
static int const UICOMPONENT_MAX_TEXTURE_SIZE = 4096;

class UIElement3D : public UiElement
{
    DV_OBJECT(UIElement3D, UiElement);
public:
    /// Construct.
    explicit UIElement3D() { }
    /// Destruct.
    ~UIElement3D() override = default;
    /// Set UIComponent which is using this element as root element.
    void SetNode(Node* node) { node_ = node; }
    /// Set active viewport through which this element is rendered. If viewport is not set, it defaults to first viewport.
    void SetViewport(Viewport* viewport) { viewport_ = viewport; }
    /// Convert element coordinates to screen coordinates.
    IntVector2 element_to_screen(const IntVector2& position) override
    {
        DV_LOGERROR("UIElement3D::element_to_screen is not implemented.");
        return {-1, -1};
    }
    /// Convert screen coordinates to element coordinates.
    IntVector2 screen_to_element(const IntVector2& screenPos) override
    {
        IntVector2 result(-1, -1);

        if (node_.Expired())
            return result;

        Scene* scene = node_->GetScene();
        auto* model = node_->GetComponent<StaticModel>();
        if (scene == nullptr || model == nullptr)
            return result;

        if (GParams::is_headless())
            return result;

        // \todo Always uses the first viewport, in case there are multiple
        auto* octree = scene->GetComponent<Octree>();
        if (viewport_ == nullptr)
            viewport_ = DV_RENDERER.GetViewportForScene(scene, 0);

        if (viewport_.Expired() || octree == nullptr)
            return result;

        if (viewport_->GetScene() != scene)
        {
            DV_LOGERROR("UIComponent and Viewport set to component's root element belong to different scenes.");
            return result;
        }

        Camera* camera = viewport_->GetCamera();

        if (camera == nullptr)
            return result;

        IntRect rect = viewport_->rect;
        if (rect == IntRect::ZERO)
        {
            Graphics& graphics = DV_GRAPHICS;
            rect.right_ = graphics.GetWidth();
            rect.bottom_ = graphics.GetHeight();
        }

        // Convert to system mouse position
        IntVector2 pos;
        pos = DV_UI.ConvertUIToSystem(screenPos);

        Ray ray(camera->GetScreenRay((float)pos.x / rect.Width(), (float)pos.y / rect.Height()));
        Vector<RayQueryResult> queryResultVector;
        RayOctreeQuery query(queryResultVector, ray, RAY_TRIANGLE_UV, M_INFINITY, DrawableTypes::Geometry, DEFAULT_VIEWMASK);

        octree->Raycast(query);

        if (queryResultVector.Empty())
            return result;

        for (unsigned i = 0; i < queryResultVector.Size(); i++)
        {
            RayQueryResult& queryResult = queryResultVector[i];
            if (queryResult.drawable_ != model)
            {
                // ignore billboard sets by default
                if (queryResult.drawable_->GetTypeInfo()->IsTypeOf(BillboardSet::GetTypeStatic()))
                    continue;
                return result;
            }

            Vector2& uv = queryResult.textureUV_;
            result = IntVector2(static_cast<int>(uv.x * GetWidth()),
                static_cast<int>(uv.y * GetHeight()));

            // Convert back to scaled UI position
            result = DV_UI.ConvertSystemToUI(result);

            return result;
        }
        return result;
    }

protected:
    /// A UIComponent which owns this element.
    WeakPtr<Node> node_;
    /// Viewport which renders this element.
    WeakPtr<Viewport> viewport_;
};

UIComponent::UIComponent()
    : viewportIndex_(0)
{
    texture_ = DV_CONTEXT.CreateObject<Texture2D>();
    texture_->SetFilterMode(FILTER_BILINEAR);
    texture_->SetAddressMode(COORD_U, ADDRESS_CLAMP);
    texture_->SetAddressMode(COORD_V, ADDRESS_CLAMP);
    texture_->SetNumLevels(1);                                        // No mipmaps

    rootElement_ = DV_CONTEXT.CreateObject<UIElement3D>();
    rootElement_->SetTraversalMode(TM_BREADTH_FIRST);
    rootElement_->SetEnabled(true);

    material_ = DV_CONTEXT.CreateObject<Material>();
    material_->SetTechnique(0, DV_RES_CACHE.GetResource<Technique>("techniques/diff.xml"));
    material_->SetTexture(TU_DIFFUSE, texture_);

    subscribe_to_event(rootElement_, E_RESIZED, DV_HANDLER(UIComponent, OnElementResized));

    // Triggers resizing of texture.
    rootElement_->SetRenderTexture(texture_);
    rootElement_->SetSize(UICOMPONENT_DEFAULT_TEXTURE_SIZE, UICOMPONENT_DEFAULT_TEXTURE_SIZE);
}

UIComponent::~UIComponent() = default;

void UIComponent::register_object()
{
    DV_CONTEXT.RegisterFactory<UIComponent>();
    DV_CONTEXT.RegisterFactory<UIElement3D>();
}

UiElement* UIComponent::GetRoot() const
{
    return rootElement_;
}

Material* UIComponent::GetMaterial() const
{
    return material_;
}

Texture2D* UIComponent::GetTexture() const
{
    return texture_;
}

void UIComponent::OnNodeSet(Node* node)
{
    rootElement_->SetNode(node);
    if (node)
    {
        auto* model = node->GetComponent<StaticModel>();
        rootElement_->SetViewport(DV_RENDERER.GetViewportForScene(GetScene(), viewportIndex_));
        if (model == nullptr)
            model_ = model = node->create_component<StaticModel>();
        model->SetMaterial(material_);
        rootElement_->SetRenderTexture(texture_);
    }
    else
    {
        rootElement_->SetRenderTexture(nullptr);
        if (model_.NotNull())
        {
            model_->Remove();
            model_ = nullptr;
        }
    }
}

void UIComponent::OnElementResized(StringHash eventType, VariantMap& args)
{
    int width = args[Resized::P_WIDTH].GetI32();
    int height = args[Resized::P_HEIGHT].GetI32();

    if (width < UICOMPONENT_MIN_TEXTURE_SIZE || width > UICOMPONENT_MAX_TEXTURE_SIZE ||
        height < UICOMPONENT_MIN_TEXTURE_SIZE || height > UICOMPONENT_MAX_TEXTURE_SIZE)
    {
        DV_LOGERRORF("UIComponent: Texture size %dx%d is not valid. Width and height should be between %d and %d.",
                         width, height, UICOMPONENT_MIN_TEXTURE_SIZE, UICOMPONENT_MAX_TEXTURE_SIZE);
        return;
    }

    if (texture_->SetSize(width, height, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET))
        texture_->GetRenderSurface()->SetUpdateMode(SURFACE_MANUALUPDATE);
    else
        DV_LOGERROR("UIComponent: resizing texture failed.");
}

void UIComponent::SetViewportIndex(unsigned int index)
{
    viewportIndex_ = index;
    if (Scene* scene = GetScene())
    {
        Viewport* viewport = DV_RENDERER.GetViewportForScene(scene, index);
        rootElement_->SetViewport(viewport);
    }
}

}
