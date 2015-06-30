#include "ServerItem.h"
#include <time.h>

int ServerItem::logMessage(char *message)
{
	FILE *logFile = fopen("ServerLog.txt", "a");
	char *wday[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);
	fprintf(logFile, "%d/%d/%d ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
	fprintf(logFile, "%s %d:%d:%d\t", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
	fprintf(logFile, "%s\n", message);
	fclose(logFile);
	return 1;
}

ServerItem::ServerItem(SOCKET *socket, SOCKADDR_IN addr, char *message, int length)
{
	fileError = false;
	blksize = 512;
	finished = false;
	this->socket = socket;
	this->addr = addr;
	package *pac = (package *)message;
	unsigned short op = ntohs(pac->opCode);
	int i = 2;
	
	for (i = 2; i < length; i++) {
		if (message[i] == 0) {
			break;
		}
	}
	filename = (char *)malloc(i - 1);
	strcpy(filename, message + 2);
	strcpy(mode, message + 2 + strlen(filename) + 1);
	int index = 2 + strlen(filename) + 1 + strlen(mode) + 1;
	while (index < length) {
		char *x = (char *)malloc(strlen(message + index) + 1);
		index += strlen(x) + 1;
		char *y = (char *)malloc(strlen(message + index) + 1);
		index += strlen(y) + 1;
		if (strcmp(x, "blksize") == 0) {
			sscanf(y, "%d", &blksize);
		}
	}
	if (op == TFTP_OP_READ || op == TFTP_OP_WRITE) {
		string x = string();
		x.append(inet_ntoa(addr.sin_addr));
		x.append(" Client connected");
		logMessage((char *)x.c_str());
	}

	if (op == TFTP_OP_READ) {
		fp = fopen(filename, "r");
		int err = errno;
		if (fp == NULL) {
			if (err == EACCES) {
				sendErr(TFTP_ERR_ACCESS_DENIED);
			}
			else if (err == ENOENT) {
				sendErr(TFTP_ERR_FILE_NOT_FOUND);
			}
			else if (err == EEXIST) {
				sendErr(TFTP_ERR_FILE_ALREADY_EXISTS);
			}
			else {
				sendErr(TFTP_ERR_UNDEFINED);
			}
			fileError = true;
		}
		else {
			sendPackage(1);
		}
	}
	else if (op == TFTP_OP_WRITE) {
		fp = fopen(filename, "w");
		int err = errno;
		if (fp == NULL) {
			if (err == EACCES) {
				sendErr(TFTP_ERR_ACCESS_DENIED);
			}
			else if (err == ENOENT) {
				sendErr(TFTP_ERR_FILE_NOT_FOUND);
			}
			else if (err == EEXIST) {
				sendErr(TFTP_ERR_FILE_ALREADY_EXISTS);
			}
			else {
				sendErr(TFTP_ERR_UNDEFINED);
			}
			fileError = true;
		}
		else {
			sendACK(0);
			packageIndex = 0;
		}
	}
	else {
		sendErr(TFTP_ERR_UNEXPECTED_OPCODE);
	}
}


ServerItem::~ServerItem()
{
	if (fp != NULL) {
		fclose(fp);
	}
	string x = string();
	x.append(inet_ntoa(addr.sin_addr));
	x.append(" Client closed");
	logMessage((char *)x.c_str());

	free(filename);
}

int ServerItem::handleAck(unsigned short index) 
{
	if (finished && index == packageIndex) {
		return 1;
	}
	int ret = sendPackage((int)index + 1);
	return ret;
}

int ServerItem::handleData(char *message, int length)
{
	package *pac = (package *)message;
	int index = (int)ntohs(pac->code);
	if (packageIndex + 1 == index) {
		fwrite(message + 4, 1, length - 4, fp);
	}
	else {

	}
	packageIndex = index;
	int ret = sendACK((unsigned short)index);
	if (length - 4 < blksize) {
		return 1;
	}
	else if (ret < 0) {
		return ret;
	}
	else {
		return 0;
	}
}

int ServerItem::sendPackage(int index)
{
	if (fp == NULL) {
		return -2;
	}
	char *data = (char *)malloc(blksize + 4);
	if (index != packageIndex + 1)
		fseek(fp, (int)(index - 1) * blksize, 0);
	package *op = (package *)data;
	op->opCode = htons(TFTP_OP_DATA);
	op->code = htons((unsigned short)index);
	int ret = fread(data + 4, 1, blksize, fp);
	if (ret < 0) {
		int ret = sendErr(TFTP_ERR_UNDEFINED);
	}
	else {
		if (ret < blksize) {
			finished = true;
		}
		int rett = mySend(data, ret + 4);
		if (rett < 0) {
			finished = false;
		}
		else {
			if (finished) {
				packageIndex = index;

			}
		}
		string x = string();
		x.append(inet_ntoa(addr.sin_addr));
		x.append(" Package sent: \t");
		char aa[5];
		itoa(rett, aa, 10);
		x.append(aa);
		logMessage((char *)x.c_str());

		packageIndex = index;
	}
	return 0;
}

int ServerItem::mySend(char *message, int length)
{
	int ret = sendto(*socket, message, length, 0, (SOCKADDR *)&addr, sizeof(addr));
	if (ret == SOCKET_ERROR) {
		cout << "sendto() failed!" << endl;
		return SOCKET_ERROR;
	}
#ifdef _DEBUG_MODE
	cout << "sendto(): " << ret << endl;
#endif
	return ret;
}


int ServerItem::sendACK(unsigned short index)
{
	char x[4];
	package *pac = (package *)x;
	pac->opCode = htons(TFTP_OP_ACK);
	pac->code = htons(index);
	int ret = mySend(x, 4);
	if (ret == SOCKET_ERROR) {
		cout << "ack error!" << endl;
	}

	string xx = string();
	xx.append(inet_ntoa(this->addr.sin_addr));
	xx.append(" ACK sent");
	logMessage((char *)xx.c_str());

	return ret;
}

int ServerItem::sendErr(TFTP_ERROR_CODE errorcode)
{
	char x[100];
	package *pac = (package *)x;
	pac->opCode = htons(TFTP_OP_ERR);
	pac->code = htons(errorcode);
	switch (errorcode)
	{
	case TFTP_ERR_UNDEFINED:
		strcpy(x + 4, "tftp error undefined!");
		break;
	case TFTP_ERR_FILE_NOT_FOUND:
		strcpy(x + 4, "tftp error file not found!");
		break;
	case TFTP_ERR_ACCESS_DENIED:
		strcpy(x + 4, "tftp error access denied!");
		break;
	case TFTP_ERR_DISK_FULL:
		strcpy(x + 4, "tftp error disk full!");
		break;
	case TFTP_ERR_UNEXPECTED_OPCODE:
		strcpy(x + 4, "tftp error unexpected opcode!");
		break;
	case TFTP_ERR_UNKNOWN_TRANSFER_ID:
		strcpy(x + 4, "tftp error unknown transfer id!");
		break;
	default:
		break;
	}
	convert(x + 4);
	int ret = mySend(x, strlen(x + 4) + 5);
	if (ret == SOCKET_ERROR) {
		cout << "sendErr() failed!" << endl;
	}

	string xx = string();
	xx.append(inet_ntoa(this->addr.sin_addr));
	xx.append(" Error sent: \t");
	xx.append(x + 4);
	logMessage((char *)xx.c_str());

#ifdef _DEBUG_MODE 
	cout << "Err: " << inet_ntoa(addr.sin_addr) << " " <<  x + 4 << endl;
#endif
	return ret;
}

void ServerItem::convert(char *l)
{
	if (strcmp(mode, "netascii") != 0)
		return;
	while (*l != 0) {
		*l = toascii(*l);
		l++;
	}
}

