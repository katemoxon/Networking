#pragma once

struct GameObject
{
	unsigned int uiOwnerClientID;
	unsigned int uiObjectID;

	float fRedColour;
	float fGreenColour;
	float fBlueColour;

	float fXPos;
	float fZPos;
};

struct LoginPing
{
};

struct LoginPingResponse
{
	bool isPossible;
	char errorMessage[256];
};

struct Authentication
{
	char userName[9];
	char password[9];
};

struct AuthenticationConfirmation
{
	bool wasPossible;
	char errorMessage[256];
};