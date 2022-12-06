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
#include "commands.h"

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char **argv)
{
    // Check the command line arguments
    if (argc != 2)
    {
        usage(argv[0]);
        return -1;
    }

    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, 10) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            char server_message[MAXDATASIZE] = "220 Custom ftp server ready\n\0";
            // printf("entering loop\n");
            int currentlyUser = 0;
            while (1) {
                char receive_message[MAXDATASIZE];
                // printf("first one\n");
                if (send(new_fd, server_message, sizeof(server_message), 0) == -1) {
                    perror("send");
                }
                for (int i = 0; i < MAXDATASIZE; i++) {
                    server_message[i] = '\0';
                    receive_message[i] = '\0';
                }
                // printf("next\n");
                // if (currentlyUser) {
                //     currentlyUser = 0;
                //     continue;
                // }
                recv(new_fd, receive_message, MAXDATASIZE - 1, 0);
                // fflush(stdout);
                printf("\n");
                struct ftp_cmd *cmd = parse_cmd(receive_message);
                // printf("type is: %s\n", cmd->cmd);
                switch (cmd->cmd) {
                    case USER:
                        currentlyUser = 1;
                        login(cmd->args, server_message);
                        // printf("\n");
                        break;
                    case TYPE:
                        type(cmd->args, server_message);
                        printf("\n");
                        break;
                    case MODE:
                        mode(cmd->args, server_message);
                        printf("\n");
                        break;
                    case STRU:
                        stru(cmd->args, server_message);
                        printf("\n");
                        break;
                    case CWD:
                        cwd(cmd->args, server_message);
                        printf("\n");
                        break;
                    case QUIT:
                        strcpy(server_message, "221 Goodbye.\n\0");
                        // printf("\n");
                        break;
                    case SYST:
                        break;
                    default:
                        strcpy(server_message, "500 invalid command.\n\0");
                }
            }
            // printf("closing inner now\n");
            close(new_fd);
            exit(0);
        }
        // printf("closing outer now\n");
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}

// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.
/* this function is run by the second thread */

// void *inc_x()
// {
//   printf("x increment finished\n");
//   return NULL;
// }

// int main(int argc, char **argv) {

//     // int i;
//     // pthread_t child;
//     // pthread_create(&child, NULL, inc_x, NULL);

//     // Check the command line arguments
//     if (argc != 2) {
//       usage(argv[0]);
//       return -1;
//     }
//     // create the server socket
//     int server_socket;
//     server_socket = socket(AF_INET, SOCK_STREAM, 0);

//     struct sockaddr_in server_address;
//     server_address.sin_family = AF_INET;
//     server_address.sin_port = htons(strtol(argv[1], NULL, 10));
//     server_address.sin_addr.s_addr = INADDR_ANY;

//     // bind the socket to the specified IP and port
//     bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
//     listen(server_socket, 10);
//     printf("before\n");
//     while(1) {
//         printf("in loop\n");
//         int client_socket = accept(server_socket, NULL, NULL);
//         if (client_socket == -1) {
//             perror("accept");
//             continue;
//         }
//         char server_message[MAXDATASIZE] = "220 You have reached the server\n";
//         while (1) {
//             char receive_message[MAXDATASIZE];
//             // send the message
//             send(client_socket, server_message, sizeof(server_message), 0);

//             recv(client_socket, receive_message, MAXDATASIZE-1, 0);

//             printf("\n");
//             struct ftp_cmd *cmd = parse_cmd(receive_message);
//             // printf("type is: %d\n", cmd->cmd);
//             switch(cmd->cmd) {
//                 case USER:
//                     login(cmd->args, server_message);
//                     printf("\n");
//                     break;
//                 case QUIT:
//                     strcpy(server_message, "221 Goodbye.\n");
//                     printf("\n");
//                     break;
//                 default:
//                     strcpy(server_message, "500 invalid command.\n");
//             }
//         }
//     }
//     // printf("cdscd\n");
//     // close the socket
//     close(server_socket);

//     // This is how to call the function in dir.c to get a listing of a directory.
//     // It requires a file descriptor, so in your code you would pass in the file descriptor
//     // returned for the ftp server's data connection

//     printf("Printed %d directory entries\n", listFiles(1, "."));
//     return 0;

// }
