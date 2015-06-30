#pragma once
#include "stdafx.h"
#include "define.h"

class ServerItem
{
public:
	SOCKET *socket;
	SOCKADDR_IN addr;
	char mode[100];
	char *filename;
	int blksize;
	int packageIndex;
	FILE *fp;
	BOOL finished;
	BOOL fileError;
	ServerItem(SOCKET *socket, SOCKADDR_IN addr, char *message, int length);
	int sendErr(TFTP_ERROR_CODE errorcode);
	int handleAck(unsigned short index);
	int handleData(char *message, int length);
	~ServerItem();

private:
	int mySend(char *message, int length);
	int sendACK(unsigned short index);
	void convert(char *l);
	int sendPackage(int index);
	int logMessage(char *message);
};

