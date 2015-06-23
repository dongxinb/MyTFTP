// MyClient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "define.h"

int bufferSize = 512;

int mySend(SOCKET socket, char *message, int length, SOCKADDR_IN addr);
int sendACK(SOCKET socket, unsigned short index, SOCKADDR_IN addr);
int getFile(SOCKET socket, SOCKADDR_IN addr, char *filename);
int putFile(SOCKET socket, SOCKADDR_IN addr, char *filename);
int handleError(char *data, int length);

int _tmain(int argc, _TCHAR* argv1[])
{
	//char *server = "127.0.0.1";
	//char *argv[] = { "tftp", "45.116.12.104", "put", "1234.txt" };
	//argc = 4;
	

	if (argc != 4) {
		cout << "Argc Error! TYPE: \"TFTP.exe SERVER_IP OPTION(get, put) filename\"" << endl;
		return 0;
	}
	char *argv[4];
	for (int i = 0; i < argc; i++) {
		int iLength;
		iLength = WideCharToMultiByte(CP_ACP, 0, argv1[i], -1, NULL, 0, NULL, NULL);
		//将tchar值赋给_char
		argv[i] = (char *)malloc(iLength + 1);
		WideCharToMultiByte(CP_ACP, 0, argv1[i], -1, argv[i], iLength, NULL, NULL);
	}
	char *server = (char *)argv[1];

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
	SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == INVALID_SOCKET) {
		cout << "Socket() failed!" << endl;
		WSACleanup();
		return 0;
	}
	/*
	int imode = 1;
	ret = ioctlsocket(clientSocket, FIONBIO, (u_long *)&imode);
	if (ret == SOCKET_ERROR) {
		cout << "ioctlsocket() failed!" << endl;
		closesocket(clientSocket);
		WSACleanup();
		return 0;
	}
	*/

	SOCKADDR_IN addr;
	addr.sin_addr.S_un.S_addr = inet_addr(server);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(69);

	/*
	int len = sizeof(addr);
	char *data;
	data = (char *)malloc(bufferSize + 4);
	unsigned short index = 0;
	*/
	if (strcmp(argv[2], "put") == 0 || strcmp(argv[2], "PUT") == 0) {
		ret = putFile(clientSocket, addr, argv[3]);
	}
	else if (strcmp(argv[2], "get") == 0 || strcmp(argv[2], "GET") == 0) {
		ret = getFile(clientSocket, addr, argv[3]);
	}
	else {
		cout << "Unknown command!" << endl;
	}





	/*


	FILE *fp;
	if (strcmp(argv[2], "put") == 0 || strcmp(argv[2], "PUT") == 0) {
		fp = fopen(argv[3], "r");
	}
	else {
		fp = fopen(argv[3], "w");
	}
	if (fp == NULL) {
		printf("File \"%s\" error.\n", argv[3]);
		closesocket(clientSocket);
		WSACleanup();
		return 0;
	}
	while (1) {
		memset(data, 0, bufferSize + 4);
		ret = recvfrom(clientSocket, data, bufferSize + 4, 0, (SOCKADDR *)&addr, &len);
		if (ret == SOCKET_ERROR) {
			cout << "recvform() failed!" << endl;
			break;
		}
		package *op = (package *)data;
		int opCode = ntohs(op->opCode);
#ifdef _DEBUG_MODE
		cout << "------------" << endl;
		cout << "recv(): " << ret << endl;
		cout << "opCode: " << opCode << endl;
#endif
		if (opCode == TFTP_OP_DATA) {
			index = ntohs(op->code);
			fwrite(data + 4, 1, ret - 4, fp);
			int ret = sendACK(clientSocket, index, addr);
			if (ret == SOCKET_ERROR) {
				break;
			}
		}
		else if (opCode == TFTP_OP_ERR) {
			unsigned short errCode = ntohs(op->code);
			cout << "Error: " << data + 4 << endl;
			break;
		}
		else if (opCode == TFTP_OP_OACK) {

		}
		else if (opCode == TFTP_OP_ACK) {

		}

	}
	*/
	/*
	while (1) {
		char x[100] = "123";
		memset(x, 0, 100);
		int len = sizeof(addr);
		int ret = sendto(clientSocket, x, 100, 0, (SOCKADDR*)&addr, sizeof(addr));
		cout << ret << endl;
		ret = recvfrom(clientSocket, x, sizeof(x), 0, (SOCKADDR*)&addr, &len);
		cout << ret << endl;
	}
	*/

	closesocket(clientSocket);
	WSACleanup();
	return 0;
}

int getFile(SOCKET socket, SOCKADDR_IN addr, char *filename)
{
	char *mode = "octet";
	char *out = (char *)malloc(MAX_BUFFER_LENGTH);
	memset(out, 0, MAX_BUFFER_LENGTH);
	int timeout = 5;
	package *op = (package *)out;
	op->opCode = htons(TFTP_OP_READ);
	int offset = 2;
	sprintf(out + offset, "%s", filename);
	offset += strlen(filename) + 1;
	sprintf(out + offset, "%s", mode);
	offset += strlen(mode) + 1;
	sprintf(out + offset, "blksize");
	offset += strlen(out + offset) + 1;
	sprintf(out + offset, "%d", bufferSize);
	offset += strlen(out + offset) + 1;
	sprintf(out + offset, "timeout");
	offset += strlen(out + offset) + 1;
	sprintf(out + offset, "%d", timeout);
	offset += strlen(out + offset) + 1;


	//sprintf(out, "%02u%s\0%s\0timeout\0%d\0blksize\0%d\0", (unsigned int)TFTP_OP_READ, filename, mode, timeout, bufferSize);
#ifdef _DEBUG_MODE
	for (int i = 0; i < offset; i++) {
		printf("%hhu ", out[i]);
	}
#endif

	int ret = mySend(socket, out, offset, addr);
	free(out);


	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		printf("Create file \"%s\" error.\n", filename);
		return -2;
	}

	int len = sizeof(addr);
	char *data;
	data = (char *)malloc(bufferSize + 4);
	unsigned short index = 0;

	while (1) {
		memset(data, 0, bufferSize + 4);
		ret = recvfrom(socket, data, bufferSize + 4, 0, (SOCKADDR *)&addr, &len);
		if (ret == SOCKET_ERROR) {
			cout << "recvform() failed!" << endl;
			break;
		}
		package *op = (package *)data;
		int opCode = ntohs(op->opCode);
#ifdef _DEBUG_MODE
		cout << "------------" << endl;
		cout << "recv(): " << ret << endl;
		cout << "opCode: " << opCode << endl;
#endif
		if (opCode == TFTP_OP_DATA) {
			index = ntohs(op->code);
			fwrite(data + 4, 1, ret - 4, fp);
			int sendRet = sendACK(socket, index, addr);
			if (sendRet == SOCKET_ERROR) {
				break;
			}
			if (ret < bufferSize + 4) {
				cout << "Get successfully!" << endl;
				break;
			}
		}
		else if (opCode == TFTP_OP_ERR) {
			handleError(data, ret);
			break;
		}
		else {
			cout << "Unknown OPCODE!" << endl;
			break;
		}
	}
	fclose(fp);
	return 1;
}

int putFile(SOCKET socket, SOCKADDR_IN addr, char *filename)
{
	char *mode = "octet";
	char *out = (char *)malloc(MAX_BUFFER_LENGTH);
	memset(out, 0, MAX_BUFFER_LENGTH);
	int timeout = 5;
	package *op = (package *)out;
	op->opCode = htons(TFTP_OP_WRITE);
	int offset = 2;
	sprintf(out + offset, "%s", filename);
	offset += strlen(filename) + 1;
	sprintf(out + offset, "%s", mode);
	offset += strlen(mode) + 1;
	sprintf(out + offset, "blksize");
	offset += strlen(out + offset) + 1;
	sprintf(out + offset, "%d", bufferSize);
	offset += strlen(out + offset) + 1;
	sprintf(out + offset, "timeout");
	offset += strlen(out + offset) + 1;
	sprintf(out + offset, "%d", timeout);
	offset += strlen(out + offset) + 1;


	//sprintf(out, "%02u%s\0%s\0timeout\0%d\0blksize\0%d\0", (unsigned int)TFTP_OP_READ, filename, mode, timeout, bufferSize);
#ifdef _DEBUG_MODE
	for (int i = 0; i < offset; i++) {
		printf("%hhu ", out[i]);
	}
#endif

	int ret = mySend(socket, out, offset, addr);
	free(out);


	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Read file \"%s\" error.\n", filename);
		return -2;
	}

	int len = sizeof(addr);
	char *data;
	data = (char *)malloc(bufferSize + 4);
	unsigned short index = 0;

	while (1) {
		memset(data, 0, bufferSize + 4);
		ret = recvfrom(socket, data, bufferSize + 4, 0, (SOCKADDR *)&addr, &len);
		if (ret == SOCKET_ERROR) {
			cout << "recvform() failed!" << endl;
			break;
		}
		package *op = (package *)data;
		int opCode = ntohs(op->opCode);
#ifdef _DEBUG_MODE
		cout << "------------" << endl;
		cout << "recv(): " << ret << endl;
		cout << "opCode: " << opCode << endl;
#endif
		if (opCode == TFTP_OP_ACK) {
			index = ntohs(op->code);
			fseek(fp, (int)index * bufferSize, 0);

			op->opCode = htons(TFTP_OP_DATA);
			op->code = htons(index + 1);
			ret = fread(data + 4, 1, bufferSize, fp) + 4;
			if (ret == 4) {
				cout << "Sent successfully!" << endl;
				break;
			}
			int sendRet = mySend(socket, data, ret, addr);
			if (sendRet == SOCKET_ERROR) {
				break;
			}
		}
		else if (opCode == TFTP_OP_ERR) {
			handleError(data, ret);
			break;
		}
		else {
			cout << "Unknown OPCODE!" << endl;
			break;
		}
	}
	fclose(fp);
	return 1;
}

int mySend(SOCKET socket, char *message, int length, SOCKADDR_IN addr)
{
	int ret = sendto(socket, message, length, 0, (SOCKADDR *)&addr, sizeof(addr));
	if (ret == SOCKET_ERROR) {
		cout << "sendto() failed!" << endl;
		return SOCKET_ERROR;
	}
#ifdef _DEBUG_MODE
	cout << "sendto(): " << ret << endl;
#endif
	return ret;
}

int sendACK(SOCKET socket, unsigned short index, SOCKADDR_IN addr)
{
	char x[4];
	package *pac = (package *)x;
	pac->opCode = htons(TFTP_OP_ACK);
	pac->code = htons(index);
	int ret = mySend(socket, x, 4, addr);
	if (ret == SOCKET_ERROR) {
		cout << "ack error!" << endl;
	}
	return ret;
}

int handleError(char *data, int length)
{
	package *op = (package *)data;
	unsigned short errCode = ntohs(op->code);
	cout << "Error: " << data + 4 << endl;
	return 1;
}