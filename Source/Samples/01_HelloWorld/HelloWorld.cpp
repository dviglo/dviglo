// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/core/process_utils.h>
#include <dviglo/input/input.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "HelloWorld.h"

#include <dviglo/debug_new.h>

// Expands to this example's entry-point
URHO3D_DEFINE_APPLICATION_MAIN(HelloWorld)

HelloWorld::HelloWorld(Context* context) :
    Sample(context)
{
}

void HelloWorld::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create "Hello World" Text
    CreateText();

    // Finally subscribe to the update event. Note that by subscribing events at this point we have already missed some events
    // like the ScreenMode event sent by the Graphics subsystem when opening the application window. To catch those as well we
    // could subscribe in the constructor instead.
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void HelloWorld::CreateText()
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Construct new Text object
    SharedPtr<Text> helloText(new Text(context_));

    // Set String to display
    helloText->SetText("Hello World from Urho3D!");

    // Set font and text color
    helloText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 30);
    helloText->SetColor(Color(0.0f, 1.0f, 0.0f));

    // Align Text center-screen
    helloText->SetHorizontalAlignment(HA_CENTER);
    helloText->SetVerticalAlignment(VA_CENTER);

    // Add Text instance to the UI root element
    GetSubsystem<UI>()->GetRoot()->AddChild(helloText);
}

void HelloWorld::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(HelloWorld, HandleUpdate));
}

void HelloWorld::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Do nothing for now, could be extended to eg. animate the display
}
