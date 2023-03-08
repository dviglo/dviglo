// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui_events.h>

#include "drag.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(UIDrag)

UIDrag::UIDrag()
{
}

void UIDrag::Start()
{
    // Execute base class startup
    Sample::Start();

    // Set mouse visible
    String platform = GetPlatform();
    if (platform != "Android" && platform != "iOS")
        DV_INPUT.SetMouseVisible(true);

    // Create the UI content
    CreateGUI();
    CreateInstructions();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void UIDrag::CreateGUI()
{
    auto* ui = GetSubsystem<UI>();

    UIElement* root = ui->GetRoot();
    // Load the style sheet from xml
    root->SetDefaultStyle(DV_RES_CACHE.GetResource<XMLFile>("UI/DefaultStyle.xml"));

    for (int i=0; i < 10; i++)
    {
        auto* b = new Button();
        root->AddChild(b);
        // Reference a style from the style sheet loaded earlier:
        b->SetStyleAuto();
        b->SetMinWidth(250);
        b->SetPosition(IntVector2(50*i, 50*i));

        // Enable the bring-to-front flag and set the initial priority
        b->SetBringToFront(true);
        b->SetPriority(i);

        // Set the layout mode to make the child text elements aligned vertically
        b->SetLayout(LM_VERTICAL, 20, {40, 40, 40, 40});
        auto dragInfos = {"Num Touch", "Text", "Event Touch"};
        for (auto name: dragInfos)
            b->CreateChild<Text>(name)->SetStyleAuto();

        if (i % 2 == 0)
            b->AddTag("SomeTag");

        SubscribeToEvent(b, E_CLICK, DV_HANDLER(UIDrag, HandleClick));
        SubscribeToEvent(b, E_DRAGMOVE, DV_HANDLER(UIDrag, HandleDragMove));
        SubscribeToEvent(b, E_DRAGBEGIN, DV_HANDLER(UIDrag, HandleDragBegin));
        SubscribeToEvent(b, E_DRAGCANCEL, DV_HANDLER(UIDrag, HandleDragCancel));
    }

    for (int i = 0; i < 10; i++)
    {
        auto* t = new Text();
        root->AddChild(t);
        t->SetStyleAuto();
        t->SetName("Touch "+ String(i));
        t->SetVisible(false);
        t->SetPriority(100);     // Make sure it has higher priority than the buttons
    }
}

void UIDrag::CreateInstructions()
{
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Drag on the buttons to move them around.\n"
                             "Touch input allows also multi-drag.\n"
                             "Press SPACE to show/hide tagged UI elements.");
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void UIDrag::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, DV_HANDLER(UIDrag, HandleUpdate));
}

void UIDrag::HandleClick(StringHash eventType, VariantMap& eventData)
{
    using namespace Click;
    auto* element = (Button*)eventData[P_ELEMENT].GetVoidPtr();
    element->BringToFront();
}

void UIDrag::HandleDragBegin(StringHash eventType, VariantMap& eventData)
{
    using namespace DragBegin;
    auto* element = (Button*)eventData[P_ELEMENT].GetVoidPtr();

    int lx = eventData[P_X].GetI32();
    int ly = eventData[P_Y].GetI32();

    IntVector2 p = element->GetPosition();
    element->SetVar("START", p);
    element->SetVar("DELTA", IntVector2(p.x_ - lx, p.y_ - ly));

    int buttons = eventData[P_BUTTONS].GetI32();
    element->SetVar("BUTTONS", buttons);

    auto* t = element->GetChildStaticCast<Text>("Text", false);
    t->SetText("Drag Begin Buttons: " + String(buttons));

    t = element->GetChildStaticCast<Text>("Num Touch", false);
    t->SetText("Number of buttons: " + String(eventData[P_NUMBUTTONS].GetI32()));
}

void UIDrag::HandleDragMove(StringHash eventType, VariantMap& eventData)
{
    using namespace DragBegin;
    auto* element = (Button*)eventData[P_ELEMENT].GetVoidPtr();
    int buttons = eventData[P_BUTTONS].GetI32();
    IntVector2 d = element->GetVar("DELTA").GetIntVector2();
    int X = eventData[P_X].GetI32() + d.x_;
    int Y = eventData[P_Y].GetI32() + d.y_;
    int BUTTONS = element->GetVar("BUTTONS").GetI32();

    auto* t = element->GetChildStaticCast<Text>("Event Touch", false);
    t->SetText("Drag Move Buttons: " + String(buttons));

    if (buttons == BUTTONS)
        element->SetPosition(IntVector2(X, Y));
}

void UIDrag::HandleDragCancel(StringHash eventType, VariantMap& eventData)
{
    using namespace DragBegin;
    auto* element = (Button*)eventData[P_ELEMENT].GetVoidPtr();
    IntVector2 P = element->GetVar("START").GetIntVector2();
    element->SetPosition(P);
}

void UIDrag::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    auto* ui = GetSubsystem<UI>();
    UIElement* root = ui->GetRoot();

    unsigned n = DV_INPUT.GetNumTouches();
    for (unsigned i = 0; i < n; i++)
    {
        Text* t = (Text*)root->GetChild("Touch " + String(i));
        TouchState* ts = DV_INPUT.GetTouch(i);
        t->SetText("Touch " + String(ts->touchID_));

        IntVector2 pos = ts->position_;
        pos.y_ -= 30;

        t->SetPosition(pos);
        t->SetVisible(true);
    }

    for (unsigned i = n; i < 10; i++)
    {
        Text* t = (Text*)root->GetChild("Touch " + String(i));
        t->SetVisible(false);
    }

    if (DV_INPUT.GetKeyPress(KEY_SPACE))
    {
        Vector<UIElement*> elements;
        root->GetChildrenWithTag(elements, "SomeTag");
        for (Vector<UIElement*>::ConstIterator i = elements.Begin(); i != elements.End(); ++i)
        {
            UIElement* element = *i;
            element->SetVisible(!element->IsVisible());
        }
    }
}
