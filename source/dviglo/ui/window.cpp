// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../input/input_events.h"
#include "cursor.h"
#include "ui.h"
#include "ui_events.h"
#include "window.h"

#include "../common/debug_new.h"

namespace dviglo
{

static const int DEFAULT_RESIZE_BORDER = 4;

extern const char* UI_CATEGORY;

Window::Window() :
    movable_(false),
    resizable_(false),
    fixedWidthResizing_(false),
    fixedHeightResizing_(false),
    resizeBorder_(DEFAULT_RESIZE_BORDER, DEFAULT_RESIZE_BORDER, DEFAULT_RESIZE_BORDER, DEFAULT_RESIZE_BORDER),
    dragMode_(DRAG_NONE),
    modal_(false),
    modalAutoDismiss_(true),
    modalShadeColor_(Color::TRANSPARENT_BLACK),
    modalFrameColor_(Color::TRANSPARENT_BLACK),
    modalFrameSize_(IntVector2::ZERO)
{
    bringToFront_ = true;
    clipChildren_ = true;
    SetEnabled(true);
}

Window::~Window() = default;

void Window::register_object()
{
    DV_CONTEXT->RegisterFactory<Window>(UI_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(BorderImage);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Bring To Front", true);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Clip Children", true);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Is Enabled", true);
    DV_ACCESSOR_ATTRIBUTE("Resize Border", GetResizeBorder, SetResizeBorder, IntRect(DEFAULT_RESIZE_BORDER, \
        DEFAULT_RESIZE_BORDER, DEFAULT_RESIZE_BORDER, DEFAULT_RESIZE_BORDER), AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Is Movable", IsMovable, SetMovable, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Is Resizable", IsResizable, SetResizable, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Fixed Width Resizing", GetFixedWidthResizing, SetFixedWidthResizing, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Fixed Height Resizing", GetFixedHeightResizing, SetFixedHeightResizing, false, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Is Modal", IsModal, SetModal, false, AM_FILE | AM_NOEDIT);
    DV_ACCESSOR_ATTRIBUTE("Modal Shade Color", GetModalShadeColor, SetModalShadeColor, Color::TRANSPARENT_BLACK, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Modal Frame Color", GetModalFrameColor, SetModalFrameColor, Color::TRANSPARENT_BLACK, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Modal Frame Size", GetModalFrameSize, SetModalFrameSize, IntVector2::ZERO, AM_FILE);
    // Modal auto dismiss is purposefully not an attribute, as using it can make the editor lock up.
    // Instead it should be set false in code when needed
}

void Window::GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor)
{
    if (modal_)
    {
        // Modal shade
        if (modalShadeColor_ != Color::TRANSPARENT_BLACK)
        {
            UiElement* rootElement = GetRoot();
            const IntVector2& rootSize = rootElement->GetSize();
            UIBatch batch(rootElement, BLEND_ALPHA, IntRect(0, 0, rootSize.x, rootSize.y), nullptr, &vertexData);
            batch.SetColor(modalShadeColor_);
            batch.add_quad(0, 0, rootSize.x, rootSize.y, 0, 0);
            UIBatch::AddOrMerge(batch, batches);
        }

        // Modal frame
        if (modalFrameColor_ != Color::TRANSPARENT_BLACK && modalFrameSize_ != IntVector2::ZERO)
        {
            UIBatch batch(this, BLEND_ALPHA, currentScissor, nullptr, &vertexData);
            int x = GetIndentWidth();
            IntVector2 size = GetSize();
            size.x -= x;
            batch.SetColor(modalFrameColor_);
            batch.add_quad(x - modalFrameSize_.x, -modalFrameSize_.y, size.x + 2 * modalFrameSize_.x,
                size.y + 2 * modalFrameSize_.y, 0, 0);
            UIBatch::AddOrMerge(batch, batches);
        }
    }

    BorderImage::GetBatches(batches, vertexData, currentScissor);
}

void Window::OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor)
{
    UiElement::OnHover(position, screenPosition, buttons, qualifiers, cursor);

    if (dragMode_ == DRAG_NONE)
    {
        WindowDragMode mode = GetDragMode(position);
        SetCursorShape(mode, cursor);
    }
    else
        SetCursorShape(dragMode_, cursor);
}

void Window::OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor)
{
    UiElement::OnDragBegin(position, screenPosition, buttons, qualifiers, cursor);

    if (buttons != MOUSEB_LEFT || !CheckAlignment())
    {
        dragMode_ = DRAG_NONE;
        return;
    }

    dragBeginCursor_ = screenPosition;
    dragBeginPosition_ = GetPosition();
    dragBeginSize_ = GetSize();
    dragMode_ = GetDragMode(position);
    SetCursorShape(dragMode_, cursor);
}

void Window::OnDragMove(const IntVector2& /*position*/, const IntVector2& screenPosition, const IntVector2& /*deltaPos*/,
    MouseButtonFlags /*buttons*/, QualifierFlags /*qualifiers*/, Cursor* cursor)
{
    if (dragMode_ == DRAG_NONE)
        return;

    IntVector2 delta = screenPosition - dragBeginCursor_;
    IntVector2 dragSize;
    IntVector2 resizeBorderSize(resizeBorder_.left_ + resizeBorder_.right_, resizeBorder_.top_ + resizeBorder_.bottom_);

    const IntVector2& position = GetPosition();
    const IntVector2& size = GetSize();
    // Use GetEffectiveMinSize() instead of GetMinSize() to prevent windows moving once the effective minimum size is reached
    const IntVector2 effectiveMinSize = GetEffectiveMinSize();
    const IntVector2& maxSize = GetMaxSize();

    switch (dragMode_)
    {
    case DRAG_MOVE:
        SetPosition(dragBeginPosition_ + delta);
        break;

    case DRAG_RESIZE_TOPLEFT:
        SetPosition(Clamp(dragBeginPosition_.x + delta.x, position.x - (maxSize.x - size.x),
            position.x + (size.x - effectiveMinSize.x)),
            Clamp(dragBeginPosition_.y + delta.y, position.y - (maxSize.y - size.y),
                position.y + (size.y - effectiveMinSize.y)));
        dragSize = dragBeginSize_ - delta;
        fixedWidthResizing_ ? SetFixedWidth(Max(dragSize.x, resizeBorderSize.x)) : SetWidth(dragSize.x);
        fixedHeightResizing_ ? SetFixedHeight(Max(dragSize.y, resizeBorderSize.y)) : SetHeight(dragSize.y);
        break;

    case DRAG_RESIZE_TOP:
        SetPosition(dragBeginPosition_.x, Clamp(dragBeginPosition_.y + delta.y, position.y - (maxSize.y - size.y),
            position.y + (size.y - effectiveMinSize.y)));
        dragSize = IntVector2(dragBeginSize_.x, dragBeginSize_.y - delta.y);
        fixedHeightResizing_ ? SetFixedHeight(Max(dragSize.y, resizeBorderSize.y)) : SetHeight(dragSize.y);
        break;

    case DRAG_RESIZE_TOPRIGHT:
        SetPosition(dragBeginPosition_.x, Clamp(dragBeginPosition_.y + delta.y, position.y - (maxSize.y - size.y),
            position.y + (size.y - effectiveMinSize.y)));
        dragSize = IntVector2(dragBeginSize_.x + delta.x, dragBeginSize_.y - delta.y);
        fixedWidthResizing_ ? SetFixedWidth(Max(dragSize.x, resizeBorderSize.x)) : SetWidth(dragSize.x);
        fixedHeightResizing_ ? SetFixedHeight(Max(dragSize.y, resizeBorderSize.y)) : SetHeight(dragSize.y);
        break;

    case DRAG_RESIZE_RIGHT:
        dragSize = IntVector2(dragBeginSize_.x + delta.x, dragBeginSize_.y);
        fixedWidthResizing_ ? SetFixedWidth(Max(dragSize.x, resizeBorderSize.x)) : SetWidth(dragSize.x);
        break;

    case DRAG_RESIZE_BOTTOMRIGHT:
        dragSize = dragBeginSize_ + delta;
        fixedWidthResizing_ ? SetFixedWidth(Max(dragSize.x, resizeBorderSize.x)) : SetWidth(dragSize.x);
        fixedHeightResizing_ ? SetFixedHeight(Max(dragSize.y, resizeBorderSize.y)) : SetHeight(dragSize.y);
        break;

    case DRAG_RESIZE_BOTTOM:
        dragSize = IntVector2(dragBeginSize_.x, dragBeginSize_.y + delta.y);
        fixedHeightResizing_ ? SetFixedHeight(Max(dragSize.y, resizeBorderSize.y)) : SetHeight(dragSize.y);
        break;

    case DRAG_RESIZE_BOTTOMLEFT:
        SetPosition(Clamp(dragBeginPosition_.x + delta.x, position.x - (maxSize.x - size.x),
            position.x + (size.x - effectiveMinSize.x)), dragBeginPosition_.y);
        dragSize = IntVector2(dragBeginSize_.x - delta.x, dragBeginSize_.y + delta.y);
        fixedWidthResizing_ ? SetFixedWidth(Max(dragSize.x, resizeBorderSize.x)) : SetWidth(dragSize.x);
        fixedHeightResizing_ ? SetFixedHeight(Max(dragSize.y, resizeBorderSize.y)) : SetHeight(dragSize.y);
        break;

    case DRAG_RESIZE_LEFT:
        SetPosition(Clamp(dragBeginPosition_.x + delta.x, position.x - (maxSize.x - size.x),
            position.x + (size.x - effectiveMinSize.x)), dragBeginPosition_.y);
        dragSize = IntVector2(dragBeginSize_.x - delta.x, dragBeginSize_.y);
        fixedWidthResizing_ ? SetFixedWidth(Max(dragSize.x, resizeBorderSize.x)) : SetWidth(dragSize.x);
        break;

    default:
        break;
    }

    ValidatePosition();
    SetCursorShape(dragMode_, cursor);
}

void Window::OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags dragButtons, MouseButtonFlags releaseButtons, Cursor* cursor)
{
    UiElement::OnDragEnd(position, screenPosition, dragButtons, releaseButtons, cursor);

    dragMode_ = DRAG_NONE;
}

void Window::OnDragCancel(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags dragButtons, MouseButtonFlags cancelButtons,
    Cursor* cursor)
{
    UiElement::OnDragCancel(position, screenPosition, dragButtons, cancelButtons, cursor);

    if (dragButtons == MOUSEB_LEFT && dragMode_ != DRAG_NONE)
    {
        dragMode_ = DRAG_NONE;
        SetPosition(dragBeginPosition_);
        SetSize(dragBeginSize_);
    }
}

void Window::SetMovable(bool enable)
{
    movable_ = enable;
}

void Window::SetResizable(bool enable)
{
    resizable_ = enable;
}

void Window::SetFixedWidthResizing(bool enable)
{
    fixedWidthResizing_ = enable;
}

void Window::SetFixedHeightResizing(bool enable)
{
    fixedHeightResizing_ = enable;
}

void Window::SetResizeBorder(const IntRect& rect)
{
    resizeBorder_.left_ = Max(rect.left_, 0);
    resizeBorder_.top_ = Max(rect.top_, 0);
    resizeBorder_.right_ = Max(rect.right_, 0);
    resizeBorder_.bottom_ = Max(rect.bottom_, 0);
}

void Window::SetModal(bool modal)
{
    if (DV_UI->SetModalElement(this, modal))
    {
        modal_ = modal;

        using namespace ModalChanged;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_MODAL] = modal;
        SendEvent(E_MODALCHANGED, eventData);
    }
}

void Window::SetModalShadeColor(const Color& color)
{
    modalShadeColor_ = color;
}

void Window::SetModalFrameColor(const Color& color)
{
    modalFrameColor_ = color;
}

void Window::SetModalFrameSize(const IntVector2& size)
{
    modalFrameSize_ = size;
}

void Window::SetModalAutoDismiss(bool enable)
{
    modalAutoDismiss_ = enable;
}

WindowDragMode Window::GetDragMode(const IntVector2& position) const
{
    WindowDragMode mode = DRAG_NONE;

    // Top row
    if (position.y < resizeBorder_.top_)
    {
        if (movable_)
            mode = DRAG_MOVE;
        if (resizable_)
        {
            mode = DRAG_RESIZE_TOP;
            if (position.x < resizeBorder_.left_)
                mode = DRAG_RESIZE_TOPLEFT;
            if (position.x >= GetWidth() - resizeBorder_.right_)
                mode = DRAG_RESIZE_TOPRIGHT;
        }
    }
    // Bottom row
    else if (position.y >= GetHeight() - resizeBorder_.bottom_)
    {
        if (movable_)
            mode = DRAG_MOVE;
        if (resizable_)
        {
            mode = DRAG_RESIZE_BOTTOM;
            if (position.x < resizeBorder_.left_)
                mode = DRAG_RESIZE_BOTTOMLEFT;
            if (position.x >= GetWidth() - resizeBorder_.right_)
                mode = DRAG_RESIZE_BOTTOMRIGHT;
        }
    }
    // Middle
    else
    {
        if (movable_)
            mode = DRAG_MOVE;
        if (resizable_)
        {
            if (position.x < resizeBorder_.left_)
                mode = DRAG_RESIZE_LEFT;
            if (position.x >= GetWidth() - resizeBorder_.right_)
                mode = DRAG_RESIZE_RIGHT;
        }
    }

    return mode;
}

void Window::SetCursorShape(WindowDragMode mode, Cursor* cursor) const
{
    CursorShape shape = CS_NORMAL;

    switch (mode)
    {
    case DRAG_RESIZE_TOP:
    case DRAG_RESIZE_BOTTOM:
        shape = CS_RESIZEVERTICAL;
        break;

    case DRAG_RESIZE_LEFT:
    case DRAG_RESIZE_RIGHT:
        shape = CS_RESIZEHORIZONTAL;
        break;

    case DRAG_RESIZE_TOPRIGHT:
    case DRAG_RESIZE_BOTTOMLEFT:
        shape = CS_RESIZEDIAGONAL_TOPRIGHT;
        break;

    case DRAG_RESIZE_TOPLEFT:
    case DRAG_RESIZE_BOTTOMRIGHT:
        shape = CS_RESIZEDIAGONAL_TOPLEFT;
        break;

    default:
        break;
    }

    if (cursor)
        cursor->SetShape(shape);
}

void Window::ValidatePosition()
{
    // Check that window does not go more than halfway outside its parent in either dimension
    if (!parent_)
        return;

    const IntVector2& parentSize = parent_->GetSize();
    IntVector2 position = GetPosition();
    IntVector2 halfSize = GetSize() / 2;

    position.x = Clamp(position.x, -halfSize.x, parentSize.x - halfSize.x);
    position.y = Clamp(position.y, -halfSize.y, parentSize.y - halfSize.y);

    SetPosition(position);
}

bool Window::CheckAlignment() const
{
    // Only top left-alignment is supported for move and resize
    if (GetHorizontalAlignment() == HA_LEFT && GetVerticalAlignment() == VA_TOP)
        return true;
    else
        return false;
}

}
