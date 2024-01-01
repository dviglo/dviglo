// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Global mouse click in the UI. Sent by the UI subsystem.
DV_EVENT(E_UIMOUSECLICK, UIMouseClick)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Global mouse click end in the UI. Sent by the UI subsystem.
DV_EVENT(E_UIMOUSECLICKEND, UIMouseClickEnd)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_BEGINELEMENT, BeginElement);    // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Global mouse double click in the UI. Sent by the UI subsystem.
DV_EVENT(E_UIMOUSEDOUBLECLICK, UIMouseDoubleClick)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_XBEGIN, XBegin);                // int
    DV_PARAM(P_YBEGIN, YBegin);                // int
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse click on a UI element. Parameters are same as in UIMouseClick event, but is sent by the element.
DV_EVENT(E_CLICK, Click)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse click end on a UI element. Parameters are same as in UIMouseClickEnd event, but is sent by the element.
DV_EVENT(E_CLICKEND, ClickEnd)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_BEGINELEMENT, BeginElement);    // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse double click on a UI element. Parameters are same as in UIMouseDoubleClick event, but is sent by the element.
DV_EVENT(E_DOUBLECLICK, DoubleClick)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_XBEGIN, XBegin);                // int
    DV_PARAM(P_YBEGIN, YBegin);                // int
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Drag and drop test.
DV_EVENT(E_DRAGDROPTEST, DragDropTest)
{
    DV_PARAM(P_SOURCE, Source);                // UiElement pointer
    DV_PARAM(P_TARGET, Target);                // UiElement pointer
    DV_PARAM(P_ACCEPT, Accept);                // bool
}

/// Drag and drop finish.
DV_EVENT(E_DRAGDROPFINISH, DragDropFinish)
{
    DV_PARAM(P_SOURCE, Source);                // UiElement pointer
    DV_PARAM(P_TARGET, Target);                // UiElement pointer
    DV_PARAM(P_ACCEPT, Accept);                // bool
}

/// Focus element changed.
DV_EVENT(E_FOCUSCHANGED, FocusChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_CLICKEDELEMENT, ClickedElement); // UiElement pointer
}

/// UI element name changed.
DV_EVENT(E_NAMECHANGED, NameChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// UI element resized.
DV_EVENT(E_RESIZED, Resized)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_WIDTH, Width);                  // int
    DV_PARAM(P_HEIGHT, Height);                // int
    DV_PARAM(P_DX, DX);                        // int
    DV_PARAM(P_DY, DY);                        // int
}

/// UI element positioned.
DV_EVENT(E_POSITIONED, Positioned)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
}

/// UI element visibility changed.
DV_EVENT(E_VISIBLECHANGED, VisibleChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_VISIBLE, Visible);              // bool
}

/// UI element focused.
DV_EVENT(E_FOCUSED, Focused)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_BYKEY, ByKey);                  // bool
}

/// UI element defocused.
DV_EVENT(E_DEFOCUSED, Defocused)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// UI element layout updated.
DV_EVENT(E_LAYOUTUPDATED, LayoutUpdated)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// UI button pressed.
DV_EVENT(E_PRESSED, Pressed)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// UI button was pressed, then released.
DV_EVENT(E_RELEASED, Released)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// UI checkbox toggled.
DV_EVENT(E_TOGGLED, Toggled)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_STATE, State);                  // bool
}

/// UI slider value changed.
DV_EVENT(E_SLIDERCHANGED, SliderChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_VALUE, Value);                  // float
}

/// UI slider being paged.
DV_EVENT(E_SLIDERPAGED, SliderPaged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_OFFSET, Offset);                // int
    DV_PARAM(P_PRESSED, Pressed);              // bool
}

/// UI progressbar value changed.
DV_EVENT(E_PROGRESSBARCHANGED, ProgressBarChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_VALUE, Value);                  // float
}

/// UI scrollbar value changed.
DV_EVENT(E_SCROLLBARCHANGED, ScrollBarChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_VALUE, Value);                  // float
}

/// UI scrollview position changed.
DV_EVENT(E_VIEWCHANGED, ViewChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
}

/// UI modal changed (currently only Window has modal flag).
DV_EVENT(E_MODALCHANGED, ModalChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_MODAL, Modal);                  // bool
}

/// Text entry into a LineEdit. The text can be modified in the event data.
DV_EVENT(E_TEXTENTRY, TextEntry)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_TEXT, Text);                    // String [in/out]
}

/// Editable text changed.
DV_EVENT(E_TEXTCHANGED, TextChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_TEXT, Text);                    // String
}

/// Text editing finished (enter pressed on a LineEdit).
DV_EVENT(E_TEXTFINISHED, TextFinished)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_TEXT, Text);                    // String
    DV_PARAM(P_VALUE, Value);                  // Float
}

/// Menu selected.
DV_EVENT(E_MENUSELECTED, MenuSelected)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// Listview or DropDownList item selected.
DV_EVENT(E_ITEMSELECTED, ItemSelected)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_SELECTION, Selection);          // int
}

/// Listview item deselected.
DV_EVENT(E_ITEMDESELECTED, ItemDeselected)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_SELECTION, Selection);          // int
}

/// Listview selection change finished.
DV_EVENT(E_SELECTIONCHANGED, SelectionChanged)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// Listview item clicked. If this is a left-click, also ItemSelected event will be sent. If this is a right-click, only this event is sent.
DV_EVENT(E_ITEMCLICKED, ItemClicked)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_ITEM, Item);                    // UiElement pointer
    DV_PARAM(P_SELECTION, Selection);          // int
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Listview item double clicked.
DV_EVENT(E_ITEMDOUBLECLICKED, ItemDoubleClicked)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_ITEM, Item);                    // UiElement pointer
    DV_PARAM(P_SELECTION, Selection);          // int
    DV_PARAM(P_BUTTON, Button);                // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// LineEdit or ListView unhandled key pressed.
DV_EVENT(E_UNHANDLEDKEY, UnhandledKey)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_KEY, Key);                      // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Fileselector choice.
DV_EVENT(E_FILESELECTED, FileSelected)
{
    DV_PARAM(P_FILENAME, FileName);            // String
    DV_PARAM(P_FILTER, Filter);                // String
    DV_PARAM(P_OK, OK);                        // bool
}

/// MessageBox acknowlegement.
DV_EVENT(E_MESSAGEACK, MessageACK)
{
    DV_PARAM(P_OK, OK);                        // bool
}

/// A child element has been added to an element. Sent by the UI root element, or element-event-sender if set.
DV_EVENT(E_ELEMENTADDED, ElementAdded)
{
    DV_PARAM(P_ROOT, Root);                    // UiElement pointer
    DV_PARAM(P_PARENT, Parent);                // UiElement pointer
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// A child element is about to be removed from an element. Sent by the UI root element, or element-event-sender if set.
DV_EVENT(E_ELEMENTREMOVED, ElementRemoved)
{
    DV_PARAM(P_ROOT, Root);                    // UiElement pointer
    DV_PARAM(P_PARENT, Parent);                // UiElement pointer
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// Hovering on an UI element has started.
DV_EVENT(E_HOVERBEGIN, HoverBegin)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_ELEMENTX, ElementX);            // int
    DV_PARAM(P_ELEMENTY, ElementY);            // int
}

/// Hovering on an UI element has ended.
DV_EVENT(E_HOVEREND, HoverEnd)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
}

/// Drag behavior of a UI Element has started.
DV_EVENT(E_DRAGBEGIN, DragBegin)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_ELEMENTX, ElementX);            // int
    DV_PARAM(P_ELEMENTY, ElementY);            // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_NUMBUTTONS, NumButtons);        // int
}

/// Drag behavior of a UI Element when the input device has moved.
DV_EVENT(E_DRAGMOVE, DragMove)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_DX, DX);                        // int
    DV_PARAM(P_DY, DY);                        // int
    DV_PARAM(P_ELEMENTX, ElementX);            // int
    DV_PARAM(P_ELEMENTY, ElementY);            // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_NUMBUTTONS, NumButtons);        // int
}

/// Drag behavior of a UI Element has finished.
DV_EVENT(E_DRAGEND, DragEnd)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_ELEMENTX, ElementX);            // int
    DV_PARAM(P_ELEMENTY, ElementY);            // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_NUMBUTTONS, NumButtons);        // int
}

/// Drag of a UI Element was canceled by pressing ESC.
DV_EVENT(E_DRAGCANCEL, DragCancel)
{
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_ELEMENTX, ElementX);            // int
    DV_PARAM(P_ELEMENTY, ElementY);            // int
    DV_PARAM(P_BUTTONS, Buttons);              // int
    DV_PARAM(P_NUMBUTTONS, NumButtons);        // int
}

/// A file was drag-dropped into the application window. Includes also coordinates and UI element if applicable.
DV_EVENT(E_UIDROPFILE, UIDropFile)
{
    DV_PARAM(P_FILENAME, FileName);            // String
    DV_PARAM(P_ELEMENT, Element);              // UiElement pointer
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
    DV_PARAM(P_ELEMENTX, ElementX);            // int (only if element is non-null)
    DV_PARAM(P_ELEMENTY, ElementY);            // int (only if element is non-null)
}

}
