
#include "event.h"

Event::Event()
{
  Type = EVENT_UNDEFINED;
}

Event::~Event()
{

#ifdef NETWORK_SUPPORT
	if( Type == EVENT_NETWORK_PACKET_RECEIVED )
	{
		enet_packet_destroy( Data.Network.Traffic.packet );
	}
#endif

#ifdef DOWNLOAD_SUPPORT
	if( Type == EVENT_DOWNLOAD_COMPLETE || Type == EVENT_DOWNLOAD_PROGRESS )
	{
		if( URL != 0 )
		{
			delete URL;
		}
		if( Contents != 0 )
		{
			delete Contents;
		}
	}
#endif

}
