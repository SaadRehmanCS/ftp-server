#include "commands.h"

struct ftp_cmd* parse_cmd(char buf[]) {
    // for (int i = 0; i < 256; i++) {
    //     printf("%d ", buf[i]);
    // }
    // printf("\n");

    struct ftp_cmd* ftp_cmd;
    char* splitString = strtok(buf, " ");

    char* command = splitString;
    splitString = strtok(NULL, " ");
    splitString = strtok(splitString, "\r");
    char* arg = splitString;

    int s_len = strlen(command);
    char commandUpper[s_len]; 

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

void login(char* arg, char server_message[]) {
    if (arg == NULL || loggedIn == 1) {
        strcpy(server_message, "");
    } else if (strcmp(arg, "cs317") == 0) {
        loggedIn = 1;
        strcpy(server_message, "230 Login successful.\n\n\0");
    } else {
        strcpy(server_message, "530 Authentication failed.\n\0");
    }
}

void type(char* arg, char server_message[]) {
    if (arg == NULL) {
        strcpy(server_message, "");
    } else if (strstr(arg, "A") != NULL || strstr(arg, "a") != NULL) {
        strcpy(server_message, "200 Set to ascii.\n\0");
    } else if (strstr(arg, "I") != NULL || strstr(arg, "i") != NULL) {
        strcpy(server_message, "200 Set to binary.\n\0");
    } else {
        strcpy(server_message, "Unknown type.\n\0");
    }
}

void mode(char* arg, char server_message[]) {
    if (arg == NULL) {
        strcpy(server_message, "");
    } else if (strstr(arg, "S") != NULL || strstr(arg, "s") != NULL) {
        strcpy(server_message, "200 Transfer mode set to: S\n\0");
    } else {
        strcpy(server_message, "501 Unrecognized MODE type.\n\0");
    }
}

void stru(char* arg, char server_message[]) {
    if (arg == NULL) {
        strcpy(server_message, "");
    } else if (strstr(arg, "F") != NULL || strstr(arg, "f") != NULL) {
        strcpy(server_message, "200 File transfer structure set to: F.\n\0");
    } else {
        strcpy(server_message, "501 Unrecognized STRU type.\n\0");
    }
}

void cwd(char* arg, char server_message[]) {
    
}