#pragma once
#include <iostream>
#include <string>

#include <thread>
#include <chrono>
#include <unordered_map>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

#include "GameMessages.h"
#include "GameObject.h"

class Server {
public:

	Server();
	~Server();

	void run();
	void handleNetworkMessages();
	//void createNewObject(RakNet::BitStream& bsIn, RakNet::SystemAddress&
	//	ownerSysAddress);

	void replyPing(RakNet::SystemAddress &ownerSysAddress);
	void confirmAuthentication(bool isConfirmed, const std::string& errorMessage, RakNet::SystemAddress& ownerSystem);
	void authenticate(const std::string& q_user, const std::string& q_pass, RakNet::SystemAddress& ownerSysAddress);
	void relayMSG(const RakNet::RakString& relayM, RakNet::SystemAddress& ownerSysAddress);
	void denyMSG(RakNet::SystemAddress& ownerSysAddress);

protected:

	unsigned int systemAddressToClientID(RakNet::SystemAddress& systemAddress);

	// connection functions
	void addNewConnection(RakNet::SystemAddress systemAddress);
	void removeConnection(RakNet::SystemAddress systemAddress);

	void sendClientIDToClient(unsigned int uiClientID);

	//void createNewObject(RakNet::BitStream& bsIn, RakNet::SystemAddress& ownerSysAddress);

private:

	struct ConnectionInfo
	{
		bool isAuthConnection;
		unsigned int uiConnectionID;
		RakNet::SystemAddress sysAddress;
	};

	const unsigned short PORT = 5456;

	RakNet::RakPeerInterface* m_pPeerInterface;

	unsigned int m_uiConnectionCounter;
	std::unordered_map<unsigned int, ConnectionInfo> m_connectedClients;
	std::vector<GameObject> m_gameObjects;
	unsigned int m_uiObjectCounter;
};
