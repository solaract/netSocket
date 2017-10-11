// netSocket.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
//#include <windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment (lib,"ws2_32.lib")

#define BUF_SIZE 512
#define DEFAULT_PORT "27015"

int main()
{
	WSADATA wsaData;
	struct addrinfo *result = NULL, 
		*ptr = NULL, 
		hints;
	SOCKET ListnSocket = INVALID_SOCKET;
	//SOCKET ClientSocket = INVALID_SOCKET;
	char recvbuf[BUF_SIZE];
	char *hello = "welcome!";
	int iResult,iSendResult;
	int recvbuflen = BUF_SIZE;
	//asd

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult) {
		printf("WSAStartup failed:%d\n", iResult);
		return 1;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult) {
		printf("getaddrinfo failed:%d\n", iResult);
		WSACleanup();
		return 1;
	}
	ListnSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListnSocket == INVALID_SOCKET) {
		printf("Error at socket():%d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	iResult = bind(ListnSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error:%d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListnSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);
	if (listen(ListnSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed width error:%d\n", WSAGetLastError());
		closesocket(ListnSocket);
		WSACleanup();
		return 1;
	}
	printf("listen port %s\n", DEFAULT_PORT);
	
	while (1)
	{
		SOCKET ClientSocket = INVALID_SOCKET;
		sockaddr_in clnAddr;
		int addrLen = sizeof(clnAddr);
		char addrStr[20];
		ClientSocket = accept(ListnSocket, (SOCKADDR*)&clnAddr, &addrLen);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed:%d\n", WSAGetLastError());
			closesocket(ListnSocket);
			WSACleanup();
			return 1;
		}
		inet_ntop(AF_INET, (void*)&clnAddr.sin_addr, addrStr, sizeof(addrStr));
		printf("%s login\n", addrStr);
		iSendResult = send(ClientSocket, hello, strlen(hello)+1, 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed:%d\n", WSAGetLastError());
			closesocket(ClientSocket);
			break;
		}
		do {
			//ZeroMemory(recvbuf, sizeof(recvbuf));
			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			
			if (iResult > 0) {
				printf("%s:%s\n", addrStr,recvbuf);
				printf("Bytes received:%d\n", iResult);
				iSendResult = send(ClientSocket, recvbuf, iResult+1, 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed:%d\n", WSAGetLastError());
					closesocket(ClientSocket);
					break;
				}
			}
			else if (iResult == 0)
			{
				printf("Connection closing...\n");
			}
			else {
				printf("recv failed:%d\n", WSAGetLastError());
				closesocket(ListnSocket);
				WSACleanup();
				return 1;
			}
		} while (iResult > 0);
		closesocket(ClientSocket);

	}
	
	WSACleanup();

    return 0;
}

