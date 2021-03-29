
#ifndef STDINC_H
#define STDINC_H
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

#define PrintError() do {                                              \
        fprintf(stderr, "%s:%d %s(): %s(%d)\n", __FILE__, __LINE__,    \
                                __FUNCTION__, strerror(errno), errno); \
} while(0)
#define BUFF_SIZE 1024
#define FILE_TRANSFER_BUFF_SIZE 1024 
#define USERNAME_MAX_LENGTH 100
#define PASSWD_MAX_LENGTH 100
#define PATH_MAX_LENGTH 1024
#define HOST_NAME_MAX_LENGTH 16
#define DEFAULT_PORT 21
#define DEFAULT_ROOT "/tmp"
struct Server {
    int listenfd;
    struct sockaddr_in listen_addr;

    // config
    char root[PATH_MAX_LENGTH];
    char hostname[HOST_NAME_MAX_LENGTH];
} serv;

typedef struct ClientSession {
    int connfd;
    int transfd;
    
    char pwd[PATH_MAX_LENGTH];
    char username[USERNAME_MAX_LENGTH];
    char passwd[PASSWD_MAX_LENGTH];
    enum AUTH { UNAUTH = 0, ONLY_USERNAME, AUTH} auth;
    enum TRANS_MODE { TRANS_NONE = 0, TRANS_PORT, TRANS_PASV} transfer_mode;
    struct sockaddr_in trans_addr;
    
    int file_pos;
    int pasv_listenfd;
    char rnfr[PATH_MAX_LENGTH];
} ClientSession;

typedef enum VERB {
    INVALID = -1,
    ABOR, APPE,
    CWD,
    LIST,
    MKD,
    PASS, PASV, PORT, PWD,
    QUIT,
    RETR, RNFR, RNTO, RMD, REST,
    STOR, SYST,
    TYPE,
    USER,
} VERB;
typedef enum DATA_SEND_RECV {
    SEND,
    RECV
} DATA_SEND_RECV;
int produceAbsPath(char* dest, const int dest_len, const char* root, const char* sub_path);
int lsFile(const char* dest_file, const char *dirname);
extern const char GREETING_MESSAGE[];
extern const char GOODBYE_MESSAGE[];
extern const char SYST_RESPONSE[];
extern const char TYPE_RESPONSE[];
extern const char UNAUTH_MESSAGE[];
extern const char UNKNOWN_COMMAND_MESSAGE[];
extern const char LOGGED_IN_MESSAGE[];
extern const char _331_NEED_PASSWD_MESSAGE[];
extern const char LOG_SUCCESS_MESSAGE[];
extern const char NEED_USERNAME_MESSAGE[];
extern const char INVALID_PARAM_MESSAGE[];
extern const char PASV_SUCCESS_MESSAGE[];
extern const char PORT_SUCCESS_MESSAGE[];
extern const char _553_INVALID_PATH_MESSAGE[];
extern const char TRANSFER_MODE_UNSET_MESSAGE[];
extern const char FILE_DISABLE_MESSAGE[];
extern const char DATA_CONNECTION_SUCCESS_MESSAGE[];
extern const char DATA_CONNECTION_STARTED_MESSAGE[];
extern const char CONNECTION_CLOSED_MESSAGE[];
extern const char LOCAL_ERROR_MESSAGE[];
extern const char DATA_CONNECTION_OPEN_FAILED[];
extern const char _550_FILE_DIR_DO_NOT_EXIST[];
extern const char _503_INVALID_COMMAND_SEQ[];
extern const char _250_RENAME_SUCCEED[];
extern const char _350_READY_FOR_RNTO[];
extern const char _250_CHANGE_DIR_SUCCEED[];
extern const char _250_REMOVE_DIR_SUCCEED[];
extern const char _257_MAKE_DIR_SUCCEED[];
extern const char _150_DATA_CONNECTION_OPEND[];
extern const char _226_DATA_CONNECTION_FINISHED[];
extern const char _530_UNSOPPORTED_USERNAME[];
#endif