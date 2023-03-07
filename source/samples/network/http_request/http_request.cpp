// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/core/process_utils.h>
#include <dviglo/input/input.h>
#include <dviglo/network/network.h>
#include <dviglo/network/http_request.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "http_request.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(HttpRequestDemo)

HttpRequestDemo::HttpRequestDemo()
{
}

void HttpRequestDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the user interface
    CreateUI();

    // Subscribe to basic events such as update
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void HttpRequestDemo::CreateUI()
{
    // Construct new Text object
    text_ = new Text();

    // Set font and text color
    text_->SetFont(DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    text_->SetColor(Color(1.0f, 1.0f, 0.0f));

    // Align Text center-screen
    text_->SetHorizontalAlignment(HA_CENTER);
    text_->SetVerticalAlignment(VA_CENTER);

    // Add Text instance to the UI root element
    GetSubsystem<UI>()->GetRoot()->AddChild(text_);
}

void HttpRequestDemo::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing HTTP request
    SubscribeToEvent(E_UPDATE, DV_HANDLER(HttpRequestDemo, HandleUpdate));
}

void HttpRequestDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();

    if (httpRequest_.Null())
#ifdef DV_SSL
        httpRequest_ = network->MakeHttpRequest("https://api.ipify.org/?format=json");
#else
        httpRequest_ = network->MakeHttpRequest("http://httpbin.org/ip");
#endif
    else
    {
        // Initializing HTTP request
        if (httpRequest_->GetState() == HTTP_INITIALIZING)
            return;
        // An error has occurred
        else if (httpRequest_->GetState() == HTTP_ERROR)
        {
            text_->SetText("An error has occurred: " + httpRequest_->GetError());
            UnsubscribeFromEvent("Update");
            DV_LOGERRORF("HttpRequest error: %s", httpRequest_->GetError().c_str());
        }
        // Get message data
        else
        {
            if (httpRequest_->GetAvailableSize() > 0)
                message_ += httpRequest_->ReadLine();
            else
            {
                text_->SetText("Processing...");

                SharedPtr<JSONFile> json(new JSONFile());
                json->FromString(message_);

#ifdef DV_SSL
                JSONValue val = json->GetRoot().Get("ip");
#else
                JSONValue val = json->GetRoot().Get("origin");
#endif

                if (val.IsNull())
                    text_->SetText("Invalid JSON response retrieved!");
                else
                    text_->SetText("Your IP is: " + val.GetString());

                UnsubscribeFromEvent("Update");
            }
        }
    }
}
