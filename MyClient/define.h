#pragma once

#ifndef _DEFINE_H
#define _DEFINE_H

typedef struct {
	unsigned short opCode;
	unsigned short code;
}package;

enum OPCODE
{
	TFTP_OP_READ = 1,
	TFTP_OP_WRITE = 2,
	TFTP_OP_DATA = 3,
	TFTP_OP_ACK = 4,
	TFTP_OP_ERR = 5,
	TFTP_OP_OACK = 6
};

enum {
	TFTP_ERR_UNDEFINED = 0,
	TFTP_ERR_FILE_NOT_FOUND = 1,
	TFTP_ERR_ACCESS_DENIED = 2,
	TFTP_ERR_DISK_FULL = 3,
	TFTP_ERR_UNEXPECTED_OPCODE = 4,
	TFTP_ERR_UNKNOWN_TRANSFER_ID = 5,
	TFTP_ERR_FILE_ALREADY_EXISTS = 6,
};

#define MAX_BUFFER_LENGTH 10000

#define _DEBUG_MODE

#define SERVER_PORT 69

#endif