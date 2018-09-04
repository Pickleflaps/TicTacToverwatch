#pragma once


#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "GetTime.h"
#include "Rand.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "FullyConnectedMesh2.h"
#include "TeamManager.h"
#include "Kbhit.h"
#include "RakSleep.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "SocketLayer.h"
#include "ReplicaManager3.h"
#include "NetworkIDManager.h"
#include "Gets.h"
#include "Itoa.h"
#include "NatPunchthroughClient.h"
#include "NatTypeDetectionClient.h"
#include "miniupnpc.h"
#include "upnpcommands.h"
#include "upnperrors.h"
#include "TCPInterface.h"
#include "ReadyEvent.h"	
#include "PacketLogger.h"
#include "RPC4Plugin.h"
#include "Kbhit.h"
#include "HTTPConnection2.h"
#include "jansson.h"

#define DEFAULT_SERVER_ADDRESS "natpunch.jenkinssoftware.com"
#define DEFAULT_SERVER_PORT "61111"
#define NAT_TYPE_DETECTION_SERVER 0
#define USE_UPNP 1
#define MASTER_SERVER_ADDRESS "masterserver2.raknet.com"	//#define MASTER_SERVER_ADDRESS "localhost"
#define MASTER_SERVER_PORT 80								//#define MASTER_SERVER_PORT 8888

using namespace RakNet;



// Forward declarations
class User;
class Team;
void PostRoomToMaster(void);

static class Game : public Replica3
{
public:

	enum Phase
	{
		CONNECTING_TO_SERVER,
		DETERMINE_NAT_TYPE,
		SEARCH_FOR_GAMES,
		NAT_PUNCH_TO_GAME_HOST,
		CONNECTING_TO_GAME_HOST,
		VERIFIED_JOIN,
		IN_LOBBY_WAITING_FOR_HOST,
		IN_LOBBY_WITH_HOST,
		IN_GAME,
		EXIT_SAMPLE
	};

	Game();
	virtual ~Game();

	//All of these are passed on from ReplicaManager3.h 
	//
	//		SEE ReplicaManager3.h for detailed explination because I honestly don't understand exactly what each function does,  only that they should be passed on so the end user/game can override 
	//
	virtual void WriteAllocationID(RakNet::Connection_RM3 *destinationConnection, RakNet::BitStream *allocationIdBitstream) const {};
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3);
	virtual RM3DestructionState QueryDestruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3);
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection);
	virtual bool QueryRelayDestruction(Connection_RM3 *sourceConnection) const;
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection) {};
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual void SerializeConstructionExisting(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual void DeserializeConstructionExisting(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection) {};
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual RakNet::RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const;
	virtual void OnPoppedConnection(RakNet::Connection_RM3 *droppedConnection);
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection);
	virtual RakNet::RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection);
	//it was at this point, 957 lines into reading the "Brief" of each function in ReplicaManager3 that i started to see a pattern and understand more what it is doing
	virtual void OnUserReplicaPreSerializeTick(void) {};
	virtual RM3SerializationResult Serialize(RakNet::SerializeParameters *serializeParameters);
	virtual void OnSerializeTransmission(RakNet::BitStream *bitStream, RakNet::Connection_RM3 *destinationConnection, BitSize_t bitsPerChannel[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS], RakNet::Time curTime);
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters);
	virtual void PostSerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual void PostDeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual void PostSerializeConstructionExisting(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual void PostDeserializeConstructionExisting(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual void PreDestruction(RakNet::Connection_RM3 *sourceConnection);

	//Not Replica3 
	void EnterPhase(Phase newPhase);
	void Reset();
	void SearchForGames(void);

	//	Serialized variables
	RakString gameName;			// Shows up in game listings
	bool lockGame;				// If host locks the game, join queries are rejected
	bool gameInLobby;			// Game is either in Lobby or Gameplay
	int masterServerRow;		// This is serialized so if the host migrates, the new host takes over that row. Otherwise, a new row would be written for the same game.

	//not serialized variables		
	RakNet::NATTypeDetectionResult myNatType;	// Store what type of router I am behind
	Phase phase;
	RakNetGUID natPunchServerGuid;				// NAT punchthrough server runs RakNet project NatCompleteServer with NAT_TYPE_DETECTION_SERVER, and NAT_PUNCHTHROUGH_SERVER
	SystemAddress natPunchServerAddress;
	char serverIPAddr[256];

	// Just tracks what other objects have been created
	DataStructures::List<User*> users;
	DataStructures::List<Team*> teams;
	RakNet::Time whenToNextUpdateMasterServer;

	// Helper function to store and read the JSON from the GET request
	void SetMasterServerQueryResult(json_t *root)
	{
		if (masterServerQueryResult)
			json_decref(masterServerQueryResult);
		masterServerQueryResult = root;
	}

	json_t* GetMasterServerQueryResult(void)
	{
		if (masterServerQueryResult == 0)
			return 0;
		void *iter = json_object_iter(masterServerQueryResult);
		while (iter)
		{
			const char *firstKey = json_object_iter_key(iter);
			//if (stricmp(firstKey, "GET") == 0)
			if (_stricmp(firstKey, "GET") == 0)
			{
				return json_object_iter_value(iter);
			}
			iter = json_object_iter_next(masterServerQueryResult, iter);
			RakAssert(iter != 0);
		}
		return 0;
	}
	// The GET request returns a string. I use http://www.digip.org/jansson/ to parse the string, and store the results.
	json_t *masterServerQueryResult;
	json_t *jsonArray;

}*game; 

class Team : public Replica3
{
public:
	Team();
	virtual ~Team();

	virtual void WriteAllocationID(RakNet::Connection_RM3 *destinationConnection, RakNet::BitStream * allocationBitstream) const;
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3);
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection);
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);

	virtual void PostSerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual void PostDeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);

	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection) {}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual RakNet::RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const;
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection);
	virtual RakNet::RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection);
	virtual RM3SerializationResult Serialize(RakNet::SerializeParameters *serializeParameters);
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {}

	// Team data managed by the TeamManager plugin
	TM_Team tmTeam;

	// Example of team data not managed by TeamManager
	RakString teamName;
};

class User : public Replica3
{
public:
	User();
	virtual ~User();

	virtual void WriteAllocationID(RakNet::Connection_RM3 *destinationConnection, RakNet::BitStream *allocationIdBitstream) const;
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3);
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection);
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	
	virtual void PostSerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	virtual void PostDeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);

	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection) {}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	virtual RakNet::RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const;
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection);
	virtual RakNet::RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection);
	virtual RM3SerializationResult Serialize(RakNet::SerializeParameters *serializeParameters);
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {}

	// Team data managed by the TeamManager plugin
	TM_TeamMember tmTeamMember;
	RakString userName;
	NATTypeDetectionResult natType;
	RakNetGUID playerGuid;
	SystemAddress playerAddress;
};

// Write roomName and a list of NATTypeDetectionResult to a bitStream
void SerializeToJSON(RakString &outputString, RakString &roomName, DataStructures::List<NATTypeDetectionResult> &natTypes);

// Required by ReplicaManager3. Acts as a class factory for Connection_RM3 derived instances
class RM3_Class_Factory : public ReplicaManager3
{
public:
	RM3_Class_Factory() {}
	virtual ~RM3_Class_Factory() {}
	virtual Connection_RM3* AllocConnection(const SystemAddress &systemAddress, RakNetGUID rakNetGUID) const;
	virtual void DeallocConnection(Connection_RM3 *connection) const;
};

// Required by ReplicaManager3. Acts as a class factory for Replica3 derived instances
class ConnectionRM3_Class_Factory : public Connection_RM3
{
public:
	ConnectionRM3_Class_Factory(const SystemAddress &_systemAddress, RakNetGUID _guid) : Connection_RM3(_systemAddress, _guid) {}
	virtual ~ConnectionRM3_Class_Factory() {}
	virtual Replica3 *AllocReplica(RakNet::BitStream *allocationIdBitstream, ReplicaManager3 *replicaManager3);
};


void UPNPOpenAsynch(unsigned short portToOpen,
	unsigned int timeout,
	void(*progressCallback)(const char *progressMsg, void *userData),
	void(*resultCallback)(bool success, unsigned short portToOpen, void *userData),
	void *userData
);

void UPNPProgressCallback(const char *progressMsg, void *userData);
void UPNPResultCallback(bool success, unsigned short portToOpen, void *userData);
void OpenUPNP(void);



// A system has connected and is ready to participate in the game
// Register this system with the plugins that need to know about new participants
// This operation happens after FullyConnectedMesh2 has told us about who the host is.
void RegisterGameParticipant(RakNetGUID guid);


void ReleaseRoomFromCloud(void);
void CreateRoom(void);

class GameNetworkFrame {
public:
	int Run(void);
};

