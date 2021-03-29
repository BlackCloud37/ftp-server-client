#include "../include/stdinc.h"
#include "../include/cmd_handler.h"

void closeServ(int status) {
    printf("Closing server...\n");
    close(serv.listenfd);
	fclose(stdout);
    exit(EXIT_SUCCESS);
}

int sendMsg(const int fd, const char* buffer) {
    printf("Sending message %s\n", buffer);
	assert(buffer);
	int len = strlen(buffer), p = 0;
	while (p < len) {
		int n = write(fd, buffer + p, len - p);
		if (n < 0) {
			PrintError();
			return -1;
		} 
		else {
			p += n;
		}
	}
    printf("Sendcount %d\n", p);
	return p;
}

int recvMsg(const int fd, char* buffer, const int buffer_len) {
    printf("Receiving message\n");
    assert(buffer);
	bzero(buffer, buffer_len);
	int recv_size = 0;
	while(1) {
        if (recv_size == buffer_len) { // buffer's full
            return -1;
        }
		int n = read(fd, buffer + recv_size, buffer_len - recv_size);
		if (n < 0) {
            PrintError();
			return -1;
		}
		else if (n == 0) {
			break;
		}
		else {
			recv_size += n;
            if (buffer[recv_size-2] == '\r' && buffer[recv_size-1] == '\n') {
                recv_size -= 2;
                break;
            }
		}
	}
	buffer[recv_size] = '\0';
    printf("Recv message %s of length (%d)\n", buffer, recv_size);
	return recv_size;
}


int sendRecvData(ClientSession* session, const char* filename, const DATA_SEND_RECV mode) {
	printf("send or recv data\n");
	int data_fd;
	struct stat _stat;
	bzero(&_stat, sizeof(struct stat));
	stat(filename, &_stat);
	if (mode == SEND) {
		if (!S_ISREG(_stat.st_mode)) {
			return sendMsg(session->connfd, FILE_DISABLE_MESSAGE);
		}
		data_fd = open(filename, O_RDONLY);
		if (data_fd < 0) {
			PrintError();
			sendMsg(session->connfd, FILE_DISABLE_MESSAGE);
			return -1;
		}
	}
	else if (mode == RECV) {
		if (!S_ISREG(_stat.st_mode)) {
			data_fd = open(filename, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
		}
		else {
			data_fd = open(filename, O_RDWR | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
		}
	}

	lseek(data_fd, session->file_pos, SEEK_SET);

	if (session->transfer_mode == TRANS_NONE) {
        return sendMsg(session->connfd, TRANSFER_MODE_UNSET_MESSAGE);
    }
    else if (session->transfer_mode == TRANS_PASV) {
        // listen and accpet
		printf("Accepting...\n");
		if (sendMsg(session->connfd, _150_DATA_CONNECTION_OPEND) < 0) {
            return -1;
        }
        session->transfd = accept(session->pasv_listenfd, NULL, NULL);
        if (session->transfd < 0) {
            sendMsg(session->connfd, DATA_CONNECTION_OPEN_FAILED);
            PrintError();
            return -1;
		}
    }
    else if (session->transfer_mode == TRANS_PORT) {
        // connect to
        if (sendMsg(session->connfd, _150_DATA_CONNECTION_OPEND) < 0) {
            return -1;
        }
        session->transfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (session->transfd < 0 || connect(session->transfd, (struct sockaddr*)&session->trans_addr, sizeof(struct sockaddr_in)) < 0) {
            PrintError();
            return -1;
        }
    }
	// start transfer
	int readCount, sendCount, from_fd, to_fd;
	char buffer[FILE_TRANSFER_BUFF_SIZE];
	if (mode == SEND) { // send
		from_fd = data_fd;
		to_fd = session->transfd;
	}
	else if (mode == RECV) {
		from_fd = session->transfd;
		to_fd = data_fd;
	}
	while(1) {
		readCount = read(from_fd, buffer, FILE_TRANSFER_BUFF_SIZE);
		if (readCount == 0) {
			break;
		}
		sendCount = write(to_fd, buffer, readCount);
	}

	// clean
	session->file_pos = 0;
	if (sendMsg(session->connfd, _226_DATA_CONNECTION_FINISHED) < 0) {
		return -1;
	}
	if (session->transfer_mode == TRANS_PASV) {
        close(session->pasv_listenfd);
        session->pasv_listenfd = 0;
    }
    close(session->transfd);
    session->transfd = 0;
	close(data_fd);
	return sendCount;
}

void closeSession(ClientSession* session) {
    printf("Close handler triggered\n");
	// sendMsg(session->connfd, CONNECTION_CLOSED_MESSAGE);
	close(session->connfd);
    free(session);
}

void* newConnectHandler(void* arg) {
    ClientSession* session = (ClientSession*)arg;
    int connfd = session->connfd;
    char buffer[BUFF_SIZE];
    char param[BUFF_SIZE];
    if (sendMsg(connfd, GREETING_MESSAGE) < 0) {
        closeSession(session);
		return 0;
    }
    while(1) {
        bzero(buffer, BUFF_SIZE);
        int recv_size = recvMsg(connfd, buffer, BUFF_SIZE);
        if (recv_size == 0) { // closed pipe
            printf("Closed pipe\n");
            closeSession(session);
			return 0;
        }
		else if (recv_size < 0) {
			PrintError();
			closeSession(session);
			return 0;
		}
        VERB verb = parseCmd(buffer, param);
        if (handleCmd(session, verb, param) < 0) {
            closeSession(session);
			return 0;
        }
    }
    return 0;
}

void getIp() {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "8.8.8.8", &(addr.sin_addr.s_addr));
	connect(fd, (struct sockaddr *)&(addr), sizeof(addr));
	socklen_t n = sizeof addr;
	getsockname(fd, (struct sockaddr *)&addr, &n);
	inet_ntop(AF_INET, &(addr.sin_addr), serv.hostname, INET_ADDRSTRLEN);
}

int main(int argc, char** argv) {
    freopen ("/dev/null", "w", stdout);
	
	int host_port = DEFAULT_PORT;
	char root[PATH_MAX_LENGTH];
	strcpy(root, DEFAULT_ROOT);
	if (argc != 1) {
		if (argc == 3) {
			if (!strcmp(argv[1], "-port")) {
				host_port = atoi(argv[2]);
			}
			else if (!strcmp(argv[1], "-root")) {
				strcpy(root, argv[2]);
			}
			else {
				printf("Wrong input arguments!(must be (-port n) or (-root /path))");
			}
		}
		else if (argc == 5) {
			if (!strcmp(argv[1], "-port")) {
				host_port = atoi(argv[2]);
				strcpy(root, argv[4]);
			}
			else if (!strcmp(argv[1], "-root")) {
				host_port = atoi(argv[4]);
				strcpy(root, argv[2]);
			}
			else {
				printf("Wrong input arguments!(must be (-port n -root /path) or (-root /path -port n))");
				exit(EXIT_FAILURE);
			}
		}
		else {
			printf("Wrong argument count, must be 3 or 5.");
			exit(EXIT_FAILURE);
		}
		// make sure the root dir doesn't have the slash as its end.
		char *p = root;
		p += strlen(root);
		if (*(p - 1) == '/') {
			p -= 1;
			*p = '\0';
		}
	}
    strcpy(serv.root, root);
	getIp();
	if ((serv.listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		PrintError();
		exit(EXIT_FAILURE);
	}
    bzero(&serv.listen_addr, sizeof(struct sockaddr_in));
	serv.listen_addr.sin_family = AF_INET;
	serv.listen_addr.sin_port = htons(host_port);
	serv.listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(serv.listenfd, (struct sockaddr *)&serv.listen_addr, sizeof(struct sockaddr_in)) == -1) {
		PrintError();
		exit(EXIT_FAILURE);
	}
    if (listen(serv.listenfd, 10) == -1) {
		PrintError();
		exit(EXIT_FAILURE);
	}
	printf("Server is listening...\n");
    // handle serv
    signal(SIGINT, closeServ);
    int connfd;
    while (1) {
		if ((connfd = accept(serv.listenfd, NULL, NULL)) == -1) {
			PrintError();
			continue;
		}
        printf("New session created, connfd = %d\n", connfd);
		pthread_t thread_id;
        ClientSession *session = (ClientSession*)malloc(sizeof(ClientSession));
        bzero(session, sizeof(ClientSession));
		session->connfd = connfd;
		strcpy(session->pwd, "/");
		pthread_create(&thread_id, NULL, newConnectHandler, (void *)session);
	}
	closeServ(0);
	return 0;
}