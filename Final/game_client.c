#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Max Socket Message Size
#define MAX_SMS 1024

struct socket_msg {
    char text[MAX_SMS]; // 텍스트 메세지
    int flag; // 동작 flag
};

typedef struct {
    char* ip;
    int port;
} socketAddress;

const socketAddress sa = { "127.0.0.1", 8080 };

void main(){
    struct sockaddr_in s_addr;
    struct socket_msg msg;
    int sock;
    int check;

    // 소켓 선언
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock <= 0){
        perror("socket failed: ");
        exit(1);
    }

    // 소켓 셋팅
    memset(&s_addr, '0', sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(sa.port);
    check = inet_pton(AF_INET, sa.ip, &s_addr.sin_addr);
    if (check<=0){
        perror("inet_pton failed: ");
        exit(1);
    }

    check = connect(sock, (struct sockaddr *) &s_addr, sizeof(s_addr));
    if(check < 0){
        perror("connect failed: ");
        exit(1);
    }

    while(1){
        recv(sock, (struct socket_msg*)&msg, sizeof(msg), 0);

        if (msg.flag == 0){ // 안내문구 출력
            printf("%s", msg.text);
        }
        else if (msg.flag == 1){ // 입력요망
            printf("%s", msg.text);
            scanf("%d", &msg.flag);
            send(sock, (struct socket_msg*)&msg, sizeof(msg), 0);
        }
        else if (msg.flag == -99){ // 종료
            printf("%s", msg.text);
            exit(0);
        }
    }
}