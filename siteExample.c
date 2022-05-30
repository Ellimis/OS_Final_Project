#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 255

int main(int argc, char **argv) {
    int server_socketFD, client_socketFD;
    int state, client_len;
    int pid;

    FILE *fp;
    struct sockaddr_in clientaddr, serveraddr;

    char buf[SIZE];
    char line[SIZE];

    if(argc != 2) {
        printf("Usage: ./server [port]\n");
        printf("ex: ./server 3000\n");
        exit(0);
    }

    memset(line, '\0', SIZE);
    state = 0;

    client_len = sizeof(clientaddr);
    if((fp = fopen("server.txt", "r")) == NULL) {
        perror("file open error : ");
        exit(0);
    }

    if((server_socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error : ");
        exit(0);
    }

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(atoi(argv[1]));

    state = bind(server_socketFD, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(state == -1) {
        perror("bind error : ");
        exit(0);
    }

    state = listen(server_socketFD, 5);
    if(state == -1) {
        perror("listen error : ");
        exit(0);
    }

    signal(SIGCHLD, SIG_IGN);
    while(1) {
        client_socketFD = accept(server_socketFD, (struct sockaddr *)&clientaddr, &client_len);

        pid = fork();
        if(pid == 0) {
            if(client_socketFD == -1) {
                perror("Accept error : ");
                exit(0);
            }

            while(1) {
                memset(buf, '\0', SIZE);
                if(read(client_socketFD, buf, SIZE) <= 0) {
                    close(client_socketFD);
                    exit(0);
                }

                if(strncmp(buf, "quit", 4) == 0) {
                    write(client_socketFD, "bye bye", 8);
                    close(client_socketFD);
                    exit(0);
                    break;
                }

                while(fgets(line, SIZE, fp) != NULL) {
                    if(strstr(line, buf) != NULL) {
                        write(client_socketFD, line, SIZE);
                    }
                    memset(line, '\0', SIZE);
                }
                write(client_socketFD, "end", SIZE);
                printf("send end\n");
                rewind(fp);
            }
        }

        if(pid == 1) {
            perror("fork error : ");
        }
    }

    close(client_socketFD);

}