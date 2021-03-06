// netSocket.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <thread>
#include <vector>
//#include <windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment (lib,"ws2_32.lib")

#define BUF_SIZE 1024
#define DEFAULT_PORT "27015"
#define FILE_NAME_MAX_SIZE 512

struct FILESEND {
	long id = 0;							//文件包编号
	float size = 0;							//文件大小
	int end = 0;							//文件结束标志
	char name[FILE_NAME_MAX_SIZE];			//文件名
	char content[BUF_SIZE];					//文件包缓冲区
	char addrStr[20];						//客户端ip
	SOCKET  ClientSocket = INVALID_SOCKET;	//客户端连接
};

//线程处理新的连接，复制新的fileS副本
void connectClient(FILESEND fileS) {
	
	//FILESEND fileS;
	char recvbuf[BUF_SIZE];
	char fileName[FILE_NAME_MAX_SIZE];
	char *hello = "welcome!";
	int iResult, iSendResult;
	int recvbuflen = BUF_SIZE;

	
	printf("%s login\n", fileS.addrStr);
	//发送欢迎
	iSendResult = send(fileS.ClientSocket, hello, strlen(hello) + 1, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed:%d\n", WSAGetLastError());
		closesocket(fileS.ClientSocket);
		return;
	}
	//接收文件名
	iResult = recv(fileS.ClientSocket, recvbuf, recvbuflen, 0);

	if (iResult > 0) {
		printf("%s download %s\n", fileS.addrStr, recvbuf);
		strncpy_s(fileName, recvbuf, strlen(recvbuf) > FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE : strlen(recvbuf));

		//打开文件
		FILE *fp;
		fopen_s(&fp, fileName, "rb");
		float fileSize = 0;
		int sendLen = 0;
		//char size[BUF_SIZE];

		if (fp == NULL) {
			printf("File: %s not found\n", fileName);
			closesocket(fileS.ClientSocket);
			return;
		}

		//ZeroMemory(sendbuf, sizeof(sendbuf));

		//计算文件大小
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);
		fileSize = fileSize / sizeof(char);					//byte
															//printf("size of char:%d\n", sizeof(char));
															//fileSize = fileSize / 1024;						//KB

															//重置文件指针位置
		fseek(fp, 0, SEEK_SET);
		strcpy_s(fileS.name, fileName);
		fileS.size = fileSize;
		//sprintf_s(size, "%f", fileSize);
		//strcpy_s(sendbuf, fileName);
		//strcat_s(sendbuf, ";");
		//strcat_s(sendbuf, size);

		//发送文件名和大小
		iSendResult = send(fileS.ClientSocket, (char*)&fileS, sizeof(fileS), 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed:%d\n", WSAGetLastError());
			closesocket(fileS.ClientSocket);
			return;
		}
		//接收文件id
		iResult = recv(fileS.ClientSocket, (char*)&fileS, sizeof(fileS), 0);
		if (iResult > 0) {
			printf("ID:%ld\n", fileS.id);
			//移动文件指针准备重传
			if (!fileS.end&&fileS.id > 0) {
				fseek(fp, BUF_SIZE*fileS.id, SEEK_CUR);
			}
		}
		else if (iResult == 0)
		{
			printf("Connection closing...\n");
			closesocket(fileS.ClientSocket);
			return;
		}
		else {
			printf("recv failed:%d\n", WSAGetLastError());
			closesocket(fileS.ClientSocket);
			return;
		}
		//FILE *fp2;
		//fopen_s(&fp2, "9.jpg", "wb");
		//ZeroMemory(sendbuf, sizeof(sendbuf));
		ZeroMemory(fileS.content, BUF_SIZE);
		//printf("file point:%d\n", ftell(fp));

		//发送文件
		while ((sendLen = fread(fileS.content, sizeof(char), BUF_SIZE, fp)) > 0) {
			//判断文件是否结束
			if (feof(fp))fileS.end = 1;
			//发送文件包
			iSendResult = send(fileS.ClientSocket, (char*)&fileS, sizeof(fileS), 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send file %s failed:%d\n", fileName, WSAGetLastError());
				closesocket(fileS.ClientSocket);
				break;
			}
			printf("send %d;ID:%ld\n", sendLen, fileS.id);
			fileS.id++;
			//fwrite(sendbuf, sizeof(char), sendLen, fp2);
			//printf("read error:%d", ferror(fp));
			//printf("file point:%d\n", ftell(fp));
			ZeroMemory(fileS.content, BUF_SIZE);

		}
		fclose(fp);
		//fclose(fp2);
		if (fileS.end) {
			printf("file %s download finished\n", fileName);
		}
		else
			printf("file %s download interrupted\n", fileName);

	}
	else if (iResult == 0)
		printf("Connection closing...\n");
	else {
		printf("recv failed:%d\n", WSAGetLastError());
	}
	closesocket(fileS.ClientSocket);
}


int main()
{
	WSADATA wsaData;
	struct addrinfo *result = NULL, 
		*ptr = NULL, 
		hints;
	SOCKET ListnSocket = INVALID_SOCKET;
	FILESEND fileS;
	int iResult;
	int recvbuflen = BUF_SIZE;
	sockaddr_in clnAddr;
	int addrLen = sizeof(clnAddr);

	//初始化WSA
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult) {
		printf("WSAStartup failed:%d\n", iResult);
		return 1;
	}
	//协议信息
	ZeroMemory(&hints, sizeof(hints));
	//ipv4
	hints.ai_family = AF_INET;
	//TCP
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult) {
		printf("getaddrinfo failed:%d\n", iResult);
		WSACleanup();
		return 1;
	}
	//初始化服务器socket
	ListnSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListnSocket == INVALID_SOCKET) {
		printf("Error at socket():%d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	//绑定ip端口
	iResult = bind(ListnSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error:%d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListnSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);
	//监听
	if (listen(ListnSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed width error:%d\n", WSAGetLastError());
		closesocket(ListnSocket);
		WSACleanup();
		return 1;
	}
	printf("listen port %s\n", DEFAULT_PORT);
	
	while (1)
	{
		ZeroMemory((char*)&fileS, sizeof(fileS));


		//初始化连接socket
		fileS.ClientSocket = accept(ListnSocket, (SOCKADDR*)&clnAddr, &addrLen);
		if (fileS.ClientSocket == INVALID_SOCKET) {
			printf("accept failed:%d\n", WSAGetLastError());
			closesocket(ListnSocket);
			WSACleanup();
			return 1;
		}
		//获取客户端ip
		inet_ntop(AF_INET, (void*)&clnAddr.sin_addr, fileS.addrStr, sizeof(fileS.addrStr));
		//std::vector<std::thread> clientThread;
		//connectClient(ListnSocket);
		std::thread clients(connectClient, fileS);
		clients.detach();
	}
	
	WSACleanup();

    return 0;
}


