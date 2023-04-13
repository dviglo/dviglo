// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

/// \file

#pragma once

#include "border_image.h"

namespace dviglo
{

/// %Window movement and resizing modes.
enum WindowDragMode
{
    DRAG_NONE,
    DRAG_MOVE,
    DRAG_RESIZE_TOPLEFT,
    DRAG_RESIZE_TOP,
    DRAG_RESIZE_TOPRIGHT,
    DRAG_RESIZE_RIGHT,
    DRAG_RESIZE_BOTTOMRIGHT,
    DRAG_RESIZE_BOTTOM,
    DRAG_RESIZE_BOTTOMLEFT,
    DRAG_RESIZE_LEFT
};

/// %Window %UI element that can optionally by moved or resized.
class DV_API Window : public BorderImage
{
    DV_OBJECT(Window);

public:
    /// Construct.
    explicit Window();
    /// Destruct.
    ~Window() override;
    /// Register object factory.
    static void register_object();

    /// Return UI rendering batches.
    void GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor) override;

    /// React to mouse hover.
    void OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to mouse drag begin.
    void
        OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to mouse drag motion.
    void OnDragMove
        (const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons, QualifierFlags qualifiers,
            Cursor* cursor) override;
    /// React to mouse drag end.
    void
        OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags dragButtons, MouseButtonFlags releaseButtons, Cursor* cursor) override;
    /// React to mouse drag cancel.
    void
        OnDragCancel(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags dragButtons, MouseButtonFlags cancelButtons, Cursor* cursor) override;

    /// Set whether can be moved.
    void SetMovable(bool enable);
    /// Set whether can be resized.
    void SetResizable(bool enable);
    /// Set whether resizing width is fixed.
    void SetFixedWidthResizing(bool enable);
    /// Set whether resizing height is fixed.
    void SetFixedHeightResizing(bool enable);
    /// Set resize area width at edges.
    void SetResizeBorder(const IntRect& rect);
    /// Set modal flag. When the modal flag is set, the focused window needs to be dismissed first to allow other UI elements to gain focus.
    void SetModal(bool modal);
    /// Set modal shade color.
    void SetModalShadeColor(const Color& color);
    /// Set modal frame color.
    void SetModalFrameColor(const Color& color);
    /// Set modal frame size.
    void SetModalFrameSize(const IntVector2& size);
    /// Set whether model window can be dismissed with the escape key. Default true.
    void SetModalAutoDismiss(bool enable);

    /// Return whether is movable.
    bool IsMovable() const { return movable_; }

    /// Return whether is resizable.
    bool IsResizable() const { return resizable_; }

    /// Return whether is resizing width is fixed.
    bool GetFixedWidthResizing() const { return fixedWidthResizing_; }

    /// Return whether is resizing height is fixed.
    bool GetFixedHeightResizing() const { return fixedHeightResizing_; }

    /// Return resize area width at edges.
    const IntRect& GetResizeBorder() const { return resizeBorder_; }

    /// Return modal flag.
    bool IsModal() const { return modal_; }

    /// Get modal shade color.
    const Color& GetModalShadeColor() const { return modalShadeColor_; }

    /// Get modal frame color.
    const Color& GetModalFrameColor() const { return modalFrameColor_; }

    /// Get modal frame size.
    const IntVector2& GetModalFrameSize() const { return modalFrameSize_; }

    /// Return whether can be dismissed with escape key.
    bool GetModalAutoDismiss() const { return modalAutoDismiss_; }

protected:
    /// Identify drag mode (move/resize).
    WindowDragMode GetDragMode(const IntVector2& position) const;
    /// Set cursor shape based on drag mode.
    void SetCursorShape(WindowDragMode mode, Cursor* cursor) const;
    /// Validate window position.
    void ValidatePosition();
    /// Check whether alignment supports moving and resizing.
    bool CheckAlignment() const;

    /// Movable flag.
    bool movable_;
    /// Resizable flag.
    bool resizable_;
    /// Fixed width resize flag.
    bool fixedWidthResizing_;
    /// Fixed height resize flag.
    bool fixedHeightResizing_;
    /// Resize area width at edges.
    IntRect resizeBorder_;
    /// Current drag mode.
    WindowDragMode dragMode_;
    /// Mouse position at drag begin.
    IntVector2 dragBeginCursor_;
    /// Original position at drag begin.
    IntVector2 dragBeginPosition_;
    /// Original size at drag begin.
    IntVector2 dragBeginSize_;
    /// Modal flag.
    bool modal_;
    /// Modal auto dismiss (with escape key) flag. Default true.
    bool modalAutoDismiss_;
    /// Modal shade color, used when modal flag is set.
    Color modalShadeColor_;
    /// Modal frame color, used when modal flag is set.
    Color modalFrameColor_;
    /// Modal frame size, used when modal flag is set.
    IntVector2 modalFrameSize_;
};

}
