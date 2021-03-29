#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H
#include "stdinc.h"
VERB str2verb(const char* str);
VERB parseCmd(const char* message, char* param);
int handleInvalidCmd(ClientSession* session);
int handleInvalidParam(ClientSession* session);
int handleCmd(ClientSession* session, VERB verb, const char* param);
int handleCmdABOR(ClientSession* session, const char* param);
int handleCmdPASS(ClientSession* session, const char* param);
int handleCmdPASV(ClientSession* session, const char* param);
int handleCmdPORT(ClientSession* session, const char* param);
int handleCmdQUIT(ClientSession* session, const char* param);
int handleCmdRETR_STOR(ClientSession* session, const char* param, const VERB verb);
int handleCmdSYST(ClientSession* session, const char* param);
int handleCmdTYPE(ClientSession* session, const char* param);
int handleCmdUSER(ClientSession* session, const char* param);
int handleCmdMKD(ClientSession* session, const char* param);
int handleCmdRMD(ClientSession* session, const char* param);
int handleCmdCWD(ClientSession* session, const char* param);
int handleCmdPWD(ClientSession* session, const char* param);
int handleCmdRNFR(ClientSession* session, const char* param);
int handleCmdRNTO(ClientSession* session, const char* param);
int handleCmdLIST(ClientSession* session, const char* param);
int handleCmdAPPE(ClientSession* session, const char* param);
int handleCmdREST(ClientSession* session, const char* param);
int parseIpPort(const char* param, char* ip_str);
int produceIpPort(char* dest, const char* ip_str, const int port);
#endif