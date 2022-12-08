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


// Keep track of the state of the server constantly
struct State {
   int loggedIn;
   char* rootDir;
   int newPasvFD;
   int currentPasvFD;
   int pasvHasBeenCalled;
};

struct State* state;

struct ftp_cmd* parse_cmd(char buf[]);

// call the accept system command to accept the socket connection from the server side.
// @return the FD of the new socket
int accept_connection(struct sockaddr_storage* their_addr, int sockfd);

// Create the socket given the port, and store the value of the new FD into sockfd
void create_socket(int* sockfd, char* port);


// For all the following functions, we handle the client message with the arg,
// and write the correct server code and message to server_message.
// @return nothing
void login(char* arg, char server_message[]);

void type(char* arg, char server_message[]);

void mode(char* arg, char server_message[]);

void stru(char* arg, char server_message[]);

void cwd(char* rootDir, char* arg, char server_message[]);

void cdup(char* rootDir, char* arg, char server_message[]);

void pasv(char server_message[], int new_fd);

void nlst(char* arg, char server_message[]);

void retr(char* arg, char server_message[]);