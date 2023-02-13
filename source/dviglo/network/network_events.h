// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Server connection established.
DV_EVENT(E_SERVERCONNECTED, ServerConnected)
{
}

/// Server connection disconnected.
DV_EVENT(E_SERVERDISCONNECTED, ServerDisconnected)
{
}

/// Server connection failed.
DV_EVENT(E_CONNECTFAILED, ConnectFailed)
{
}

/// Server connection failed because its already connected or tries to connect already.
DV_EVENT(E_CONNECTIONINPROGRESS, ConnectionInProgress)
{
}

/// New client connection established.
DV_EVENT(E_CLIENTCONNECTED, ClientConnected)
{
    DV_PARAM(P_CONNECTION, Connection);        // Connection pointer
}

/// Client connection disconnected.
DV_EVENT(E_CLIENTDISCONNECTED, ClientDisconnected)
{
    DV_PARAM(P_CONNECTION, Connection);        // Connection pointer
}

/// Client has sent identity: identity map is in the event data.
DV_EVENT(E_CLIENTIDENTITY, ClientIdentity)
{
    DV_PARAM(P_CONNECTION, Connection);        // Connection pointer
    DV_PARAM(P_ALLOW, Allow);                  // bool
}

/// Client has informed to have loaded the scene.
DV_EVENT(E_CLIENTSCENELOADED, ClientSceneLoaded)
{
    DV_PARAM(P_CONNECTION, Connection);        // Connection pointer
}

/// Unhandled network message received.
DV_EVENT(E_NETWORKMESSAGE, NetworkMessage)
{
    DV_PARAM(P_CONNECTION, Connection);        // Connection pointer
    DV_PARAM(P_MESSAGEID, MessageID);          // int
    DV_PARAM(P_DATA, Data);                    // Buffer
}

/// About to send network update on the client or server.
DV_EVENT(E_NETWORKUPDATE, NetworkUpdate)
{
}

/// Network update has been sent on the client or server.
DV_EVENT(E_NETWORKUPDATESENT, NetworkUpdateSent)
{
}

/// Scene load failed, either due to file not found or checksum error.
DV_EVENT(E_NETWORKSCENELOADFAILED, NetworkSceneLoadFailed)
{
    DV_PARAM(P_CONNECTION, Connection);      // Connection pointer
}

/// Remote event: adds Connection parameter to the event data.
DV_EVENT(E_REMOTEEVENTDATA, RemoteEventData)
{
    DV_PARAM(P_CONNECTION, Connection);      // Connection pointer
}

/// Server refuses client connection because of the ban.
DV_EVENT(E_NETWORKBANNED, NetworkBanned)
{
}

/// Server refuses connection because of invalid password.
DV_EVENT(E_NETWORKINVALIDPASSWORD, NetworkInvalidPassword)
{
}

/// When LAN discovery found hosted server.
DV_EVENT(E_NETWORKHOSTDISCOVERED, NetworkHostDiscovered)
{
    DV_PARAM(P_ADDRESS, Address);   // String
    DV_PARAM(P_PORT, Port);         // int
    DV_PARAM(P_BEACON, Beacon);     // VariantMap
}

/// NAT punchtrough succeeds.
DV_EVENT(E_NETWORKNATPUNCHTROUGHSUCCEEDED, NetworkNatPunchtroughSucceeded)
{
    DV_PARAM(P_ADDRESS, Address);   // String
    DV_PARAM(P_PORT, Port);         // int
}

/// NAT punchtrough fails.
DV_EVENT(E_NETWORKNATPUNCHTROUGHFAILED, NetworkNatPunchtroughFailed)
{
    DV_PARAM(P_ADDRESS, Address);   // String
    DV_PARAM(P_PORT, Port);         // int
}

/// Connecting to NAT master server failed.
DV_EVENT(E_NATMASTERCONNECTIONFAILED, NetworkNatMasterConnectionFailed)
{
}

/// Connecting to NAT master server succeeded.
DV_EVENT(E_NATMASTERCONNECTIONSUCCEEDED, NetworkNatMasterConnectionSucceeded)
{
}

/// Disconnected from NAT master server.
DV_EVENT(E_NATMASTERDISCONNECTED, NetworkNatMasterDisconnected)
{
}

}
