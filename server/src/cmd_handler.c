#include "../include/cmd_handler.h"
#include "../include/server.h"

VERB parseCmd(const char* message, char* param) {
	if (!message) {
        return INVALID;
    } 
    if (strlen(message) < 2) {
        return INVALID;
    }

    char* _param;
    char* _message = (char*)malloc(sizeof(char) * strlen(message));
    strcpy(_message, message);
	char* token = strtok_r(_message, " ", &_param);
    strcpy(param, _param);
    VERB verb = str2verb(token);
    
    free(_message);
	return verb;
}
int parseIpPort(const char* param, char* ip_str) {
	assert(param); // use assert cuz it shouldn't happen if the code is right
    assert(ip_str);
	char* _param = (char*)malloc(sizeof(char) * strlen(param));
	strcpy(_param, param);
	
    // parse port
	char* last_comma_pos = strrchr(_param, ',');
    if (!last_comma_pos) {
        return 0;
    }
    int _p1, _p2;
	if ((_p1 = atoi(last_comma_pos + 1)) == 0) {
        if (*(last_comma_pos+1) != '0') {
            return 0;
        }
    }
	*last_comma_pos = '\0';
	last_comma_pos = strrchr(_param, ',');
    if (!last_comma_pos) {
        return 0;
    }
    if ((_p2 = atoi(last_comma_pos + 1)) == 0) {
        if (*(last_comma_pos+1) != '0') {
            return 0;
        }
    }
    *last_comma_pos = '\0';
	
    int port = _p1 + 256 * _p2;
    // parse ip
	char* curr_p = _param;
	while(*curr_p != '\0') {
		if (*curr_p == ',') {
			*curr_p = '.';
		}
		curr_p++;
	}
	strcpy(ip_str, _param);
	free(_param);
	return port;
}
int produceIpPort(char* dest, const char* ip_str, const int port) {
    assert(dest);
	assert(ip_str);
    assert(port > 0);

	strcpy(dest, ip_str);
	int len = strlen(dest);
	dest[len++] = ',';
	sprintf(dest+len, "%d", port/256);
	len = strlen(dest);
	dest[len++] = ',';
	sprintf(dest+len, "%d", port%256);
	char* p = dest;
	while(*p != '\0') {
		if (*p == '.') {
			*p = ',';
		}
		p++;
	}
	return 1;
}
int checkAuth(ClientSession* session) {
    if (session->auth == UNAUTH) {
        sendMsg(session->connfd, UNAUTH_MESSAGE);
        return 0;
    } 
    else if (session->auth == ONLY_USERNAME) {
        sendMsg(session->connfd, _331_NEED_PASSWD_MESSAGE);
        return 0;
    }
    else {
        return 1;
    }
}
int handleCmd(ClientSession* session, VERB verb, const char* param) {
	int status;
	switch (verb) {
		case INVALID:
			printf("handleCmd(): INVALID CMD\n");
            status = handleInvalidCmd(session);
			break;
		case ABOR:
			status = handleCmdABOR(session, param);
			break;
		case PASS:
			status = handleCmdPASS(session, param);
			break;
		case PASV:
			status = handleCmdPASV(session, param);
			break;
		case PORT:
			status = handleCmdPORT(session, param);
			break;
		case QUIT:
			status = handleCmdQUIT(session, param);
			break;
		case RETR:
			status = handleCmdRETR_STOR(session, param, RETR);
			break;
		case STOR:
			status = handleCmdRETR_STOR(session, param, STOR);
			break;
		case SYST:
			status = handleCmdSYST(session, param);
			break;
		case TYPE:
			status = handleCmdTYPE(session, param);
			break;
		case USER:
			status = handleCmdUSER(session, param);
			break;
        case PWD:
			status = handleCmdPWD(session, param);
			break;
        case MKD:
            status = handleCmdMKD(session, param);
            break;
        case RMD:
			status = handleCmdRMD(session, param);
			break;
        case RNFR:
			status = handleCmdRNFR(session, param);
			break;
        case RNTO:
			status = handleCmdRNTO(session, param);
			break;
        case CWD:
			status = handleCmdCWD(session, param);
			break;
        case LIST:
			status = handleCmdLIST(session, param);
			break;
        case APPE: 
            status = handleCmdAPPE(session, param);
            break;
        case REST: 
            status = handleCmdREST(session, param);
            break;
    }
	return status;
}
int handleInvalidCmd(ClientSession* session) {
    return sendMsg(session->connfd, UNKNOWN_COMMAND_MESSAGE);
}
int handleInvalidParam(ClientSession* session) {
    return sendMsg(session->connfd, INVALID_PARAM_MESSAGE);
}
int handleCmdABOR(ClientSession* session, const char* param) {
    printf("handling ABOR...\n");
    return handleCmdQUIT(session, param);
}
int handleCmdAPPE(ClientSession* session, const char* param) {
    printf("handling APPE...\n");
    if (!checkAuth(session)) {
        return 0;
    }
    session->file_pos = atoi(param);
    char response[BUFF_SIZE];
    sprintf(response, "350 Appending at byte %d.\r\n", session->file_pos);
	return sendMsg(session->connfd, response);
}
int handleCmdREST(ClientSession* session, const char* param) {
    printf("handling APPE...\n");
    if (!checkAuth(session)) {
        return 0;
    }
    session->file_pos = atoi(param);
    char response[BUFF_SIZE];
    sprintf(response, "350 Restarting at byte %d.\r\n", session->file_pos);
	return sendMsg(session->connfd, response);
}
int handleCmdPASS(ClientSession* session, const char* param) {
	assert(param);
    if (session->auth == AUTH) {
        return sendMsg(session->connfd, LOGGED_IN_MESSAGE);
    }
    else if (session->auth == UNAUTH) {
        return sendMsg(session->connfd, NEED_USERNAME_MESSAGE);
    }
	strncpy(session->passwd, param, PASSWD_MAX_LENGTH);
    session->auth = AUTH;
    return sendMsg(session->connfd, LOG_SUCCESS_MESSAGE);
}
int handleCmdPASV(ClientSession* session, const char* param) {
    printf("handling PASV...\n");
    if (!checkAuth(session)) {
        return 0;
    }
    session->transfer_mode = TRANS_PASV;

    if ((session->pasv_listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        sendMsg(session->connfd, LOCAL_ERROR_MESSAGE);
        PrintError();
        return -1;
    }
    bzero(&session->trans_addr, sizeof(struct sockaddr_in));
    session->trans_addr.sin_family = AF_INET;
    session->trans_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    session->trans_addr.sin_port = htons(0);
    
    if (bind(session->pasv_listenfd, (struct sockaddr*)&session->trans_addr, sizeof(struct sockaddr)) < 0) {
        sendMsg(session->connfd, DATA_CONNECTION_OPEN_FAILED);
        PrintError();
        return -1;
    }

    if (listen(session->pasv_listenfd, 1) < -1) {
        sendMsg(session->connfd, DATA_CONNECTION_OPEN_FAILED);
        PrintError();
        return -1;
    }

    // get actual port
    socklen_t socklen = sizeof(struct sockaddr);
	if (getsockname(session->pasv_listenfd, (struct sockaddr *)&session->trans_addr, &socklen) < 0) {
        sendMsg(session->connfd, LOCAL_ERROR_MESSAGE);
        PrintError();
        return -1;
    }
	int port = ntohs(session->trans_addr.sin_port);

    // produce ip_port format
    char ip_port[HOST_NAME_MAX_LENGTH + 10];
    produceIpPort(ip_port, serv.hostname, port);
    char response[BUFF_SIZE];
    sprintf(response, PASV_SUCCESS_MESSAGE, ip_port);
    return sendMsg(session->connfd, response);
}
int handleCmdPORT(ClientSession* session, const char* param) {
    printf("handling PORT...\n");
    assert(param);
    if (!checkAuth(session)) {
        return 0;
    }
    if (session->transfer_mode == TRANS_PASV) {
        close(session->pasv_listenfd); 
    }
    session->transfer_mode = TRANS_PORT;

    char ip_str[BUFF_SIZE];
	int port;
    if ((port = parseIpPort(param, ip_str)) < 0) {
        return handleInvalidParam(session);
    }
    bzero(&session->trans_addr, sizeof(struct sockaddr_in));
    session->trans_addr.sin_family = AF_INET;
    session->trans_addr.sin_port = htons(port);
    int _status = inet_pton(AF_INET, ip_str, &session->trans_addr.sin_addr);
    if (_status < 0) {
        sendMsg(session->connfd, LOCAL_ERROR_MESSAGE);
        PrintError();
        return -1;
    }
    else if (_status == 0) {
        return handleInvalidParam(session);
    }
    return sendMsg(session->connfd, PORT_SUCCESS_MESSAGE);
}
int handleCmdQUIT(ClientSession* session, const char* param) {
    printf("handling QUIT...\n");
    checkAuth(session);
	sendMsg(session->connfd, GOODBYE_MESSAGE);
    return -1;
}
int handleCmdRETR_STOR(ClientSession* session, const char* param, const VERB verb) {
    printf("handling RETR_STOR...\n");
    if (!checkAuth(session)) {
        return 0;
    }
    char absolute_path[PATH_MAX_LENGTH];
    if (!produceAbsPath(absolute_path, PATH_MAX_LENGTH, session->pwd, param)) {
        return sendMsg(session->connfd, _553_INVALID_PATH_MESSAGE);
    }
    int status;
    if (verb == RETR) {
        status = sendRecvData(session, absolute_path, SEND);
    }
    else if (verb == STOR) {
        status = sendRecvData(session, absolute_path, RECV);
    }
    return status;
}
int handleCmdSYST(ClientSession* session, const char* param) {
    printf("handling SYST...\n");
    if (!checkAuth(session)) {
        return 0;
    }
	return sendMsg(session->connfd, SYST_RESPONSE);
}
int handleCmdTYPE(ClientSession* session, const char* param) {
    printf("handling TYPE...\n");
    if (!checkAuth(session)) {
        return 0;
    }
    if (strlen(param) != 1 && param[0] != 'I') {
        return handleInvalidParam(session);
    }
	return sendMsg(session->connfd, TYPE_RESPONSE);
}
int handleCmdUSER(ClientSession* session, const char* param) {
    printf("handling USER...\n");
	assert(param);
    if (session->auth == AUTH) {
        return sendMsg(session->connfd, LOGGED_IN_MESSAGE);
    }
    if (strncasecmp(param, "anonymous", 9)) {
        return sendMsg(session->connfd, _530_UNSOPPORTED_USERNAME);
    }
	strncpy(session->username, param, USERNAME_MAX_LENGTH);
    session->auth = ONLY_USERNAME;
    return sendMsg(session->connfd, _331_NEED_PASSWD_MESSAGE);
}
int handleCmdMKD(ClientSession* session, const char* param) {
    printf("handling MKD...\n");
	assert(param);
    if (!checkAuth(session)) {
        return 0;
    }
    char response[BUFF_SIZE];
    char absolute_path[PATH_MAX_LENGTH];
    if (produceAbsPath(absolute_path, PATH_MAX_LENGTH, session->pwd, param)) {
        // check path ok
        printf("Path: %s\n", absolute_path);
        if (mkdir(absolute_path, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
            return sendMsg(session->connfd, _550_FILE_DIR_DO_NOT_EXIST);
        }
        else {
            return sendMsg(session->connfd, _257_MAKE_DIR_SUCCEED);
        }
    }
    else {
        return sendMsg(session->connfd, _553_INVALID_PATH_MESSAGE);
    }
	return sendMsg(session->connfd, response);
}
int handleCmdRMD(ClientSession* session, const char* param) {
    printf("handling RMD...\n");
	assert(param);
    if (!checkAuth(session)) {
        return 0;
    }
    char absolute_path[PATH_MAX_LENGTH];
    if (produceAbsPath(absolute_path, PATH_MAX_LENGTH, session->pwd, param)) {
        // check path ok
        printf("Path: %s\n", absolute_path);
        struct stat _stat = {0};
        stat(absolute_path, &_stat);
        if (!S_ISDIR(_stat.st_mode)) {
            return sendMsg(session->connfd, _550_FILE_DIR_DO_NOT_EXIST);
        }
        else {
            char cmd[PATH_MAX_LENGTH+10];
            sprintf(cmd, "rm -rf %s", absolute_path);
            FILE* f = popen(cmd, "r");
            fclose(f);
            return sendMsg(session->connfd, _250_REMOVE_DIR_SUCCEED); 
        }
    }
    else {
        return sendMsg(session->connfd, _553_INVALID_PATH_MESSAGE);
    }
}
int handleCmdCWD(ClientSession* session, const char* param) {
    printf("handling CWD...\n");
	assert(param);
    if (!checkAuth(session)) {
        return 0;
    }
    char absolute_path[PATH_MAX_LENGTH];
    if (produceAbsPath(absolute_path, PATH_MAX_LENGTH, session->pwd, param)) {
        // check path ok
        printf("Path: %s\n", absolute_path);
        struct stat _stat = {0};
        stat(absolute_path, &_stat);
        if (!S_ISDIR(_stat.st_mode)) {
            return sendMsg(session->connfd, _550_FILE_DIR_DO_NOT_EXIST);
        }
        else {
            const char* end_of_root = serv.root+strlen(serv.root);
            if (*(end_of_root-1) == '/') {
                strcpy(session->pwd, absolute_path + strlen(serv.root) - 1);
            }
            else {
                strcpy(session->pwd, absolute_path + strlen(serv.root));
            }
            if (!strlen(session->pwd)) {
                session->pwd[0] = '/';
                session->pwd[1] = '\0';
            }
            printf("pwd: %s\n", session->pwd);
            return sendMsg(session->connfd, _250_CHANGE_DIR_SUCCEED);
        }
    }
    else {
        return sendMsg(session->connfd, _553_INVALID_PATH_MESSAGE);
    }
}
int handleCmdPWD(ClientSession* session, const char* param) {
    printf("handling PWD...\n");
	assert(param);
    if (!checkAuth(session)) {
        return 0;
    }
    char response[BUFF_SIZE+10];
    sprintf(response, "257 %s\r\n", session->pwd);
	return sendMsg(session->connfd, response);
}
int handleCmdRNFR(ClientSession* session, const char* param) {
    printf("handling RNFR...\n");
	assert(param);
    if (!checkAuth(session)) {
        return 0;
    }
    char absolute_path[PATH_MAX_LENGTH];
    if (produceAbsPath(absolute_path, PATH_MAX_LENGTH, session->pwd, param)) {
        // check path ok
        printf("Path: %s\n", absolute_path);
        struct stat _stat = {0};
        stat(absolute_path, &_stat);
        if (!S_ISDIR(_stat.st_mode) && !S_ISREG(_stat.st_mode)) {
            return sendMsg(session->connfd, _550_FILE_DIR_DO_NOT_EXIST);
        }
        else {
            strcpy(session->rnfr, absolute_path);
            return sendMsg(session->connfd, _350_READY_FOR_RNTO);
        }
    }
    else {
        return sendMsg(session->connfd, _553_INVALID_PATH_MESSAGE);
    }
}
int handleCmdRNTO(ClientSession* session, const char* param) {
    printf("handling RNTO...\n");
	assert(param);
    if (!checkAuth(session)) {
        return 0;
    }
    char response[BUFF_SIZE];
    if (!strlen(session->rnfr)) {
        return sendMsg(session->connfd, _503_INVALID_COMMAND_SEQ);
    }
    
    char absolute_path[PATH_MAX_LENGTH];
    if (produceAbsPath(absolute_path, PATH_MAX_LENGTH, session->pwd, param)) {
        // check path ok
        printf("Path: %s\n", absolute_path);
        if (rename(session->rnfr, absolute_path) < 0) {
            return sendMsg(session->connfd, _553_INVALID_PATH_MESSAGE);
        }
        else {
            bzero(session->rnfr, PATH_MAX_LENGTH);
            return sendMsg(session->connfd, _250_RENAME_SUCCEED);
        }
    }
    else {
        return sendMsg(session->connfd, _553_INVALID_PATH_MESSAGE);
    }
	return sendMsg(session->connfd, response);
}

int handleCmdLIST(ClientSession* session, const char* param) {
    char list_path[PATH_MAX_LENGTH];
    produceAbsPath(list_path, PATH_MAX_LENGTH, session->pwd, param);
    struct stat _stat = {0};
    stat(list_path, &_stat);
    if (!S_ISDIR(_stat.st_mode) && !S_ISREG(_stat.st_mode)) {
        printf("Not a dir!\n");
        return sendMsg(session->connfd, _550_FILE_DIR_DO_NOT_EXIST);
    }

    char ls_tmp_filename[PATH_MAX_LENGTH];
    strcpy(ls_tmp_filename, list_path);
    char *pos = ls_tmp_filename + strlen(ls_tmp_filename);
    if (*(pos - 1) != '/') {
        *(pos++) = '/';
        *(pos) = '\0';
    }
    *(pos++) = '.';
    while (1) {
        *(pos++) = '-';
        *pos = '\0';
        struct stat _s = {0};
        stat(ls_tmp_filename, &_s);
        if (!S_ISDIR(_s.st_mode) && !S_ISREG(_s.st_mode)) {
            break;
        }
    }
    lsFile(ls_tmp_filename, list_path);
    sendRecvData(session, ls_tmp_filename, SEND);
    if(remove(ls_tmp_filename) < 0) {
        printf("Remove failed.\n");
    }
    return 1;
}
VERB str2verb(const char* str) {
    printf("Token(%s) to verb\n", str);
	if (!str) {
        return INVALID;
    }
	if (!strncasecmp(str, "ABOR", 4)) return ABOR;
	if (!strncasecmp(str, "PASS", 4)) return PASS;
	if (!strncasecmp(str, "PASV", 4)) return PASV;
	if (!strncasecmp(str, "PORT", 4)) return PORT;
	if (!strncasecmp(str, "QUIT", 4)) return QUIT;
	if (!strncasecmp(str, "RETR", 4)) return RETR;
	if (!strncasecmp(str, "STOR", 4)) return STOR;
	if (!strncasecmp(str, "SYST", 4)) return SYST;
	if (!strncasecmp(str, "TYPE", 4)) return TYPE;
	if (!strncasecmp(str, "USER", 4)) return USER;
    if (!strncasecmp(str, "PWD", 3)) return PWD;
    if (!strncasecmp(str, "MKD", 3)) return MKD;
    if (!strncasecmp(str, "RMD", 3)) return RMD;
    if (!strncasecmp(str, "LIST", 4)) return LIST;
    if (!strncasecmp(str, "RNFR", 4)) return RNFR;
    if (!strncasecmp(str, "RNTO", 4)) return RNTO;
    if (!strncasecmp(str, "CWD", 4)) return CWD;
    if (!strncasecmp(str, "APPE", 4)) return APPE;
    if (!strncasecmp(str, "REST", 4)) return REST;
	return INVALID;
}
