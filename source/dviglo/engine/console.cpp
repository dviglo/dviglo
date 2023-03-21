// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../core/core_events.h"
#include "console.h"
#include "engine_events.h"
#include "../graphics/graphics.h"
#include "../input/input.h"
#include "../io/io_events.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "../ui/dropdown_list.h"
#include "../ui/font.h"
#include "../ui/line_edit.h"
#include "../ui/list_view.h"
#include "../ui/scroll_bar.h"
#include "../ui/text.h"
#include "../ui/ui.h"
#include "../ui/ui_events.h"

#include <algorithm>

#include "../common/debug_new.h"

namespace dviglo
{

static const int DEFAULT_CONSOLE_ROWS = 16;
static const int DEFAULT_HISTORY_SIZE = 16;

const char* logStyles[] =
{
    "ConsoleTraceText",
    "ConsoleDebugText",
    "ConsoleInfoText",
    "ConsoleWarningText",
    "ConsoleErrorText",
    "ConsoleText"
};

#ifdef _DEBUG
// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool console_destructed = false;
#endif

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
Console& Console::get_instance()
{
    assert(!console_destructed);
    static Console instance;
    return instance;
}


Console::Console() :
    autoVisibleOnError_(false),
    historyRows_(DEFAULT_HISTORY_SIZE),
    historyPosition_(0),
    autoCompletePosition_(0),
    historyOrAutoCompleteChange_(false),
    printing_(false)
{
    assert(!GParams::is_headless());

    UIElement* uiRoot = DV_UI.GetRoot();

    focusOnShow_ = true;

    background_ = uiRoot->CreateChild<BorderImage>();
    background_->SetBringToBack(false);
    background_->SetClipChildren(true);
    background_->SetEnabled(true);
    background_->SetVisible(false); // Hide by default
    background_->SetPriority(200); // Show on top of the debug HUD
    background_->SetBringToBack(false);
    background_->SetLayout(LM_VERTICAL);

    rowContainer_ = background_->CreateChild<ListView>();
    rowContainer_->SetHighlightMode(HM_ALWAYS);
    rowContainer_->SetMultiselect(true);

    commandLine_ = background_->CreateChild<UIElement>();
    commandLine_->SetLayoutMode(LM_HORIZONTAL);
    commandLine_->SetLayoutSpacing(1);
    interpreters_ = commandLine_->CreateChild<DropDownList>();
    lineEdit_ = commandLine_->CreateChild<LineEdit>();
    lineEdit_->SetFocusMode(FM_FOCUSABLE);  // Do not allow defocus with ESC

    closeButton_ = uiRoot->CreateChild<Button>();
    closeButton_->SetVisible(false);
    closeButton_->SetPriority(background_->GetPriority() + 1);  // Show on top of console's background
    closeButton_->SetBringToBack(false);

    SetNumRows(DEFAULT_CONSOLE_ROWS);

    SubscribeToEvent(interpreters_, E_ITEMSELECTED, DV_HANDLER(Console, HandleInterpreterSelected));
    SubscribeToEvent(lineEdit_, E_TEXTCHANGED, DV_HANDLER(Console, HandleTextChanged));
    SubscribeToEvent(lineEdit_, E_TEXTFINISHED, DV_HANDLER(Console, HandleTextFinished));
    SubscribeToEvent(lineEdit_, E_UNHANDLEDKEY, DV_HANDLER(Console, HandleLineEditKey));
    SubscribeToEvent(closeButton_, E_RELEASED, DV_HANDLER(Console, HandleCloseButtonPressed));
    SubscribeToEvent(uiRoot, E_RESIZED, DV_HANDLER(Console, HandleRootElementResized));
    SubscribeToEvent(E_LOGMESSAGE, DV_HANDLER(Console, HandleLogMessage));
    SubscribeToEvent(E_POSTUPDATE, DV_HANDLER(Console, HandlePostUpdate));

    DV_LOGDEBUG("Singleton Console constructed");
}

Console::~Console()
{
    background_->Remove();
    closeButton_->Remove();

    DV_LOGDEBUG("Singleton Console destructed");

#ifdef _DEBUG
    console_destructed = true;
#endif
}

void Console::SetDefaultStyle(XMLFile* style)
{
    if (!style)
        return;

    background_->SetDefaultStyle(style);
    background_->SetStyle("ConsoleBackground");
    rowContainer_->SetStyleAuto();
    for (i32 i = 0; i < rowContainer_->GetNumItems(); ++i)
        rowContainer_->GetItem(i)->SetStyle("ConsoleText");
    interpreters_->SetStyleAuto();
    for (i32 i = 0; i < interpreters_->GetNumItems(); ++i)
        interpreters_->GetItem(i)->SetStyle("ConsoleText");
    lineEdit_->SetStyle("ConsoleLineEdit");

    closeButton_->SetDefaultStyle(style);
    closeButton_->SetStyle("CloseButton");

    UpdateElements();
}

void Console::SetVisible(bool enable)
{
    Input& input = DV_INPUT;
    Cursor* cursor = DV_UI.GetCursor();

    background_->SetVisible(enable);
    closeButton_->SetVisible(enable);

    if (enable)
    {
        // Check if we have receivers for E_CONSOLECOMMAND every time here in case the handler is being added later dynamically
        bool hasInterpreter = PopulateInterpreter();
        commandLine_->SetVisible(hasInterpreter);
        if (hasInterpreter && focusOnShow_)
            DV_UI.SetFocusElement(lineEdit_);

        // Ensure the background has no empty space when shown without the lineedit
        background_->SetHeight(background_->GetMinHeight());

        if (!cursor)
        {
            // Show OS mouse
            input.SetMouseMode(MM_FREE, true);
            input.SetMouseVisible(true, true);
        }

        input.SetMouseGrabbed(false, true);
    }
    else
    {
        rowContainer_->SetFocus(false);
        interpreters_->SetFocus(false);
        lineEdit_->SetFocus(false);

        if (!cursor)
        {
            // Restore OS mouse visibility
            input.ResetMouseMode();
            input.ResetMouseVisible();
        }

        input.ResetMouseGrabbed();
    }
}

void Console::Toggle()
{
    SetVisible(!IsVisible());
}

void Console::SetNumBufferedRows(i32 rows)
{
    assert(rows >= 0);

    if (rows < displayedRows_)
        return;

    rowContainer_->DisableLayoutUpdate();

    int delta = rowContainer_->GetNumItems() - rows;
    if (delta > 0)
    {
        // We have more, remove oldest rows first
        for (int i = 0; i < delta; ++i)
            rowContainer_->RemoveItem(0);
    }
    else
    {
        // We have less, add more rows at the top
        for (int i = 0; i > delta; --i)
        {
            auto* text = new Text();
            // If style is already set, apply here to ensure proper height of the console when
            // amount of rows is changed
            if (background_->GetDefaultStyle())
                text->SetStyle("ConsoleText");
            rowContainer_->InsertItem(0, text);
        }
    }

    rowContainer_->EnsureItemVisibility(rowContainer_->GetItem(rowContainer_->GetNumItems() - 1));
    rowContainer_->EnableLayoutUpdate();
    rowContainer_->UpdateLayout();

    UpdateElements();
}

void Console::SetNumRows(i32 rows)
{
    assert(rows >= 0);

    if (!rows)
        return;

    displayedRows_ = rows;
    if (GetNumBufferedRows() < rows)
        SetNumBufferedRows(rows);

    UpdateElements();
}

void Console::SetNumHistoryRows(i32 rows)
{
    assert(rows >= 0);

    historyRows_ = rows;
    if (history_.Size() > rows)
        history_.Resize(rows);
    if (historyPosition_ > rows)
        historyPosition_ = rows;
}

void Console::SetFocusOnShow(bool enable)
{
    focusOnShow_ = enable;
}

void Console::AddAutoComplete(const String& option)
{
    // Sorted insertion
    Vector<String>::Iterator iter = std::upper_bound(autoComplete_.Begin(), autoComplete_.End(), option);
    if (!iter.ptr_)
        autoComplete_.Push(option);
    // Make sure it isn't a duplicate
    else if (iter == autoComplete_.Begin() || *(iter - 1) != option)
        autoComplete_.Insert(iter, option);
}

void Console::RemoveAutoComplete(const String& option)
{
    // Erase and keep ordered
    autoComplete_.Erase(std::lower_bound(autoComplete_.Begin(), autoComplete_.End(), option));
    if (autoCompletePosition_ > autoComplete_.Size())
        autoCompletePosition_ = autoComplete_.Size();
}

void Console::UpdateElements()
{
    int width = DV_UI.GetRoot()->GetWidth();
    const IntRect& border = background_->GetLayoutBorder();
    const IntRect& panelBorder = rowContainer_->GetScrollPanel()->GetClipBorder();
    rowContainer_->SetFixedWidth(width - border.left_ - border.right_);
    rowContainer_->SetFixedHeight(
        displayedRows_ * rowContainer_->GetItem((unsigned)0)->GetHeight() + panelBorder.top_ + panelBorder.bottom_ +
        (rowContainer_->GetHorizontalScrollBar()->IsVisible() ? rowContainer_->GetHorizontalScrollBar()->GetHeight() : 0));
    background_->SetFixedWidth(width);
    background_->SetHeight(background_->GetMinHeight());
}

XMLFile* Console::GetDefaultStyle() const
{
    return background_->GetDefaultStyle(false);
}

bool Console::IsVisible() const
{
    return background_ && background_->IsVisible();
}

i32 Console::GetNumBufferedRows() const
{
    return rowContainer_->GetNumItems();
}

void Console::CopySelectedRows() const
{
    rowContainer_->CopySelectedItemsToClipboard();
}

const String& Console::GetHistoryRow(i32 index) const
{
    assert(index >= 0);
    return index < history_.Size() ? history_[index] : String::EMPTY;
}

bool Console::PopulateInterpreter()
{
    interpreters_->RemoveAllItems();

    EventReceiverGroup* group = DV_CONTEXT.GetEventReceivers(E_CONSOLECOMMAND);
    if (!group || group->receivers_.Empty())
        return false;

    Vector<String> names;
    for (const Object* receiver : group->receivers_)
    {
        if (receiver)
            names.Push(receiver->GetTypeName());
    }
    std::sort(names.Begin(), names.End());

    i32 selection = NINDEX;
    for (i32 i = 0; i < names.Size(); ++i)
    {
        const String& name = names[i];
        if (name == commandInterpreter_)
            selection = i;
        Text* text = new Text();
        text->SetStyle("ConsoleText");
        text->SetText(name);
        interpreters_->AddItem(text);
    }

    const IntRect& border = interpreters_->GetPopup()->GetLayoutBorder();
    interpreters_->SetMaxWidth(interpreters_->GetListView()->GetContentElement()->GetWidth() + border.left_ + border.right_);
    bool enabled = interpreters_->GetNumItems() > 1;
    interpreters_->SetEnabled(enabled);
    interpreters_->SetFocusMode(enabled ? FM_FOCUSABLE_DEFOCUSABLE : FM_NOTFOCUSABLE);

    if (selection == NINDEX)
    {
        selection = 0;
        commandInterpreter_ = names[selection];
    }
    interpreters_->SetSelection(selection);

    return true;
}

void Console::HandleInterpreterSelected(StringHash eventType, VariantMap& eventData)
{
    commandInterpreter_ = static_cast<Text*>(interpreters_->GetSelectedItem())->GetText();
    lineEdit_->SetFocus(true);
}

void Console::HandleTextChanged(StringHash eventType, VariantMap & eventData)
{
    // Save the original line
    // Make sure the change isn't caused by auto complete or history
    if (!historyOrAutoCompleteChange_)
        autoCompleteLine_ = eventData[TextEntry::P_TEXT].GetString();

    historyOrAutoCompleteChange_ = false;
}

void Console::HandleTextFinished(StringHash eventType, VariantMap& eventData)
{
    using namespace TextFinished;

    String line = lineEdit_->GetText();
    if (!line.Empty())
    {
        // Send the command as an event for script subsystem
        using namespace ConsoleCommand;

        SendEvent(E_CONSOLECOMMAND,
            P_COMMAND, line,
            P_ID, static_cast<Text*>(interpreters_->GetSelectedItem())->GetText());

        // Make sure the line isn't the same as the last one
        if (history_.Empty() || line != history_.Back())
        {
            // Store to history, then clear the lineedit
            history_.Push(line);
            if (history_.Size() > historyRows_)
                history_.Erase(history_.Begin());
        }

        historyPosition_ = history_.Size(); // Reset
        autoCompletePosition_ = autoComplete_.Size(); // Reset

        currentRow_.Clear();
        lineEdit_->SetText(currentRow_);
    }
}

void Console::HandleLineEditKey(StringHash eventType, VariantMap& eventData)
{
    if (!historyRows_)
        return;

    using namespace UnhandledKey;

    bool changed = false;

    switch (eventData[P_KEY].GetI32())
    {
    case KEY_UP:
        if (autoCompletePosition_ == 0)
            autoCompletePosition_ = autoComplete_.Size();

        if (autoCompletePosition_ < autoComplete_.Size())
        {
            // Search for auto completion that contains the contents of the line
            for (--autoCompletePosition_; autoCompletePosition_ >= 0; --autoCompletePosition_)
            {
                const String& current = autoComplete_[autoCompletePosition_];
                if (current.StartsWith(autoCompleteLine_))
                {
                    historyOrAutoCompleteChange_ = true;
                    lineEdit_->SetText(current);
                    break;
                }
            }

            // If not found
            if (autoCompletePosition_ < 0)
            {
                // Reset the position
                autoCompletePosition_ = autoComplete_.Size();
                // Reset history position
                historyPosition_ = history_.Size();
            }
        }

        // If no more auto complete options and history options left
        if (autoCompletePosition_ == autoComplete_.Size() && historyPosition_ > 0)
        {
            // If line text is not a history, save the current text value to be restored later
            if (historyPosition_ == history_.Size())
                currentRow_ = lineEdit_->GetText();
            // Use the previous option
            --historyPosition_;
            changed = true;
        }
        break;

    case KEY_DOWN:
        // If history options left
        if (historyPosition_ < history_.Size())
        {
            // Use the next option
            ++historyPosition_;
            changed = true;
        }
        else
        {
            // Loop over
            if (autoCompletePosition_ >= autoComplete_.Size())
                autoCompletePosition_ = 0;
            else
                ++autoCompletePosition_; // If not starting over, skip checking the currently found completion

            i32 startPosition = autoCompletePosition_;

            // Search for auto completion that contains the contents of the line
            for (; autoCompletePosition_ < autoComplete_.Size(); ++autoCompletePosition_)
            {
                const String& current = autoComplete_[autoCompletePosition_];
                if (current.StartsWith(autoCompleteLine_))
                {
                    historyOrAutoCompleteChange_ = true;
                    lineEdit_->SetText(current);
                    break;
                }
            }

            // Continue to search the complete range
            if (autoCompletePosition_ == autoComplete_.Size())
            {
                for (autoCompletePosition_ = 0; autoCompletePosition_ != startPosition; ++autoCompletePosition_)
                {
                    const String& current = autoComplete_[autoCompletePosition_];
                    if (current.StartsWith(autoCompleteLine_))
                    {
                        historyOrAutoCompleteChange_ = true;
                        lineEdit_->SetText(current);
                        break;
                    }
                }
            }
        }
        break;

    default: break;
    }

    if (changed)
    {
        historyOrAutoCompleteChange_ = true;
        // Set text to history option
        if (historyPosition_ < history_.Size())
            lineEdit_->SetText(history_[historyPosition_]);
        else // restore the original line value before it was set to history values
        {
            lineEdit_->SetText(currentRow_);
            // Set the auto complete position according to the currentRow
            for (autoCompletePosition_ = 0; autoCompletePosition_ < autoComplete_.Size(); ++autoCompletePosition_)
                if (autoComplete_[autoCompletePosition_].StartsWith(currentRow_))
                    break;
        }
    }
}

void Console::HandleCloseButtonPressed(StringHash eventType, VariantMap& eventData)
{
    SetVisible(false);
}

void Console::HandleRootElementResized(StringHash eventType, VariantMap& eventData)
{
    UpdateElements();
}

void Console::HandleLogMessage(StringHash eventType, VariantMap& eventData)
{
    // If printing a log message causes more messages to be logged (error accessing font), disregard them
    if (printing_)
        return;

    using namespace LogMessage;

    int level = eventData[P_LEVEL].GetI32();
    // The message may be multi-line, so split to rows in that case
    Vector<String> rows = eventData[P_MESSAGE].GetString().Split('\n');

    for (const String& row : rows)
        pendingRows_.Push(MakePair(level, row));

    if (autoVisibleOnError_ && level == LOG_ERROR && !IsVisible())
        SetVisible(true);
}

void Console::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    // Ensure UI-elements are not detached
    if (!background_->GetParent())
    {
        UIElement* uiRoot = DV_UI.GetRoot();
        uiRoot->AddChild(background_);
        uiRoot->AddChild(closeButton_);
    }

    if (!rowContainer_->GetNumItems() || pendingRows_.Empty())
        return;

    printing_ = true;
    rowContainer_->DisableLayoutUpdate();

    Text* text = nullptr;
    for (const Pair<i32, String>& pendingRow : pendingRows_)
    {
        rowContainer_->RemoveItem(0);
        text = new Text();
        text->SetText(pendingRow.second_);

        // Highlight console messages based on their type
        text->SetStyle(logStyles[pendingRow.first_]);

        rowContainer_->AddItem(text);
    }

    pendingRows_.Clear();

    rowContainer_->EnsureItemVisibility(text);
    rowContainer_->EnableLayoutUpdate();
    rowContainer_->UpdateLayout();
    UpdateElements();   // May need to readjust the height due to scrollbar visibility changes
    printing_ = false;
}

}
