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
    if (arg == NULL || loggedIn == 1) {
        strcpy(server_message, "\r\n");
    } else if (strcmp(arg, "cs317") == 0) {
        loggedIn = 1;
        strcpy(server_message, "230 Login successful.\r\n\n\0");
    } else {
        strcpy(server_message, "530 Authentication failed.\r\n\0");
    }
}

// type process
void type(char* arg, char server_message[]) {
    if (arg == NULL) {
        strcpy(server_message, "");
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
        strcpy(server_message, "");
    } else if (strstr(arg, "S") != NULL || strstr(arg, "s") != NULL) {
        strcpy(server_message, "200 Transfer mode set to: S\r\n\0");
    } else {
        strcpy(server_message, "501 Unrecognized MODE type.\r\n\0");
    }
}

// stru process
void stru(char* arg, char server_message[]) {
    if (arg == NULL) {
        strcpy(server_message, "");
    } else if (strstr(arg, "F") != NULL || strstr(arg, "f") != NULL) {
        strcpy(server_message, "200 File transfer structure set to: F.\r\n\0");
    } else {
        strcpy(server_message, "501 Unrecognized STRU type.\r\n\0");
    }
}

// cwd process
void cwd(char* arg, char server_message[]) {
    if (arg == NULL) {
        strcpy(server_message, "\r\n");
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

// pasv process
void pasv(char* arg, char server_message[], int connectionFD) {
    int ip[4];
    char buff[255];
    char *response = "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n\0";

    srand(time(NULL));
    int p1 = 128 + (rand() % 64);
    int p2 = rand() % 0xff;

    socklen_t addr_size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    getsockname(connectionFD, (struct sockaddr *)&addr, &addr_size);
 
    char* host = inet_ntoa(addr.sin_addr);
    sscanf(host,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);

    char port[10];
    sprintf(port, "%d", (256 * p1) + p2);

    int pasvSock;
    /* Start listening here, but don't accept the connection */
    create_socket(&pasvSock, port);
    // printf("port: %d\n", 256 * p1 + p2);
    sprintf(buff,response,ip[0],ip[1],ip[2],ip[3], p1, p2);
    strcpy(server_message, buff);
}

// nlst process
void nlst(char* arg, char server_message[]) {}

// retr process
void retr(char* arg, char server_message[]) {
    int connection;
    int fd;
    struct stat stat_buf;
    off_t offset = 0;
    int sent_total = 0;

      /* Passive mode */
    if(access(arg, R_OK) == 0 && (fd = open(arg, O_RDONLY))) {
        fstat(fd,&stat_buf);
        strcpy(server_message, "150 Opening BINARY mode data connection.\r\n\0");
        // connection = accept_connection(state->sock_pasv);
        // close(state->sock_pasv);
        // if(sent_total = sendfile(connection, fd, &offset, stat_buf.st_size)) {
        //     if(sent_total != stat_buf.st_size) {
        //         perror("ftp_retr:sendfile");
        //         exit(EXIT_SUCCESS);
        //     }
        //     strcpy(server_message, "226 File send OK.\r\n\0");
        // } else {
        //     strcpy(server_message, "550 Failed to read file.\r\n\0");
        // }
    } else {
        strcpy(server_message, "550 Failed to get file.\r\n\0");
    }

}