
#include "network.h"
#include "framework.h"

#ifdef NETWORK_SUPPORT

Network::Network( int Port )
{

	serverAddress.host = ENET_HOST_ANY;
	serverAddress.port = Port;
	networkPeer = 0;
	localHost = enet_host_create ( &serverAddress /* the address to bind the server host to */,
		32 /* allow up to 32 clients and/or outgoing connections */,
		1 /* allow up to 2 channels to be used, 0 and 1 */,
		0 /* assume any amount of incoming bandwidth */,
		0 /* assume any amount of outgoing bandwidth */);

	if( localHost == 0 )
	{
		return;
	}
}

Network::Network( std::string Server, int Port )
{
	localHost = enet_host_create (NULL /* create a client host */,
		1 /* only allow 1 outgoing connection */,
		1 /* allow up 2 channels to be used, 0 and 1 */,
		// 57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
		// 14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);
		0 /* assume any amount of incoming bandwidth */,
		0 /* assume any amount of outgoing bandwidth */);
	if( localHost == 0 )
	{
		return;
	}
	enet_address_set_host( &serverAddress, Server.c_str() );
	serverAddress.port = Port;
	networkPeer = enet_host_connect( localHost, &serverAddress, 1, 0 );
	if( networkPeer == 0 )
	{
		enet_host_destroy( localHost );
		localHost = 0;
		return;
	}

	/* Wait up to 6 seconds for the connection attempt to succeed. */
	ENetEvent ev;
	if( enet_host_service( localHost, &ev, 6000 ) > 0 && ev.type == ENET_EVENT_TYPE_CONNECT )
	{
	} else {
		enet_peer_reset( networkPeer );
		networkPeer = 0;
		enet_host_destroy( localHost );
		localHost = 0;
		return;
	}
}

Network::~Network()
{
	if( networkPeer != 0 )
	{
		Disconnect();
	}
	if( localHost != 0 )
	{
		enet_host_destroy( localHost );
		localHost = 0;
	}
}

void Network::Disconnect()
{
	if( IsActive() && IsConnected() )
	{
		enet_peer_disconnect( networkPeer, 0 );
		enet_peer_reset( networkPeer );
		networkPeer = 0;
	}
}

bool Network::IsActive()
{
	return ( localHost != 0 );
}

bool Network::IsConnected()
{
	return ( networkPeer != 0 );
}

void Network::Update()
{
	ENetEvent netEvent;
	Event* fwEvent;

	while( enet_host_service( localHost, &netEvent, 0 ) > 0 )
	{
		switch( netEvent.type )
		{
			case ENET_EVENT_TYPE_CONNECT:
				fwEvent = new Event();
				fwEvent->Type = EVENT_NETWORK_CONNECTION_REQUEST;
				memcpy( &fwEvent->Data.Network.Traffic, &netEvent, sizeof(ENetEvent) );
				Framework::System->PushEvent( fwEvent );
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				fwEvent = new Event();
				fwEvent->Type = EVENT_NETWORK_PACKET_RECEIVED;
				memcpy( &fwEvent->Data.Network.Traffic, &netEvent, sizeof(ENetEvent) );
				Framework::System->PushEvent( fwEvent );
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				enet_peer_reset( netEvent.peer );
				fwEvent = new Event();
				fwEvent->Type = EVENT_NETWORK_DISCONNECTED;
				memcpy( &fwEvent->Data.Network.Traffic, &netEvent, sizeof(ENetEvent) );
				Framework::System->PushEvent( fwEvent );
				break;
		}
	}
}

#endif