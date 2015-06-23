// MyServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "define.h"
#include "ServerItem.h"
#include <vector>

int _tmain(int argc, _TCHAR* argv1[])
{
	vector<ServerItem *>servers;

	int port = 69;
	WORD myVersionRequest;
	WSADATA wsaData;
	myVersionRequest = MAKEWORD(1, 1);
	int ret;
	ret = WSAStartup(myVersionRequest, &wsaData);
	if (ret != 0) {
		cout << "Socket Opened Error!" << endl;
		return 0;
	}
	SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket == INVALID_SOCKET) {
		cout << "Socket() failed!" << endl;
		WSACleanup();
		return 0;
	}
	
	int imode = 1;
	ret = ioctlsocket(serverSocket, FIONBIO, (u_long *)&imode);
	if (ret == SOCKET_ERROR) {
		cout << "ioctlsocket() failed!" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	

	SOCKADDR_IN addr;
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);

	ret = bind(serverSocket, (SOCKADDR *)&addr, sizeof(addr));
	if (ret == SOCKET_ERROR) {
		cout << "Bind() failed!" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}

	cout << "This is my socket server!" << endl;

	char *data = (char *)malloc(MAX_BUFFER_LENGTH);

	struct fd_set fds;
	struct timeval timeout = { 0, 0 };
	while (1) {
		FD_ZERO(&fds); //每次循环都要清空集合，否则不能检测描述符变化 
		FD_SET(serverSocket, &fds); //添加描述符 
		switch (select(serverSocket + 1, &fds, &fds, NULL, &timeout)) {
		case -1:
			cout << "Select() Error!" << endl;
			return 0;
			break;
		case 0:
			break;
		default:
			if (FD_ISSET(serverSocket, &fds)) {
				SOCKADDR_IN cAddr;
				int len = sizeof(cAddr);
				ret = recvfrom(serverSocket, data, MAX_BUFFER_LENGTH, 0, (SOCKADDR *)&cAddr, &len);

				if (ret > 0) {
					vector<ServerItem *>::iterator it;
					for (it = servers.begin(); it != servers.end(); it++) {
						if (((*it)->addr.sin_addr.S_un.S_addr == cAddr.sin_addr.S_un.S_addr) && (*it)->addr.sin_port == cAddr.sin_port) {
							break;
						}
					}
					ServerItem *cserver;
					if (it != servers.end()) {
						cserver = *it;
						package *pac = (package *)data;
						unsigned short opCode = ntohs(pac->opCode);
						if (opCode == TFTP_OP_READ || opCode == TFTP_OP_WRITE) {
							servers.erase(it);
							delete(cserver);

							cout << "[RENew] " << inet_ntoa(cAddr.sin_addr) << " connected." << endl;
							cserver = new ServerItem(&serverSocket, cAddr, data, ret);
							if (cserver->fileError) {
								delete(cserver);
							}
							else {
								servers.push_back(cserver);
							}
						}
						else if (opCode == TFTP_OP_ACK) {
							
							unsigned short index = ntohs(pac->code);
							cout << inet_ntoa(cAddr.sin_addr) << ": ACK " << index << endl;
							int ret = cserver->handleAck(index);
							if (ret == 1) {
								cout << inet_ntoa(cAddr.sin_addr) << ": " << cserver->filename << "sent successfully!" << endl;
								servers.erase(it);
								delete(cserver);
							}
						}
						else if (opCode == TFTP_OP_DATA) {
							unsigned short index = ntohs(pac->code);
							cout << inet_ntoa(cAddr.sin_addr) << ": DATA " << index << endl;
							int out = cserver->handleData(data, ret);
							if (out == 1) {
								cout << inet_ntoa(cAddr.sin_addr) << ": " << cserver->filename << "save successfully!" << endl;
								servers.erase(it);
								delete(cserver);
							}
						}
						else {
							cout << inet_ntoa(cAddr.sin_addr) << ": Error " << TFTP_ERR_UNEXPECTED_OPCODE << endl;
							cserver->sendErr(TFTP_ERR_UNEXPECTED_OPCODE);
						}
					}
					else {
						package *pac = (package *)data;
						if (ntohs(pac->opCode) == TFTP_OP_READ || ntohs(pac->opCode) == TFTP_OP_WRITE) {
							cout << "[New] " << inet_ntoa(cAddr.sin_addr) << ": Connected." << endl;
							cserver = new ServerItem(&serverSocket, cAddr, data, ret);
							if (cserver->fileError) {
								delete(cserver);
							}
							else {
								servers.push_back(cserver);
							}
							
						}
					}
				}
				memset(data, 0, MAX_BUFFER_LENGTH);
				ret = 0;
			}
			else {

			}
		}

	}


	return 0;
}

