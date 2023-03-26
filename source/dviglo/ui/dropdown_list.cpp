// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../input/input_events.h"
#include "../io/log.h"
#include "dropdown_list.h"
#include "list_view.h"
#include "text.h"
#include "ui.h"
#include "ui_events.h"
#include "window.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* UI_CATEGORY;

DropDownList::DropDownList() :
    resizePopup_(false),
    selectionAttr_(0)
{
    focusMode_ = FM_FOCUSABLE_DEFOCUSABLE;

    auto* window = new Window();
    window->SetInternal(true);
    SetPopup(window);

    listView_ = new ListView();
    listView_->SetInternal(true);
    listView_->SetScrollBarsVisible(false, false);
    popup_->SetLayout(LM_VERTICAL);
    popup_->AddChild(listView_);
    placeholder_ = create_child<UiElement>("DDL_Placeholder");
    placeholder_->SetInternal(true);
    auto* text = placeholder_->create_child<Text>("DDL_Placeholder_Text");
    text->SetInternal(true);
    text->SetVisible(false);

    subscribe_to_event(listView_, E_ITEMCLICKED, DV_HANDLER(DropDownList, HandleItemClicked));
    subscribe_to_event(listView_, E_UNHANDLEDKEY, DV_HANDLER(DropDownList, HandleListViewKey));
    subscribe_to_event(listView_, E_SELECTIONCHANGED, DV_HANDLER(DropDownList, HandleSelectionChanged));
}

DropDownList::~DropDownList() = default;

void DropDownList::register_object()
{
    DV_CONTEXT.RegisterFactory<DropDownList>(UI_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(Menu);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Focus Mode", FM_FOCUSABLE_DEFOCUSABLE);
    DV_ACCESSOR_ATTRIBUTE("Selection", GetSelection, SetSelectionAttr, 0, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Resize Popup", GetResizePopup, SetResizePopup, false, AM_FILE);
}

void DropDownList::apply_attributes()
{
    // Reapply selection after possible items have been loaded
    SetSelection(selectionAttr_);
}

void DropDownList::GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor)
{
    Menu::GetBatches(batches, vertexData, currentScissor);

    if (!placeholder_->IsVisible())
        return;

    UiElement* selectedItem = GetSelectedItem();
    if (selectedItem)
    {
        // Can not easily copy the selected item. However, it can be re-rendered on the placeholder's position
        const IntVector2& targetPos = placeholder_->GetScreenPosition();
        const IntVector2& originalPos = selectedItem->GetScreenPosition();
        IntVector2 offset = targetPos - originalPos;

        // GetBatches() usually resets the hover flag. Therefore get its value and then reset it for the real rendering
        // Render the selected item without its selection color, so temporarily reset the item's selected attribute
        bool hover = selectedItem->IsHovering();
        selectedItem->SetSelected(false);
        selectedItem->SetHovering(false);
        selectedItem->GetBatchesWithOffset(offset, batches, vertexData, currentScissor);
        selectedItem->SetSelected(true);
        selectedItem->SetHovering(hover);
    }
}

void DropDownList::OnShowPopup()
{
    // Resize the popup to match the size of the list content, and optionally match the button width
    UiElement* content = listView_->GetContentElement();
    content->UpdateLayout();
    const IntVector2& contentSize = content->GetSize();
    const IntRect& border = popup_->GetLayoutBorder();
    popup_->SetSize(resizePopup_ ? GetWidth() : contentSize.x + border.left_ + border.right_,
        contentSize.y + border.top_ + border.bottom_);

    // Check if popup fits below the button. If not, show above instead
    bool showAbove = false;
    UiElement* root = GetRoot();
    if (root)
    {
        const IntVector2& screenPos = GetScreenPosition();
        if (screenPos.y + GetHeight() + popup_->GetHeight() > root->GetHeight() && screenPos.y - popup_->GetHeight() >= 0)
            showAbove = true;
    }
    SetPopupOffset(0, showAbove ? -popup_->GetHeight() : GetHeight());

    // Focus the ListView to allow making the selection with keys
    DV_UI.SetFocusElement(listView_);
}

void DropDownList::OnHidePopup()
{
    // When the popup is hidden, propagate the selection
    using namespace ItemSelected;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_ELEMENT] = this;
    eventData[P_SELECTION] = GetSelection();
    SendEvent(E_ITEMSELECTED, eventData);
}

void DropDownList::OnSetEditable()
{
    listView_->SetEditable(editable_);
}

void DropDownList::AddItem(UiElement* item)
{
    InsertItem(ENDPOS, item);
}

void DropDownList::InsertItem(i32 index, UiElement* item)
{
    assert(index >= 0 || index == ENDPOS);
    listView_->InsertItem(index, item);

    // If there was no selection, set to the first
    if (GetSelection() == NINDEX)
        SetSelection(0);
}

void DropDownList::RemoveItem(UiElement* item)
{
    listView_->RemoveItem(item);
}

void DropDownList::RemoveItem(i32 index)
{
    assert(index >= 0);
    listView_->RemoveItem(index);
}

void DropDownList::RemoveAllItems()
{
    listView_->RemoveAllItems();
}

void DropDownList::SetSelection(i32 index)
{
    assert(index >= 0);
    listView_->SetSelection(index);
}

void DropDownList::SetPlaceholderText(const String& text)
{
    placeholder_->GetChildStaticCast<Text>(0)->SetText(text);
}

void DropDownList::SetResizePopup(bool enable)
{
    resizePopup_ = enable;
}

i32 DropDownList::GetNumItems() const
{
    return listView_->GetNumItems();
}

UiElement* DropDownList::GetItem(i32 index) const
{
    assert(index >= 0);
    return listView_->GetItem(index);
}

Vector<UiElement*> DropDownList::GetItems() const
{
    return listView_->GetItems();
}

i32 DropDownList::GetSelection() const
{
    return listView_->GetSelection();
}

UiElement* DropDownList::GetSelectedItem() const
{
    return listView_->GetSelectedItem();
}

const String& DropDownList::GetPlaceholderText() const
{
    return placeholder_->GetChildStaticCast<Text>(0)->GetText();
}

void DropDownList::SetSelectionAttr(i32 index)
{
    assert(index >= 0);
    selectionAttr_ = index;

    // We may not have the list items yet. Apply the index again in apply_attributes().
    SetSelection(index);
}

bool DropDownList::FilterImplicitAttributes(XmlElement& dest) const
{
    if (!Menu::FilterImplicitAttributes(dest))
        return false;

    if (!RemoveChildXML(dest, "Popup Offset"))
        return false;

    XmlElement childElem = dest.GetChild("element");
    if (!childElem)
        return false;
    if (!RemoveChildXML(childElem, "Name", "DDL_Placeholder"))
        return false;
    if (!RemoveChildXML(childElem, "Size"))
        return false;

    childElem = childElem.GetChild("element");
    if (!childElem)
        return false;
    if (!RemoveChildXML(childElem, "Name", "DDL_Placeholder_Text"))
        return false;
    if (!RemoveChildXML(childElem, "Is Visible"))
        return false;

    return true;
}

bool DropDownList::FilterPopupImplicitAttributes(XmlElement& dest) const
{
    if (!Menu::FilterPopupImplicitAttributes(dest))
        return false;

    // Window popup
    if (dest.GetAttribute("style").Empty() && !dest.SetAttribute("style", "none"))
        return false;
    if (!RemoveChildXML(dest, "Layout Mode", "Vertical"))
        return false;
    if (!RemoveChildXML(dest, "Size"))
        return false;

    // ListView
    XmlElement childElem = dest.GetChild("element");
    if (!childElem)
        return false;
    if (!listView_->FilterAttributes(childElem))
        return false;
    if (childElem.GetAttribute("style").Empty() && !childElem.SetAttribute("style", "none"))
        return false;
    if (!RemoveChildXML(childElem, "Focus Mode", "NotFocusable"))
        return false;
    if (!RemoveChildXML(childElem, "Auto Show/Hide Scrollbars", "false"))
        return false;

    // Horizontal scroll bar
    XmlElement hScrollElem = childElem.GetChild("element");
    // Vertical scroll bar
    XmlElement vScrollElem = hScrollElem.GetNext("element");
    // Scroll panel
    XmlElement panelElem = vScrollElem.GetNext("element");

    if (hScrollElem && !hScrollElem.GetParent().RemoveChild(hScrollElem))
        return false;
    if (vScrollElem && !vScrollElem.GetParent().RemoveChild(vScrollElem))
        return false;

    if (panelElem)
    {
        if (panelElem.GetAttribute("style").Empty() && !panelElem.SetAttribute("style", "none"))
            return false;
        // Item container
        XmlElement containerElem = panelElem.GetChild("element");
        if (containerElem)
        {
            if (containerElem.GetAttribute("style").Empty() && !containerElem.SetAttribute("style", "none"))
                return false;
        }
    }

    return true;
}

void DropDownList::HandleItemClicked(StringHash eventType, VariantMap& eventData)
{
    // Resize the selection placeholder to match the selected item
    UiElement* selectedItem = GetSelectedItem();
    if (selectedItem)
        placeholder_->SetSize(selectedItem->GetSize());

    // Close and defocus the popup. This will actually send the selection forward
    if (listView_->HasFocus())
        DV_UI.SetFocusElement(focusMode_ < FM_FOCUSABLE ? nullptr : this);
    ShowPopup(false);
}

void DropDownList::HandleListViewKey(StringHash eventType, VariantMap& eventData)
{
    using namespace UnhandledKey;

    // If enter pressed in the list view, close and propagate selection
    int key = eventData[P_KEY].GetI32();
    if (key == KEY_RETURN || key == KEY_RETURN2 || key == KEY_KP_ENTER)
        HandleItemClicked(eventType, eventData);
}

void DropDownList::HandleSelectionChanged(StringHash eventType, VariantMap& eventData)
{
    // Display the place holder text when there is no selection, however, the place holder text is only visible when the place holder itself is set to visible
    placeholder_->GetChild(0)->SetVisible(GetSelection() == NINDEX);
}

}
