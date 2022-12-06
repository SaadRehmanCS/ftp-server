#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXDATASIZE 256

enum FTP_CMD {
    INVALID = -1,
    USER = 1,
    PASS = 2,
    RETR = 3,
    PASV = 4,
    TYPE = 5,
    QUIT = 6,
    CWD = 7,
    CDUP = 8,
    MODE = 9,
    STRU = 10,
    NLST = 11,
    SYST = 12
 };

 struct ftp_cmd {
    enum FTP_CMD cmd;
    char* args;
 };

static int loggedIn = 0;

struct ftp_cmd* parse_cmd(char buf[]);

void login(char* arg, char server_message[]);

void type(char* arg, char server_message[]);

void mode(char* arg, char server_message[]);

void stru(char* arg, char server_message[]);

void cwd(char* arg, char server_message[]);