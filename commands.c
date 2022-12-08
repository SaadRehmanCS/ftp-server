#include "commands.h"


// parse through the client response and extract the command along with the argument
struct ftp_cmd* parse_cmd(char buf[]) {
    
    // initialize the structure
    struct ftp_cmd* ftp_cmd;
    ftp_cmd = malloc(sizeof(struct ftp_cmd));
    
    // split the string and extract relevant info
    char* splitString = strtok(buf, " ");
    char* command = splitString;
    splitString = strtok(NULL, " ");
    splitString = strtok(splitString, "\r\n");
    char* arg = splitString;

    int s_len = strlen(command);
    char commandUpper[s_len];

    // convert info to uppercase
    for (int i = 0; i < s_len; i++) {
        commandUpper[i] = toupper(command[i]);
    }
    command = commandUpper;
    if (strstr(command, "USER") != NULL) {
        ftp_cmd->cmd = USER;
    } else if (strstr(command, "PASS") != NULL) {
        ftp_cmd->cmd = PASS;
    } else if (strstr(command, "QUIT") != NULL) {
        ftp_cmd->cmd = QUIT;
    } else if (strstr(command, "CWD") != NULL) {
        ftp_cmd->cmd = CWD;
    } else if (strstr(command, "CDUP") != NULL) {
        ftp_cmd->cmd = CDUP;
    } else if (strstr(command, "TYPE") != NULL) {
        ftp_cmd->cmd = TYPE;
    } else if (strstr(command, "MODE") != NULL) {
        ftp_cmd->cmd = MODE;
    } else if (strstr(command, "STRU") != NULL) {
        ftp_cmd->cmd = STRU;
    } else if (strstr(command, "RETR") != NULL) {
        ftp_cmd->cmd = RETR;
    } else if (strstr(command, "PASV") != NULL) {
        ftp_cmd->cmd = PASV;
    } else if (strstr(command, "NLST") != NULL) {
        ftp_cmd->cmd = NLST;
    } else if (strstr(command, "SYST") != NULL || strstr(command, "\r\n") != NULL) {
        ftp_cmd->cmd = SYST;
    } else {
        ftp_cmd->cmd = INVALID;
    }
    ftp_cmd->args = arg;

    return ftp_cmd;
}

// login process
void login(char* arg, char server_message[]) {
    if (arg == NULL || state->loggedIn == 1) {
        strcpy(server_message, "\r\n");
    } else if (strcmp(arg, "cs317") == 0) {
        state->loggedIn = 1;
        strcpy(server_message, "230 Login successful.\r\n\n\0");
    } else {
        strcpy(server_message, "530 Authentication failed.\r\n\0");
    }
}

// type process
void type(char* arg, char server_message[]) {
    if (arg == NULL) {
        strcpy(server_message, "501 Syntax error: command needs an argument.\r\n\0");
    } else if (strstr(arg, "A") != NULL || strstr(arg, "a") != NULL) {
        strcpy(server_message, "200 Set to ascii.\r\n\0");
    } else if (strstr(arg, "I") != NULL || strstr(arg, "i") != NULL) {
        strcpy(server_message, "200 Set to binary.\r\n\0");
    } else {
        strcpy(server_message, "Unknown type.\r\n\0");
    }
}

// mode process
void mode(char* arg, char server_message[]) {
    if (arg == NULL) {
        strcpy(server_message, "501 Syntax error: command needs an argument.\r\n\0");
    } else if (strstr(arg, "S") != NULL || strstr(arg, "s") != NULL) {
        strcpy(server_message, "200 Transfer mode set to: S\r\n\0");
    } else {
        strcpy(server_message, "501 Unrecognized MODE type.\r\n\0");
    }
}

// stru process
void stru(char* arg, char server_message[]) {
    if (arg == NULL) {
        strcpy(server_message, "501 Syntax error: command needs an argument.\r\n\0");
    } else if (strstr(arg, "F") != NULL || strstr(arg, "f") != NULL) {
        strcpy(server_message, "200 File transfer structure set to: F.\r\n\0");
    } else {
        strcpy(server_message, "501 Unrecognized STRU type.\r\n\0");
    }
}

// cwd process
void cwd(char* rootDir, char* arg, char server_message[]) {
    if (arg == NULL) {
        if (chdir(rootDir) == 0) {
            strcpy(server_message, "250 Directory changed successfully.\r\n\0");
        } else {
            strcpy(server_message, "550 No such file or directory.\r\n\0");
        }
    } else if (strstr(arg, "../") != NULL || (strlen(arg) > 1 && arg[0] == '.' && arg[1] == '/')) {
        strcpy(server_message, "550 Permission denied.\r\n\0");
    } else {
        if (chdir(arg) == 0) {
            strcpy(server_message, "250 Directory changed successfully.\r\n\0");
        } else {
            strcpy(server_message, "550 No such file or directory.\r\n\0");
        }
    }
}

// cdup process
void cdup(char* rootDir, char* arg, char server_message[]) {
    char tempDirBuf[256];
    char* tempRootDir = getcwd(tempDirBuf, 256);
    if (arg != NULL) {
        strcpy(server_message, "501 Syntax error: command does not accept arguments.\r\n");
    } else if (strstr(tempRootDir, rootDir) != NULL && strlen(tempRootDir) == strlen(rootDir)) {
        strcpy(server_message, "550 Permission denied.\r\n\0");
    } else {
        if (chdir("..") == 0) {
            strcpy(server_message, "250 Directory changed successfully.\r\n\0");
        } else {
            strcpy(server_message, "550 No such file or directory.\r\n\0");
        }
    }
}

// Citation: some source materials for PASV/RETR/NLST were inspired by other fully implemented FTP servers available online
// at github.com. I used the same basic ideas to write the code for these functions.

// set up the pasv connection here
// This is the callback for the pthread
void *pasvConnect(void *currentPasvFD) {
    int pasvLength;
    struct sockaddr_in pasvAddr;
    int *psfd = ((int *) currentPasvFD);

    // Listen to up to 5 connections
    listen(*psfd, 5);

    fd_set rfds;
    struct timeval tv;
    int retval;
    FD_ZERO(&rfds);
    FD_SET(*psfd, &rfds);

    // set wait time to 20s
    tv.tv_sec = 20;
    tv.tv_usec = 0;
    retval = select(*psfd + 1, &rfds, NULL, NULL, &tv);
    
    if (retval) {
        if (FD_ISSET(*psfd, &rfds)) {
            pasvLength = sizeof(pasvAddr);
            state->newPasvFD = accept(*psfd, (struct sockaddr *) &pasvAddr, &pasvLength);
        }
    } else {
        state->pasvHasBeenCalled = 0;
        close(state->newPasvFD);
        close(*psfd);
        // timeout
        state->newPasvFD = -500;
        *psfd = -1;
    }
}

// pasv process
void pasv(char server_message[], int new_fd) {
    struct sockaddr_in pasvAddr;
    struct hostent *hostEnt;
    struct in_addr **inAddr;
    int passivePort;
    struct sockaddr_in sockAddr;
    int sockAddrLen;
    int i;
    char hostname[MAXDATASIZE];
    char addr[MAXDATASIZE];
    int ip1, ip2, ip3, ip4;
    char *ipStringSplit;
    pthread_t pasv_thread;


    state->currentPasvFD = socket(AF_INET, SOCK_STREAM, 0);
    if (state->currentPasvFD < 0) {
        exit(-1);
    }

    bzero((char *) &pasvAddr, sizeof(pasvAddr));
    pasvAddr.sin_family = AF_INET;
    pasvAddr.sin_port = 0;
    pasvAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(state->currentPasvFD, (struct sockaddr *) &pasvAddr, sizeof(pasvAddr)) < 0) {
        exit(-1);
    }

    sockAddrLen = sizeof(sockAddr);
    getsockname(state->currentPasvFD, (struct sockaddr *) &sockAddr, &sockAddrLen);
    passivePort = (int) ntohs(sockAddr.sin_port);

    gethostname(hostname, sizeof hostname);
    if ((hostEnt = gethostbyname(hostname)) == NULL) {
            herror("gethostbyname");
            return;
    }

    inAddr = (struct in_addr **)hostEnt->h_addr_list;
    for(i = 0; inAddr[i] != NULL; i++) {
        strcpy(addr, inet_ntoa(*inAddr[i]));
    }

    ipStringSplit = strtok(addr, ".\r\n");
    if (ipStringSplit != NULL) {
        ip1 = atoi(ipStringSplit);
        ipStringSplit = strtok(NULL, ".\r\n");
    }
    if (ipStringSplit != NULL) {
        ip2 = atoi(ipStringSplit);
        ipStringSplit = strtok(NULL, ".\r\n");
    }
    if (ipStringSplit != NULL) {
        ip3 = atoi(ipStringSplit);
        ipStringSplit = strtok(NULL, ".\r\n");
    }
    if (ipStringSplit != NULL) {
        ip4 = atoi(ipStringSplit);
    }

    char buf[MAXDATASIZE];
    sprintf(buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\r\n", ip1, ip2, ip3, ip4, passivePort / 256, passivePort % 256);
    strcpy(server_message, buf);

    state->pasvHasBeenCalled = 1;

    // Create the alternate process and call pasvConnect in the callback to set up the process
    if(pthread_create(&pasv_thread, NULL, pasvConnect, (void *) &state->currentPasvFD)) {
        exit(-1);
    }
}

// nlst process
void nlst(char* arg, char server_message[]) {
    char dir[MAXDATASIZE];

    if (arg != NULL) {
        strcpy(server_message, "501 NLST doesn't support argument.\r\n\0");
        return;
    }

    if (state->pasvHasBeenCalled == 0) {
        strcpy(server_message, "425 Data connection is not open. Use PASV first.\r\n\0");
    } else {
        strcpy(server_message, "150 File status okay; about to open data connection. Here comes the directory listing.\r\n\0");

        getcwd(dir, MAXDATASIZE);
        listFiles(state->newPasvFD, dir);

        // Close all states
        state->pasvHasBeenCalled = 0;
        close(state->newPasvFD);
        close(state->currentPasvFD);
        state->newPasvFD = -1;
        state->currentPasvFD = -1;

        strcpy(server_message, "226 Directory send OK. Closing data connection. Requested file action successful.\r\n\0");
    }
}

// retr process
void retr(char* arg, char server_message[]) {
    char sendMessage[MAXDATASIZE];
    FILE *file;
    int blockSizeFS;
    char sendBuffer[MAXDATASIZE * 2];
    int fs_total;

    if (state->pasvHasBeenCalled == 0){
        strcpy(server_message, "425 Data connection is not open. Use PASV first.\r\n\0");
    } else {
        if (arg == NULL) {
            strcpy(server_message, "550 Requested action not taken. Failed to open file.\r\n\0");
        } else {
            if (access(arg, R_OK) != -1) {
                // Open file and send data
                file = fopen(arg, "r");
                int prev = ftell(file);
                fseek(file, 0L, SEEK_END);
                int sz = ftell(file);
                fseek(file, prev,SEEK_SET);
                fs_total = sz;

                sprintf(sendMessage, "150 File status okay; about to open data connection. Opening BINARY mode data connection for %s (%d bytes).\n", arg, fs_total);
                strcpy(server_message, sendMessage);

                bzero(sendBuffer, MAXDATASIZE * 2);
                while((blockSizeFS = fread(sendBuffer, sizeof(char), MAXDATASIZE * 2, file)) > 0) {
                    write(state->newPasvFD, sendBuffer, blockSizeFS);
                    bzero(sendBuffer, MAXDATASIZE * 2);
                }

                // Close all states
                fclose(file);
                state->pasvHasBeenCalled = 0;
                close(state->newPasvFD);
                close(state->currentPasvFD);
                state->newPasvFD = -1;
                state->currentPasvFD = -1;

                strcpy(server_message, "226 Closing data connection. Transfer complete.\n");
            } else {
                strcpy(server_message, "550 Requested action not taken. Failed to open file.\n");
            }
        }
    }
}