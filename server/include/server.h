#ifndef SERVER_H
#define SERVER_H
#include "stdinc.h"

int recvMsg(const int fd, char* buffer, const int buffer_len);
int sendMsg(const int fd, const char* buffer);
int sendRecvData(ClientSession* session, const char* filename, const DATA_SEND_RECV mode);

#endif