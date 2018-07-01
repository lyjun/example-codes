#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define CB_SRV_PORT			6666
#define CB_MAX_CNN_REQ		10	

/*
 * 	Breif		Create TCP Socket
 * 	return		return fd if success;  return negative errno number if failed
 */
int CreateSocket()
{
     int fdSocket = socket(PF_INET, SOCK_STREAM, 0);
	 if(-1 == fdSocket)
	 {
		 perror("failed to create socket");
		 return -errno;
	 }
	 return fdSocket;
}

/*
 * 	Breif		Connect to TCP server. Recv data from server and send data to server.
 * 	return		None.
 */
void client()
{
	int fdSocket = 0;
	// socket
	if(0 > (fdSocket = CreateSocket()))
	{
		perror("Client-Socket");
		_exit(0);
	}

	// connect
	struct sockaddr_in stRemoteAddr;
	stRemoteAddr.sin_family = PF_INET;
	stRemoteAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	stRemoteAddr.sin_port = htons(CB_SRV_PORT);
	socklen_t iAddrSize = sizeof(stRemoteAddr);
	if(-1 == connect(fdSocket, (struct sockaddr *) &stRemoteAddr, iAddrSize))
	{
		perror("Client-connect");
		_exit(0);
	}

	// recv
	char szBuf[UCHAR_MAX] = "";
	if(-1 == recv(fdSocket, szBuf, UCHAR_MAX, 0))
	{
		perror("Client-Recv");
		_exit(0);
	}
	printf("client(recv) => %s\n", szBuf);

	// send 
	snprintf(szBuf, UCHAR_MAX, "hello sever.");
	printf("client(send) => %s\n", szBuf);
	if(-1 == send(fdSocket, szBuf, sizeof(szBuf), 0))
	{
		perror("client-send");
		_exit(0);
	}
}

/*
 * 	Breif		Handle client's connection
 * 	return		None.
 */
void Req_Handler(int fdSocket, struct sockaddr_in stRmtAddr)
{
	char szBuf[UCHAR_MAX] = "";
	char szClientIP[INET_ADDRSTRLEN];

	// send
	inet_ntop(AF_INET, &stRmtAddr.sin_addr, szClientIP, INET_ADDRSTRLEN);
	snprintf(szBuf, UCHAR_MAX, "hello ! %s(client)", szClientIP);
	printf("Server(send) => hello ! %s(client)\n", szClientIP);
	if(-1 == send(fdSocket, szBuf, UCHAR_MAX, 0))
	{
		perror("Handler-send");
		_exit(0);
	}

	// recv
	if(-1 == recv(fdSocket, szBuf, UCHAR_MAX, 0))
	{
		perror("Handler-recv");
		_exit(0);
	}
	printf("Server(recv) => %s\n", szBuf);
}

/*
 * 	Breif		Accept request and fork a process to handle
 * 	return		None.
 */
void server()
{
	int fdSocket = 0;
	// create socket
	if(0 > (fdSocket = CreateSocket()))
	{
		perror("Server-Socket");
		exit(0);
	}

	// binding
	struct sockaddr_in stServerAddr;
	bzero(&stServerAddr, sizeof(stServerAddr));
	stServerAddr.sin_family = PF_INET;
	stServerAddr.sin_addr.s_addr = INADDR_ANY;
	stServerAddr.sin_port = htons(CB_SRV_PORT);
	if(-1 == bind(fdSocket, (struct sockaddr *) &stServerAddr, sizeof(stServerAddr)))
	{
		perror("Server-bind");
		exit(0);
	}

	// listen connection
	if(-1 == listen(fdSocket, CB_MAX_CNN_REQ))
	{
		perror("Server-listen");
		exit(0);
	}

	for(; 1; )
	{
		// accept the connecion 
		int fdRemoteSocket = 0;
		struct sockaddr_in stClientAddr;
		socklen_t iClinetAddrSize = sizeof(stClientAddr);
		if(-1 == (fdRemoteSocket = accept(fdSocket, (struct sockaddr *) &stClientAddr, &iClinetAddrSize)))
		{
			perror("Server-accept");
			break;
		}

		switch(fork())
		{
			// if erorr
			case -1:
				perror("Server-fork");
				exit(0);

			// handler(fork-server)
			case 0:
				close(fdSocket);
				Req_Handler(fdRemoteSocket, stClientAddr);
				close(fdRemoteSocket);
				_exit(0);

			// server(parent)
			default:
				wait(NULL);
				close(fdRemoteSocket);
				close(fdSocket);
				return;
		}
	}
}

int main()
{
	int iClientPid = 0;

	printf("------------------------\n");
	printf("(NET) tcp exmaple \n");
	printf("------------------------\n");
	switch((iClientPid = fork()))
	{
		case -1:
			perror("failed to fork");
			exit(0);

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
