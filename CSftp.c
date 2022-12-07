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

// create the socket by calling socket(), bind(), and listen()
// write the new FD to sockfd
void create_socket(int* sockfd, char* port) {
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((*sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(*sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    // don't need this anymore
    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(*sockfd, 10) == -1) {
        perror("listen");
        exit(1);
    }

    // handle all dead processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

// call the accept system command to accept the socket connection from the server side.
// @return the FD of the new socket
int accept_connection(struct sockaddr_storage* their_addr, int sockfd) {
    socklen_t sin_size = sizeof their_addr;
    int new_fd = accept(sockfd, (struct sockaddr *)their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
    }
    return new_fd;
}

int main(int argc, char **argv)
{
    // Check the command line arguments
    if (argc != 2)
    {
        usage(argv[0]);
        return -1;
    }
    
    char s[INET6_ADDRSTRLEN];

    // address of connection
    struct sockaddr_storage their_addr;
    
    // listen on sock_fd, new connection on new_fd
    int sockfd, new_fd;
    
    create_socket(&sockfd, argv[1]);

    char dirBuf[256];
    rootDir = getcwd(dirBuf, 256);

    printf("server: waiting for connections...\r\n");

    while (1) {
        int new_fd = accept_connection(&their_addr, sockfd);

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\r\n", s);

        // create the child process here. AKA the client
        if (!fork()) {
            close(sockfd);
            char server_message[MAXDATASIZE] = "220 Custom ftp server ready\r\n\0";

            int currentlyUser = 0;

            // this is the main infinite loop that iterates for a single client
            while (1) {
                char receive_message[MAXDATASIZE];

                if (send(new_fd, server_message, sizeof(server_message), 0) == -1) {
                    perror("send");
                }
                // set all the message content to null
                for (int i = 0; i < MAXDATASIZE; i++) {
                    server_message[i] = '\0';
                    receive_message[i] = '\0';
                }

                recv(new_fd, receive_message, MAXDATASIZE - 1, 0);

                char buf[MAXDATASIZE];
                for (int i = 0; i < MAXDATASIZE; i++) {}
                
                struct ftp_cmd *cmd = parse_cmd(receive_message);
                
                // switch across all the commands here
                switch (cmd->cmd) {
                    case USER:
                        currentlyUser = 1;
                        login(cmd->args, server_message);
                        printf("USER has logged in\n");
                        break;
                    case TYPE:
                        type(cmd->args, server_message);
                        printf("TYPE has been set\n");
                        break;
                    case MODE:
                        mode(cmd->args, server_message);
                        printf("MODE has been set\n");
                        break;
                    case STRU:
                        stru(cmd->args, server_message);
                        printf("STRU has been set\n");
                        break;
                    case CDUP:
                        cdup(rootDir, cmd->args, server_message);
                        printf("CDUP has been called\n");
                        break;
                    case RETR:
                        retr(cmd->args, server_message);
                        printf("RETR has been called\n");
                        break;
                    case PASV:
                        pasv(cmd->args, server_message, new_fd);
                        printf("PASV has been set\n");
                        break;
                    case NLST:
                        nlst(cmd->args, server_message);
                        printf("NLST has been called\n");
                        break;
                    case CWD:
                        cwd(cmd->args, server_message);
                        printf("CWD has been set\n");
                        break;
                    case QUIT:
                        strcpy(server_message, "221 Goodbye.\n\0");
                        printf("User has QUIT\n");
                        break;
                    case SYST:
                        break;
                    default:
                        strcpy(server_message, "500 invalid command.\n\0");
                }
                free(cmd);
            }
            close(new_fd);
            exit(0);
        }

        // parent doesn't need this
        close(new_fd);
    }

    return 0;
}
