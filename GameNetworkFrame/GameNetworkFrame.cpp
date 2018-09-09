#include "GameNetworkFrame.h"


RakPeerInterface* rakPeerInterface;
TeamManager *teamManager;
ReplicaManager3 * replicaManager3;
NetworkIDManager *networkIDManager;
TCPInterface *tcp;
NatPunchthroughClient *natPunchthroughClient;
#ifdef NAT_TYPE_DETECTION_SERVER
NatTypeDetectionClient *natTypeDetectionClient;
#endif
RPC4 *rpc4;
ReadyEvent* readyEvent;
FullyConnectedMesh2* fullyConnectedMesh2;
HTTPConnection2 *httpConnection2;




// Game Class
Game::Game()
{
	myNatType = NAT_TYPE_UNKNOWN; masterServerRow = -1;
	Reset();
	whenToNextUpdateMasterServer = 0;
	masterServerQueryResult = 0;
}

Game::~Game()
{
	if (masterServerQueryResult)
		json_decref(masterServerQueryResult);
}

RM3ConstructionState Game::QueryConstruction(RakNet::Connection_RM3 * destinationConnection, ReplicaManager3 * replicaManager3)
{
	if (fullyConnectedMesh2->IsConnectedHost())
		return QueryConstruction_PeerToPeer(destinationConnection, R3P2PM_STATIC_OBJECT_CURRENTLY_AUTHORITATIVE);
	else
		return QueryConstruction_PeerToPeer(destinationConnection, R3P2PM_STATIC_OBJECT_NOT_CURRENTLY_AUTHORITATIVE);
}



RM3DestructionState Game::QueryDestruction(RakNet::Connection_RM3 * destinationConnection, ReplicaManager3 * replicaManager3)
{
	(void)destinationConnection; (void)replicaManager3; return RM3DS_DO_NOT_QUERY_DESTRUCTION;
}

bool Game::QueryRemoteConstruction(RakNet::Connection_RM3 * sourceConnection)
{
	return true;
}

bool Game::QueryRelayDestruction(Connection_RM3 * sourceConnection) const
{
	(void)sourceConnection;
	return true;
}

bool Game::DeserializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{
	return true;
}

void Game::SerializeConstructionExisting(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * destinationConnection)
{
	constructionBitstream->Write(gameName);
	constructionBitstream->Write(lockGame);
	constructionBitstream->Write(gameInLobby);
	constructionBitstream->Write(masterServerRow);
}

void Game::DeserializeConstructionExisting(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{
	constructionBitstream->Read(gameName);
	constructionBitstream->Read(lockGame);
	constructionBitstream->Read(gameInLobby);
	constructionBitstream->Read(masterServerRow);
	printf("Downloaded game. locked=%i. inLobby=%i\n", lockGame, gameInLobby);
}

bool Game::DeserializeDestruction(RakNet::BitStream * destructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{
	return true;
}

RakNet::RM3ActionOnPopConnection Game::QueryActionOnPopConnection(RakNet::Connection_RM3 * droppedConnection) const
{
	return RM3AOPC_DO_NOTHING;
}

void Game::OnPoppedConnection(RakNet::Connection_RM3 * droppedConnection)
{
	(void)droppedConnection;
}

void Game::DeallocReplica(RakNet::Connection_RM3 * sourceConnection)
{
	delete this;
}

RakNet::RM3QuerySerializationResult Game::QuerySerialization(RakNet::Connection_RM3 * destinationConnection)
{
	if (fullyConnectedMesh2->IsConnectedHost())
		return QuerySerialization_PeerToPeer(destinationConnection, R3P2PM_STATIC_OBJECT_CURRENTLY_AUTHORITATIVE);
	else
		return QuerySerialization_PeerToPeer(destinationConnection, R3P2PM_STATIC_OBJECT_NOT_CURRENTLY_AUTHORITATIVE);
}

RM3SerializationResult Game::Serialize(RakNet::SerializeParameters * serializeParameters)
{
	serializeParameters->outputBitstream[0].Write(lockGame);
	serializeParameters->outputBitstream[0].Write(gameInLobby);
	serializeParameters->outputBitstream[0].Write(masterServerRow);
	return RM3SR_BROADCAST_IDENTICALLY;
}

void Game::OnSerializeTransmission(RakNet::BitStream * bitStream, RakNet::Connection_RM3 * destinationConnection, BitSize_t bitsPerChannel[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS], RakNet::Time curTime)
{
	(void)bitStream;
	(void)destinationConnection;
	(void)bitsPerChannel;
	(void)curTime;
}

void Game::Deserialize(RakNet::DeserializeParameters * deserializeParameters)
{
	if (deserializeParameters->bitstreamWrittenTo[0])
	{
		bool b;
		deserializeParameters->serializationBitstream[0].Read(b);
		if (b != lockGame)
		{
			lockGame = b;
			if (lockGame)
				printf("Game is no longer locked\n");
			else
				printf("Game is now locked\n");
		}
		deserializeParameters->serializationBitstream[0].Read(b);
		if (b != gameInLobby)
		{
			gameInLobby = b;
			if (gameInLobby)
			{
				readyEvent->DeleteEvent(0);
				game->EnterPhase(Game::IN_LOBBY_WITH_HOST);
				printf("Game is now in the lobby\n");
			}
			else
			{
				readyEvent->ForceCompletion(0);
				game->EnterPhase(Game::IN_GAME);
			}
		}
		deserializeParameters->serializationBitstream[0].Read(masterServerRow);
	}
}

void Game::PostSerializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * destinationConnection)
{
	(void)constructionBitstream;
	(void)destinationConnection;
}

void Game::PostDeserializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{
	(void)constructionBitstream;
	(void)sourceConnection;
}
void Game::PostSerializeConstructionExisting(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * destinationConnection)
{
	(void)constructionBitstream;
	(void)destinationConnection;
}

void Game::PostDeserializeConstructionExisting(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{
	(void)constructionBitstream;
	(void)sourceConnection;
}

void Game::PreDestruction(RakNet::Connection_RM3 * sourceConnection)
{
	(void)sourceConnection;
}

void Game::EnterPhase(Phase newPhase)
{
	phase = newPhase;
	switch (newPhase)
	{
	case CONNECTING_TO_SERVER:
	{
		char port[256];
		printf("Enter address of server running the NATCompleteServer project.\nEnter for default: ");
		Gets(game->serverIPAddr, 256);
		if (game->serverIPAddr[0] == 0) {
			//strcpy(game->serverIPAddr, DEFAULT_SERVER_ADDRESS);
			strcpy_s(game->serverIPAddr, DEFAULT_SERVER_ADDRESS);
		}
		printf("Enter server port, or enter for default: ");
		Gets(port, 256);
		if (port[0] == 0) {
			//strcpy(port, DEFAULT_SERVER_PORT);
			strcpy_s(port, DEFAULT_SERVER_PORT);
		}

		ConnectionAttemptResult car = rakPeerInterface->Connect(serverIPAddr, atoi(port), 0, 0);
		if (car != RakNet::CONNECTION_ATTEMPT_STARTED)
		{
			printf("Failed connect call to %s. Code=%i\n", serverIPAddr, car);
			phase = EXIT_SAMPLE;
		}
	}
	break;
#ifdef NAT_TYPE_DETECTION_SERVER
	case DETERMINE_NAT_TYPE:
		printf("Determining NAT type...\n");
		natTypeDetectionClient->DetectNATType(natPunchServerAddress);
		break;
#endif
	case SEARCH_FOR_GAMES:
		SearchForGames();
		break;
	case NAT_PUNCH_TO_GAME_HOST:
		printf("Attempting NAT punch to host of game session...\n");
		break;
	case IN_LOBBY_WITH_HOST:
		printf("(1) to join team 1.\n");
		printf("(2) to join team 2.\n");
		printf("(R)eady to start\n");
		printf("(U)nready to start\n");
		break;
	case IN_GAME:
		printf("Game started.\n(C)hat in-game\n");
		break;
	}
}

void Game::Reset()
{
	lockGame = false;
	gameInLobby = true;
}

void Game::SearchForGames(void)
{
	printf("Downloading rooms...\n");

	RakString rsRequest = RakString::FormatForGET(MASTER_SERVER_ADDRESS "/testServer?__gameId=comprehensivePCGame");
	httpConnection2->TransmitRequest(rsRequest, MASTER_SERVER_ADDRESS, MASTER_SERVER_PORT);
}

void PostRoomToMaster(void)
{
	BitStream bsOut;
	RakString jsonSerializedRoom;
	DataStructures::List<NATTypeDetectionResult> natTypes;
	for (unsigned int i = 0; i < game->users.Size(); i++)
		natTypes.Push(game->users[i]->natType, _FILE_AND_LINE_);
	SerializeToJSON(jsonSerializedRoom, game->gameName, natTypes);

	RakString rowStr;
	if (game->masterServerRow != -1)
		rowStr.Set("\"__rowId\": %i,", game->masterServerRow);

	// See http://masterserver2.raknet.com/
	RakString rsRequest = RakString::FormatForPOST(
		(const char*)MASTER_SERVER_ADDRESS "/testServer",
		"text/plain; charset=UTF-8",
		RakString("{'__gameId': 'comprehensivePCGame', '__clientReqId': '0', %s '__timeoutSec': '30', %s }", rowStr.C_String(), jsonSerializedRoom.C_String()));

	// Refresh the room again slightly less than every 30 seconds
	game->whenToNextUpdateMasterServer = RakNet::GetTime() + 30000 - 1000;

	httpConnection2->TransmitRequest(rsRequest, MASTER_SERVER_ADDRESS, MASTER_SERVER_PORT);

	printf("Posted game session. In room.\n");
}

void SerializeToJSON(RakString & outputString, RakString & roomName, DataStructures::List<NATTypeDetectionResult>& natTypes)
{
	outputString.Set("'roomName': '%s', 'guid': '%s', 'natTypes' : [ ", roomName.C_String(), rakPeerInterface->GetMyGUID().ToString());
	for (unsigned short i = 0; i < natTypes.Size(); i++)
	{
		if (i != 0)
			outputString += ", ";
		RakString appendStr("{'type': %i}", natTypes[i]);
		outputString += appendStr;
	}
	outputString += " ] ";
}

#if USE_UPNP!=0
struct UPNPOpenWorkerArgs
{
	char buff[256];
	unsigned short portToOpen;
	unsigned int timeout;
	void *userData;
	void(*resultCallback)(bool success, unsigned short portToOpen, void *userData);
	void(*progressCallback)(const char *progressMsg, void *userData);
};
RAK_THREAD_DECLARATION(UPNPOpenWorker)
{
	UPNPOpenWorkerArgs *args = (UPNPOpenWorkerArgs *)arguments;
	bool success = false;

	// Behind a NAT. Try to open with UPNP to avoid doing NAT punchthrough
	struct UPNPDev * devlist = 0;
	RakNet::Time t1 = GetTime();
	devlist = upnpDiscover(args->timeout, 0, 0, 0, 0, 0);
	RakNet::Time t2 = GetTime();
	if (devlist)
	{
		if (args->progressCallback)
			args->progressCallback("List of UPNP devices found on the network :\n", args->userData);
		struct UPNPDev * device;
		for (device = devlist; device; device = device->pNext)
		{
			sprintf(args->buff, " desc: %s\n st: %s\n\n", device->descURL, device->st);
			if (args->progressCallback)
				args->progressCallback(args->buff, args->userData);
		}

		char lanaddr[64];	/* my ip address on the LAN */
		struct UPNPUrls urls;
		struct IGDdatas data;
		if (UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)) == 1)
		{
			char iport[32];
			Itoa(args->portToOpen, iport, 10);
			char eport[32];
			strcpy(eport, iport);

			// Version miniupnpc-1.6.20120410
			int r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
				eport, iport, lanaddr, 0, "UDP", 0, "0");

			if (r != UPNPCOMMAND_SUCCESS)
				printf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
					eport, iport, lanaddr, r, strupnperror(r));

			char intPort[6];
			char intClient[16];

			// Version miniupnpc-1.6.20120410
			char desc[128];
			char enabled[128];
			char leaseDuration[128];
			r = UPNP_GetSpecificPortMappingEntry(urls.controlURL,
				data.first.servicetype,
				eport, "UDP",
				intClient, intPort,
				desc, enabled, leaseDuration);

			if (r != UPNPCOMMAND_SUCCESS)
			{
				sprintf(args->buff, "GetSpecificPortMappingEntry() failed with code %d (%s)\n",
					r, strupnperror(r));
				if (args->progressCallback)
					args->progressCallback(args->buff, args->userData);
			}
			else
			{
				if (args->progressCallback)
					args->progressCallback("UPNP success.\n", args->userData);
				// game->myNatType=NAT_TYPE_SUPPORTS_UPNP;

				success = true;
			}
		}
	}

	if (args->resultCallback)
		args->resultCallback(success, args->portToOpen, args->userData);
	RakNet::OP_DELETE(args, _FILE_AND_LINE_);
	return 1;
}

void UPNPOpenAsynch(unsigned short portToOpen,
	unsigned int timeout,
	void(*progressCallback)(const char *progressMsg, void *userData),
	void(*resultCallback)(bool success, unsigned short portToOpen, void *userData),
	void *userData
)
{
	UPNPOpenWorkerArgs *args = RakNet::OP_NEW<UPNPOpenWorkerArgs>(_FILE_AND_LINE_);
	args->portToOpen = portToOpen;
	args->timeout = timeout;
	args->userData = userData;
	args->progressCallback = progressCallback;
	args->resultCallback = resultCallback;
	RakThread::Create(UPNPOpenWorker, args);
}

void UPNPProgressCallback(const char *progressMsg, void *userData)
{
	printf(progressMsg);
}
void UPNPResultCallback(bool success, unsigned short portToOpen, void *userData)
{
	if (success)
		game->myNatType = NAT_TYPE_SUPPORTS_UPNP;
	game->EnterPhase(Game::SEARCH_FOR_GAMES);
}

void OpenUPNP(void)
{
	printf("Discovering UPNP...\n");

	DataStructures::List<RakNetSocket2* > sockets;
	rakPeerInterface->GetSockets(sockets);
	UPNPOpenAsynch(sockets[0]->GetBoundAddress().GetPort(), 2000, UPNPProgressCallback, UPNPResultCallback, 0);
}

#endif

void RegisterGameParticipant(RakNetGUID guid)
{
	Connection_RM3 *connection = replicaManager3->AllocConnection(rakPeerInterface->GetSystemAddressFromGuid(guid), guid);
	if (replicaManager3->PushConnection(connection) == false)
		replicaManager3->DeallocConnection(connection);
	teamManager->GetWorldAtIndex(0)->AddParticipant(guid);
	readyEvent->AddToWaitList(0, guid);
}

void ReleaseRoomFromCloud(void)
{
	RakString rsRequest = RakString::FormatForDELETE(
		RakString(MASTER_SERVER_ADDRESS "/testServer?__gameId=comprehensivePCGame&__rowId=%i", game->masterServerRow));
	httpConnection2->TransmitRequest(rsRequest, MASTER_SERVER_ADDRESS, MASTER_SERVER_PORT);
	game->masterServerRow = -1;
}

void CreateRoom(void)
{

	size_t arraySize;
	if (game->GetMasterServerQueryResult())
		arraySize = json_array_size(game->GetMasterServerQueryResult());
	else
		arraySize = 0;

	if (arraySize > 0)
	{
		printf("Enter room name: ");
		char rn[128];
		Gets(rn, 128);
		if (rn[0] == 0)
			strcpy_s(rn, "Unnamed");
		game->gameName = rn;
	}
	else
	{
		game->gameName = "Default room name";
	}

	// Upload the room to the server
	PostRoomToMaster();

	// Room owner creates two teams and registers them for replication
	Team *team1 = new Team;
	team1->SetNetworkIDManager(networkIDManager);
	team1->teamName = "Team1";
	teamManager->GetWorldAtIndex(0)->ReferenceTeam(&team1->tmTeam, team1->GetNetworkID(), false);
	Team *team2 = new Team;
	team2->SetNetworkIDManager(networkIDManager);
	team2->teamName = "Team2";
	teamManager->GetWorldAtIndex(0)->ReferenceTeam(&team2->tmTeam, team2->GetNetworkID(), false);

	game->EnterPhase(Game::IN_LOBBY_WAITING_FOR_HOST);

	// So that time spent in single player does not count towards which system has been running the longest in multiplayer
	fullyConnectedMesh2->ResetHostCalculation();

	printf("(E)xit session\n");
}

// Team Class
Team::Team()
{
	game->teams.Push(this, _FILE_AND_LINE_);
	tmTeam.SetOwner(this);
}

Team::~Team()
{
	game->teams.RemoveAtIndex(game->teams.GetIndexOf(this));
}

void Team::WriteAllocationID(RakNet::Connection_RM3 * destinationConnection, RakNet::BitStream * allocationBitstream) const
{
	allocationBitstream->Write("Team");
}

RM3ConstructionState Team::QueryConstruction(RakNet::Connection_RM3 * destinationConnection, ReplicaManager3 * replicaManager3)
{

	// This implementation has the host create the Team instances initially
	// If the original host disconnects, the new host as determined by FullyConnectedMesh2 takes over replication duties
	if (fullyConnectedMesh2->IsConnectedHost())
		return QueryConstruction_PeerToPeer(destinationConnection, R3P2PM_MULTI_OWNER_CURRENTLY_AUTHORITATIVE);
	else
		return QueryConstruction_PeerToPeer(destinationConnection, R3P2PM_MULTI_OWNER_NOT_CURRENTLY_AUTHORITATIVE);

}

bool Team::QueryRemoteConstruction(RakNet::Connection_RM3 * sourceConnection)
{
	return true;
}

void Team::SerializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * destinationConnection)
{
	constructionBitstream->Write(teamName);
}

bool Team::DeserializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{

	constructionBitstream->Read(teamName);
	printf("Downloaded team. name=%s\n", teamName.C_String());
	// When ReplicaManager3 creates the team from a network command, the TeamManager class has to be informed of the new TM_Team instance
	teamManager->GetWorldAtIndex(0)->ReferenceTeam(&tmTeam, GetNetworkID(), false);
	return true;
}

void Team::PostSerializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * destinationConnection)
{
	tmTeam.SerializeConstruction(constructionBitstream);
}

void Team::PostDeserializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{
	tmTeam.DeserializeConstruction(teamManager, constructionBitstream);
}

bool Team::DeserializeDestruction(RakNet::BitStream * destructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{
	return true;
}

RakNet::RM3ActionOnPopConnection Team::QueryActionOnPopConnection(RakNet::Connection_RM3 * droppedConnection) const
{
	// Do not destroy the object when the connection that created it disconnects.
	return RM3AOPC_DO_NOTHING;
}

void Team::DeallocReplica(RakNet::Connection_RM3 * sourceConnection)
{
	delete this;
}

RakNet::RM3QuerySerializationResult Team::QuerySerialization(RakNet::Connection_RM3 * destinationConnection)
{
	// Whoever is currently the host serializes the class
	if (fullyConnectedMesh2->IsConnectedHost())
		return QuerySerialization_PeerToPeer(destinationConnection, R3P2PM_MULTI_OWNER_CURRENTLY_AUTHORITATIVE);
	else
		return QuerySerialization_PeerToPeer(destinationConnection, R3P2PM_MULTI_OWNER_NOT_CURRENTLY_AUTHORITATIVE);
}

RM3SerializationResult Team::Serialize(RakNet::SerializeParameters * serializeParameters)
{
	return RM3SR_BROADCAST_IDENTICALLY;
}


//User Class
User::User()
{
	game->users.Push(this, _FILE_AND_LINE_); tmTeamMember.SetOwner(this); natType = NAT_TYPE_UNKNOWN;
}

User::~User()
{
	game->users.RemoveAtIndex(game->users.GetIndexOf(this));
}

void User::WriteAllocationID(RakNet::Connection_RM3 * destinationConnection, RakNet::BitStream * allocationIdBitstream) const
{
	allocationIdBitstream->Write("User");
}

RM3ConstructionState User::QueryConstruction(RakNet::Connection_RM3 * destinationConnection, ReplicaManager3 * replicaManager3)
{
	// Whoever created the user replicates it.
	return QueryConstruction_PeerToPeer(destinationConnection);
}

bool User::QueryRemoteConstruction(RakNet::Connection_RM3 * sourceConnection)
{
	return true;
}

void User::SerializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * destinationConnection)
{
	constructionBitstream->Write(userName);
	constructionBitstream->WriteCasted<unsigned char>(natType);
	constructionBitstream->Write(playerGuid);
	constructionBitstream->Write(playerAddress);
}

bool User::DeserializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{

	// The TeamManager plugin has to be informed of TM_TeamMember instances created over the network
	teamManager->GetWorldAtIndex(0)->ReferenceTeamMember(&tmTeamMember, GetNetworkID());
	constructionBitstream->Read(userName);
	constructionBitstream->ReadCasted<unsigned char>(natType);
	constructionBitstream->Read(playerGuid);
	constructionBitstream->Read(playerAddress);
	return true;
}

void User::PostSerializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * destinationConnection)
{
	// TeamManager requires that TM_Team was created before TM_TeamMember that uses it.
	// PostSerializeConstruction and PostDeserializeConstruction ensure that all objects have been created before serialization
	tmTeamMember.SerializeConstruction(constructionBitstream);
}

void User::PostDeserializeConstruction(RakNet::BitStream * constructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{
	tmTeamMember.DeserializeConstruction(teamManager, constructionBitstream);
	printf("Downloaded user. name=%s", userName.C_String());
	if (tmTeamMember.GetCurrentTeam() == 0)
		printf(" not on a team\n");
	else
		printf(" on team %s\n", ((Team*)(tmTeamMember.GetCurrentTeam()->GetOwner()))->teamName.C_String());

	// Update the user count on the master server as new users join
	if (fullyConnectedMesh2->IsConnectedHost())
		PostRoomToMaster();
}

bool User::DeserializeDestruction(RakNet::BitStream * destructionBitstream, RakNet::Connection_RM3 * sourceConnection)
{
	return true;
}

RakNet::RM3ActionOnPopConnection User::QueryActionOnPopConnection(RakNet::Connection_RM3 * droppedConnection) const
{
	return QueryActionOnPopConnection_PeerToPeer(droppedConnection);
}

void User::DeallocReplica(RakNet::Connection_RM3 * sourceConnection)
{
	delete this;
}

RakNet::RM3QuerySerializationResult User::QuerySerialization(RakNet::Connection_RM3 * destinationConnection)
{
	return QuerySerialization_PeerToPeer(destinationConnection);
}

RM3SerializationResult User::Serialize(RakNet::SerializeParameters * serializeParameters)
{
	return RM3SR_BROADCAST_IDENTICALLY;
}


// Main
int RunGameNetworkFrame(void)
{

	// Allocate plugins. 
	rakPeerInterface = RakNet::RakPeerInterface::GetInstance();
	teamManager = TeamManager::GetInstance();
	fullyConnectedMesh2 = FullyConnectedMesh2::GetInstance();
	networkIDManager = NetworkIDManager::GetInstance();
	tcp = TCPInterface::GetInstance();
	natPunchthroughClient = NatPunchthroughClient::GetInstance();
#ifdef NAT_TYPE_DETECTION_SERVER
	natTypeDetectionClient = NatTypeDetectionClient::GetInstance();
#endif
	rpc4 = RPC4::GetInstance();
	readyEvent = ReadyEvent::GetInstance();
	replicaManager3 = new RM3_Class_Factory;
	httpConnection2 = HTTPConnection2::GetInstance();

	// Attach plugins
	rakPeerInterface->AttachPlugin(fullyConnectedMesh2);
	rakPeerInterface->AttachPlugin(teamManager);
	rakPeerInterface->AttachPlugin(natPunchthroughClient);
#ifdef NAT_InterfaceTYPE_DETECTION_SERVER
	rakPeerInterface->AttachPlugin(natTypeDetectionClient);
#endif	  
	rakPeerInterface->AttachPlugin(rpc4);
	rakPeerInterface->AttachPlugin(readyEvent);
	rakPeerInterface->AttachPlugin(replicaManager3);
	/// TCPInterface supports plugins too
	tcp->AttachPlugin(httpConnection2);


	// Setup plugins: Disable automatically adding new connections. Allocate initial objects and register for replication

	// Allocate a world instance to be used for team operations
	teamManager->AddWorld(0);

	// do not automatically count new connections
	teamManager->SetAutoManageConnections(false);

	//new connections do not count until after login.
	fullyConnectedMesh2->SetAutoparticipateConnections(false);

	// Tell ReplicaManager3 which networkIDManager to use for object lookup, used for automatic serialization
	replicaManager3->SetNetworkIDManager(networkIDManager);
	// Do not automatically count new connections, but do drop lost connections automatically
	replicaManager3->SetAutoManageConnections(false, true);

	// Reference static game objects that always exist
	game = new Game;
	game->SetNetworkIDManager(networkIDManager);
	game->SetNetworkID(0);
	replicaManager3->Reference(game);

	// Setup my own user
	User *user = new User;
	user->SetNetworkIDManager(networkIDManager);
	user->userName = rakPeerInterface->GetMyGUID().ToString();
	// Inform TeamManager of my user's team member info
	teamManager->GetWorldAtIndex(0)->ReferenceTeamMember(&user->tmTeamMember, user->GetNetworkID());

	// Startup RakNet on first available port
	RakNet::SocketDescriptor sd;
	sd.socketFamily = AF_INET; // Only IPV4 supports broadcast on 255.255.255.255
	sd.port = 0;
	StartupResult sr = rakPeerInterface->Startup(8, &sd, 1);
	RakAssert(sr == RAKNET_STARTED);
	rakPeerInterface->SetMaximumIncomingConnections(8);
	rakPeerInterface->SetTimeoutTime(30000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
	printf("Our guid is %s\n", rakPeerInterface->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());
	printf("Started on %s\n", rakPeerInterface->GetMyBoundAddress().ToString(true));

	// Start TCPInterface and begin connecting to the NAT punchthrough server
	tcp->Start(0, 0, 1);

	// Connect to hosting server
	game->EnterPhase(Game::CONNECTING_TO_SERVER);

	// Read packets loop
	char ch;
	Packet *packet;
	while (game->phase != Game::EXIT_SAMPLE)
	{
		for (packet = rakPeerInterface->Receive(); packet; rakPeerInterface->DeallocatePacket(packet), packet = rakPeerInterface->Receive())
		{
			switch (packet->data[0])
			{
			case ID_NEW_INCOMING_CONNECTION:
			{
				printf("ID_NEW_INCOMING_CONNECTION from %s. guid=%s.\n", packet->systemAddress.ToString(true), packet->guid.ToString());
			}
			break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
				printf("ID_CONNECTION_REQUEST_ACCEPTED from %s,guid=%s\n", packet->systemAddress.ToString(true), packet->guid.ToString());

				if (game->phase == Game::CONNECTING_TO_SERVER)
				{
					game->natPunchServerAddress = packet->systemAddress;
					game->natPunchServerGuid = packet->guid;
					// PC self-hosted servers only: Use the NAT punch server to determine NAT type. Attempt to open router if needed.
					if (NAT_TYPE_DETECTION_SERVER)
					{
						game->EnterPhase(Game::DETERMINE_NAT_TYPE);
					}
					else
					{
						OpenUPNP();
					}
				}
				else if (game->phase == Game::CONNECTING_TO_GAME_HOST)
				{
					printf("Asking host to join session...\n");

					// So time in single player does not count towards which system has been running multiplayer the longest
					fullyConnectedMesh2->ResetHostCalculation();

					// Custom message to ask to join the game
					// We first connect to the game host, and the game host is responsible for calling StartVerifiedJoin() for us to join the session
					BitStream bsOut;
					bsOut.Write((MessageID)ID_USER_PACKET_ENUM);
					rakPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->guid, false);
				}
			}
			break;
			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
				if (game->phase == Game::DETERMINE_NAT_TYPE)
				{
					printf("Lost connection during NAT type detection. Reason %s. Retrying...\n", PacketLogger::BaseIDTOString(packet->data[0]));
					game->EnterPhase(Game::CONNECTING_TO_SERVER);
				}
				else if (game->phase == Game::NAT_PUNCH_TO_GAME_HOST)
				{
					printf("Lost connection during NAT punch to game host. Reason %s.\n", PacketLogger::BaseIDTOString(packet->data[0]));
					game->EnterPhase(Game::SEARCH_FOR_GAMES);
				}
				else
				{
					if (packet->guid == game->natPunchServerGuid)
					{
						printf("Server connection lost. Reason %s.\nGame session is no longer searchable.\n", PacketLogger::BaseIDTOString(packet->data[0]));
					}
					else
					{
						printf("Peer connection lost. Reason %s.\n", PacketLogger::BaseIDTOString(packet->data[0]));
					}
				}
				break;

			case ID_ALREADY_CONNECTED:
				printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", packet->guid);   
				break;

			case ID_INVALID_PASSWORD:
			case ID_NO_FREE_INCOMING_CONNECTIONS:
			case ID_CONNECTION_ATTEMPT_FAILED:
			case ID_CONNECTION_BANNED:
			case ID_IP_RECENTLY_CONNECTED:
			case ID_INCOMPATIBLE_PROTOCOL_VERSION:
				// Note: Failing to connect to another system does not automatically mean we cannot join a session, since that system may be disconnecting from the host simultaneously
				// FullyConnectedMesh2::StartVerifiedJoin() internally handles success or failure and notifies the client through ID_FCM2_VERIFIED_JOIN_FAILED if needed.
				printf("Failed to connect to %s. Reason %s\n", packet->systemAddress.ToString(true), PacketLogger::BaseIDTOString(packet->data[0]));

				if (game->phase == Game::CONNECTING_TO_SERVER)
					game->EnterPhase(Game::EXIT_SAMPLE);
				break;

			case ID_FCM2_NEW_HOST:
			{
				RakNet::BitStream bs(packet->data, packet->length, false);
				bs.IgnoreBytes(1);
				RakNetGUID oldHost;
				bs.Read(oldHost);

				if (packet->guid == rakPeerInterface->GetMyGUID())
				{
					if (oldHost != UNASSIGNED_RAKNET_GUID)
					{
						if (game->phase == Game::IN_LOBBY_WAITING_FOR_HOST)
							game->phase = Game::IN_LOBBY_WITH_HOST;
						PostRoomToMaster();
						printf("ID_FCM2_NEW_HOST: Taking over as host from the old host.\nNew options:\n");
					}
					else
					{
						// Room not hosted if we become host the first time since this was done in CreateRoom() already
						printf("ID_FCM2_NEW_HOST: We have become host for the first time. New options:\n");
					}

					printf("(L)ock and unlock game\n");
				}
				else
				{
					if (oldHost != UNASSIGNED_RAKNET_GUID)
						printf("ID_FCM2_NEW_HOST: A new system %s has become host, GUID=%s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
					else
						printf("ID_FCM2_NEW_HOST: System %s is host, GUID=%s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
				}

				if (oldHost == UNASSIGNED_RAKNET_GUID)
				{
					// First time calculated host. Add existing connections to ReplicaManager3
					DataStructures::List<RakNetGUID> participantList;
					fullyConnectedMesh2->GetParticipantList(participantList);
					for (unsigned int i = 0; i < participantList.Size(); i++)
						RegisterGameParticipant(participantList[i]);

					// Reference previously created replicated objects, which cannot be serialized until host is known the first time
					if (packet->guid == rakPeerInterface->GetMyGUID())
					{
						// As host, reference the teams we created
						for (unsigned int i = 0; i < game->teams.Size(); i++)
							replicaManager3->Reference(game->teams[i]);
					}

					// Reference the user we created (host or not)
					for (unsigned int i = 0; i < game->users.Size(); i++)
						replicaManager3->Reference(game->users[i]);
				}
			}
			break;
			case ID_TEAM_BALANCER_TEAM_ASSIGNED:
			{
				printf("ID_TEAM_BALANCER_TEAM_ASSIGNED for ");
				TM_World *world;
				TM_TeamMember *teamMember;
				teamManager->DecodeTeamAssigned(packet, &world, &teamMember);
				printf("worldId=%i teamMember=%s", world->GetWorldId(), ((User*)teamMember->GetOwner())->userName.C_String());
				if (teamMember->GetCurrentTeam() == 0)
					printf(" not on team\n");
				else
					printf(" on team %s\n", ((Team*)(teamMember->GetCurrentTeam()->GetOwner()))->teamName.C_String());
			}
			break;
			case ID_TEAM_BALANCER_REQUESTED_TEAM_FULL:
			{
				printf("ID_TEAM_BALANCER_REQUESTED_TEAM_FULL\n");
			}
			break;
			case ID_TEAM_BALANCER_REQUESTED_TEAM_LOCKED:
			{
				printf("ID_TEAM_BALANCER_REQUESTED_TEAM_LOCKED\n");
			}
			break;
			case ID_TEAM_BALANCER_TEAM_REQUESTED_CANCELLED:
			{
				printf("ID_TEAM_BALANCER_TEAM_REQUESTED_CANCELLED\n");
			}
			break;
			case ID_NAT_TARGET_NOT_CONNECTED:
			case ID_NAT_TARGET_UNRESPONSIVE:
			case ID_NAT_CONNECTION_TO_TARGET_LOST:
			case ID_NAT_PUNCHTHROUGH_FAILED:
			{
				// As with connection failed, this does not automatically mean we cannot join the session
				// We only fail on ID_FCM2_VERIFIED_JOIN_FAILED
				printf("NAT punch to %s failed. Reason %s\n", packet->guid.ToString(), PacketLogger::BaseIDTOString(packet->data[0]));

				if (game->phase == Game::NAT_PUNCH_TO_GAME_HOST)
					game->EnterPhase(Game::SEARCH_FOR_GAMES);
			}

			case ID_NAT_ALREADY_IN_PROGRESS:
				// Can ignore this
				break;

			case ID_NAT_PUNCHTHROUGH_SUCCEEDED:
			{
				if (game->phase == Game::NAT_PUNCH_TO_GAME_HOST || game->phase == Game::VERIFIED_JOIN)
				{
					// Connect to the session host
					ConnectionAttemptResult car = rakPeerInterface->Connect(packet->systemAddress.ToString(false), packet->systemAddress.GetPort(), 0, 0);
					if (car != RakNet::CONNECTION_ATTEMPT_STARTED)
					{
						printf("Failed connect call to %s. Code=%i\n", packet->systemAddress.ToString(false), car);
						game->EnterPhase(Game::SEARCH_FOR_GAMES);
					}
					else
					{
						if (game->phase == Game::NAT_PUNCH_TO_GAME_HOST)
						{
							printf("NAT punch completed. Connecting to %s (game host)...\n", packet->systemAddress.ToString(true));
							game->EnterPhase(Game::CONNECTING_TO_GAME_HOST);
						}
						else
						{
							printf("NAT punch completed. Connecting to %s (game client)...\n", packet->systemAddress.ToString(true));
						}
					}
				}
			}
			break;



			case ID_NAT_TYPE_DETECTION_RESULT:
			{
				game->myNatType = (RakNet::NATTypeDetectionResult) packet->data[1];
				printf("NAT Type is %s (%s)\n", NATTypeDetectionResultToString(game->myNatType), NATTypeDetectionResultToStringFriendly(game->myNatType));

				if (game->myNatType != RakNet::NAT_TYPE_NONE)
				{
					OpenUPNP();
				}

				if (game->myNatType == RakNet::NAT_TYPE_PORT_RESTRICTED || game->myNatType == RakNet::NAT_TYPE_SYMMETRIC)
				{
					printf("Note: Your router must support UPNP or have the user manually forward ports.\n");
					printf("Otherwise NATPunchthrough may not always succeed.\n");
				}

				game->EnterPhase(Game::SEARCH_FOR_GAMES);
			}
			break;

			case ID_READY_EVENT_ALL_SET:
				printf("Got ID_READY_EVENT_ALL_SET from %s\n", packet->systemAddress.ToString(true));
				printf("All users ready.\n");
				if (fullyConnectedMesh2->IsConnectedHost())
					printf("New options:\n(B)egin gameplay\n");
				break;

			case ID_READY_EVENT_SET:
				printf("Got ID_READY_EVENT_SET from %s\n", packet->systemAddress.ToString(true));
				break;

			case ID_READY_EVENT_UNSET:
				printf("Got ID_READY_EVENT_UNSET from %s\n", packet->systemAddress.ToString(true));
				break;

				// ID_USER_PACKET_ENUM is used by this sample as a custom message to ask to join a game
			case ID_USER_PACKET_ENUM:
				if (game->phase > Game::SEARCH_FOR_GAMES)
				{
					printf("Got request from client to join session.\nExecuting StartVerifiedJoin()\n");
					fullyConnectedMesh2->StartVerifiedJoin(packet->guid);
				}
				else
				{
					BitStream bsOut;
					bsOut.Write((MessageID)(ID_USER_PACKET_ENUM + 1));
					rakPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->guid, false);
				}
				break;
				// ID_USER_PACKET_ENUM+1 is used by this sample as a custom message to reject a join game request
				// Requests may also be later rejected through FullyConnectedMesh2::RespondOnVerifiedJoinCapable() to send ID_FCM2_VERIFIED_JOIN_REJECTED
			case (ID_USER_PACKET_ENUM + 1):
				printf("Join request denied\n");
				game->EnterPhase(Game::SEARCH_FOR_GAMES);
				break;
			case ID_FCM2_VERIFIED_JOIN_START:
			{
				game->EnterPhase(Game::VERIFIED_JOIN);

				// This message means the session host sent us a list of systems in the session
				// Once we connect to, or fail to connect to, each of these systems we will get ID_FCM2_VERIFIED_JOIN_FAILED, ID_FCM2_VERIFIED_JOIN_ACCEPTED, or ID_FCM2_VERIFIED_JOIN_REJECTED
				printf("Host sent us system list. Doing NAT punch to each system...\n");
				DataStructures::List<SystemAddress> addresses;
				DataStructures::List<RakNetGUID> guids;

				DataStructures::List<BitStream*> userData;//added this in to remove an incomplete argument list error in fullyConnectedMesh2->GetVerifiedJoinRequiredProcessingList(...)  below
				fullyConnectedMesh2->GetVerifiedJoinRequiredProcessingList(packet->guid, addresses, guids, userData);
				for (unsigned int i = 0; i < guids.Size(); i++)
					natPunchthroughClient->OpenNAT(guids[i], game->natPunchServerAddress);
			}
			break;

			case ID_FCM2_VERIFIED_JOIN_CAPABLE:
				printf("Client is capable of joining FullyConnectedMesh2.\n");
				if (game->lockGame)
				{
					RakNet::BitStream bsOut;
					bsOut.Write("Game is locked");
					fullyConnectedMesh2->RespondOnVerifiedJoinCapable(packet, false, &bsOut);
				}
				else
					fullyConnectedMesh2->RespondOnVerifiedJoinCapable(packet, true, 0);
				break;

			case ID_FCM2_VERIFIED_JOIN_ACCEPTED:
			{
				DataStructures::List<RakNetGUID> systemsAccepted;
				bool thisSystemAccepted;
				fullyConnectedMesh2->GetVerifiedJoinAcceptedAdditionalData(packet, &thisSystemAccepted, systemsAccepted, 0);
				if (thisSystemAccepted)
					printf("Game join request accepted\n");
				else
					printf("System %s joined the mesh\n", systemsAccepted[0].ToString());

				// Add the new participant to the game if we already know who the host is. Otherwise do this
				// once ID_FCM2_NEW_HOST arrives
				if (fullyConnectedMesh2->GetConnectedHost() != UNASSIGNED_RAKNET_GUID)
				{
					// FullyConnectedMesh2 already called AddParticipant() for each accepted system
					// Still need to add those systems to the other plugins though
					for (unsigned int i = 0; i < systemsAccepted.Size(); i++)
						RegisterGameParticipant(systemsAccepted[i]);

					if (thisSystemAccepted)
						game->EnterPhase(Game::IN_LOBBY_WITH_HOST);
				}
				else
				{
					if (thisSystemAccepted)
						game->EnterPhase(Game::IN_LOBBY_WAITING_FOR_HOST);
				}

				printf("(E)xit room\n");
			}
			break;

			case ID_FCM2_VERIFIED_JOIN_REJECTED:
			{
				BitStream additionalData;
				fullyConnectedMesh2->GetVerifiedJoinRejectedAdditionalData(packet, &additionalData);
				RakString reason;
				additionalData.Read(reason);
				printf("Join rejected. Reason=%s\n", reason.C_String());
				rakPeerInterface->CloseConnection(packet->guid, true);
				game->EnterPhase(Game::SEARCH_FOR_GAMES);
				break;
			}

			case ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE:
			{
				if (replicaManager3->GetAllConnectionDownloadsCompleted() == true)
				{
					printf("Completed all remote downloads\n");

					if (game->gameInLobby)
						game->EnterPhase(Game::IN_LOBBY_WITH_HOST);
					else
						game->EnterPhase(Game::IN_GAME);
				}

				break;
			}
			}
		}

	

	// The following code is TCP operations for talking to the master server, and parsing the reply
	SystemAddress sa;
	// This is kind of crappy, but for TCP plugins, always do HasCompletedConnectionAttempt, then Receive(), then HasFailedConnectionAttempt(),HasLostConnection()
	sa = tcp->HasCompletedConnectionAttempt();
	for (packet = tcp->Receive(); packet; tcp->DeallocatePacket(packet), packet = tcp->Receive())
		;
	sa = tcp->HasFailedConnectionAttempt();
	sa = tcp->HasLostConnection();

	RakString stringTransmitted;
	RakString hostTransmitted;
	RakString responseReceived;
	SystemAddress hostReceived;
	int contentOffset;
	if (httpConnection2->GetResponse(stringTransmitted, hostTransmitted, responseReceived, hostReceived, contentOffset))
	{
		if (responseReceived.IsEmpty() == false)
		{
			if (contentOffset == -1)
			{
				// No content
				printf(responseReceived.C_String());
			}
			else
			{
				json_error_t error;
				json_t *root = json_loads(responseReceived.C_String() + contentOffset, JSON_REJECT_DUPLICATES, &error);
				if (!root)
				{
					printf("Error parsing JSON\n", __LINE__);
				}
				else
				{
					void *iter = json_object_iter(root);
					while (iter)
					{
						const char *firstKey = json_object_iter_key(iter);
						if (_stricmp(firstKey, "GET") == 0)
						{
							game->SetMasterServerQueryResult(root);
							root = 0;

							json_t* jsonArray = json_object_iter_value(iter);
							size_t arraySize = json_array_size(jsonArray);
							for (unsigned int i = 0; i < arraySize; i++)
							{
								json_t* object = json_array_get(jsonArray, i);
								json_t* roomNameVal = json_object_get(object, "roomName");
								RakAssert(roomNameVal->type == JSON_STRING);
								json_t* natTypesVal = json_object_get(object, "natTypes");
								RakAssert(natTypesVal->type == JSON_ARRAY);
								size_t natTypesSize = json_array_size(natTypesVal);
								printf("Room name: %s. Players: %i\n", json_string_value(roomNameVal), natTypesSize);
							}

							if (arraySize == 0)
								printf("No rooms.\n");


							printf("(J)oin room\n");
							printf("(C)reate room\n");
							printf("(S)earch rooms\n");
							break;
						}
						else if (_stricmp(firstKey, "POST") == 0)
						{
							RakAssert(_stricmp(firstKey, "POST") == 0);

							json_t* jsonObject = json_object_iter_value(iter);
							json_t* val1 = json_object_get(jsonObject, "__rowId");
							RakAssert(val1->type == JSON_INTEGER);
							game->masterServerRow = (int)json_integer_value(val1);

							printf("Session posted to row %i\n", game->masterServerRow);
							break;
						}
						else
						{
							iter = json_object_iter_next(root, iter);
							RakAssert(iter != 0);
						}
					}

					json_decref(root);
				}
			}
		}
	}

	if (_kbhit())
	{
		ch = _getch();

		if (game->phase == Game::SEARCH_FOR_GAMES)
		{
			if (ch == 'c' || ch == 'C')
			{
				CreateRoom();
			}
			if (ch == 's' || ch == 'S')
			{
				game->SearchForGames();
			}
			else if (ch == 'j' || ch == 'J')
			{
				size_t arraySize = 0;
				json_t *jsonArray = game->GetMasterServerQueryResult();
				if (jsonArray)
				{
					arraySize = json_array_size(jsonArray);
				}

				// Join room
				if (arraySize == 0)
				{
					printf("No rooms to join.\n");
				}
				else
				{
					int index;
					if (arraySize > 1)
					{
						printf("Enter index of room to join.\n");
						char indexstr[64];
						Gets(indexstr, 64);
						index = atoi(indexstr);
					}
					else
					{
						index = 0;
					}

					if (index < 0 || (unsigned int)index >= arraySize)
					{
						printf("Index out of range.\n");
					}
					else
					{
						json_t* object = json_array_get(jsonArray, index);
						json_t* guidVal = json_object_get(object, "guid");
						RakAssert(guidVal->type == JSON_STRING);
						RakNetGUID clientGUID;
						clientGUID.FromString(json_string_value(guidVal));
						if (clientGUID != rakPeerInterface->GetMyGUID())
						{
							natPunchthroughClient->OpenNAT(clientGUID, game->natPunchServerAddress);
							game->EnterPhase(Game::NAT_PUNCH_TO_GAME_HOST);
						}
						else
						{
							printf("Cannot join your own room\n");
						}
					}
				}
			}
		}
		else
		{
			if (game->phase == Game::IN_GAME)
			{
				if (ch == 'c' || ch == 'C')
				{
					DataStructures::List<RakNetGUID> participantList;
					fullyConnectedMesh2->GetParticipantList(participantList);

					if (participantList.Size() > 0)
					{
						printf("Enter in-game chat message: ");
						char str[256];
						Gets(str, 256);
						RakString rs;
						// Don't use RakString constructor to assign str, or will process % escape characters
						rs = str;
						BitStream bsOut;
						bsOut.Write(rs);
						for (unsigned int i = 0; i < participantList.Size(); i++)
							rpc4->Signal("InGameChat", &bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, participantList[i], false, false);
					}
				}
			}

			if (ch == '1')
			{
				user->tmTeamMember.RequestTeamSwitch(&game->teams[0]->tmTeam, 0);
			}
			else if (ch == '2')
			{
				user->tmTeamMember.RequestTeamSwitch(&game->teams[1]->tmTeam, 0);
			}
			else if (ch == 'r' || ch == 'R')
			{
				if (readyEvent->SetEvent(0, true))
					printf("We are ready to start.\n");
			}
			else if (ch == 'u' || ch == 'U')
			{
				if (readyEvent->SetEvent(0, false))
					printf("We are no longer ready to start.\n");
			}
			else if (ch == 'l' || ch == 'L')
			{
				if (fullyConnectedMesh2->IsConnectedHost())
				{
					if (game->lockGame)
					{
						printf("Game is no longer locked\n");
						game->lockGame = false;
					}
					else
					{
						printf("Game is now locked\n");
						game->lockGame = true;
					}
				}
			}
			else if (ch == 'b' || ch == 'B')
			{
				if (fullyConnectedMesh2->IsConnectedHost())
				{
					if (game->gameInLobby)
					{
						readyEvent->ForceCompletion(0);
						game->gameInLobby = false;
						game->EnterPhase(Game::IN_GAME);
					}
					else
					{
						readyEvent->DeleteEvent(0);
						printf("Game ended, and now in lobby\n");
						game->gameInLobby = true;
						game->EnterPhase(Game::IN_LOBBY_WITH_HOST);
					}
				}
			}
			else if (ch == 'e' || ch == 'E')
			{
				// Disconnect from FullyConnectedMesh2 participants
				DataStructures::List<RakNetGUID> participantList;
				fullyConnectedMesh2->GetParticipantList(participantList);
				for (unsigned int i = 0; i < participantList.Size(); i++)
					rakPeerInterface->CloseConnection(participantList[i], true);

				// User instances are deleted automatically from ReplicaManager3.
				// However, teams are not deleted since the Team class can migrate between systems. So delete Team instances manually
				while (game->teams.Size())
					delete game->teams[game->teams.Size() - 1];

				// If we were the host, no longer list this session
				// The new host will call PostRoomToCloud to reupload under a new IP address on ID_FCM2_NEW_HOST
				ReleaseRoomFromCloud();

				// Clear out state data from plugins
				fullyConnectedMesh2->Clear();
				readyEvent->DeleteEvent(0);
				replicaManager3->Clear(false);
				replicaManager3->Reference(game);

				game->Reset();
				game->EnterPhase(Game::SEARCH_FOR_GAMES);
			}
			else if (ch == 'q' || ch == 'Q')
			{
				printf("Quitting.\n");

				RakString rspost = RakString::FormatForGET(
					RakString(MASTER_SERVER_ADDRESS "/testServer?row=%i", game->masterServerRow));
				httpConnection2->TransmitRequest(rspost, MASTER_SERVER_ADDRESS, MASTER_SERVER_PORT);

				game->EnterPhase(Game::EXIT_SAMPLE);
			}
		}
	}

	// The game host updates the master server
	RakNet::Time t = RakNet::GetTime();
	if ((fullyConnectedMesh2->IsConnectedHost() || game->users.Size() == 1) &&
		t > game->whenToNextUpdateMasterServer &&
		(game->phase == Game::IN_LOBBY_WITH_HOST ||
			game->phase == Game::IN_GAME ||
			game->phase == Game::IN_LOBBY_WAITING_FOR_HOST)
		)
	{
		PostRoomToMaster();
	}

	RakSleep(30);
}

rakPeerInterface->Shutdown(100);

while (game->teams.Size())
delete game->teams[game->teams.Size() - 1];
while (game->users.Size())
delete game->users[game->users.Size() - 1];
delete game;

RakPeerInterface::DestroyInstance(rakPeerInterface);
TeamManager::DestroyInstance(teamManager);
FullyConnectedMesh2::DestroyInstance(fullyConnectedMesh2);
NatPunchthroughClient::DestroyInstance(natPunchthroughClient);
NatTypeDetectionClient::DestroyInstance(natTypeDetectionClient);
RPC4::DestroyInstance(rpc4);
ReadyEvent::DestroyInstance(readyEvent);
delete replicaManager3;
NetworkIDManager::DestroyInstance(networkIDManager);
HTTPConnection2::DestroyInstance(httpConnection2);

return 1;
}




Connection_RM3 * RM3_Class_Factory::AllocConnection(const SystemAddress & systemAddress, RakNetGUID rakNetGUID) const
{
	return new ConnectionRM3_Class_Factory(systemAddress, rakNetGUID);
}

void RM3_Class_Factory::DeallocConnection(Connection_RM3 * connection) const
{
	delete connection;
}

Replica3 * ConnectionRM3_Class_Factory::AllocReplica(RakNet::BitStream * allocationIdBitstream, ReplicaManager3 * replicaManager3)
{
	RakString objectType;
	// Types are written by WriteAllocationID()
	allocationIdBitstream->Read(objectType);
	if (objectType == "User") return new User;
	if (objectType == "Team") return new Team;
	RakAssert("Unknown type in AllocReplica" && 0);
	return 0;
}


