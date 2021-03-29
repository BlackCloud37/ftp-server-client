#include "../include/stdinc.h"
const char GREETING_MESSAGE[] = "220 Anonymous FTP server ready.\r\n";
const char GOODBYE_MESSAGE[] = "221 Goodbye.\r\n";
const char SYST_RESPONSE[] = "215 UNIX Type: L8\r\n";
const char UNAUTH_MESSAGE[] = "530 Not logged in.\r\n";
const char UNKNOWN_COMMAND_MESSAGE[] = "500 Unknown command.\r\n";
const char TYPE_RESPONSE[] = "200 Type set to I.\r\n";
const char LOGGED_IN_MESSAGE[] = "230 User have already logged in.\r\n";
const char _331_NEED_PASSWD_MESSAGE[] = "331 Guest login ok, send your complete e-mail address as password.\r\n";
const char LOG_SUCCESS_MESSAGE[] = "230 Guest login ok, access restrictions apply.\r\n";
const char NEED_USERNAME_MESSAGE[] = "332 Please set your username first.\r\n";
const char INVALID_PARAM_MESSAGE[] = "504 Invalid parameter.\r\n";
const char PASV_SUCCESS_MESSAGE[] = "227 Entering Passive Mode (%s)\r\n";
const char PORT_SUCCESS_MESSAGE[] = "200 PORT command successful.\r\n";
const char _553_INVALID_PATH_MESSAGE[] = "553 Invalid filename or path.\r\n";
const char TRANSFER_MODE_UNSET_MESSAGE[] = "425 File transfer mode(PASV/PORT) unset.\r\n";
const char DATA_CONNECTION_SUCCESS_MESSAGE[] = "150 Data connection opened.\r\n";
const char DATA_CONNECTION_STARTED_MESSAGE[] = "125 Data transfer started.\r\n";
const char FILE_DISABLE_MESSAGE[] = "450 File unavailable.\r\n";
const char CONNECTION_CLOSED_MESSAGE[] = "426 Connection closed.\r\n";
const char LOCAL_ERROR_MESSAGE[] = "451 Internal server error.\r\n";
const char DATA_CONNECTION_OPEN_FAILED[] = "425 Data connection created failed.\r\n";
const char _550_FILE_DIR_DO_NOT_EXIST[] = "550 File or directory doesn't exist.\r\n";
const char _503_INVALID_COMMAND_SEQ[] = "503 Invalid command sequence.\r\n";
const char _250_RENAME_SUCCEED[] = "250 Rename succeed.\r\n";
const char _350_READY_FOR_RNTO[] = "350 Ready for RNTO.\r\n";
const char _250_CHANGE_DIR_SUCCEED[] = "250 Change dir succeed.\r\n";
const char _250_REMOVE_DIR_SUCCEED[] = "250 Remove dir succeed.\r\n";
const char _257_MAKE_DIR_SUCCEED[] = "257 Make dir succeed.\r\n";
const char _150_DATA_CONNECTION_OPEND[] = "150 Data conncection opend.\r\n";
const char _226_DATA_CONNECTION_FINISHED[] = "226 Transmission finished.\r\n";
const char _530_UNSOPPORTED_USERNAME[] = "530 Only anonymous user is supported.\r\n";


/*
* Example: 
*   @root can be '/absolute/path/to/tmp' or '/absolute/path/to/tmp/'
*   @sub_path can be 'file', '/path/to/file', './path/to/file', sub_path of dir is not supported now
*/
int produceAbsPath(char* dest, const int dest_len, const char* root, const char* sub_path) {
    assert(dest);
    assert(root);
    assert(sub_path);
    char base[PATH_MAX_LENGTH];
    strcpy(base, serv.root);
    char* end_of_base = base + strlen(base);
	if (*(end_of_base - 1) == '/') {
		end_of_base -= 1;
		*(end_of_base) = '\0';
	}

    if (sub_path[0] == '/') {
        strcpy(end_of_base, sub_path);
    }
    else {
        if (root[0] == '/') {
            strcpy(end_of_base, root);
        }
        else {
            *end_of_base = '/';
            end_of_base += 1;
            strcpy(end_of_base, root);
        }
        end_of_base = base + strlen(base);
        if (*(end_of_base - 1) == '/') {
            end_of_base -= 1;
            *(end_of_base) = '\0';
        }
        char _sub_path[PATH_MAX_LENGTH];
        strcpy(_sub_path, sub_path);
        char *end_of_sub = _sub_path + strlen(_sub_path);
        char *left = _sub_path;
        while (left < end_of_sub) {
            char *slash_pos = strchr(left, '/');
            if (!slash_pos) {
                slash_pos = end_of_sub;
            }
            *slash_pos = '\0';
            // [left, slash_pos) now is a dirname or filename
            if (!strcmp(left, "."))
            {
                left = slash_pos + 1;
                continue;
            }
            if (*left == '.' && *(left + 1) == '.') { // have .. , remove the last basename in base
                while (end_of_base > base && *(end_of_base - 1) != '/') {
                    end_of_base -= 1;
                }
                if (end_of_base == base) {
                    return 0;
                }
                end_of_base -= 1;
                *end_of_base = '\0';
                left = slash_pos + 1;
                continue;
            }
            if (*left == '.' && *(left + 1) == '.' && *(left + 2) == '.') { // have ... which is invalid
                return 0;
            }
            *end_of_base = '/';
            end_of_base += 1;
            strcpy(end_of_base, left);
            end_of_base += slash_pos - left;
            left = slash_pos + 1;
        }
    }
    strcpy(dest, base);
    char* end_of_dest = dest + strlen(dest);
    if (*(end_of_dest-1) == '/') {
        *(end_of_dest-1) = '\0';
    }
    printf("ABS: %s\n", dest);
    if (strlen(dest) < strlen(serv.root) - 1) {
        return 0;
    }
    return 1;
}

int lsFile(const char* dest_file, const char *dirname) {
    char cmd[PATH_MAX_LENGTH];
    sprintf(cmd, "ls %s -lh", dirname);
    FILE* output = popen(cmd, "r");
    FILE* dest = fopen(dest_file, "w");

    char buff[BUFF_SIZE];
    while(1) { // skip first line
        int count = fread(buff, 1, 1, output);
        if (!count) {
            return -1;
        }
        if (buff[0] == '\n') {
            break;
        }
    }
    while(1) {
        int count = fread(buff, 1, BUFF_SIZE, output);
        if (!count) {
            break;
        }
        fwrite(buff, 1, count, dest);
    }
    fclose(dest);
    fclose(output);
    return 1;
}