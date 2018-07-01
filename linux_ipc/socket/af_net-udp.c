#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <limits.h>
#include <arpa/inet.h>

#define SZ_UDP_SRV_PORT		"9466"
#define CB_UDP_SRV_PORT		9466

void client()
{
	// ==== <old way> ===
	int fdClientSocket = socket(PF_INET, SOCK_DGRAM, 0);
	if(-1 == fdClientSocket)
	{
		perror("failed to create socket");
		_exit(0);
	}

	// set up address 
	struct sockaddr_in stRemoteAddr;
	stRemoteAddr.sin_family = PF_INET;
	stRemoteAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	stRemoteAddr.sin_port = htons(CB_UDP_SRV_PORT);
	socklen_t iAddrSize = sizeof(stRemoteAddr);
	// end ==== <old way> ===

	// communication
	char szBuf[UCHAR_MAX] = "";
	// 1. send
	snprintf(szBuf, UCHAR_MAX, "hello ~ I'm Client");
	if(-1 == sendto(fdClientSocket, szBuf, UCHAR_MAX, 0, (struct sockaddr *) &stRemoteAddr, iAddrSize))
	{
		perror("client-sendto");
		_exit(0);
	}
	printf("client(send) => %s\n", szBuf);

	// 2. recv
	if(-1 == recvfrom(fdClientSocket, szBuf, UCHAR_MAX, 0, (struct sockaddr *) &stRemoteAddr, &iAddrSize))
	{
		perror("client-recvfrom");
		_exit(0);
	}
	printf("Client(Recv) => %s\n", szBuf);
}

void server()
{
	// ===== <new way> ====
	//  get address information (socket & address)
	struct addrinfo  stHints, *pstRes = NULL;
	memset(&stHints, 0, sizeof(stHints));
	stHints.ai_family = PF_INET;
	stHints.ai_socktype = SOCK_DGRAM;
	stHints.ai_flags = AI_PASSIVE;
	int iRet = 0;
	if((iRet = getaddrinfo(NULL, SZ_UDP_SRV_PORT, &stHints, &pstRes)))
	{
		perror("server-getaddrinfo");
		exit(0);
	}

	// create socket
	int fsSrvSocket = 0;
	if(0 > (fsSrvSocket = socket(pstRes->ai_family, pstRes->ai_socktype, pstRes->ai_protocol)))
	{
		perror("server-socket");
		exit(0);
	}

	// binding
	if(-1 == bind(fsSrvSocket, pstRes->ai_addr, pstRes->ai_addrlen))
	{
		perror("Server-bind");
		exit(0);
	}
	// end ===== <new way> ====

	// communication
	char szBuf[UCHAR_MAX] = "";
	struct sockaddr_in stRemoteAddr;
	socklen_t iClientAddrSize = sizeof(stRemoteAddr);
	char szClinetIP[INET_ADDRSTRLEN];
	// 1. recv
	if(-1 == recvfrom(fsSrvSocket, szBuf, UCHAR_MAX, 0, (struct sockaddr *) &stRemoteAddr, &iClientAddrSize))
	{
		perror("server-recvfrom");
		exit(0);
	}
	inet_ntop(AF_INET, &stRemoteAddr.sin_addr, szClinetIP, INET_ADDRSTRLEN);
	printf("Server(Recv) => %s(%s)\n", szBuf, szClinetIP);

	// 2. send
	snprintf(szBuf, UCHAR_MAX, "bye ~ %s", szClinetIP);
	printf("Server(send) => %s\n", szBuf);
	if(-1 == sendto(fsSrvSocket, szBuf, UCHAR_MAX, 0, (struct sockaddr *) &stRemoteAddr, iClientAddrSize))
	{
		perror("server-sendto");
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	int iChildPid = 0;
	printf("------------------------\n");
	printf("(NET) udp exmaple \n");
	printf("------------------------\n");
	switch((iChildPid = fork()))
	{
		case -1:
			perror("main-fork");
			break;

		case 0:
			// wait server setup
			sleep(1);
			client();
			_exit(0);

		default:
			server();
			break;
	}
	return 0;
}
