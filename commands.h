#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include "dir.h"
#include "usage.h"
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <dirent.h>

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
static char* rootDir;



struct ftp_cmd* parse_cmd(char buf[]);

int accept_connection(struct sockaddr_storage* their_addr, int sockfd);

void create_socket(int* sockfd, char* port);

void login(char* arg, char server_message[]);

void type(char* arg, char server_message[]);

void mode(char* arg, char server_message[]);

void stru(char* arg, char server_message[]);

void cwd(char* arg, char server_message[]);

void cdup(char* rootDir, char* arg, char server_message[]);

void pasv(char* arg, char server_message[], int connectionFD);

void nlst(char* arg, char server_message[]);

void retr(char* arg, char server_message[]);