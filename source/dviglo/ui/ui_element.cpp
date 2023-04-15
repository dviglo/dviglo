// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../core/core_events.h"
#include "../containers/hash_set.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "../scene/object_animation.h"
#include "cursor.h"
#include "ui.h"
#include "ui_element.h"
#include "ui_events.h"

#include "../common/debug_new.h"

using namespace std;

namespace dviglo
{

const char* horizontalAlignments[] =
{
    "Left",
    "Center",
    "Right",
    "Custom",
    nullptr
};

const char* verticalAlignments[] =
{
    "Top",
    "Center",
    "Bottom",
    "Custom",
    nullptr
};

static const char* focusModes[] =
{
    "NotFocusable",
    "ResetFocus",
    "Focusable",
    "FocusableDefocusable",
    nullptr
};

static const char* dragDropModes[] =
{
    "Disabled",
    "Source",
    "Target",
    "SourceAndTarget",
    nullptr
};

static const char* layoutModes[] =
{
    "Free",
    "Horizontal",
    "Vertical",
    nullptr
};

extern const char* UI_CATEGORY;

static bool CompareUIElements(const UiElement* lhs, const UiElement* rhs)
{
    return lhs->GetPriority() < rhs->GetPriority();
}

XPathQuery UiElement::styleXPathQuery_("/elements/element[@type=$typeName]", "typeName:String");

UiElement::UiElement() :
    pivot_(numeric_limits<float>::max(), numeric_limits<float>::max())
{
    SetEnabled(false);
}

UiElement::~UiElement()
{
    // If child elements have outside references, detach them
    for (Vector<SharedPtr<UiElement>>::Iterator i = children_.Begin(); i < children_.End(); ++i)
    {
        if (i->Refs() > 1)
            (*i)->Detach();
    }
}

void UiElement::register_object()
{
    DV_CONTEXT.RegisterFactory<UiElement>(UI_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Name", GetName, SetName, String::EMPTY, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Position", GetPosition, SetPosition, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Min Size", GetMinSize, SetMinSize, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Max Size", GetMaxSize, SetMaxSize, IntVector2(M_MAX_INT, M_MAX_INT), AM_FILE);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Horiz Alignment", GetHorizontalAlignment, SetHorizontalAlignment,
        horizontalAlignments, HA_LEFT, AM_FILEREADONLY);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Vert Alignment", GetVerticalAlignment, SetVerticalAlignment, verticalAlignments,
        VA_TOP, AM_FILEREADONLY);
    DV_ACCESSOR_ATTRIBUTE("Min Anchor", GetMinAnchor, SetMinAnchor, Vector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Max Anchor", GetMaxAnchor, SetMaxAnchor, Vector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Min Offset", GetMinOffset, SetMinOffset, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Max Offset", GetMaxOffset, SetMaxOffset, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Pivot", GetPivot, SetPivot, Vector2(numeric_limits<float>::max(), numeric_limits<float>::max()), AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Enable Anchor", GetEnableAnchor, SetEnableAnchor, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Clip Border", GetClipBorder, SetClipBorder, IntRect::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Priority", GetPriority, SetPriority, 0, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Opacity", GetOpacity, SetOpacity, 1.0f, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Color", GetColorAttr, SetColor, Color::WHITE, AM_FILE);
    DV_ATTRIBUTE("Top Left Color", colors_[0], Color::WHITE, AM_FILE);
    DV_ATTRIBUTE("Top Right Color", colors_[1], Color::WHITE, AM_FILE);
    DV_ATTRIBUTE("Bottom Left Color", colors_[2], Color::WHITE, AM_FILE);
    DV_ATTRIBUTE("Bottom Right Color", colors_[3], Color::WHITE, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Is Editable", IsEditable, SetEditable, true, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Is Selected", IsSelected, SetSelected, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Is Visible", IsVisible, SetVisible, true, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Bring To Front", GetBringToFront, SetBringToFront, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Bring To Back", GetBringToBack, SetBringToBack, true, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Clip Children", GetClipChildren, SetClipChildren, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Use Derived Opacity", GetUseDerivedOpacity, SetUseDerivedOpacity, true, AM_FILE);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Focus Mode", GetFocusMode, SetFocusMode, focusModes, FM_NOTFOCUSABLE, AM_FILE);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Drag And Drop Mode", GetDragDropMode, SetDragDropMode, dragDropModes, DD_DISABLED, AM_FILE);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Layout Mode", GetLayoutMode, SetLayoutMode, layoutModes, LM_FREE, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Layout Spacing", GetLayoutSpacing, SetLayoutSpacing, 0, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Layout Border", GetLayoutBorder, SetLayoutBorder, IntRect::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Layout Flex Scale", GetLayoutFlexScale, SetLayoutFlexScale, Vector2::ONE, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Indent", GetIndent, SetIndent, 0, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Indent Spacing", GetIndentSpacing, SetIndentSpacing, 16, AM_FILE);
    DV_ATTRIBUTE("Variables", vars_, Variant::emptyVariantMap, AM_FILE);
    DV_ATTRIBUTE("Tags", tags_, Variant::emptyStringVector, AM_FILE);
}

void UiElement::apply_attributes()
{
    colorGradient_ = false;
    derivedColorDirty_ = true;

    for (i32 i = 1; i < MAX_UIELEMENT_CORNERS; ++i)
    {
        if (colors_[i] != colors_[0])
            colorGradient_ = true;
    }
}

bool UiElement::load_xml(const XmlElement& source)
{
    return load_xml(source, nullptr);
}

bool UiElement::load_xml(const XmlElement& source, XmlFile* styleFile)
{
    // Get style override if defined
    String styleName = source.GetAttribute("style");

    // Apply the style first, if the style file is available
    if (styleFile)
    {
        // If not defined, use type name
        if (styleName.Empty())
            styleName = GetTypeName();

        SetStyle(styleName, styleFile);
    }
    // The 'style' attribute value in the style file cannot be equals to original's applied style to prevent infinite loop
    else if (!styleName.Empty() && styleName != appliedStyle_)
    {
        // Attempt to use the default style file
        styleFile = GetDefaultStyle();

        if (styleFile)
        {
            // Remember the original applied style
            String appliedStyle(appliedStyle_);
            SetStyle(styleName, styleFile);
            appliedStyle_ = appliedStyle;
        }
    }

    // Prevent updates while loading attributes
    DisableLayoutUpdate();

    // Then load rest of the attributes from the source
    if (!Animatable::load_xml(source))
        return false;

    i32 nextInternalChild = 0;

    // Load child elements. Internal elements are not to be created as they already exist
    XmlElement childElem = source.GetChild("element");
    while (childElem)
    {
        bool internalElem = childElem.GetBool("internal");
        String typeName = childElem.GetAttribute("type");
        if (typeName.Empty())
            typeName = "UiElement";
        i32 index = childElem.HasAttribute("index") ? childElem.GetI32("index") : ENDPOS;
        UiElement* child = nullptr;

        if (!internalElem)
        {
            child = create_child(typeName, String::EMPTY, index);
        }
        else
        {
            for (i32 i = nextInternalChild; i < children_.Size(); ++i)
            {
                if (children_[i]->IsInternal() && children_[i]->GetTypeName() == typeName)
                {
                    child = children_[i];
                    nextInternalChild = i + 1;
                    break;
                }
            }

            if (!child)
                DV_LOGWARNING("Could not find matching internal child element of type " + typeName + " in " + GetTypeName());
        }

        if (child)
        {
            if (!styleFile)
                styleFile = GetDefaultStyle();
            if (!child->load_xml(childElem, styleFile))
                return false;
        }

        childElem = childElem.GetNext("element");
    }

    apply_attributes();

    EnableLayoutUpdate();
    UpdateLayout();

    return true;
}

UiElement* UiElement::LoadChildXML(const XmlElement& childElem, XmlFile* styleFile)
{
    bool internalElem = childElem.GetBool("internal");
    if (internalElem)
    {
        DV_LOGERROR("Loading internal child element is not supported");
        return nullptr;
    }

    String typeName = childElem.GetAttribute("type");
    if (typeName.Empty())
        typeName = "UiElement";
    i32 index = childElem.HasAttribute("index") ? childElem.GetU32("index") : ENDPOS;
    UiElement* child = create_child(typeName, String::EMPTY, index);

    if (child)
    {
        if (!styleFile)
            styleFile = GetDefaultStyle();
        if (!child->load_xml(childElem, styleFile))
        {
            RemoveChild(child, index);
            return nullptr;
        }
    }

    return child;
}

bool UiElement::save_xml(XmlElement& dest) const
{
    // Write type
    if (GetTypeName() != "UiElement")
    {
        if (!dest.SetString("type", GetTypeName()))
            return false;
    }

    // Write internal flag
    if (internal_)
    {
        if (!dest.SetBool("internal", internal_))
            return false;
    }

    // Write style
    if (!appliedStyle_.Empty() && appliedStyle_ != "UiElement")
    {
        if (!dest.SetAttribute("style", appliedStyle_))
            return false;
    }
    else if (internal_)
    {
        if (!dest.SetAttribute("style", "none"))
            return false;
    }

    // Write attributes
    if (!Animatable::save_xml(dest))
        return false;

    // Write child elements
    for (const SharedPtr<UiElement>& element : children_)
    {
        if (element->IsTemporary())
            continue;

        XmlElement childElem = dest.create_child("element");
        if (!element->save_xml(childElem))
            return false;
    }

    // Filter UI-style and implicit attributes
    return FilterAttributes(dest);
}

void UiElement::Update(float timeStep)
{
}

void UiElement::GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor)
{
    // Reset hovering for next frame
    hovering_ = false;
}

void UiElement::GetDebugDrawBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor)
{
    UIBatch batch(this, BLEND_ALPHA, currentScissor, nullptr, &vertexData);

    int horizontalThickness = 1;
    int verticalThickness = 1;
    if (parent_)
    {
        switch (parent_->layoutMode_)
        {
        case LM_HORIZONTAL:
            verticalThickness += 2;
            break;

        case LM_VERTICAL:
            horizontalThickness += 2;
            break;

        default:
            break;
        }
    }

    batch.SetColor(Color::BLUE, true);
    // Left
    batch.add_quad(0, 0, horizontalThickness, size_.y, 0, 0);
    // Top
    batch.add_quad(0, 0, size_.x, verticalThickness, 0, 0);
    // Right
    batch.add_quad(size_.x - horizontalThickness, 0, horizontalThickness, size_.y, 0, 0);
    // Bottom
    batch.add_quad(0, size_.y - verticalThickness, size_.x, verticalThickness, 0, 0);

    UIBatch::AddOrMerge(batch, batches);
}

bool UiElement::IsWithinScissor(const IntRect& currentScissor)
{
    if (!visible_)
        return false;

    const IntVector2& screenPos = GetScreenPosition();
    return screenPos.x < currentScissor.right_ && screenPos.x + GetWidth() > currentScissor.left_ &&
           screenPos.y < currentScissor.bottom_ && screenPos.y + GetHeight() > currentScissor.top_;
}

const IntVector2& UiElement::GetScreenPosition() const
{
    if (positionDirty_)
    {
        IntVector2 pos = position_;
        const UiElement* parent = parent_;

        if (parent)
        {
            const IntVector2& parentScreenPos = parent->GetScreenPosition();

            pos.x += parentScreenPos.x + (int)Lerp(0.0f, (float)parent->size_.x, anchorMin_.x);
            pos.y += parentScreenPos.y + (int)Lerp(0.0f, (float)parent->size_.y, anchorMin_.y);
            pos.x -= (int)(size_.x * pivot_.x);
            pos.y -= (int)(size_.y * pivot_.y);

            pos += parent_->childOffset_;
        }

        screenPosition_ = pos;
        positionDirty_ = false;
    }

    return screenPosition_;
}

void UiElement::OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor)
{
    hovering_ = true;
}

void UiElement::OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers,
    Cursor* cursor)
{
    dragButtonCombo_ = buttons;
    dragButtonCount_ = CountSetBits((u32)dragButtonCombo_);
}

void UiElement::OnDragMove(const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons,
    QualifierFlags qualifiers, Cursor* cursor)
{
}

void UiElement::OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags /*dragButtons*/, MouseButtonFlags /*releaseButtons*/,
    Cursor* /*cursor*/)
{
    dragButtonCombo_ = MOUSEB_NONE;
    dragButtonCount_ = 0;
}

void UiElement::OnDragCancel(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags /*dragButtons*/, MouseButtonFlags /*cancelButtons*/,
    Cursor* /*cursor*/)
{
    dragButtonCombo_ = MOUSEB_NONE;
    dragButtonCount_ = 0;
}

bool UiElement::OnDragDropTest(UiElement* source)
{
    return true;
}

bool UiElement::OnDragDropFinish(UiElement* source)
{
    return true;
}

IntVector2 UiElement::screen_to_element(const IntVector2& screenPosition)
{
    return screenPosition - GetScreenPosition();
}

IntVector2 UiElement::element_to_screen(const IntVector2& position)
{
    return position + GetScreenPosition();
}

bool UiElement::load_xml(Deserializer& source)
{
    SharedPtr<XmlFile> xml(new XmlFile());
    return xml->Load(source) && load_xml(xml->GetRoot());
}

bool UiElement::save_xml(Serializer& dest, const String& indentation) const
{
    SharedPtr<XmlFile> xml(new XmlFile());
    XmlElement rootElem = xml->CreateRoot("element");
    return save_xml(rootElem) && xml->Save(dest, indentation);
}

bool UiElement::FilterAttributes(XmlElement& dest) const
{
    // Filter UI styling attributes
    XmlFile* styleFile = GetDefaultStyle();
    if (styleFile)
    {
        String style = dest.GetAttribute("style");
        if (!style.Empty() && style != "none")
        {
            if (styleXPathQuery_.SetVariable("typeName", style))
            {
                XmlElement styleElem = GetDefaultStyle()->GetRoot().SelectSinglePrepared(styleXPathQuery_);
                if (styleElem && !FilterUIStyleAttributes(dest, styleElem))
                    return false;
            }
        }
    }

    // Filter implicit attributes
    if (!FilterImplicitAttributes(dest))
    {
        DV_LOGERROR("Could not remove implicit attributes");
        return false;
    }

    return true;
}

void UiElement::SetName(const String& name)
{
    name_ = name;

    using namespace NameChanged;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_ELEMENT] = this;

    SendEvent(E_NAMECHANGED, eventData);
}

void UiElement::SetPosition(const IntVector2& position)
{
    if (position != position_)
    {
        position_ = position;
        OnPositionSet(position);
        MarkDirty();

        using namespace Positioned;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_X] = position.x;
        eventData[P_Y] = position.y;
        SendEvent(E_POSITIONED, eventData);
    }
}

void UiElement::SetPosition(int x, int y)
{
    SetPosition(IntVector2(x, y));
}

void UiElement::SetSize(const IntVector2& size)
{
    ++resizeNestingLevel_;

    IntVector2 oldSize = size_;
    IntVector2 validatedSize;
    IntVector2 effectiveMinSize = GetEffectiveMinSize();
    validatedSize.x = Clamp(size.x, effectiveMinSize.x, maxSize_.x);
    validatedSize.y = Clamp(size.y, effectiveMinSize.y, maxSize_.y);

    if (validatedSize != size_)
    {
        size_ = validatedSize;

        if (resizeNestingLevel_ == 1)
        {
            // Check if parent element's layout needs to be updated first
            if (parent_)
                parent_->UpdateLayout();

            IntVector2 delta = size_ - oldSize;
            MarkDirty();
            OnResize(size_, delta);
            UpdateLayout();

            using namespace Resized;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_ELEMENT] = this;
            eventData[P_WIDTH] = size_.x;
            eventData[P_HEIGHT] = size_.y;
            eventData[P_DX] = delta.x;
            eventData[P_DY] = delta.y;
            SendEvent(E_RESIZED, eventData);
        }
    }

    --resizeNestingLevel_;
}

void UiElement::SetSize(int width, int height)
{
    SetSize(IntVector2(width, height));
}

void UiElement::SetWidth(int width)
{
    SetSize(IntVector2(width, size_.y));
}

void UiElement::SetHeight(int height)
{
    SetSize(IntVector2(size_.x, height));
}

void UiElement::SetMinSize(const IntVector2& minSize)
{
    minSize_.x = Max(minSize.x, 0);
    minSize_.y = Max(minSize.y, 0);
    SetSize(size_);
}

void UiElement::SetMinSize(int width, int height)
{
    SetMinSize(IntVector2(width, height));
}

void UiElement::SetMinWidth(int width)
{
    SetMinSize(IntVector2(width, minSize_.y));
}

void UiElement::SetMinHeight(int height)
{
    SetMinSize(IntVector2(minSize_.x, height));
}

void UiElement::SetMaxSize(const IntVector2& maxSize)
{
    maxSize_.x = Max(maxSize.x, 0);
    maxSize_.y = Max(maxSize.y, 0);
    SetSize(size_);
}

void UiElement::SetMaxSize(int width, int height)
{
    SetMaxSize(IntVector2(width, height));
}

void UiElement::SetMaxWidth(int width)
{
    SetMaxSize(IntVector2(width, maxSize_.y));
}

void UiElement::SetMaxHeight(int height)
{
    SetMaxSize(IntVector2(maxSize_.x, height));
}

void UiElement::SetFixedSize(const IntVector2& size)
{
    minSize_ = maxSize_ = IntVector2(Max(size.x, 0), Max(size.y, 0));
    SetSize(size);
}

void UiElement::SetFixedSize(int width, int height)
{
    SetFixedSize(IntVector2(width, height));
}

void UiElement::SetFixedWidth(int width)
{
    minSize_.x = maxSize_.x = Max(width, 0);
    SetWidth(width);
}

void UiElement::SetFixedHeight(int height)
{
    minSize_.y = maxSize_.y = Max(height, 0);
    SetHeight(height);
}

void UiElement::SetAlignment(HorizontalAlignment hAlign, VerticalAlignment vAlign)
{
    SetHorizontalAlignment(hAlign);
    SetVerticalAlignment(vAlign);
}

void UiElement::SetHorizontalAlignment(HorizontalAlignment align)
{
    if (align != HA_LEFT && parent_ && parent_->GetLayoutMode() == LM_HORIZONTAL)
    {
        DV_LOGWARNING("Forcing left alignment because parent element has horizontal layout");
        align = HA_LEFT;
    }

    Vector2 min = anchorMin_;
    Vector2 max = anchorMax_;
    float pivot = pivot_.x;
    float anchorSize = max.x - min.x;

    if (align == HA_CENTER)
        min.x = pivot = 0.5f;
    else if (align == HA_LEFT)
        min.x = pivot = 0.0f;
    else if (align == HA_RIGHT)
        min.x = pivot = 1.0f;

    max.x = enableAnchor_ ? (min.x + anchorSize) : min.x;

    if (min.x != anchorMin_.x || max.x != anchorMax_.x || pivot != pivot_.x)
    {
        anchorMin_.x = min.x;
        anchorMax_.x = max.x;
        pivot_.x = pivot;
        if (enableAnchor_)
            UpdateAnchoring();
        MarkDirty();
    }
}

void UiElement::SetVerticalAlignment(VerticalAlignment align)
{
    if (align != VA_TOP && parent_ && parent_->GetLayoutMode() == LM_VERTICAL)
    {
        DV_LOGWARNING("Forcing top alignment because parent element has vertical layout");
        align = VA_TOP;
    }

    Vector2 min = anchorMin_;
    Vector2 max = anchorMax_;
    float pivot = pivot_.y;
    float anchorSize = max.y - min.y;

    if (align == VA_CENTER)
        min.y = pivot = 0.5f;
    else if (align == VA_TOP)
        min.y = pivot = 0.0f;
    else if (align == VA_BOTTOM)
        min.y = pivot = 1.0f;

    max.y = enableAnchor_ ? (min.y + anchorSize) : min.y;

    if (min.y != anchorMin_.y || max.y != anchorMax_.y || pivot != pivot_.y)
    {
        anchorMin_.y = min.y;
        anchorMax_.y = max.y;
        pivot_.y = pivot;
        if (enableAnchor_)
            UpdateAnchoring();
        MarkDirty();
    }
}

void UiElement::SetEnableAnchor(bool enable)
{
    enableAnchor_ = enable;
    if (enableAnchor_)
        UpdateAnchoring();
}

void UiElement::SetMinOffset(const IntVector2& offset)
{
    if (offset != minOffset_)
    {
        minOffset_ = offset;
        if (enableAnchor_)
            UpdateAnchoring();
    }
}

void UiElement::SetMaxOffset(const IntVector2& offset)
{
    if (offset != maxOffset_)
    {
        maxOffset_ = offset;
        if (enableAnchor_)
            UpdateAnchoring();
    }
}

void UiElement::SetMinAnchor(const Vector2& anchor)
{
    if (anchor != anchorMin_)
    {
        anchorMin_ = anchor;
        if (enableAnchor_)
            UpdateAnchoring();
    }
}

void UiElement::SetMinAnchor(float x, float y)
{
    SetMinAnchor(Vector2(x, y));
}

void UiElement::SetMaxAnchor(const Vector2& anchor)
{
    if (anchor != anchorMax_)
    {
        anchorMax_ = anchor;
        if (enableAnchor_)
            UpdateAnchoring();
    }
}

void UiElement::SetMaxAnchor(float x, float y)
{
    SetMaxAnchor(Vector2(x, y));
}

void UiElement::SetPivot(const Vector2& pivot)
{
    if (pivot != pivot_)
    {
        pivotSet_ = true;
        pivot_ = pivot;
        MarkDirty();
    }
}

void UiElement::SetPivot(float x, float y)
{
    SetPivot(Vector2(x, y));
}

void UiElement::SetClipBorder(const IntRect& rect)
{
    clipBorder_.left_ = Max(rect.left_, 0);
    clipBorder_.top_ = Max(rect.top_, 0);
    clipBorder_.right_ = Max(rect.right_, 0);
    clipBorder_.bottom_ = Max(rect.bottom_, 0);
}

void UiElement::SetColor(const Color& color)
{
    for (Color& cornerColor : colors_)
        cornerColor = color;
    colorGradient_ = false;
    derivedColorDirty_ = true;
}

void UiElement::SetColor(Corner corner, const Color& color)
{
    colors_[corner] = color;
    colorGradient_ = false;
    derivedColorDirty_ = true;

    for (i32 i = 0; i < MAX_UIELEMENT_CORNERS; ++i)
    {
        if (i != corner && colors_[i] != colors_[corner])
            colorGradient_ = true;
    }
}

void UiElement::SetPriority(int priority)
{
    if (priority_ == priority)
        return;

    priority_ = priority;
    if (parent_)
        parent_->sortOrderDirty_ = true;
}

void UiElement::SetOpacity(float opacity)
{
    opacity_ = Clamp(opacity, 0.0f, 1.0f);
    MarkDirty();
}

void UiElement::SetBringToFront(bool enable)
{
    bringToFront_ = enable;
}

void UiElement::SetBringToBack(bool enable)
{
    bringToBack_ = enable;
}

void UiElement::SetClipChildren(bool enable)
{
    clipChildren_ = enable;
}

void UiElement::SetSortChildren(bool enable)
{
    if (!sortChildren_ && enable)
        sortOrderDirty_ = true;

    sortChildren_ = enable;
}

void UiElement::SetUseDerivedOpacity(bool enable)
{
    useDerivedOpacity_ = enable;
}

void UiElement::SetEnabled(bool enable)
{
    enabled_ = enable;
    enabledPrev_ = enable;
}

void UiElement::SetDeepEnabled(bool enable)
{
    enabled_ = enable;

    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->SetDeepEnabled(enable);
}

void UiElement::ResetDeepEnabled()
{
    enabled_ = enabledPrev_;

    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->ResetDeepEnabled();
}

void UiElement::SetEnabledRecursive(bool enable)
{
    enabled_ = enable;
    enabledPrev_ = enable;

    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->SetEnabledRecursive(enable);
}

void UiElement::SetEditable(bool enable)
{
    editable_ = enable;
    OnSetEditable();
}

void UiElement::SetFocusMode(FocusMode mode)
{
    focusMode_ = mode;
}

void UiElement::SetFocus(bool enable)
{
    // Invisible elements should not receive focus
    if (focusMode_ < FM_FOCUSABLE || !IsVisibleEffective())
        enable = false;

    if (enable)
    {
        if (DV_UI.GetFocusElement() != this)
            DV_UI.SetFocusElement(this);
    }
    else
    {
        if (DV_UI.GetFocusElement() == this)
            DV_UI.SetFocusElement(nullptr);
    }
}

void UiElement::SetSelected(bool enable)
{
    selected_ = enable;
}

void UiElement::SetVisible(bool enable)
{
    if (enable != visible_)
    {
        visible_ = enable;

        // Parent's layout may change as a result of visibility change
        if (parent_)
            parent_->UpdateLayout();

        using namespace VisibleChanged;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_VISIBLE] = visible_;
        SendEvent(E_VISIBLECHANGED, eventData);

        // If the focus element becomes effectively hidden, clear focus
        if (!enable)
        {
            UiElement* focusElement = DV_UI.GetFocusElement();
            if (focusElement && !focusElement->IsVisibleEffective())
                focusElement->SetFocus(false);
        }
    }
}

void UiElement::SetDragDropMode(DragAndDropModeFlags mode)
{
    dragDropMode_ = mode;
}

bool UiElement::SetStyle(const String& styleName, XmlFile* file)
{
    // If empty style was requested, replace with type name
    String actualStyleName = !styleName.Empty() ? styleName : GetTypeName();

    appliedStyle_ = actualStyleName;
    if (styleName == "none")
        return true;

    if (!file)
    {
        file = GetDefaultStyle();
        if (!file)
            return false;
    }
    else
    {
        // If a custom style file specified, remember it
        defaultStyle_ = file;
    }

    // Remember the effectively applied style file, either custom or default
    appliedStyleFile_ = file;

    styleXPathQuery_.SetVariable("typeName", actualStyleName);
    XmlElement styleElem = file->GetRoot().SelectSinglePrepared(styleXPathQuery_);
    return styleElem && SetStyle(styleElem);
}

bool UiElement::SetStyle(const XmlElement& element)
{
    appliedStyle_ = element.GetAttribute("type");

    // Consider style attribute values as instance-level attribute default values
    SetInstanceDefault(true);
    bool success = load_xml(element);
    SetInstanceDefault(false);
    return success;
}

bool UiElement::SetStyleAuto(XmlFile* file)
{
    return SetStyle(String::EMPTY, file);
}

void UiElement::SetDefaultStyle(XmlFile* style)
{
    defaultStyle_ = style;
}

void UiElement::SetLayout(LayoutMode mode, int spacing, const IntRect& border)
{
    layoutMode_ = mode;
    layoutSpacing_ = Max(spacing, 0);
    layoutBorder_ = IntRect(Max(border.left_, 0), Max(border.top_, 0), Max(border.right_, 0), Max(border.bottom_, 0));
    VerifyChildAlignment();
    UpdateLayout();
}

void UiElement::SetLayoutMode(LayoutMode mode)
{
    layoutMode_ = mode;
    VerifyChildAlignment();
    UpdateLayout();
}

void UiElement::SetLayoutSpacing(int spacing)
{
    layoutSpacing_ = Max(spacing, 0);
    UpdateLayout();
}

void UiElement::SetLayoutBorder(const IntRect& border)
{
    layoutBorder_ = IntRect(Max(border.left_, 0), Max(border.top_, 0), Max(border.right_, 0), Max(border.bottom_, 0));
    UpdateLayout();
}

void UiElement::SetLayoutFlexScale(const Vector2& scale)
{
    layoutFlexScale_ = Vector2(Max(scale.x, 0.0f), Max(scale.y, 0.0f));
}

void UiElement::SetIndent(int indent)
{
    indent_ = indent;
    if (parent_)
        parent_->UpdateLayout();
    UpdateLayout();
    OnIndentSet();
}

void UiElement::SetIndentSpacing(int indentSpacing)
{
    indentSpacing_ = Max(indentSpacing, 0);
    if (parent_)
        parent_->UpdateLayout();
    UpdateLayout();
    OnIndentSet();
}

void UiElement::UpdateLayout()
{
    if (layoutNestingLevel_)
        return;

    // Prevent further updates while this update happens
    DisableLayoutUpdate();

    Vector<int> positions;
    Vector<int> sizes;
    Vector<int> minSizes;
    Vector<int> maxSizes;
    Vector<float> flexScales;

    int baseIndentWidth = GetIndentWidth();

    if (layoutMode_ == LM_HORIZONTAL)
    {
        int minChildHeight = 0;

        for (const SharedPtr<UiElement>& child : children_)
        {
            if (!child->IsVisible())
                continue;

            positions.Push(baseIndentWidth);
            i32 indent = child->GetIndentWidth();
            sizes.Push(child->GetWidth() + indent);
            minSizes.Push(child->GetEffectiveMinSize().x + indent);
            maxSizes.Push(child->GetMaxWidth() + indent);
            flexScales.Push(child->GetLayoutFlexScale().x);
            minChildHeight = Max(minChildHeight, child->GetEffectiveMinSize().y);
        }

        CalculateLayout(positions, sizes, minSizes, maxSizes, flexScales, GetWidth(), layoutBorder_.left_, layoutBorder_.right_,
            layoutSpacing_);

        int width = CalculateLayoutParentSize(sizes, layoutBorder_.left_, layoutBorder_.right_, layoutSpacing_);
        int height = Max(GetHeight(), minChildHeight + layoutBorder_.top_ + layoutBorder_.bottom_);
        int minWidth = CalculateLayoutParentSize(minSizes, layoutBorder_.left_, layoutBorder_.right_, layoutSpacing_);
        int minHeight = minChildHeight + layoutBorder_.top_ + layoutBorder_.bottom_;
        layoutMinSize_ = IntVector2(minWidth, minHeight);
        SetSize(width, height);
        // Validate the size before resizing child elements, in case of min/max limits
        width = size_.x;
        height = size_.y;

        i32 j = 0;
        for (const SharedPtr<UiElement>& child : children_)
        {
            if (!child->IsVisible())
                continue;

            child->SetPosition(positions[j], GetLayoutChildPosition(child).y);
            child->SetSize(sizes[j], height - layoutBorder_.top_ - layoutBorder_.bottom_);
            ++j;
        }
    }
    else if (layoutMode_ == LM_VERTICAL)
    {
        int minChildWidth = 0;

        for (const SharedPtr<UiElement>& child : children_)
        {
            if (!child->IsVisible())
                continue;

            positions.Push(0);
            sizes.Push(child->GetHeight());
            minSizes.Push(child->GetEffectiveMinSize().y);
            maxSizes.Push(child->GetMaxHeight());
            flexScales.Push(child->GetLayoutFlexScale().y);
            minChildWidth = Max(minChildWidth, child->GetEffectiveMinSize().x + child->GetIndentWidth());
        }

        CalculateLayout(positions, sizes, minSizes, maxSizes, flexScales, GetHeight(), layoutBorder_.top_, layoutBorder_.bottom_,
            layoutSpacing_);

        int height = CalculateLayoutParentSize(sizes, layoutBorder_.top_, layoutBorder_.bottom_, layoutSpacing_);
        int width = Max(GetWidth(), minChildWidth + layoutBorder_.left_ + layoutBorder_.right_);
        int minHeight = CalculateLayoutParentSize(minSizes, layoutBorder_.top_, layoutBorder_.bottom_, layoutSpacing_);
        int minWidth = minChildWidth + layoutBorder_.left_ + layoutBorder_.right_;
        layoutMinSize_ = IntVector2(minWidth, minHeight);
        SetSize(width, height);
        width = size_.x;
        height = size_.y;

        i32 j = 0;
        for (const SharedPtr<UiElement>& child : children_)
        {
            if (!child->IsVisible())
                continue;

            child->SetPosition(GetLayoutChildPosition(child).x + baseIndentWidth, positions[j]);
            child->SetSize(width - layoutBorder_.left_ - layoutBorder_.right_, sizes[j]);
            ++j;
        }
    }
    else
    {
        for (const SharedPtr<UiElement>& child : children_)
        {
            if (child->GetEnableAnchor())
                child->UpdateAnchoring();
        }
    }

    using namespace LayoutUpdated;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_ELEMENT] = this;
    SendEvent(E_LAYOUTUPDATED, eventData);

    EnableLayoutUpdate();
}

void UiElement::DisableLayoutUpdate()
{
    ++layoutNestingLevel_;
}

void UiElement::EnableLayoutUpdate()
{
    --layoutNestingLevel_;
}

void UiElement::BringToFront()
{
    // Follow the parent chain to the top level window. If it has BringToFront mode, bring it to front now
    UiElement* root = GetRoot();
    // If element is detached from hierarchy, this must be a no-op
    if (!root)
        return;

    UiElement* ptr = this;
    while (ptr && ptr->GetParent() != root)
        ptr = ptr->GetParent();
    if (!ptr || !ptr->GetBringToFront())
        return;

    // Get the highest priority used by all other top level elements, assign that to the new front element
    // and decrease others' priority where necessary. However, take into account only input-enabled
    // elements and those which have the BringToBack flag set
    HashSet<int> usedPriorities;

    int maxPriority = M_MIN_INT;
    const Vector<SharedPtr<UiElement>>& rootChildren = root->GetChildren();
    for (Vector<SharedPtr<UiElement>>::ConstIterator i = rootChildren.Begin(); i != rootChildren.End(); ++i)
    {
        UiElement* other = *i;
        if (other->IsEnabled() && other->bringToBack_ && other != ptr)
        {
            int priority = other->GetPriority();
            // M_MAX_INT is used by popups and tooltips. Disregard these to avoid an "arms race" with the priorities
            if (priority == M_MAX_INT)
                continue;
            usedPriorities.Insert(priority);
            maxPriority = Max(priority, maxPriority);
        }
    }

    if (maxPriority != M_MIN_INT && maxPriority >= ptr->GetPriority())
    {
        ptr->SetPriority(maxPriority);

        int minPriority = maxPriority;
        while (usedPriorities.Contains(minPriority))
            --minPriority;

        for (Vector<SharedPtr<UiElement>>::ConstIterator i = rootChildren.Begin(); i != rootChildren.End(); ++i)
        {
            UiElement* other = *i;
            int priority = other->GetPriority();

            if (other->IsEnabled() && other->bringToBack_ && other != ptr && priority >= minPriority && priority <= maxPriority)
                other->SetPriority(priority - 1);
        }
    }
}

UiElement* UiElement::create_child(StringHash type, const String& name, i32 index/* = ENDPOS*/)
{
    assert(index == ENDPOS || (index >= 0 && index <= children_.Size()));

    // Check that creation succeeds and that the object in fact is a UI element
    SharedPtr<UiElement> newElement = DynamicCast<UiElement>(DV_CONTEXT.CreateObject(type));

    if (!newElement)
    {
        DV_LOGERROR("Could not create unknown UI element type " + type.ToString());
        return nullptr;
    }

    if (!name.Empty())
        newElement->SetName(name);

    InsertChild(index, newElement);
    return newElement;
}

void UiElement::AddChild(UiElement* element)
{
    InsertChild(ENDPOS, element);
}

void UiElement::InsertChild(i32 index, UiElement* element)
{
    assert(index == ENDPOS || (index >= 0 && index <= children_.Size()));

    // Check for illegal or redundant parent assignment
    if (!element || element == this || element->parent_ == this)
        return;

    // Check for possible cyclic parent assignment
    UiElement* parent = parent_;

    while (parent)
    {
        if (parent == element)
            return;

        parent = parent->parent_;
    }

    // Add first, then remove from old parent, to ensure the element does not get deleted
    if (index >= children_.Size() || index == ENDPOS)
        children_.Push(SharedPtr<UiElement>(element));
    else
        children_.Insert(children_.Begin() + index, SharedPtr<UiElement>(element));

    element->Remove();

    if (sortChildren_)
        sortOrderDirty_ = true;

    element->parent_ = this;
    element->MarkDirty();

    // Apply style now if child element (and its children) has it defined
    ApplyStyleRecursive(element);

    VerifyChildAlignment();
    UpdateLayout();

    // Send change event
    UiElement* root = GetRoot();
    UiElement* sender = GetElementEventSender();

    if (sender)
    {
        using namespace ElementAdded;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ROOT] = root;
        eventData[P_PARENT] = this;
        eventData[P_ELEMENT] = element;

        sender->SendEvent(E_ELEMENTADDED, eventData);
    }
}

void UiElement::RemoveChild(UiElement* element, i32 index/* = 0*/)
{
    assert(index >= 0);

    for (i32 i = index; i < children_.Size(); ++i)
    {
        if (children_[i] == element)
        {
            // Send change event if not already being destroyed
            UiElement* sender = Refs() > 0 ? GetElementEventSender() : nullptr;

            if (sender)
            {
                using namespace ElementRemoved;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_ROOT] = GetRoot();
                eventData[P_PARENT] = this;
                eventData[P_ELEMENT] = element;

                sender->SendEvent(E_ELEMENTREMOVED, eventData);
            }

            element->Detach();
            children_.Erase(i);
            UpdateLayout();
            return;
        }
    }
}

void UiElement::RemoveChildAtIndex(i32 index)
{
    assert(index >= 0);

    if (index >= children_.Size())
        return;

    // Send change event if not already being destroyed
    UiElement* sender = Refs() > 0 ? GetElementEventSender() : nullptr;
    if (sender)
    {
        using namespace ElementRemoved;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ROOT] = GetRoot();
        eventData[P_PARENT] = this;
        eventData[P_ELEMENT] = children_[index];

        sender->SendEvent(E_ELEMENTREMOVED, eventData);
    }

    children_[index]->Detach();
    children_.Erase(index);
    UpdateLayout();
}

void UiElement::RemoveAllChildren()
{
    UiElement* root = GetRoot();
    UiElement* sender = Refs() > 0 ? GetElementEventSender() : nullptr;

    for (Vector<SharedPtr<UiElement>>::Iterator i = children_.Begin(); i < children_.End();)
    {
        // Send change event if not already being destroyed
        if (sender)
        {
            using namespace ElementRemoved;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_ROOT] = root;
            eventData[P_PARENT] = this;
            eventData[P_ELEMENT] = (*i).Get();

            sender->SendEvent(E_ELEMENTREMOVED, eventData);
        }

        (*i++)->Detach();
    }
    children_.Clear();
    UpdateLayout();
}

void UiElement::Remove()
{
    if (parent_)
        parent_->RemoveChild(this);
}

i32 UiElement::FindChild(UiElement* element) const
{
    Vector<SharedPtr<UiElement>>::ConstIterator it = children_.Find(SharedPtr<UiElement>(element));
    return it != children_.End() ? (i32)(it - children_.Begin()) : NINDEX;
}

void UiElement::SetParent(UiElement* parent, i32 index/* = ENDPOS*/)
{
    if (parent)
        parent->InsertChild(index, this);
}

void UiElement::SetVar(StringHash key, const Variant& value)
{
    vars_[key] = value;
}

void UiElement::SetInternal(bool enable)
{
    internal_ = enable;
}

void UiElement::SetTraversalMode(TraversalMode traversalMode)
{
    traversalMode_ = traversalMode;
}

void UiElement::SetElementEventSender(bool flag)
{
    elementEventSender_ = flag;
}

void UiElement::set_tags(const StringVector& tags)
{
    remove_all_tags();
    add_tags(tags);
}

void UiElement::add_tag(const String& tag)
{
    if (tag.Empty() || HasTag(tag))
        return;

    tags_.Push(tag);
}

void UiElement::add_tags(const String& tags, char separator)
{
    StringVector tagVector = tags.Split(separator);
    add_tags(tagVector);
}

void UiElement::add_tags(const StringVector& tags)
{
    for (const String& tag : tags)
        add_tag(tag);
}

bool UiElement::remove_tag(const String& tag)
{
    return tags_.Remove(tag);
}

void UiElement::remove_all_tags()
{
    tags_.Clear();
}

HorizontalAlignment UiElement::GetHorizontalAlignment() const
{
    if (anchorMin_.x == 0.0f && anchorMax_.x == 0.0f && (!pivotSet_ || pivot_.x == 0.0f))
        return HA_LEFT;
    else if (anchorMin_.x == 0.5f && anchorMax_.x == 0.5f && (!pivotSet_ || pivot_.x == 0.5f))
        return HA_CENTER;
    else if (anchorMin_.x == 1.0f && anchorMax_.x == 1.0f && (!pivotSet_ || pivot_.x == 1.0f))
        return HA_RIGHT;

    return HA_CUSTOM;
}

VerticalAlignment UiElement::GetVerticalAlignment() const
{
    if (anchorMin_.y == 0.0f && anchorMax_.y == 0.0f && (!pivotSet_ || pivot_.y == 0.0f))
        return VA_TOP;
    else if (anchorMin_.y == 0.5f && anchorMax_.y == 0.5f && (!pivotSet_ || pivot_.y == 0.5f))
        return VA_CENTER;
    else if (anchorMin_.y == 1.0f && anchorMax_.y == 1.0f && (!pivotSet_ || pivot_.y == 1.0f))
        return VA_BOTTOM;

    return VA_CUSTOM;
}

float UiElement::GetDerivedOpacity() const
{
    if (!useDerivedOpacity_)
        return opacity_;

    if (opacityDirty_)
    {
        derivedOpacity_ = opacity_;
        const UiElement* parent = parent_;

        while (parent)
        {
            derivedOpacity_ *= parent->opacity_;
            parent = parent->parent_;
        }

        opacityDirty_ = false;
    }

    return derivedOpacity_;
}

bool UiElement::HasFocus() const
{
    return DV_UI.GetFocusElement() == this;
}

bool UiElement::IsChildOf(UiElement* element) const
{
    UiElement* parent = parent_;
    while (parent)
    {
        if (parent == element)
            return true;
        parent = parent->parent_;
    }
    return false;
}

bool UiElement::IsVisibleEffective() const
{
    bool visible = visible_;
    const UiElement* element = parent_;

    // Traverse the parent chain
    while (visible && element)
    {
        visible &= element->visible_;
        element = element->parent_;
    }

    return visible;
}

const String& UiElement::GetAppliedStyle() const
{
    return appliedStyle_ == GetTypeName() ? String::EMPTY : appliedStyle_;
}

XmlFile* UiElement::GetDefaultStyle(bool recursiveUp) const
{
    if (recursiveUp)
    {
        const UiElement* element = this;
        while (element)
        {
            if (element->defaultStyle_)
                return element->defaultStyle_;
            element = element->parent_;
        }
        return nullptr;
    }
    else
        return defaultStyle_;
}

void UiElement::GetChildren(Vector<UiElement*>& dest, bool recursive) const
{
    dest.Clear();

    if (!recursive)
    {
        dest.Reserve(children_.Size());
        for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
            dest.Push(*i);
    }
    else
        GetChildrenRecursive(dest);
}

Vector<UiElement*> UiElement::GetChildren(bool recursive) const
{
    Vector<UiElement*> dest;
    GetChildren(dest, recursive);
    return dest;
}

i32 UiElement::GetNumChildren(bool recursive/* = false*/) const
{
    if (!recursive)
    {
        return children_.Size();
    }
    else
    {
        i32 allChildren = children_.Size();

        for (const SharedPtr<UiElement>& child : children_)
            allChildren += child->GetNumChildren(true);

        return allChildren;
    }
}

UiElement* UiElement::GetChild(i32 index) const
{
    assert(index >= 0 || index == NINDEX);
    return (index >= 0 && index < children_.Size()) ? children_[index] : nullptr;
}

UiElement* UiElement::GetChild(const String& name, bool recursive) const
{
    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        if ((*i)->name_ == name)
            return *i;

        if (recursive)
        {
            UiElement* element = (*i)->GetChild(name, true);
            if (element)
                return element;
        }
    }

    return nullptr;
}

UiElement* UiElement::GetChild(const StringHash& key, const Variant& value, bool recursive) const
{
    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        const Variant& varValue = (*i)->GetVar(key);
        if (value != Variant::EMPTY ? varValue == value : varValue != Variant::EMPTY)
            return *i;

        if (recursive)
        {
            UiElement* element = (*i)->GetChild(key, value, true);
            if (element)
                return element;
        }
    }

    return nullptr;
}

UiElement* UiElement::GetRoot() const
{
    UiElement* root = parent_;
    if (!root)
        return nullptr;
    while (root->GetParent())
        root = root->GetParent();
    return root;
}

const Color& UiElement::GetDerivedColor() const
{
    if (derivedColorDirty_)
    {
        derivedColor_ = colors_[C_TOPLEFT];
        derivedColor_.a_ *= GetDerivedOpacity();
        derivedColorDirty_ = false;
    }

    return derivedColor_;
}

const Variant& UiElement::GetVar(const StringHash& key) const
{
    VariantMap::ConstIterator i = vars_.Find(key);
    return i != vars_.End() ? i->second_ : Variant::EMPTY;
}

bool UiElement::HasTag(const String& tag) const
{
    return tags_.Contains(tag);
}

void UiElement::GetChildrenWithTag(Vector<UiElement*>& dest, const String& tag, bool recursive) const
{
    dest.Clear();

    if (!recursive)
    {
        for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            UiElement* element = *i;
            if (element->HasTag(tag))
                dest.Push(element);
        }
    }
    else
        GetChildrenWithTagRecursive(dest, tag);
}

Vector<UiElement*> UiElement::GetChildrenWithTag(const String& tag, bool recursive) const
{
    Vector<UiElement*> dest;
    GetChildrenWithTag(dest, tag, recursive);
    return dest;
}

void UiElement::GetChildrenWithTagRecursive(Vector<UiElement*>& dest, const String& tag) const
{
    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        UiElement* element = *i;
        if (element->HasTag(tag))
            dest.Push(element);
        if (!element->children_.Empty())
            element->GetChildrenWithTagRecursive(dest, tag);
    }
}

bool UiElement::IsInside(IntVector2 position, bool isScreen)
{
    if (isScreen)
        position = screen_to_element(position);
    return position.x >= 0 && position.y >= 0 && position.x < size_.x && position.y < size_.y;
}

bool UiElement::IsInsideCombined(IntVector2 position, bool isScreen)
{
    // If child elements are clipped, no need to expand the rect
    if (clipChildren_)
        return IsInside(position, isScreen);

    if (!isScreen)
        position = element_to_screen(position);

    IntRect combined = GetCombinedScreenRect();
    return position.x >= combined.left_ && position.y >= combined.top_ && position.x < combined.right_ &&
        position.y < combined.bottom_;
}

IntRect UiElement::GetCombinedScreenRect()
{
    IntVector2 screenPosition(GetScreenPosition());
    IntRect combined(screenPosition.x, screenPosition.y, screenPosition.x + size_.x, screenPosition.y + size_.y);

    if (!clipChildren_)
    {
        for (Vector<SharedPtr<UiElement>>::Iterator i = children_.Begin(); i != children_.End(); ++i)
        {
            IntRect childCombined((*i)->GetCombinedScreenRect());

            if (childCombined.left_ < combined.left_)
                combined.left_ = childCombined.left_;
            if (childCombined.right_ > combined.right_)
                combined.right_ = childCombined.right_;
            if (childCombined.top_ < combined.top_)
                combined.top_ = childCombined.top_;
            if (childCombined.bottom_ > combined.bottom_)
                combined.bottom_ = childCombined.bottom_;
        }
    }

    return combined;
}

void UiElement::SortChildren()
{
    if (sortChildren_ && sortOrderDirty_)
    {
        // Only sort when there is no layout
        if (layoutMode_ == LM_FREE)
            stable_sort(children_.Begin().ptr_, children_.End().ptr_, CompareUIElements);
        sortOrderDirty_ = false;
    }
}

void UiElement::SetChildOffset(const IntVector2& offset)
{
    if (offset != childOffset_)
    {
        childOffset_ = offset;
        for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
            (*i)->MarkDirty();
    }
}

void UiElement::SetHovering(bool enable)
{
    hovering_ = enable;
}

void UiElement::AdjustScissor(IntRect& currentScissor)
{
    if (clipChildren_)
    {
        IntVector2 screenPos = GetScreenPosition();
        currentScissor.left_ = Max(currentScissor.left_, screenPos.x + clipBorder_.left_);
        currentScissor.top_ = Max(currentScissor.top_, screenPos.y + clipBorder_.top_);
        currentScissor.right_ = Min(currentScissor.right_, screenPos.x + size_.x - clipBorder_.right_);
        currentScissor.bottom_ = Min(currentScissor.bottom_, screenPos.y + size_.y - clipBorder_.bottom_);

        if (currentScissor.right_ < currentScissor.left_)
            currentScissor.right_ = currentScissor.left_;
        if (currentScissor.bottom_ < currentScissor.top_)
            currentScissor.bottom_ = currentScissor.top_;
    }
}

void UiElement::GetBatchesWithOffset(IntVector2& offset, Vector<UIBatch>& batches, Vector<float>& vertexData,
    IntRect currentScissor)
{
    Vector2 floatOffset((float)offset.x, (float)offset.y);
    i32 initialSize = vertexData.Size();

    GetBatches(batches, vertexData, currentScissor);
    for (i32 i = initialSize; i < vertexData.Size(); i += 6)
    {
        vertexData[i] += floatOffset.x;
        vertexData[i + 1] += floatOffset.y;
    }

    AdjustScissor(currentScissor);
    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        if ((*i)->IsVisible())
            (*i)->GetBatchesWithOffset(offset, batches, vertexData, currentScissor);
    }
}

UiElement* UiElement::GetElementEventSender() const
{
    UiElement* element = const_cast<UiElement*>(this);
    if (elementEventSender_)
        return element;

    while (element->parent_)
    {
        element = element->parent_;
        if (element->elementEventSender_)
            return element;
    }

    // If no predefined element event sender in the parental chain, return ultimate root element
    return element;
}

IntVector2 UiElement::GetEffectiveMinSize() const
{
    if (IsFixedSize() || layoutMode_ == LM_FREE || layoutMinSize_ == IntVector2::ZERO)
        return minSize_;
    else
        return IntVector2(Max(minSize_.x, layoutMinSize_.x), Max(minSize_.y, layoutMinSize_.y));
}

void UiElement::OnAttributeAnimationAdded()
{
    if (attributeAnimationInfos_.Size() == 1)
        subscribe_to_event(E_POSTUPDATE, DV_HANDLER(UiElement, HandlePostUpdate));
}

void UiElement::OnAttributeAnimationRemoved()
{
    if (attributeAnimationInfos_.Empty())
        unsubscribe_from_event(E_POSTUPDATE);
}

Animatable* UiElement::FindAttributeAnimationTarget(const String& name, String& outName)
{
    Vector<String> names = name.Split('/');
    // Only attribute name
    if (names.Size() == 1)
    {
        outName = name;
        return this;
    }
    else
    {
        // Name must in following format: "#0/#1/attribute"
        UiElement* element = this;
        for (i32 i = 0; i < names.Size() - 1; ++i)
        {
            if (names[i].Front() != '#')
            {
                DV_LOGERROR("Invalid name " + name);
                return nullptr;
            }

            String name = names[i].Substring(1, names[i].Length() - 1);
            char s = name.Front();
            if (s >= '0' && s <= '9')
            {
                i32 index = ToI32(name);
                element = element->GetChild(index);
            }
            else
            {
                element = element->GetChild(name, true);
            }

            if (!element)
            {
                DV_LOGERROR("Could not find element by name " + name);
                return nullptr;
            }
        }

        outName = names.Back();
        return element;
    }
}

void UiElement::MarkDirty()
{
    positionDirty_ = true;
    opacityDirty_ = true;
    derivedColorDirty_ = true;

    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->MarkDirty();
}

bool UiElement::RemoveChildXML(XmlElement& parent, const String& name) const
{
    static XPathQuery matchXPathQuery("./attribute[@name=$attributeName]", "attributeName:String");

    if (!matchXPathQuery.SetVariable("attributeName", name))
        return false;

    XmlElement removeElem = parent.SelectSinglePrepared(matchXPathQuery);
    return !removeElem || parent.RemoveChild(removeElem);
}

bool UiElement::RemoveChildXML(XmlElement& parent, const String& name, const String& value) const
{
    static XPathQuery matchXPathQuery
        ("./attribute[@name=$attributeName and @value=$attributeValue]", "attributeName:String, attributeValue:String");

    if (!matchXPathQuery.SetVariable("attributeName", name))
        return false;
    if (!matchXPathQuery.SetVariable("attributeValue", value))
        return false;

    XmlElement removeElem = parent.SelectSinglePrepared(matchXPathQuery);
    return !removeElem || parent.RemoveChild(removeElem);
}

bool UiElement::FilterUIStyleAttributes(XmlElement& dest, const XmlElement& styleElem) const
{
    // Remove style attribute only when its value is identical to the value stored in style file
    String style = styleElem.GetAttribute("style");
    if (!style.Empty())
    {
        if (style == dest.GetAttribute("style"))
        {
            if (!dest.RemoveAttribute("style"))
            {
                DV_LOGWARNING("Could not remove style attribute");
                return false;
            }
        }
    }

    // Perform the same action recursively for internal child elements stored in style file
    XmlElement childDest = dest.GetChild("element");
    XmlElement childElem = styleElem.GetChild("element");
    while (childDest && childElem)
    {
        if (!childElem.GetBool("internal"))
        {
            DV_LOGERROR("Invalid style file, style element can only contain internal child elements");
            return false;
        }
        if (!FilterUIStyleAttributes(childDest, childElem))
            return false;

        childDest = childDest.GetNext("element");
        childElem = childElem.GetNext("element");
    }

    // Remove style attribute when it is the same as its type, however, if it is an internal element then replace it to "none" instead
    if (!dest.GetAttribute("style").Empty() && dest.GetAttribute("style") == dest.GetAttribute("type"))
    {
        if (internal_)
        {
            if (!dest.SetAttribute("style", "none"))
                return false;
        }
        else
        {
            if (!dest.RemoveAttribute("style"))
                return false;
        }
    }

    return true;
}

bool UiElement::FilterImplicitAttributes(XmlElement& dest) const
{
    // Remove positioning and sizing attributes when they are under the influence of layout mode
    if (layoutMode_ != LM_FREE && !IsFixedWidth() && !IsFixedHeight())
    {
        if (!RemoveChildXML(dest, "Min Size"))
            return false;
    }
    if (parent_ && parent_->layoutMode_ != LM_FREE)
    {
        if (!RemoveChildXML(dest, "Position"))
            return false;
        if (!RemoveChildXML(dest, "Size"))
            return false;
    }

    return true;
}

void UiElement::UpdateAnchoring()
{
    if (parent_ && enableAnchor_)
    {
        IntVector2 newSize;
        newSize.x = (int)(parent_->size_.x * Clamp(anchorMax_.x - anchorMin_.x, 0.0f, 1.0f)) + maxOffset_.x - minOffset_.x;
        newSize.y = (int)(parent_->size_.y * Clamp(anchorMax_.y - anchorMin_.y, 0.0f, 1.0f)) + maxOffset_.y - minOffset_.y;

        if (position_ != minOffset_)
            SetPosition(minOffset_);
        if (size_ != newSize)
            SetSize(newSize);
    }
}

void UiElement::GetChildrenRecursive(Vector<UiElement*>& dest) const
{
    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        UiElement* element = *i;
        dest.Push(element);
        if (!element->children_.Empty())
            element->GetChildrenRecursive(dest);
    }
}

void UiElement::ApplyStyleRecursive(UiElement* element)
{
    // If child element style file changes as result of being (re)parented and it has a defined style, apply it now
    if (!element->appliedStyle_.Empty() && element->appliedStyleFile_.Get() != element->GetDefaultStyle())
    {
        element->SetStyle(element->appliedStyle_);
        for (Vector<SharedPtr<UiElement>>::ConstIterator i = element->children_.Begin(); i != element->children_.End(); ++i)
            element->ApplyStyleRecursive(*i);
    }
}

int UiElement::CalculateLayoutParentSize(const Vector<int>& sizes, int begin, int end, int spacing)
{
    int width = begin + end;
    if (sizes.Empty())
        return width;

    for (i32 size : sizes)
    {
        // If calculating maximum size, and the default is specified, do not overflow it
        if (size == M_MAX_INT)
            return M_MAX_INT;
        width += size + spacing;
    }
    // The last spacing is not needed
    return width - spacing;
}

void UiElement::CalculateLayout(Vector<int>& positions, Vector<int>& sizes, const Vector<int>& minSizes,
    const Vector<int>& maxSizes, const Vector<float>& flexScales, int targetSize, int begin, int end, int spacing)
{
    i32 numChildren = sizes.Size();
    if (!numChildren)
        return;
    int targetTotalSize = targetSize - begin - end - (numChildren - 1) * spacing;
    if (targetTotalSize < 0)
        targetTotalSize = 0;
    int targetChildSize = targetTotalSize / numChildren;
    int remainder = targetTotalSize % numChildren;
    float add = (float)remainder / numChildren;
    float acc = 0.0f;

    // Initial pass
    for (i32 i = 0; i < numChildren; ++i)
    {
        i32 targetSize = (i32)(targetChildSize * flexScales[i]);
        if (remainder)
        {
            acc += add;
            if (acc >= 0.5f)
            {
                acc -= 1.0f;
                ++targetSize;
                --remainder;
            }
        }
        sizes[i] = Clamp(targetSize, minSizes[i], maxSizes[i]);
    }

    // Error correction passes
    for (;;)
    {
        int actualTotalSize = 0;
        for (i32 i = 0; i < numChildren; ++i)
            actualTotalSize += sizes[i];
        int error = targetTotalSize - actualTotalSize;
        // Break if no error
        if (!error)
            break;

        // Check which of the children can be resized to correct the error. If none, must break
        Vector<i32> resizable;
        for (i32 i = 0; i < numChildren; ++i)
        {
            if (error < 0 && sizes[i] > minSizes[i])
                resizable.Push(i);
            else if (error > 0 && sizes[i] < maxSizes[i])
                resizable.Push(i);
        }
        if (resizable.Empty())
            break;

        int numResizable = resizable.Size();
        int errorPerChild = error / numResizable;
        remainder = (abs(error)) % numResizable;
        add = (float)remainder / numResizable;
        acc = 0.0f;

        for (int i = 0; i < numResizable; ++i)
        {
            i32 index = resizable[i];
            int targetSize = sizes[index] + errorPerChild;
            if (remainder)
            {
                acc += add;
                if (acc >= 0.5f)
                {
                    acc -= 1.0f;
                    targetSize = error < 0 ? targetSize - 1 : targetSize + 1;
                    --remainder;
                }
            }

            sizes[index] = Clamp(targetSize, minSizes[index], maxSizes[index]);
        }
    }

    // Calculate final positions and store the maximum child element size for optimizations
    layoutElementMaxSize_ = 0;
    int position = begin;
    for (i32 i = 0; i < numChildren; ++i)
    {
        positions[i] = position;
        position += sizes[i] + spacing;
        if (sizes[i] > layoutElementMaxSize_)
            layoutElementMaxSize_ = sizes[i];
    }
}

IntVector2 UiElement::GetLayoutChildPosition(UiElement* child)
{
    IntVector2 ret(IntVector2::ZERO);

    HorizontalAlignment ha = child->GetHorizontalAlignment();
    switch (ha)
    {
    case HA_LEFT:
        ret.x = layoutBorder_.left_;
        break;

    case HA_RIGHT:
        ret.x = -layoutBorder_.right_;
        break;

    default:
        break;
    }

    VerticalAlignment va = child->GetVerticalAlignment();
    switch (va)
    {
    case VA_TOP:
        ret.y = layoutBorder_.top_;
        break;

    case VA_BOTTOM:
        ret.y = -layoutBorder_.bottom_;
        break;

    default:
        break;
    }

    return ret;
}

void UiElement::Detach()
{
    parent_ = nullptr;
    MarkDirty();
}

void UiElement::VerifyChildAlignment()
{
    for (Vector<SharedPtr<UiElement>>::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        // Reapply child alignments. If they are illegal compared to layout, they will be set left/top as neded
        (*i)->SetHorizontalAlignment((*i)->GetHorizontalAlignment());
        (*i)->SetVerticalAlignment((*i)->GetVerticalAlignment());
    }
}

void UiElement::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace PostUpdate;

    UpdateAttributeAnimations(eventData[P_TIMESTEP].GetFloat());
}

void UiElement::SetRenderTexture(Texture2D* texture)
{
    DV_UI.SetElementRenderTexture(this, texture);
}

}
