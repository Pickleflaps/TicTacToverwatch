// NetworkLibrary.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <RakPeerInterface.h> 



#ifdef NETWORK_LIBRARY_EXPORTS // ADD THIS TO ANY CLASS I WANT TO EXPORT
#define NETWORK_LIBRARY_API __declspec(dllexport)
#else 
#define NETWORK_LIBRARY_API __declspec(dllimport)
#endif

namespace NetworkLibrary
{
	//TODO this is just thinking out loud, I may need all or none of these forward declarations
	//also, how i want to structure my dll, there are core functions on the surface, then dive deeper for security or ping functions etc
	class Security;
	class ConnectionManagement;
	class PingFunctions;
	class NetworkFunctions;

	//add message ID for users wanting a custom enum to tack to the start of their packet
	void AddUserPacketID() {};




}