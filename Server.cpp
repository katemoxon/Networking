#include "Server.h"
#include <cstring>

Server::Server()
{
	// initialize the Raknet peer interface first
	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();

	m_uiConnectionCounter = 1;
	m_uiObjectCounter = 1;
}

Server::~Server()
{

}

void Server::run()
{

	// startup the server, and start it listening to clients
	std::cout << "Starting up the server..." << std::endl;

	// create a socket descriptor to describe this connection
	RakNet::SocketDescriptor sd(PORT, 0);

	// now call startup - max of 32 connections, on the assigned port
	m_pPeerInterface->Startup(32, &sd, 1);
	m_pPeerInterface->SetMaximumIncomingConnections(32);

	handleNetworkMessages();
}

void Server::handleNetworkMessages()
{
	RakNet::Packet* packet = nullptr;

	while (true)
	{
		for (packet = m_pPeerInterface->Receive();
			packet;
			m_pPeerInterface->DeallocatePacket(packet),
			packet = m_pPeerInterface->Receive())
		{
			switch (packet->data[0])
			{
			case ID_NEW_INCOMING_CONNECTION:
			{
				addNewConnection(packet->systemAddress);
				std::cout << "A connection is incoming.\n";
				break;
			}
			case ID_SERVER_TEXT_MESSAGE:
			{
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				RakNet::RakString str;
				bsIn.Read(str);
				std::cout << str.C_String() << std::endl;
				break;
			}
			//case ID_CLIENT_CREATE_OBJECT:
			//{
			//	RakNet::BitStream bsIn(packet->data, packet->length, false);
			//	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
			//	createNewObject(bsIn, packet->systemAddress);
			//	break;
			//}
			case ID_LOGIN_PING:
			{
				replyPing(packet->systemAddress);
				break;
			}
			case ID_AUTHENTICATION:
			{
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				RakNet::RakString userID;
				RakNet::RakString passID;
				bsIn.Read(userID);
				bsIn.Read(passID);

				authenticate(userID.C_String(), passID.C_String(), packet->systemAddress);

				break;
			}
			case ID_CLIENT_SEND_TEXT_MESSAGE:
			{
				if (m_connectedClients[systemAddressToClientID(packet->systemAddress)].isAuthConnection)
				{
					RakNet::BitStream bsIn(packet->data, packet->length, false);
					bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
					RakNet::RakString newMSG;
					bsIn.Read(newMSG);

					relayMSG(newMSG, packet->systemAddress);
				}
				else
				{
					denyMSG(packet->systemAddress);
				}
				break;
			}
			case ID_DISCONNECTION_NOTIFICATION:
				std::cout << "A client has disconnected.\n";
				removeConnection(packet->systemAddress);
				break;
			case ID_CONNECTION_LOST:
				std::cout << "A client lost the connection.\n";
				removeConnection(packet->systemAddress);
				break;

			default:
				std::cout << "Received a message with a unknown id: " << packet->data[0];
				break;
			}
		}
	}


}

void Server::addNewConnection(RakNet::SystemAddress systemAddress)
{
	ConnectionInfo info;
	info.sysAddress = systemAddress;
	info.uiConnectionID = m_uiConnectionCounter++;
	info.isAuthConnection = false;
	m_connectedClients[info.uiConnectionID] = info;

	sendClientIDToClient(info.uiConnectionID);
}

void Server::removeConnection(RakNet::SystemAddress systemAddress)
{
	for (auto it = m_connectedClients.begin(); it != m_connectedClients.end(); it++)
	{
		if (it->second.sysAddress == systemAddress)
		{
			m_connectedClients.erase(it);
			break;
		}
	}
}

unsigned int Server::systemAddressToClientID(RakNet::SystemAddress& systemAddress)
{
	for (auto it = m_connectedClients.begin(); it != m_connectedClients.end(); it++)
	{
		if (it->second.sysAddress == systemAddress)
		{
			return it->first;
		}
	}

	return 0;
}

void Server::sendClientIDToClient(unsigned int uiClientID)
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)GameMessages::ID_SERVER_CLIENT_ID);
	bs.Write(uiClientID);

	m_pPeerInterface->Send(&bs, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, m_connectedClients[uiClientID].sysAddress, false);
}

//void Server::createNewObject(RakNet::BitStream& bsIn, RakNet::SystemAddress&
//	ownerSysAddress)
//{
//	GameObject newGameObject;
//	//Read in the information from the packet
//	bsIn.Read(newGameObject.fXPos);
//	bsIn.Read(newGameObject.fZPos);
//	bsIn.Read(newGameObject.fRedColour);
//	bsIn.Read(newGameObject.fGreenColour);
//	bsIn.Read(newGameObject.fBlueColour);
//
//	newGameObject.uiOwnerClientID = systemAddressToClientID(ownerSysAddress);
//	newGameObject.uiObjectID = m_uiObjectCounter++;
//}

void Server::replyPing(RakNet::SystemAddress &ownerSysAddress)
{
	LoginPingResponse pResponse;
	pResponse.isPossible = true;
	std::strcpy(pResponse.errorMessage, "");

	RakNet::BitStream sendBack;

	sendBack.Write((RakNet::MessageID) GameMessages::ID_LOGIN_PING_REPONSE);
	sendBack.Write(pResponse.isPossible);
	sendBack.Write(pResponse.errorMessage);

	m_pPeerInterface->Send(&sendBack, HIGH_PRIORITY, RELIABLE_ORDERED, 0, ownerSysAddress, false);
}

void Server::confirmAuthentication(bool isConfirmed, const std::string& errorMessage, RakNet::SystemAddress& ownerSystem)
{
	AuthenticationConfirmation authenticationConfirmation;
	authenticationConfirmation.wasPossible = isConfirmed;
	std::strcpy(authenticationConfirmation.errorMessage, errorMessage.c_str());

	RakNet::BitStream sendBack;

	sendBack.Write((RakNet::MessageID) GameMessages::ID_AUTHENTICATION_CONFIRMATION);
	sendBack.Write(authenticationConfirmation.wasPossible);
	sendBack.Write(authenticationConfirmation.errorMessage);

	m_pPeerInterface->Send(&sendBack, HIGH_PRIORITY, RELIABLE_ORDERED, 0, ownerSystem, false);
}

void Server::authenticate(const std::string& q_user, const std::string& q_pass, RakNet::SystemAddress& ownerSysAddress)
{
	if (q_user != "username" || q_pass != "password")
	{
		confirmAuthentication(false, "Username or password is incorrect", ownerSysAddress);
	}
	else
	{
		confirmAuthentication(true, " ", ownerSysAddress);
		m_connectedClients[systemAddressToClientID(ownerSysAddress)].isAuthConnection = true;
	}
}

void Server::relayMSG(const RakNet::RakString& relayM, RakNet::SystemAddress& ownerSysAddress)
{
	RakNet::BitStream bsRelay;
	bsRelay.Write((RakNet::MessageID) GameMessages::ID_CLIENT_RECEIVE_TEXT_MESSAGE);
	bsRelay.Write(relayM);

	m_pPeerInterface->Send(&bsRelay, HIGH_PRIORITY, RELIABLE_ORDERED, 0, ownerSysAddress, true);
}

void Server::denyMSG(RakNet::SystemAddress& ownerSysAddress)
{
	RakNet::BitStream bsDeny;
	bsDeny.Write((RakNet::MessageID) GameMessages::ID_MESSAGE_NOT_SENT);
	bsDeny.Write("Your message was not sent. Log in and try again.");

	m_pPeerInterface->Send(&bsDeny, HIGH_PRIORITY, RELIABLE_ORDERED, 0, ownerSysAddress, false);
}