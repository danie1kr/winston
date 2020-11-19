#pragma once

#include "../libwinston/HAL.h"

class UDPSocketLWIP : public winston::hal::UDPSocket, winston::Shared_Ptr<UDPSocketLWIP>
{
public:
	using winston::Shared_Ptr<UDPSocketLWIP>::Shared;
	using winston::Shared_Ptr<UDPSocketLWIP>::make;

	UDPSocketLWIP(const std::string ip, const unsigned short port);
	const winston::Result send(const std::vector<unsigned char> data);
private:

	const winston::Result connect();

	SOCKET udpSocket;
	SOCKADDR_IN addr;
};