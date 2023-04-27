// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "ui.h"

#include "../core/context.h"
#include "../core/core_events.h"
#include "../core/profiler.h"
#include "../graphics/camera.h"
#include "../graphics/graphics.h"
#include "../graphics/graphics_events.h"
#include "../graphics/octree.h"
#include "../graphics/technique.h"
#include "../graphics/viewport.h"
#include "../graphics_api/shader.h"
#include "../graphics_api/shader_variation.h"
#include "../graphics_api/texture_2d.h"
#include "../graphics_api/vertex_buffer.h"
#include "../input/input.h"
#include "../input/input_events.h"
#include "../io/log.h"
#include "../math/matrix3x4.h"
#include "../resource/resource_cache.h"
#include "../scene/scene.h"
#include "check_box.h"
#include "cursor.h"
#include "dropdown_list.h"
#include "file_selector.h"
#include "font.h"
#include "line_edit.h"
#include "list_view.h"
#include "message_box.h"
#include "progress_bar.h"
#include "scroll_bar.h"
#include "slider.h"
#include "sprite.h"
#include "text.h"
#include "text3d.h"
#include "tooltip.h"
#include "ui_component.h"
#include "ui_events.h"
#include "view3d.h"
#include "window.h"

#include <cassert>
#include <SDL3/SDL.h>

#include "../common/debug_new.h"

namespace dviglo
{

static MouseButton MakeTouchIDMask(int id)
{
    return static_cast<MouseButton>(1u << static_cast<MouseButtonFlags::Integer>(id)); // NOLINT(misc-misplaced-widening-cast)
}

StringHash VAR_ORIGIN("Origin");
const StringHash VAR_ORIGINAL_PARENT("OriginalParent");
const StringHash VAR_ORIGINAL_CHILD_INDEX("OriginalChildIndex");
const StringHash VAR_PARENT_CHANGED("ParentChanged");

const float DEFAULT_DOUBLECLICK_INTERVAL = 0.5f;
const float DEFAULT_DRAGBEGIN_INTERVAL = 0.5f;
const float DEFAULT_TOOLTIP_DELAY = 0.5f;
const int DEFAULT_DRAGBEGIN_DISTANCE = 5;
const int DEFAULT_FONT_TEXTURE_MAX_SIZE = 2048;

const char* UI_CATEGORY = "UI";

UI::UI() :
    rootElement_(new UiElement()),
    rootModalElement_(new UiElement()),
    doubleClickInterval_(DEFAULT_DOUBLECLICK_INTERVAL),
    dragBeginInterval_(DEFAULT_DRAGBEGIN_INTERVAL),
    defaultToolTipDelay_(DEFAULT_TOOLTIP_DELAY),
    dragBeginDistance_(DEFAULT_DRAGBEGIN_DISTANCE),
    mouseButtons_(0),
    lastMouseButtons_(0),
    maxDoubleClickDist_(M_LARGE_VALUE),
    qualifiers_(0),
    max_font_texture_size_(DEFAULT_FONT_TEXTURE_MAX_SIZE),
    initialized_(false),
    usingTouchInput_(false),
#ifdef _WIN32
    nonFocusedMouseWheel_(false),    // Default MS Windows behaviour
#else
    nonFocusedMouseWheel_(true),     // Default Mac OS X and Linux behaviour
#endif
    useSystemClipboard_(false),
    useMutableGlyphs_(false),
    forceAutoHint_(false),
    fontHintLevel_(FONT_HINT_LEVEL_NORMAL),
    fontSubpixelThreshold_(12),
    fontOversampling_(2),
    uiRendered_(false),
    nonModalBatchSize_(0),
    dragElementsCount_(0),
    dragConfirmedCount_(0),
    uiScale_(1.0f),
    customSize_(IntVector2::ZERO)
{
    rootElement_->SetTraversalMode(TM_DEPTH_FIRST);
    rootModalElement_->SetTraversalMode(TM_DEPTH_FIRST);

    // Register UI library object factories
    register_ui_library();

    subscribe_to_event(E_SCREENMODE, DV_HANDLER(UI, HandleScreenMode));
    subscribe_to_event(E_MOUSEBUTTONDOWN, DV_HANDLER(UI, HandleMouseButtonDown));
    subscribe_to_event(E_MOUSEBUTTONUP, DV_HANDLER(UI, HandleMouseButtonUp));
    subscribe_to_event(E_MOUSEMOVE, DV_HANDLER(UI, HandleMouseMove));
    subscribe_to_event(E_MOUSEWHEEL, DV_HANDLER(UI, HandleMouseWheel));
    subscribe_to_event(E_TOUCHBEGIN, DV_HANDLER(UI, HandleTouchBegin));
    subscribe_to_event(E_TOUCHEND, DV_HANDLER(UI, HandleTouchEnd));
    subscribe_to_event(E_TOUCHMOVE, DV_HANDLER(UI, HandleTouchMove));
    subscribe_to_event(E_KEYDOWN, DV_HANDLER(UI, HandleKeyDown));
    subscribe_to_event(E_TEXTINPUT, DV_HANDLER(UI, HandleTextInput));
    subscribe_to_event(E_DROPFILE, DV_HANDLER(UI, HandleDropFile));

    // Try to initialize right now, but skip if screen mode is not yet set
    Initialize();

    instance_ = this;

    DV_LOGDEBUG("Ui constructed");
}

UI::~UI()
{
    instance_ = nullptr;

    DV_LOGDEBUG("Ui destructed");
}

void UI::SetCursor(Cursor* cursor)
{
    if (cursor_ == cursor)
        return;

    // Remove old cursor (if any) and set new
    if (cursor_)
    {
        rootElement_->RemoveChild(cursor_);
        cursor_.Reset();
    }
    if (cursor)
    {
        rootElement_->AddChild(cursor);
        cursor_ = cursor;

        IntVector2 pos = cursor_->GetPosition();
        const IntVector2& rootSize = rootElement_->GetSize();
        const IntVector2& rootPos = rootElement_->GetPosition();
        pos.x = Clamp(pos.x, rootPos.x, rootPos.x + rootSize.x - 1);
        pos.y = Clamp(pos.y, rootPos.y, rootPos.y + rootSize.y - 1);
        cursor_->SetPosition(pos);
    }
}

void UI::SetFocusElement(UiElement* element, bool byKey)
{
    using namespace FocusChanged;

    UiElement* originalElement = element;

    if (element)
    {
        // Return if already has focus
        if (focusElement_ == element)
            return;

        // Only allow child elements of the modal element to receive focus
        if (HasModalElement())
        {
            UiElement* topLevel = element->GetParent();
            while (topLevel && topLevel->GetParent() != rootElement_)
                topLevel = topLevel->GetParent();
            if (topLevel)   // If parented to non-modal root then ignore
                return;
        }

        // Search for an element in the hierarchy that can alter focus. If none found, exit
        element = GetFocusableElement(element);
        if (!element)
            return;
    }

    // Remove focus from the old element
    if (focusElement_)
    {
        UiElement* oldFocusElement = focusElement_;
        focusElement_.Reset();

        VariantMap& focusEventData = GetEventDataMap();
        focusEventData[Defocused::P_ELEMENT] = oldFocusElement;
        oldFocusElement->SendEvent(E_DEFOCUSED, focusEventData);
    }

    // Then set focus to the new
    if (element && element->GetFocusMode() >= FM_FOCUSABLE)
    {
        focusElement_ = element;

        VariantMap& focusEventData = GetEventDataMap();
        focusEventData[Focused::P_ELEMENT] = element;
        focusEventData[Focused::P_BYKEY] = byKey;
        element->SendEvent(E_FOCUSED, focusEventData);
    }

    VariantMap& eventData = GetEventDataMap();
    eventData[P_CLICKEDELEMENT] = originalElement;
    eventData[P_ELEMENT] = element;
    SendEvent(E_FOCUSCHANGED, eventData);
}

bool UI::SetModalElement(UiElement* modalElement, bool enable)
{
    if (!modalElement)
        return false;

    // Currently only allow modal window
    if (modalElement->GetType() != Window::GetTypeStatic())
        return false;

    assert(rootModalElement_);
    UiElement* currParent = modalElement->GetParent();
    if (enable)
    {
        // Make sure it is not already the child of the root modal element
        if (currParent == rootModalElement_)
            return false;

        // Adopt modal root as parent
        modalElement->SetVar(VAR_ORIGINAL_PARENT, currParent);
        modalElement->SetVar(VAR_ORIGINAL_CHILD_INDEX, currParent ? currParent->FindChild(modalElement) : M_MAX_UNSIGNED);
        modalElement->SetParent(rootModalElement_);

        // If it is a popup element, bring along its top-level parent
        auto* originElement = static_cast<UiElement*>(modalElement->GetVar(VAR_ORIGIN).GetPtr());
        if (originElement)
        {
            UiElement* element = originElement;
            while (element && element->GetParent() != rootElement_)
                element = element->GetParent();
            if (element)
            {
                originElement->SetVar(VAR_PARENT_CHANGED, element);
                UiElement* oriParent = element->GetParent();
                element->SetVar(VAR_ORIGINAL_PARENT, oriParent);
                element->SetVar(VAR_ORIGINAL_CHILD_INDEX, oriParent ? oriParent->FindChild(element) : M_MAX_UNSIGNED);
                element->SetParent(rootModalElement_);
            }
        }

        return true;
    }
    else
    {
        // Only the modal element can disable itself
        if (currParent != rootModalElement_)
            return false;

        // Revert back to original parent
        modalElement->SetParent(static_cast<UiElement*>(modalElement->GetVar(VAR_ORIGINAL_PARENT).GetPtr()),
            modalElement->GetVar(VAR_ORIGINAL_CHILD_INDEX).GetU32());
        auto& vars = const_cast<VariantMap&>(modalElement->GetVars());
        vars.Erase(VAR_ORIGINAL_PARENT);
        vars.Erase(VAR_ORIGINAL_CHILD_INDEX);

        // If it is a popup element, revert back its top-level parent
        auto* originElement = static_cast<UiElement*>(modalElement->GetVar(VAR_ORIGIN).GetPtr());
        if (originElement)
        {
            auto* element = static_cast<UiElement*>(originElement->GetVar(VAR_PARENT_CHANGED).GetPtr());
            if (element)
            {
                const_cast<VariantMap&>(originElement->GetVars()).Erase(VAR_PARENT_CHANGED);
                element->SetParent(static_cast<UiElement*>(element->GetVar(VAR_ORIGINAL_PARENT).GetPtr()),
                    element->GetVar(VAR_ORIGINAL_CHILD_INDEX).GetU32());
                vars = const_cast<VariantMap&>(element->GetVars());
                vars.Erase(VAR_ORIGINAL_PARENT);
                vars.Erase(VAR_ORIGINAL_CHILD_INDEX);
            }
        }

        return true;
    }
}

void UI::Clear()
{
    rootElement_->RemoveAllChildren();
    rootModalElement_->RemoveAllChildren();
    if (cursor_)
        rootElement_->AddChild(cursor_);
}

void UI::Update(float timeStep)
{
    assert(rootElement_ && rootModalElement_);

    DV_PROFILE(UpdateUI);

    // Expire hovers
    for (HashMap<WeakPtr<UiElement>, bool>::Iterator i = hoveredElements_.Begin(); i != hoveredElements_.End(); ++i)
        i->second_ = false;

    Input& input = DV_INPUT;
    bool mouseGrabbed = input.IsMouseGrabbed();

    IntVector2 cursorPos;
    bool cursorVisible;
    GetCursorPositionAndVisible(cursorPos, cursorVisible);

    // Drag begin based on time
    if (dragElementsCount_ > 0 && !mouseGrabbed)
    {
        for (HashMap<WeakPtr<UiElement>, UI::DragData*>::Iterator i = dragElements_.Begin(); i != dragElements_.End();)
        {
            WeakPtr<UiElement> dragElement = i->first_;
            UI::DragData* dragData = i->second_;

            if (!dragElement)
            {
                i = DragElementErase(i);
                continue;
            }

            if (!dragData->dragBeginPending)
            {
                ++i;
                continue;
            }

            if (dragData->dragBeginTimer.GetMSec(false) >= (unsigned)(dragBeginInterval_ * 1000))
            {
                dragData->dragBeginPending = false;
                IntVector2 beginSendPos = dragData->dragBeginSumPos / dragData->numDragButtons;
                dragConfirmedCount_++;
                if (!usingTouchInput_)
                    dragElement->OnDragBegin(dragElement->screen_to_element(beginSendPos), beginSendPos, dragData->dragButtons,
                        qualifiers_, cursor_);
                else
                    dragElement->OnDragBegin(dragElement->screen_to_element(beginSendPos), beginSendPos, dragData->dragButtons, QUAL_NONE, nullptr);

                SendDragOrHoverEvent(E_DRAGBEGIN, dragElement, beginSendPos, IntVector2::ZERO, dragData);
            }

            ++i;
        }
    }

    // Mouse hover
    if (!mouseGrabbed && !input.GetTouchEmulation())
    {
        if (!usingTouchInput_ && cursorVisible)
            ProcessHover(cursorPos, mouseButtons_, qualifiers_, cursor_);
    }

    // Touch hover
    unsigned numTouches = input.GetNumTouches();
    for (unsigned i = 0; i < numTouches; ++i)
    {
        TouchState* touch = input.GetTouch(i);
        IntVector2 touchPos = touch->position_;
        touchPos = ConvertSystemToUI(touchPos);
        ProcessHover(touchPos, MakeTouchIDMask(touch->touchID_), QUAL_NONE, nullptr);
    }

    // End hovers that expired without refreshing
    for (HashMap<WeakPtr<UiElement>, bool>::Iterator i = hoveredElements_.Begin(); i != hoveredElements_.End();)
    {
        if (i->first_.Expired() || !i->second_)
        {
            UiElement* element = i->first_;
            if (element)
            {
                using namespace HoverEnd;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_ELEMENT] = element;
                element->SendEvent(E_HOVEREND, eventData);
            }
            i = hoveredElements_.Erase(i);
        }
        else
            ++i;
    }

    Update(timeStep, rootElement_);
    Update(timeStep, rootModalElement_);
}

void UI::RenderUpdate()
{
    assert(!GParams::is_headless());
    assert(rootElement_ && rootModalElement_);

    DV_PROFILE(GetUIBatches);

    uiRendered_ = false;

    // If the OS cursor is visible, do not render the UI's own cursor
    bool osCursorVisible = DV_INPUT.IsMouseVisible();

    // Get rendering batches from the non-modal UI elements
    batches_.Clear();
    vertexData_.Clear();
    const IntVector2& rootSize = rootElement_->GetSize();
    const IntVector2& rootPos = rootElement_->GetPosition();
    // Note: the scissors operate on unscaled coordinates. Scissor scaling is only performed during render
    IntRect currentScissor = IntRect(rootPos.x, rootPos.y, rootPos.x + rootSize.x, rootPos.y + rootSize.y);
    if (rootElement_->IsVisible())
        GetBatches(batches_, vertexData_, rootElement_, currentScissor);

    // Save the batch size of the non-modal batches for later use
    nonModalBatchSize_ = batches_.Size();

    // Get rendering batches from the modal UI elements
    GetBatches(batches_, vertexData_, rootModalElement_, currentScissor);

    // Get batches from the cursor (and its possible children) last to draw it on top of everything
    if (cursor_ && cursor_->IsVisible() && !osCursorVisible)
    {
        currentScissor = IntRect(0, 0, rootSize.x, rootSize.y);
        cursor_->GetBatches(batches_, vertexData_, currentScissor);
        GetBatches(batches_, vertexData_, cursor_, currentScissor);
    }

    // Get batches for UI elements rendered into textures. Each element rendered into texture is treated as root element.
    for (auto it = renderToTexture_.Begin(); it != renderToTexture_.End();)
    {
        RenderToTextureData& data = it->second_;
        if (data.rootElement_.Expired())
        {
            it = renderToTexture_.Erase(it);
            continue;
        }

        if (data.rootElement_->IsEnabled())
        {
            data.batches_.Clear();
            data.vertexData_.Clear();
            UiElement* element = data.rootElement_;
            const IntVector2& size = element->GetSize();
            const IntVector2& pos = element->GetPosition();
            // Note: the scissors operate on unscaled coordinates. Scissor scaling is only performed during render
            IntRect scissor = IntRect(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
            GetBatches(data.batches_, data.vertexData_, element, scissor);

            // UiElement does not have anything to show. Insert dummy batch that will clear the texture.
            if (data.batches_.Empty())
            {
                UIBatch batch(element, BLEND_REPLACE, scissor, nullptr, &data.vertexData_);
                batch.SetColor(Color::BLACK);
                batch.add_quad(scissor.left_, scissor.top_, scissor.right_, scissor.bottom_, 0, 0);
                data.batches_.Push(batch);
            }
        }
        ++it;
    }
}

void UI::Render(bool renderUICommand)
{
    DV_PROFILE(RenderUI);

    // If the OS cursor is visible, apply its shape now if changed
    if (!renderUICommand)
    {
        bool osCursorVisible = DV_INPUT.IsMouseVisible();
        if (cursor_ && osCursorVisible)
            cursor_->ApplyOSCursorShape();
    }

    Graphics& graphics = DV_GRAPHICS;

    // Perform the default backbuffer render only if not rendered yet, or additional renders through RenderUI command
    if (renderUICommand || !uiRendered_)
    {
        SetVertexData(vertexBuffer_, vertexData_);
        SetVertexData(debugVertexBuffer_, debugVertexData_);

        if (!renderUICommand)
            graphics.ResetRenderTargets();
        // Render non-modal batches
        Render(vertexBuffer_, batches_, 0, nonModalBatchSize_);
        // Render debug draw
        Render(debugVertexBuffer_, debugDrawBatches_, 0, debugDrawBatches_.Size());
        // Render modal batches
        Render(vertexBuffer_, batches_, nonModalBatchSize_, batches_.Size());
    }

    // Render to UiComponent textures. This is skipped when called from the RENDERUI command
    if (!renderUICommand)
    {
        for (auto& item : renderToTexture_)
        {
            RenderToTextureData& data = item.second_;
            if (data.rootElement_->IsEnabled())
            {
                SetVertexData(data.vertexBuffer_, data.vertexData_);
                SetVertexData(data.debugVertexBuffer_, data.debugVertexData_);

                RenderSurface* surface = data.texture_->GetRenderSurface();
                graphics.SetDepthStencil(surface->GetLinkedDepthStencil());
                graphics.SetRenderTarget(0, surface);
                graphics.SetViewport(IntRect(0, 0, surface->GetWidth(), surface->GetHeight()));
                graphics.Clear(dviglo::CLEAR_COLOR);

                Render(data.vertexBuffer_, data.batches_, 0, data.batches_.Size());
                Render(data.debugVertexBuffer_, data.debugDrawBatches_, 0, data.debugDrawBatches_.Size());
                data.debugDrawBatches_.Clear();
                data.debugVertexData_.Clear();
            }
        }

        if (renderToTexture_.Size())
            graphics.ResetRenderTargets();
    }

    // Clear the debug draw batches and data
    debugDrawBatches_.Clear();
    debugVertexData_.Clear();

    uiRendered_ = true;
}

void UI::DebugDraw(UiElement* element)
{
    if (element)
    {
        UiElement* root = element->GetRoot();
        if (!root)
            root = element;
        const IntVector2& rootSize = root->GetSize();
        const IntVector2& rootPos = root->GetPosition();
        IntRect scissor(rootPos.x, rootPos.y, rootPos.x + rootSize.x, rootPos.y + rootSize.y);
        if (root == rootElement_ || root == rootModalElement_)
            element->GetDebugDrawBatches(debugDrawBatches_, debugVertexData_, scissor);
        else
        {
            for (auto& item : renderToTexture_)
            {
                RenderToTextureData& data = item.second_;
                if (!data.rootElement_.Expired() && data.rootElement_ == root && data.rootElement_->IsEnabled())
                {
                    element->GetDebugDrawBatches(data.debugDrawBatches_, data.debugVertexData_, scissor);
                    break;
                }
            }
        }
    }
}

SharedPtr<UiElement> UI::LoadLayout(Deserializer& source, XmlFile* styleFile)
{
    SharedPtr<XmlFile> xml(new XmlFile());
    if (!xml->Load(source))
        return SharedPtr<UiElement>();
    else
        return LoadLayout(xml, styleFile);
}

SharedPtr<UiElement> UI::LoadLayout(XmlFile* file, XmlFile* styleFile)
{
    DV_PROFILE(LoadUILayout);

    SharedPtr<UiElement> root;

    if (!file)
    {
        DV_LOGERROR("Null UI layout XML file");
        return root;
    }

    DV_LOGDEBUG("Loading UI layout " + file->GetName());

    XmlElement rootElem = file->GetRoot("element");
    if (!rootElem)
    {
        DV_LOGERROR("No root UI element in " + file->GetName());
        return root;
    }

    String typeName = rootElem.GetAttribute("type");
    if (typeName.Empty())
        typeName = "UiElement";

    root = DynamicCast<UiElement>(DV_CONTEXT.CreateObject(typeName));
    if (!root)
    {
        DV_LOGERROR("Could not create unknown UI element " + typeName);
        return root;
    }

    // Use default style file of the root element if it has one
    if (!styleFile)
        styleFile = rootElement_->GetDefaultStyle(false);
    // Set it as default for later use by children elements
    if (styleFile)
        root->SetDefaultStyle(styleFile);

    root->load_xml(rootElem, styleFile);
    return root;
}

bool UI::SaveLayout(Serializer& dest, UiElement* element)
{
    DV_PROFILE(SaveUILayout);

    return element && element->save_xml(dest);
}

void UI::SetClipboardText(const String& text)
{
    clipBoard_ = text;
    if (useSystemClipboard_)
        SDL_SetClipboardText(text.c_str());
}

void UI::SetDoubleClickInterval(float interval)
{
    doubleClickInterval_ = Max(interval, 0.0f);
}

void UI::SetMaxDoubleClickDistance(float distPixels)
{
    maxDoubleClickDist_ = distPixels;
}

void UI::SetDragBeginInterval(float interval)
{
    dragBeginInterval_ = Max(interval, 0.0f);
}

void UI::SetDragBeginDistance(int pixels)
{
    dragBeginDistance_ = Max(pixels, 0);
}

void UI::SetDefaultToolTipDelay(float delay)
{
    defaultToolTipDelay_ = Max(delay, 0.0f);
}

void UI::SetMaxFontTextureSize(int size)
{
    if (IsPowerOfTwo((unsigned)size) && size >= FONT_TEXTURE_MIN_SIZE)
    {
        if (size != max_font_texture_size_)
        {
            max_font_texture_size_ = size;
            ReleaseFontFaces();
        }
    }
}

void UI::SetNonFocusedMouseWheel(bool nonFocusedMouseWheel)
{
    nonFocusedMouseWheel_ = nonFocusedMouseWheel;
}

void UI::SetUseSystemClipboard(bool enable)
{
    useSystemClipboard_ = enable;
}

void UI::SetUseMutableGlyphs(bool enable)
{
    if (enable != useMutableGlyphs_)
    {
        useMutableGlyphs_ = enable;
        ReleaseFontFaces();
    }
}

void UI::SetForceAutoHint(bool enable)
{
    if (enable != forceAutoHint_)
    {
        forceAutoHint_ = enable;
        ReleaseFontFaces();
    }
}

void UI::SetFontHintLevel(FontHintLevel level)
{
    if (level != fontHintLevel_)
    {
        fontHintLevel_ = level;
        ReleaseFontFaces();
    }
}

void UI::SetFontSubpixelThreshold(float threshold)
{
    assert(threshold >= 0);
    if (threshold != fontSubpixelThreshold_)
    {
        fontSubpixelThreshold_ = threshold;
        ReleaseFontFaces();
    }
}

void UI::SetFontOversampling(int oversampling)
{
    assert(oversampling >= 1);
    oversampling = Clamp(oversampling, 1, 8);
    if (oversampling != fontOversampling_)
    {
        fontOversampling_ = oversampling;
        ReleaseFontFaces();
    }
}

void UI::SetScale(float scale)
{
    uiScale_ = Max(scale, M_EPSILON);
    ResizeRootElement();
}

void UI::SetWidth(float width)
{
    IntVector2 size = GetEffectiveRootElementSize(false);
    SetScale((float)size.x / width);
}

void UI::SetHeight(float height)
{
    IntVector2 size = GetEffectiveRootElementSize(false);
    SetScale((float)size.y / height);
}

void UI::SetCustomSize(const IntVector2& size)
{
    customSize_ = IntVector2(Max(0, size.x), Max(0, size.y));
    ResizeRootElement();
}

void UI::SetCustomSize(int width, int height)
{
    customSize_ = IntVector2(Max(0, width), Max(0, height));
    ResizeRootElement();
}

IntVector2 UI::GetCursorPosition() const
{
    if (cursor_)
        return cursor_->GetPosition();

    return ConvertSystemToUI(DV_INPUT.GetMousePosition());
}

UiElement* UI::GetElementAt(const IntVector2& position, bool enabledOnly, IntVector2* elementScreenPosition)
{
    UiElement* result = nullptr;

    if (HasModalElement())
        result = GetElementAt(rootModalElement_, position, enabledOnly);

    if (!result)
        result = GetElementAt(rootElement_, position, enabledOnly);

    // Mouse was not hovering UI element. Check elements rendered on 3D objects.
    if (!result && renderToTexture_.Size())
    {
        for (auto& item : renderToTexture_)
        {
            RenderToTextureData& data = item.second_;
            if (data.rootElement_.Expired() || !data.rootElement_->IsEnabled())
                continue;

            IntVector2 screenPosition = data.rootElement_->screen_to_element(position);
            if (data.rootElement_->GetCombinedScreenRect().IsInside(screenPosition) == INSIDE)
            {
                result = GetElementAt(data.rootElement_, screenPosition, enabledOnly);
                if (result)
                {
                    if (elementScreenPosition)
                        *elementScreenPosition = screenPosition;
                    break;
                }
            }
        }
    }
    else if (elementScreenPosition)
        *elementScreenPosition = position;

    return result;
}

UiElement* UI::GetElementAt(const IntVector2& position, bool enabledOnly)
{
    return GetElementAt(position, enabledOnly, nullptr);
}

UiElement* UI::GetElementAt(UiElement* root, const IntVector2& position, bool enabledOnly)
{
    IntVector2 positionCopy(position);
    const IntVector2& rootSize = root->GetSize();
    const IntVector2& rootPos = root->GetPosition();

    // If position is out of bounds of root element return null.
    if (position.x < rootPos.x || position.x > rootPos.x + rootSize.x)
        return nullptr;

    if (position.y < rootPos.y || position.y > rootPos.y + rootSize.y)
        return nullptr;

    // If UI is smaller than the screen, wrap if necessary
    if (rootSize.x > 0 && rootSize.y > 0)
    {
        if (positionCopy.x >= rootPos.x + rootSize.x)
            positionCopy.x = rootPos.x + ((positionCopy.x - rootPos.x) % rootSize.x);
        if (positionCopy.y >= rootPos.y + rootSize.y)
            positionCopy.y = rootPos.y + ((positionCopy.y - rootPos.y) % rootSize.y);
    }

    UiElement* result = nullptr;
    GetElementAt(result, root, positionCopy, enabledOnly);
    return result;
}

UiElement* UI::GetElementAt(int x, int y, bool enabledOnly)
{
    return GetElementAt(IntVector2(x, y), enabledOnly);
}

IntVector2 UI::ConvertSystemToUI(const IntVector2& systemPos) const
{
    return VectorFloorToInt(static_cast<Vector2>(systemPos) / GetScale());
}

IntVector2 UI::ConvertUIToSystem(const IntVector2& uiPos) const
{
    return VectorFloorToInt(static_cast<Vector2>(uiPos) * GetScale());
}

UiElement* UI::GetFrontElement() const
{
    const Vector<SharedPtr<UiElement>>& rootChildren = rootElement_->GetChildren();
    int maxPriority = M_MIN_INT;
    UiElement* front = nullptr;

    for (const SharedPtr<UiElement>& rootChild : rootChildren)
    {
        // Do not take into account input-disabled elements, hidden elements or those that are always in the front
        if (!rootChild->IsEnabled() || !rootChild->IsVisible() || !rootChild->GetBringToBack())
            continue;

        int priority = rootChild->GetPriority();
        if (priority > maxPriority)
        {
            maxPriority = priority;
            front = rootChild;
        }
    }

    return front;
}

const Vector<UiElement*> UI::GetDragElements()
{
    // Do not return the element until drag begin event has actually been posted
    if (!dragElementsConfirmed_.Empty())
        return dragElementsConfirmed_;

    for (HashMap<WeakPtr<UiElement>, UI::DragData*>::Iterator i = dragElements_.Begin(); i != dragElements_.End();)
    {
        WeakPtr<UiElement> dragElement = i->first_;
        UI::DragData* dragData = i->second_;

        if (!dragElement)
        {
            i = DragElementErase(i);
            continue;
        }

        if (!dragData->dragBeginPending)
            dragElementsConfirmed_.Push(dragElement);

        ++i;
    }

    return dragElementsConfirmed_;
}

UiElement* UI::GetDragElement(unsigned index)
{
    GetDragElements();
    if (index >= dragElementsConfirmed_.Size())
        return nullptr;

    return dragElementsConfirmed_[index];
}

const String& UI::GetClipboardText() const
{
    if (useSystemClipboard_)
    {
        char* text = SDL_GetClipboardText();
        clipBoard_ = String(text);
        if (text)
            SDL_free(text);
    }

    return clipBoard_;
}

bool UI::HasModalElement() const
{
    return rootModalElement_->GetNumChildren() > 0;
}

void UI::Initialize()
{
    if (GParams::is_headless() || !DV_GRAPHICS.IsInitialized())
        return;

    DV_PROFILE(InitUI);

    // Set initial root element size
    ResizeRootElement();

    vertexBuffer_ = new VertexBuffer();
    debugVertexBuffer_ = new VertexBuffer();

    initialized_ = true;

    subscribe_to_event(E_BEGINFRAME, DV_HANDLER(UI, HandleBeginFrame));
    subscribe_to_event(E_POSTUPDATE, DV_HANDLER(UI, HandlePostUpdate));
    subscribe_to_event(E_RENDERUPDATE, DV_HANDLER(UI, HandleRenderUpdate));

    DV_LOGINFO("Initialized user interface");
}

void UI::Update(float timeStep, UiElement* element)
{
    // Keep a weak pointer to the element in case it destroys itself on update
    WeakPtr<UiElement> elementWeak(element);

    element->Update(timeStep);
    if (elementWeak.Expired())
        return;

    const Vector<SharedPtr<UiElement>>& children = element->GetChildren();
    // Update of an element may modify its child vector. Use just index-based iteration to be safe
    for (i32 i = 0; i < children.Size(); ++i)
        Update(timeStep, children[i]);
}

void UI::SetVertexData(VertexBuffer* dest, const Vector<float>& vertexData)
{
    if (vertexData.Empty())
        return;

    // Update quad geometry into the vertex buffer
    // Resize the vertex buffer first if too small or much too large
    i32 numVertices = vertexData.Size() / UI_VERTEX_SIZE;
    if (dest->GetVertexCount() < numVertices || dest->GetVertexCount() > numVertices * 2)
        dest->SetSize(numVertices, VertexElements::Position | VertexElements::Color | VertexElements::TexCoord1, true);

    dest->SetData(&vertexData[0]);
}

void UI::Render(VertexBuffer* buffer, const Vector<UIBatch>& batches, unsigned batchStart, unsigned batchEnd)
{
    assert(!GParams::is_headless());

    Graphics& graphics = DV_GRAPHICS;

    // Engine does not render when window is closed or device is lost
    assert(graphics.IsInitialized() && !graphics.IsDeviceLost());

    if (batches.Empty())
        return;

    unsigned alphaFormat = Graphics::GetAlphaFormat();
    RenderSurface* surface = graphics.GetRenderTarget(0);
    IntVector2 viewSize = graphics.GetViewport().Size();
    Vector2 invScreenSize(1.0f / (float)viewSize.x, 1.0f / (float)viewSize.y);
    Vector2 scale(2.0f * invScreenSize.x, -2.0f * invScreenSize.y);
    Vector2 offset(-1.0f, 1.0f);

    if (GParams::get_gapi() == GAPI_OPENGL && surface)
    {
        // On OpenGL, flip the projection if rendering to a texture so that the texture can be addressed in the
        // same way as a render texture produced on Direct3D.
        offset.y = -offset.y;
        scale.y = -scale.y;
    }

    Matrix4 projection(Matrix4::IDENTITY);
    projection.m00_ = scale.x * uiScale_;
    projection.m03_ = offset.x;
    projection.m11_ = scale.y * uiScale_;
    projection.m13_ = offset.y;
    projection.m22_ = 1.0f;
    projection.m23_ = 0.0f;
    projection.m33_ = 1.0f;

    graphics.ClearParameterSources();
    graphics.SetColorWrite(true);

    // Reverse winding if rendering to texture on OpenGL
    if (GParams::get_gapi() == GAPI_OPENGL && surface)
        graphics.SetCullMode(CULL_CW);
    else
        graphics.SetCullMode(CULL_CCW);

    graphics.SetDepthTest(CMP_ALWAYS);
    graphics.SetDepthWrite(false);
    graphics.SetFillMode(FILL_SOLID);
    graphics.SetStencilTest(false);
    graphics.SetVertexBuffer(buffer);

    ShaderVariation* noTextureVS = graphics.GetShader(VS, "basic", "VERTEXCOLOR");
    ShaderVariation* diffTextureVS = graphics.GetShader(VS, "basic", "DIFFMAP VERTEXCOLOR");
    ShaderVariation* noTexturePS = graphics.GetShader(PS, "basic", "VERTEXCOLOR");
    ShaderVariation* diffTexturePS = graphics.GetShader(PS, "basic", "DIFFMAP VERTEXCOLOR");
    ShaderVariation* diffMaskTexturePS = graphics.GetShader(PS, "basic", "DIFFMAP ALPHAMASK VERTEXCOLOR");
    ShaderVariation* alphaTexturePS = graphics.GetShader(PS, "basic", "ALPHAMAP VERTEXCOLOR");


    for (unsigned i = batchStart; i < batchEnd; ++i)
    {
        const UIBatch& batch = batches[i];
        if (batch.vertexStart_ == batch.vertexEnd_)
            continue;

        ShaderVariation* ps;
        ShaderVariation* vs;

        if (!batch.customMaterial_)
        {
            if (!batch.texture_)
            {
                ps = noTexturePS;
                vs = noTextureVS;
            } else
            {
                // If texture contains only an alpha channel, use alpha shader (for fonts)
                vs = diffTextureVS;

                if (batch.texture_->GetFormat() == alphaFormat)
                    ps = alphaTexturePS;
                else if (batch.blend_mode_ != BLEND_ALPHA && batch.blend_mode_ != BLEND_ADDALPHA && batch.blend_mode_ != BLEND_PREMULALPHA)
                    ps = diffMaskTexturePS;
                else
                    ps = diffTexturePS;
            }
        } else
        {
            vs = diffTextureVS;
            ps = diffTexturePS;

            Technique* technique = batch.customMaterial_->GetTechnique(0);
            if (technique)
            {
                Pass* pass = nullptr;
                for (int i = 0; i < technique->GetNumPasses(); ++i)
                {
                    pass = technique->GetPass(i);
                    if (pass)
                    {
                        vs = graphics.GetShader(VS, pass->GetVertexShader(), batch.customMaterial_->GetVertexShaderDefines());
                        ps = graphics.GetShader(PS, pass->GetPixelShader(), batch.customMaterial_->GetPixelShaderDefines());
                        break;
                    }
                }
            }
        }

        graphics.SetShaders(vs, ps);
        if (graphics.NeedParameterUpdate(SP_OBJECT, this))
            graphics.SetShaderParameter(VSP_MODEL, Matrix3x4::IDENTITY);
        if (graphics.NeedParameterUpdate(SP_CAMERA, this))
            graphics.SetShaderParameter(VSP_VIEWPROJ, projection);
        if (graphics.NeedParameterUpdate(SP_MATERIAL, this))
            graphics.SetShaderParameter(PSP_MATDIFFCOLOR, Color(1.0f, 1.0f, 1.0f, 1.0f));

        float elapsedTime = DV_TIME.GetElapsedTime();
        graphics.SetShaderParameter(VSP_ELAPSEDTIME, elapsedTime);
        graphics.SetShaderParameter(PSP_ELAPSEDTIME, elapsedTime);

        IntRect scissor = batch.scissor_;
        scissor.left_ = (int)(scissor.left_ * uiScale_);
        scissor.top_ = (int)(scissor.top_ * uiScale_);
        scissor.right_ = (int)(scissor.right_ * uiScale_);
        scissor.bottom_ = (int)(scissor.bottom_ * uiScale_);

        // Flip scissor vertically if using OpenGL texture rendering
        if (GParams::get_gapi() == GAPI_OPENGL && surface)
        {
            int top = scissor.top_;
            int bottom = scissor.bottom_;
            scissor.top_ = viewSize.y - bottom;
            scissor.bottom_ = viewSize.y - top;
        }

        graphics.SetBlendMode(batch.blend_mode_);
        graphics.SetScissorTest(true, scissor);
        if (!batch.customMaterial_)
        {
            graphics.SetTexture(0, batch.texture_);
        }
        else
        {
            // Update custom shader parameters if needed
            if (graphics.NeedParameterUpdate(SP_MATERIAL, reinterpret_cast<const void*>(batch.customMaterial_->GetShaderParameterHash())))
            {
                auto shader_parameters = batch.customMaterial_->GetShaderParameters();
                for (auto it = shader_parameters.Begin(); it != shader_parameters.End(); ++it)
                {
                    graphics.SetShaderParameter(it->second_.name_, it->second_.value_);
                }
            }
            // Apply custom shader textures
            auto textures = batch.customMaterial_->GetTextures();
            for (auto it = textures.Begin(); it != textures.End(); ++it)
            {
                graphics.SetTexture(it->first_, it->second_);
            }
        }

        graphics.Draw(TRIANGLE_LIST, batch.vertexStart_ / UI_VERTEX_SIZE,
            (batch.vertexEnd_ - batch.vertexStart_) / UI_VERTEX_SIZE);

        if (batch.customMaterial_)
        {
            // Reset textures used by the batch custom material
            auto textures = batch.customMaterial_->GetTextures();
            for (auto it = textures.Begin(); it != textures.End(); ++it)
                graphics.SetTexture(it->first_, 0);
        }
    }
}

void UI::GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, UiElement* element, IntRect currentScissor)
{
    // Set clipping scissor for child elements. No need to draw if zero size
    element->AdjustScissor(currentScissor);
    if (currentScissor.left_ == currentScissor.right_ || currentScissor.top_ == currentScissor.bottom_)
        return;

    element->SortChildren();
    const Vector<SharedPtr<UiElement>>& children = element->GetChildren();
    if (children.Empty())
        return;

    // For non-root elements draw all children of same priority before recursing into their children: assumption is that they have
    // same renderstate
    Vector<SharedPtr<UiElement>>::ConstIterator i = children.Begin();
    if (element->GetTraversalMode() == TM_BREADTH_FIRST)
    {
        Vector<SharedPtr<UiElement>>::ConstIterator j = i;
        while (i != children.End())
        {
            int currentPriority = (*i)->GetPriority();
            while (j != children.End() && (*j)->GetPriority() == currentPriority)
            {
                if ((*j)->IsWithinScissor(currentScissor) && (*j) != cursor_)
                    (*j)->GetBatches(batches, vertexData, currentScissor);
                ++j;
            }
            // Now recurse into the children
            while (i != j)
            {
                if ((*i)->IsVisible() && (*i) != cursor_)
                    GetBatches(batches, vertexData, *i, currentScissor);
                ++i;
            }
        }
    }
    // On the root level draw each element and its children immediately after to avoid artifacts
    else
    {
        while (i != children.End())
        {
            if ((*i) != cursor_)
            {
                if ((*i)->IsWithinScissor(currentScissor))
                    (*i)->GetBatches(batches, vertexData, currentScissor);
                if ((*i)->IsVisible())
                    GetBatches(batches, vertexData, *i, currentScissor);
            }
            ++i;
        }
    }
}

void UI::GetElementAt(UiElement*& result, UiElement* current, const IntVector2& position, bool enabledOnly)
{
    if (!current)
        return;

    current->SortChildren();
    const Vector<SharedPtr<UiElement>>& children = current->GetChildren();
    LayoutMode parentLayoutMode = current->GetLayoutMode();

    for (unsigned i = 0; i < children.Size(); ++i)
    {
        UiElement* element = children[i];
        bool hasChildren = element->GetNumChildren() > 0;

        if (element != cursor_.Get() && element->IsVisible())
        {
            if (element->IsInside(position, true))
            {
                // Store the current result, then recurse into its children. Because children
                // are sorted from lowest to highest priority, the topmost match should remain
                if (element->IsEnabled() || !enabledOnly)
                    result = element;

                if (hasChildren)
                    GetElementAt(result, element, position, enabledOnly);
                // Layout optimization: if the element has no children, can break out after the first match
                else if (parentLayoutMode != LM_FREE)
                    break;
            }
            else
            {
                if (hasChildren)
                {
                    if (element->IsInsideCombined(position, true))
                        GetElementAt(result, element, position, enabledOnly);
                }
                // Layout optimization: if position is much beyond the visible screen, check how many elements we can skip,
                // or if we already passed all visible elements
                else if (parentLayoutMode != LM_FREE)
                {
                    if (!i)
                    {
                        int screenPos = (parentLayoutMode == LM_HORIZONTAL) ? element->GetScreenPosition().x :
                            element->GetScreenPosition().y;
                        int layoutMaxSize = current->GetLayoutElementMaxSize();
                        int spacing = current->GetLayoutSpacing();

                        if (screenPos < 0 && layoutMaxSize > 0)
                        {
                            auto toSkip = (unsigned)(-screenPos / (layoutMaxSize + spacing));
                            if (toSkip > 0)
                                i += (toSkip - 1);
                        }
                    }
                    // Note: we cannot check for the up / left limits of positioning, since the element may be off the visible
                    // screen but some of its layouted children will yet be visible. In down & right directions we can terminate
                    // the loop, since all further children will be further down or right.
                    else if (parentLayoutMode == LM_HORIZONTAL)
                    {
                        if (element->GetScreenPosition().x >= rootElement_->GetPosition().x + rootElement_->GetSize().x)
                            break;
                    }
                    else if (parentLayoutMode == LM_VERTICAL)
                    {
                        if (element->GetScreenPosition().y >= rootElement_->GetPosition().y + rootElement_->GetSize().y)
                            break;
                    }
                }
            }
        }
    }
}

UiElement* UI::GetFocusableElement(UiElement* element)
{
    while (element)
    {
        if (element->GetFocusMode() != FM_NOTFOCUSABLE)
            break;
        element = element->GetParent();
    }
    return element;
}

void UI::GetCursorPositionAndVisible(IntVector2& pos, bool& visible)
{
    // Prefer software cursor then OS-specific cursor
    if (cursor_ && cursor_->IsVisible())
    {
        pos = cursor_->GetPosition();
        visible = true;
    }
    else if (DV_INPUT.GetMouseMode() == MM_RELATIVE)
    {
        visible = true;
    }
    else
    {
        visible = DV_INPUT.IsMouseVisible();

        if (!visible && cursor_)
        {
            pos = cursor_->GetPosition();
        }
        else
        {
            pos = DV_INPUT.GetMousePosition();
            pos = ConvertSystemToUI(pos);
        }
    }
}

void UI::SetCursorShape(CursorShape shape)
{
    if (cursor_)
        cursor_->SetShape(shape);
}

void UI::ReleaseFontFaces()
{
    DV_LOGDEBUG("Reloading font faces");

    Vector<Font*> fonts;
    DV_RES_CACHE.GetResources<Font>(fonts);

    for (Font* font : fonts)
        font->ReleaseFaces();
}

void UI::ProcessHover(const IntVector2& windowCursorPos, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor)
{
    IntVector2 cursorPos;
    WeakPtr<UiElement> element(GetElementAt(windowCursorPos, true, &cursorPos));

    for (HashMap<WeakPtr<UiElement>, UI::DragData*>::Iterator i = dragElements_.Begin(); i != dragElements_.End();)
    {
        WeakPtr<UiElement> dragElement = i->first_;
        UI::DragData* dragData = i->second_;

        if (!dragElement)
        {
            i = DragElementErase(i);
            continue;
        }

        bool dragSource = dragElement && (dragElement->GetDragDropMode() & DD_SOURCE);
        bool dragTarget = element && (element->GetDragDropMode() & DD_TARGET);
        bool dragDropTest = dragSource && dragTarget && element != dragElement;
        // If drag start event has not been posted yet, do not do drag handling here
        if (dragData->dragBeginPending)
            dragSource = dragTarget = dragDropTest = false;

        // Hover effect
        // If a drag is going on, transmit hover only to the element being dragged, unless it's a drop target
        if (element && element->IsEnabled())
        {
            if (dragElement == element || dragDropTest)
            {
                element->OnHover(element->screen_to_element(cursorPos), cursorPos, buttons, qualifiers, cursor);

                // Begin hover event
                if (!hoveredElements_.Contains(element))
                {
                    SendDragOrHoverEvent(E_HOVERBEGIN, element, cursorPos, IntVector2::ZERO, nullptr);
                    // Exit if element is destroyed by the event handling
                    if (!element)
                        return;
                }
                hoveredElements_[element] = true;
            }
        }

        // Drag and drop test
        if (dragDropTest)
        {
            bool accept = element->OnDragDropTest(dragElement);
            if (accept)
            {
                using namespace DragDropTest;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_SOURCE] = dragElement.Get();
                eventData[P_TARGET] = element.Get();
                eventData[P_ACCEPT] = accept;
                SendEvent(E_DRAGDROPTEST, eventData);
                accept = eventData[P_ACCEPT].GetBool();
            }

            if (cursor)
                cursor->SetShape(accept ? CS_ACCEPTDROP : CS_REJECTDROP);
        }
        else if (dragSource && cursor)
            cursor->SetShape(dragElement == element ? CS_ACCEPTDROP : CS_REJECTDROP);

        ++i;
    }

    // Hover effect
    // If no drag is going on, transmit hover event.
    if (element && element->IsEnabled())
    {
        if (dragElementsCount_ == 0)
        {
            element->OnHover(element->screen_to_element(cursorPos), cursorPos, buttons, qualifiers, cursor);

            // Begin hover event
            if (!hoveredElements_.Contains(element))
            {
                SendDragOrHoverEvent(E_HOVERBEGIN, element, cursorPos, IntVector2::ZERO, nullptr);
                // Exit if element is destroyed by the event handling
                if (!element)
                    return;
            }
            hoveredElements_[element] = true;
        }
    }
}

void UI::ProcessClickBegin(const IntVector2& windowCursorPos, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor, bool cursorVisible)
{
    if (cursorVisible)
    {
        IntVector2 cursorPos;
        WeakPtr<UiElement> element(GetElementAt(windowCursorPos, true, &cursorPos));

        bool newButton;
        if (usingTouchInput_)
            newButton = (buttons & button) == MOUSEB_NONE;
        else
            newButton = true;
        buttons |= button;

        if (element)
            SetFocusElement(element);

        // Focus change events may destroy the element, check again.
        if (element)
        {
            // Handle focusing & bringing to front
            element->BringToFront();

            // Handle click
            element->OnClickBegin(element->screen_to_element(cursorPos), cursorPos, button, buttons, qualifiers, cursor);
            SendClickEvent(E_UIMOUSECLICK, nullptr, element, cursorPos, button, buttons, qualifiers);

            // Fire double click event if element matches and is in time and is within max distance from the original click
            if (doubleClickElement_ && element == doubleClickElement_ &&
                (clickTimer_.GetMSec(true) < (unsigned)(doubleClickInterval_ * 1000)) && lastMouseButtons_ == buttons && (windowCursorPos - doubleClickFirstPos_).Length() < maxDoubleClickDist_)
            {
                element->OnDoubleClick(element->screen_to_element(cursorPos), cursorPos, button, buttons, qualifiers, cursor);
                doubleClickElement_.Reset();
                SendDoubleClickEvent(nullptr, element, doubleClickFirstPos_, cursorPos, button, buttons, qualifiers);
            }
            else
            {
                doubleClickElement_ = element;
                doubleClickFirstPos_ = windowCursorPos;
                clickTimer_.Reset();
            }

            // Handle start of drag. Click handling may have caused destruction of the element, so check the pointer again
            bool dragElementsContain = dragElements_.Contains(element);
            if (element && !dragElementsContain)
            {
                auto* dragData = new DragData();
                dragElements_[element] = dragData;
                dragData->dragBeginPending = true;
                dragData->sumPos = cursorPos;
                dragData->dragBeginSumPos = cursorPos;
                dragData->dragBeginTimer.Reset();
                dragData->dragButtons = button;
                dragData->numDragButtons = CountSetBits((u32)dragData->dragButtons);
                dragElementsCount_++;

                dragElementsContain = dragElements_.Contains(element);
            }
            else if (element && dragElementsContain && newButton)
            {
                DragData* dragData = dragElements_[element];
                dragData->sumPos += cursorPos;
                dragData->dragBeginSumPos += cursorPos;
                dragData->dragButtons |= button;
                dragData->numDragButtons = CountSetBits((u32)dragData->dragButtons);
            }
        }
        else
        {
            // If clicked over no element, or a disabled element, lose focus (but not if there is a modal element)
            if (!HasModalElement())
                SetFocusElement(nullptr);
            SendClickEvent(E_UIMOUSECLICK, nullptr, element, cursorPos, button, buttons, qualifiers);

            if (clickTimer_.GetMSec(true) < (unsigned)(doubleClickInterval_ * 1000) && lastMouseButtons_ == buttons && (windowCursorPos - doubleClickFirstPos_).Length() < maxDoubleClickDist_)
                SendDoubleClickEvent(nullptr, element, doubleClickFirstPos_, cursorPos, button, buttons, qualifiers);
        }

        lastMouseButtons_ = buttons;
    }
}

void UI::ProcessClickEnd(const IntVector2& windowCursorPos, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor, bool cursorVisible)
{
    WeakPtr<UiElement> element;
    IntVector2 cursorPos = windowCursorPos;
    if (cursorVisible)
        element = GetElementAt(cursorPos, true, &cursorPos);

    // Handle end of drag
    for (HashMap<WeakPtr<UiElement>, UI::DragData*>::Iterator i = dragElements_.Begin(); i != dragElements_.End();)
    {
        WeakPtr<UiElement> dragElement = i->first_;
        UI::DragData* dragData = i->second_;

        if (!dragElement || !cursorVisible)
        {
            i = DragElementErase(i);
            continue;
        }

        if (dragData->dragButtons & button)
        {
            // Handle end of click
            if (element)
                element->OnClickEnd(element->screen_to_element(cursorPos), cursorPos, button, buttons, qualifiers, cursor,
                    dragElement);

            SendClickEvent(E_UIMOUSECLICKEND, dragElement, element, cursorPos, button, buttons, qualifiers);

            if (dragElement && dragElement->IsEnabled() && dragElement->IsVisible() && !dragData->dragBeginPending)
            {
                dragElement->OnDragEnd(dragElement->screen_to_element(cursorPos), cursorPos, dragData->dragButtons, buttons,
                    cursor);
                SendDragOrHoverEvent(E_DRAGEND, dragElement, cursorPos, IntVector2::ZERO, dragData);

                bool dragSource = dragElement && (dragElement->GetDragDropMode() & DD_SOURCE);
                if (dragSource)
                {
                    bool dragTarget = element && (element->GetDragDropMode() & DD_TARGET);
                    bool dragDropFinish = dragSource && dragTarget && element != dragElement;

                    if (dragDropFinish)
                    {
                        bool accept = element->OnDragDropFinish(dragElement);

                        // OnDragDropFinish() may have caused destruction of the elements, so check the pointers again
                        if (accept && dragElement && element)
                        {
                            using namespace DragDropFinish;

                            VariantMap& eventData = GetEventDataMap();
                            eventData[P_SOURCE] = dragElement.Get();
                            eventData[P_TARGET] = element.Get();
                            eventData[P_ACCEPT] = accept;
                            SendEvent(E_DRAGDROPFINISH, eventData);
                        }
                    }
                }
            }

            i = DragElementErase(i);
        }
        else
            ++i;
    }
}

void UI::ProcessMove(const IntVector2& windowCursorPos, const IntVector2& cursorDeltaPos, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor,
    bool cursorVisible)
{
    if (cursorVisible && dragElementsCount_ > 0 && buttons)
    {
        IntVector2 cursorPos;
        GetElementAt(windowCursorPos, true, &cursorPos);

        bool mouseGrabbed = DV_INPUT.IsMouseGrabbed();
        for (HashMap<WeakPtr<UiElement>, UI::DragData*>::Iterator i = dragElements_.Begin(); i != dragElements_.End();)
        {
            WeakPtr<UiElement> dragElement = i->first_;
            UI::DragData* dragData = i->second_;

            if (!dragElement)
            {
                i = DragElementErase(i);
                continue;
            }

            if (!(dragData->dragButtons & buttons))
            {
                ++i;
                continue;
            }

            // Calculate the position that we should send for this drag event.
            IntVector2 sendPos;
            if (usingTouchInput_)
            {
                dragData->sumPos += cursorDeltaPos;
                sendPos.x = dragData->sumPos.x / dragData->numDragButtons;
                sendPos.y = dragData->sumPos.y / dragData->numDragButtons;
            }
            else
            {
                dragData->sumPos = cursorPos;
                sendPos = cursorPos;
            }

            if (dragElement->IsEnabled() && dragElement->IsVisible())
            {
                // Signal drag begin if distance threshold was exceeded

                if (dragData->dragBeginPending && !mouseGrabbed)
                {
                    IntVector2 beginSendPos;
                    beginSendPos.x = dragData->dragBeginSumPos.x / dragData->numDragButtons;
                    beginSendPos.y = dragData->dragBeginSumPos.y / dragData->numDragButtons;

                    IntVector2 offset = cursorPos - beginSendPos;
                    if (Abs(offset.x) >= dragBeginDistance_ || Abs(offset.y) >= dragBeginDistance_)
                    {
                        dragData->dragBeginPending = false;
                        dragConfirmedCount_++;
                        dragElement->OnDragBegin(dragElement->screen_to_element(beginSendPos), beginSendPos, buttons, qualifiers,
                            cursor);
                        SendDragOrHoverEvent(E_DRAGBEGIN, dragElement, beginSendPos, IntVector2::ZERO, dragData);
                    }
                }

                if (!dragData->dragBeginPending)
                {
                    dragElement->OnDragMove(dragElement->screen_to_element(sendPos), sendPos, cursorDeltaPos, buttons, qualifiers,
                        cursor);
                    SendDragOrHoverEvent(E_DRAGMOVE, dragElement, sendPos, cursorDeltaPos, dragData);
                }
            }
            else
            {
                dragElement->OnDragEnd(dragElement->screen_to_element(sendPos), sendPos, dragData->dragButtons, buttons, cursor);
                SendDragOrHoverEvent(E_DRAGEND, dragElement, sendPos, IntVector2::ZERO, dragData);
                dragElement.Reset();
            }

            ++i;
        }
    }
}

void UI::SendDragOrHoverEvent(StringHash eventType, UiElement* element, const IntVector2& screenPos, const IntVector2& deltaPos,
    UI::DragData* dragData)
{
    if (!element)
        return;

    IntVector2 relativePos = element->screen_to_element(screenPos);

    using namespace DragMove;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_ELEMENT] = element;
    eventData[P_X] = screenPos.x;
    eventData[P_Y] = screenPos.y;
    eventData[P_ELEMENTX] = relativePos.x;
    eventData[P_ELEMENTY] = relativePos.y;

    if (eventType == E_DRAGMOVE)
    {
        eventData[P_DX] = deltaPos.x;
        eventData[P_DY] = deltaPos.y;
    }

    if (dragData)
    {
        eventData[P_BUTTONS] = (unsigned)dragData->dragButtons;
        eventData[P_NUMBUTTONS] = dragData->numDragButtons;
    }

    element->SendEvent(eventType, eventData);
}

void UI::SendClickEvent(StringHash eventType, UiElement* beginElement, UiElement* endElement, const IntVector2& pos, MouseButton button,
    MouseButtonFlags buttons, QualifierFlags qualifiers)
{
    VariantMap& eventData = GetEventDataMap();
    eventData[UIMouseClick::P_ELEMENT] = endElement;
    eventData[UIMouseClick::P_X] = pos.x;
    eventData[UIMouseClick::P_Y] = pos.y;
    eventData[UIMouseClick::P_BUTTON] = button;
    eventData[UIMouseClick::P_BUTTONS] = (unsigned)buttons;
    eventData[UIMouseClick::P_QUALIFIERS] = (unsigned)qualifiers;

    // For click end events, send also the element the click began on
    if (eventType == E_UIMOUSECLICKEND)
        eventData[UIMouseClickEnd::P_BEGINELEMENT] = beginElement;

    if (endElement)
    {
        // Send also element version of the event
        if (eventType == E_UIMOUSECLICK)
            endElement->SendEvent(E_CLICK, eventData);
        else if (eventType == E_UIMOUSECLICKEND)
            endElement->SendEvent(E_CLICKEND, eventData);
    }

    // Send the global event from the UI subsystem last
    SendEvent(eventType, eventData);
}

void UI::SendDoubleClickEvent(UiElement* beginElement, UiElement* endElement, const IntVector2& firstPos, const IntVector2& secondPos, MouseButton button,
    MouseButtonFlags buttons, QualifierFlags qualifiers)
{
    VariantMap& eventData = GetEventDataMap();
    eventData[UIMouseDoubleClick::P_ELEMENT] = endElement;
    eventData[UIMouseDoubleClick::P_X] = secondPos.x;
    eventData[UIMouseDoubleClick::P_Y] = secondPos.y;
    eventData[UIMouseDoubleClick::P_XBEGIN] = firstPos.x;
    eventData[UIMouseDoubleClick::P_YBEGIN] = firstPos.y;
    eventData[UIMouseDoubleClick::P_BUTTON] = button;
    eventData[UIMouseDoubleClick::P_BUTTONS] = (unsigned)buttons;
    eventData[UIMouseDoubleClick::P_QUALIFIERS] = (unsigned)qualifiers;


    if (endElement)
    {
        // Send also element version of the event
        endElement->SendEvent(E_DOUBLECLICK, eventData);
    }

    // Send the global event from the UI subsystem last
    SendEvent(E_UIMOUSEDOUBLECLICK, eventData);
}


void UI::HandleScreenMode(StringHash eventType, VariantMap& eventData)
{
    using namespace ScreenMode;

    if (!initialized_)
        Initialize();
    else
        ResizeRootElement();
}

void UI::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
    using namespace MouseButtonDown;

    mouseButtons_ = MouseButtonFlags(eventData[P_BUTTONS].GetU32());
    qualifiers_ = QualifierFlags(eventData[P_QUALIFIERS].GetU32());
    usingTouchInput_ = false;

    IntVector2 cursorPos;
    bool cursorVisible;
    GetCursorPositionAndVisible(cursorPos, cursorVisible);

    // Handle drag cancelling
    ProcessDragCancel();

    if (!DV_INPUT.IsMouseGrabbed())
        ProcessClickBegin(cursorPos, MouseButton(eventData[P_BUTTON].GetU32()), mouseButtons_, qualifiers_, cursor_, cursorVisible);
}

void UI::HandleMouseButtonUp(StringHash eventType, VariantMap& eventData)
{
    using namespace MouseButtonUp;

    mouseButtons_ = MouseButtonFlags(eventData[P_BUTTONS].GetU32());
    qualifiers_ = QualifierFlags(eventData[P_QUALIFIERS].GetU32());

    IntVector2 cursorPos;
    bool cursorVisible;
    GetCursorPositionAndVisible(cursorPos, cursorVisible);

    ProcessClickEnd(cursorPos, (MouseButton)eventData[P_BUTTON].GetU32(), mouseButtons_, qualifiers_, cursor_, cursorVisible);
}

void UI::HandleMouseMove(StringHash eventType, VariantMap& eventData)
{
    using namespace MouseMove;

    mouseButtons_ = MouseButtonFlags(eventData[P_BUTTONS].GetU32());
    qualifiers_ = QualifierFlags(eventData[P_QUALIFIERS].GetU32());
    usingTouchInput_ = false;

    const IntVector2& rootSize = rootElement_->GetSize();
    const IntVector2& rootPos = rootElement_->GetPosition();

    const IntVector2 mouseDeltaPos{ eventData[P_DX].GetI32(), eventData[P_DY].GetI32() };
    const IntVector2 mousePos{ eventData[P_X].GetI32(), eventData[P_Y].GetI32() };

    if (cursor_)
    {
        if (!DV_INPUT.IsMouseVisible())
        {
            if (!DV_INPUT.IsMouseLocked())
                cursor_->SetPosition(ConvertSystemToUI(mousePos));
            else if (cursor_->IsVisible())
            {
                // Relative mouse motion: move cursor only when visible
                IntVector2 pos = cursor_->GetPosition();
                pos += ConvertSystemToUI(mouseDeltaPos);
                pos.x = Clamp(pos.x, rootPos.x, rootPos.x + rootSize.x - 1);
                pos.y = Clamp(pos.y, rootPos.y, rootPos.y + rootSize.y - 1);
                cursor_->SetPosition(pos);
            }
        }
        else
        {
            // Absolute mouse motion: move always
            cursor_->SetPosition(ConvertSystemToUI(mousePos));
        }
    }

    IntVector2 cursorPos;
    bool cursorVisible;
    GetCursorPositionAndVisible(cursorPos, cursorVisible);

    ProcessMove(cursorPos, mouseDeltaPos, mouseButtons_, qualifiers_, cursor_, cursorVisible);
}

void UI::HandleMouseWheel(StringHash eventType, VariantMap& eventData)
{
    if (DV_INPUT.IsMouseGrabbed())
        return;

    using namespace MouseWheel;

    mouseButtons_ = MouseButtonFlags(eventData[P_BUTTONS].GetI32());
    qualifiers_ = QualifierFlags(eventData[P_QUALIFIERS].GetI32());
    int delta = eventData[P_WHEEL].GetI32();
    usingTouchInput_ = false;

    IntVector2 cursorPos;
    bool cursorVisible;
    GetCursorPositionAndVisible(cursorPos, cursorVisible);

    if (!nonFocusedMouseWheel_ && focusElement_)
        focusElement_->OnWheel(delta, mouseButtons_, qualifiers_);
    else
    {
        // If no element has actual focus or in non-focused mode, get the element at cursor
        if (cursorVisible)
        {
            UiElement* element = GetElementAt(cursorPos);
            if (nonFocusedMouseWheel_)
            {
                // Going up the hierarchy chain to find element that could handle mouse wheel
                while (element && !element->IsWheelHandler())
                {
                    element = element->GetParent();
                }
            }
            else
                // If the element itself is not focusable, search for a focusable parent,
                // although the focusable element may not actually handle mouse wheel
                element = GetFocusableElement(element);

            if (element && (nonFocusedMouseWheel_ || element->GetFocusMode() >= FM_FOCUSABLE))
                element->OnWheel(delta, mouseButtons_, qualifiers_);
        }
    }
}

void UI::HandleTouchBegin(StringHash eventType, VariantMap& eventData)
{
    if (DV_INPUT.IsMouseGrabbed())
        return;

    using namespace TouchBegin;

    IntVector2 pos(eventData[P_X].GetI32(), eventData[P_Y].GetI32());
    pos = ConvertSystemToUI(pos);
    usingTouchInput_ = true;

    const MouseButton touchId = MakeTouchIDMask(eventData[P_TOUCHID].GetI32());
    WeakPtr<UiElement> element(GetElementAt(pos));

    if (element)
    {
        ProcessClickBegin(pos, touchId, touchDragElements_[element], QUAL_NONE, nullptr, true);
        touchDragElements_[element] |= touchId;
    }
    else
        ProcessClickBegin(pos, touchId, touchId, QUAL_NONE, nullptr, true);
}

void UI::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
{
    using namespace TouchEnd;

    IntVector2 pos(eventData[P_X].GetI32(), eventData[P_Y].GetI32());
    pos = ConvertSystemToUI(pos);

    // Get the touch index
    const MouseButton touchId = MakeTouchIDMask(eventData[P_TOUCHID].GetI32());

    // Transmit hover end to the position where the finger was lifted
    WeakPtr<UiElement> element(GetElementAt(pos));

    // Clear any drag events that were using the touch id
    for (auto i = touchDragElements_.Begin(); i != touchDragElements_.End();)
    {
        const MouseButtonFlags touches = i->second_;
        if (touches & touchId)
            i = touchDragElements_.Erase(i);
        else
            ++i;
    }

    if (element && element->IsEnabled())
        element->OnHover(element->screen_to_element(pos), pos, MOUSEB_NONE, QUAL_NONE, nullptr);

    ProcessClickEnd(pos, touchId, MOUSEB_NONE, QUAL_NONE, nullptr, true);
}

void UI::HandleTouchMove(StringHash eventType, VariantMap& eventData)
{
    using namespace TouchMove;

    IntVector2 pos(eventData[P_X].GetI32(), eventData[P_Y].GetI32());
    IntVector2 deltaPos(eventData[P_DX].GetI32(), eventData[P_DY].GetI32());
    pos = ConvertSystemToUI(pos);
    deltaPos = ConvertSystemToUI(deltaPos);
    usingTouchInput_ = true;

    const MouseButton touchId = MakeTouchIDMask(eventData[P_TOUCHID].GetI32());

    ProcessMove(pos, deltaPos, touchId, QUAL_NONE, nullptr, true);
}

void UI::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyDown;

    mouseButtons_ = MouseButtonFlags(eventData[P_BUTTONS].GetU32());
    qualifiers_ = QualifierFlags(eventData[P_QUALIFIERS].GetU32());
    auto key = (Key)eventData[P_KEY].GetU32();

    // Cancel UI dragging
    if (key == KEY_ESCAPE && dragElementsCount_ > 0)
    {
        ProcessDragCancel();

        return;
    }

    // Dismiss modal element if any when ESC key is pressed
    if (key == KEY_ESCAPE && HasModalElement())
    {
        UiElement* element = rootModalElement_->GetChild(rootModalElement_->GetNumChildren() - 1);
        if (element->GetVars().Contains(VAR_ORIGIN))
            // If it is a popup, dismiss by defocusing it
            SetFocusElement(nullptr);
        else
        {
            // If it is a modal window, by resetting its modal flag
            auto* window = dynamic_cast<Window*>(element);
            if (window && window->GetModalAutoDismiss())
                window->SetModal(false);
        }

        return;
    }

    UiElement* element = focusElement_;
    if (element)
    {
        // Switch focus between focusable elements in the same top level window
        if (key == KEY_TAB)
        {
            UiElement* topLevel = element->GetParent();
            while (topLevel && topLevel->GetParent() != rootElement_ && topLevel->GetParent() != rootModalElement_)
                topLevel = topLevel->GetParent();
            if (topLevel)
            {
                topLevel->GetChildren(tempElements_, true);
                for (Vector<UiElement*>::Iterator i = tempElements_.Begin(); i != tempElements_.End();)
                {
                    if ((*i)->GetFocusMode() < FM_FOCUSABLE)
                        i = tempElements_.Erase(i);
                    else
                        ++i;
                }
                for (unsigned i = 0; i < tempElements_.Size(); ++i)
                {
                    if (tempElements_[i] == element)
                    {
                        int dir = (qualifiers_ & QUAL_SHIFT) ? -1 : 1;
                        unsigned nextIndex = (tempElements_.Size() + i + dir) % tempElements_.Size();
                        UiElement* next = tempElements_[nextIndex];
                        SetFocusElement(next, true);
                        return;
                    }
                }
            }
        }
        // Defocus the element
        else if (key == KEY_ESCAPE && element->GetFocusMode() == FM_FOCUSABLE_DEFOCUSABLE)
            element->SetFocus(false);
        // If none of the special keys, pass the key to the focused element
        else
            element->OnKey(key, mouseButtons_, qualifiers_);
    }
}

void UI::HandleTextInput(StringHash eventType, VariantMap& eventData)
{
    using namespace TextInput;

    UiElement* element = focusElement_;
    if (element)
        element->OnTextInput(eventData[P_TEXT].GetString());
}

void UI::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    // If have a cursor, and a drag is not going on, reset the cursor shape. Application logic that wants to apply
    // custom shapes can do it after this, but needs to do it each frame
    if (cursor_ && dragElementsCount_ == 0)
        cursor_->SetShape(CS_NORMAL);
}

void UI::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace PostUpdate;

    Update(eventData[P_TIMESTEP].GetFloat());
}

void UI::HandleRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    RenderUpdate();
}

void UI::HandleDropFile(StringHash eventType, VariantMap& eventData)
{
    // Sending the UI variant of the event only makes sense if the OS cursor is visible (not locked to window center)
    if (DV_INPUT.IsMouseVisible())
    {
        IntVector2 screenPos = DV_INPUT.GetMousePosition();
        screenPos = ConvertSystemToUI(screenPos);

        UiElement* element = GetElementAt(screenPos);

        using namespace UIDropFile;

        VariantMap uiEventData;
        uiEventData[P_FILENAME] = eventData[P_FILENAME];
        uiEventData[P_X] = screenPos.x;
        uiEventData[P_Y] = screenPos.y;
        uiEventData[P_ELEMENT] = element;

        if (element)
        {
            IntVector2 relativePos = element->screen_to_element(screenPos);
            uiEventData[P_ELEMENTX] = relativePos.x;
            uiEventData[P_ELEMENTY] = relativePos.y;
        }

        SendEvent(E_UIDROPFILE, uiEventData);
    }
}

HashMap<WeakPtr<UiElement>, UI::DragData*>::Iterator UI::DragElementErase(HashMap<WeakPtr<UiElement>, UI::DragData*>::Iterator i)
{
    // If running the engine frame in response to an event (re-entering UI frame logic) the dragElements_ may already be empty
    if (dragElements_.Empty())
        return dragElements_.End();

    dragElementsConfirmed_.Clear();

    DragData* dragData = i->second_;

    if (!dragData->dragBeginPending)
        --dragConfirmedCount_;
    i = dragElements_.Erase(i);
    --dragElementsCount_;

    delete dragData;
    return i;
}

void UI::ProcessDragCancel()
{
    // How to tell difference between drag cancel and new selection on multi-touch?
    if (usingTouchInput_)
        return;

    IntVector2 cursorPos;
    bool cursorVisible;
    GetCursorPositionAndVisible(cursorPos, cursorVisible);

    for (HashMap<WeakPtr<UiElement>, UI::DragData*>::Iterator i = dragElements_.Begin(); i != dragElements_.End();)
    {
        WeakPtr<UiElement> dragElement = i->first_;
        UI::DragData* dragData = i->second_;

        if (dragElement && dragElement->IsEnabled() && dragElement->IsVisible() && !dragData->dragBeginPending)
        {
            dragElement->OnDragCancel(dragElement->screen_to_element(cursorPos), cursorPos, dragData->dragButtons, mouseButtons_,
                cursor_);
            SendDragOrHoverEvent(E_DRAGCANCEL, dragElement, cursorPos, IntVector2::ZERO, dragData);
            i = DragElementErase(i);
        }
        else
            ++i;
    }
}

IntVector2 UI::SumTouchPositions(UI::DragData* dragData, const IntVector2& oldSendPos)
{
    IntVector2 sendPos = oldSendPos;
    if (usingTouchInput_)
    {
        MouseButtonFlags buttons = dragData->dragButtons;
        dragData->sumPos = IntVector2::ZERO;
        for (unsigned i = 0; (1u << i) <= (unsigned)buttons; i++)
        {
            auto mouseButton = static_cast<MouseButton>(1u << i); // NOLINT(misc-misplaced-widening-cast)
            if (buttons & mouseButton)
            {
                TouchState* ts = DV_INPUT.GetTouch((unsigned)i);
                if (!ts)
                    break;
                IntVector2 pos = ts->position_;
                pos = ConvertSystemToUI(pos);
                dragData->sumPos += pos;
            }
        }
        sendPos.x = dragData->sumPos.x / dragData->numDragButtons;
        sendPos.y = dragData->sumPos.y / dragData->numDragButtons;
    }
    return sendPos;
}

void UI::ResizeRootElement()
{
    IntVector2 effectiveSize = GetEffectiveRootElementSize();
    rootElement_->SetSize(effectiveSize);
    rootModalElement_->SetSize(effectiveSize);
}

IntVector2 UI::GetEffectiveRootElementSize(bool applyScale) const
{
    // Use a fake size in headless mode
    IntVector2 size = !GParams::is_headless() ? IntVector2(DV_GRAPHICS.GetWidth(), DV_GRAPHICS.GetHeight()) : IntVector2(1024, 768);
    if (customSize_.x > 0 && customSize_.y > 0)
        size = customSize_;

    if (applyScale)
    {
        size.x = RoundToInt((float)size.x / uiScale_);
        size.y = RoundToInt((float)size.y / uiScale_);
    }

    return size;
}

void UI::SetElementRenderTexture(UiElement* element, Texture2D* texture)
{
    if (element == nullptr)
    {
        DV_LOGERROR("UI::SetElementRenderTexture called with null element.");
        return;
    }

    auto it = renderToTexture_.Find(element);
    if (texture && it == renderToTexture_.End())
    {
        RenderToTextureData data;
        data.texture_ = texture;
        data.rootElement_ = element;
        data.vertexBuffer_ = new VertexBuffer();
        data.debugVertexBuffer_ = new VertexBuffer();
        renderToTexture_[element] = data;
    }
    else if (it != renderToTexture_.End())
    {
        if (texture == nullptr)
            renderToTexture_.Erase(it);
        else
            it->second_.texture_ = texture;
    }
}

void register_ui_library()
{
    Font::register_object();

    UiElement::register_object();
    UISelectable::register_object();
    BorderImage::register_object();
    Sprite::register_object();
    Button::register_object();
    CheckBox::register_object();
    Cursor::register_object();
    Text::register_object();
    Text3D::register_object();
    Window::register_object();
    View3D::register_object();
    LineEdit::register_object();
    Slider::register_object();
    ScrollBar::register_object();
    ScrollView::register_object();
    ListView::register_object();
    Menu::register_object();
    DropDownList::register_object();
    FileSelector::register_object();
    MessageBox::register_object();
    ProgressBar::register_object();
    ToolTip::register_object();
    UiComponent::register_object();
}

}
