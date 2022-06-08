#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
//#include <fcntl.h>

// Max Player Card Count
#define MAX_PCC 2
// Max Socket Message Size
#define MAX_SMS 1024
// Max Question & Turn Count
#define MAX_QNTC 3

// struct type for socket address
typedef struct {
    char* ip;
    int port;
} socketAddress;

// struct type for user information
struct user {
    int hp;
    int whois;
};

// struct type for card information
struct card {
    char cards[MAX_PCC];
    int cardCount;
    int rerollCount;
};

// struct type for player information
// data for send/receive between main server and client server
struct gameInfo {
    struct card cardInfo;
    struct user userInfo;
};

// struct type for send/receive message between client server and client
struct socket_msg {
    // 텍스트 메세지, 동작 flag
    char text[MAX_SMS];
    int flag; 
};

// 전역변수 선언
// socket address information
const socketAddress sa = { "127.0.0.1", 8080 };
// client id
const int clientA = 0;
const int clientB = 1;
// player card symbol
const char symbol[4] = { '+', '-', '*', '/' };
// use only + -
const int symbolCount = 2;
// 시그널 사용을 위해 client 소켓 저장 (child 프로세스만 사용)
int child_client_sock;

//게임 진행을 위해 필요한 함수 정의
// 소켓 수신 함수
struct socket_msg receive_sock(int sock);
// 소켓 송신 함수
void send_sock(int sock, struct socket_msg msg);
// player(card, user) information setting function
struct gameInfo setPlayer(int clientNum);
// return each client's question result function
int calcQuestion(char* question);
// signal handling function
void signalHandler(int sig);

void main(){
    /*********************************************************
     1. 소켓 통신을 위한 셋팅 ************************************
    **********************************************************/
    int s_sock_fd, new_sock_fd;
    struct sockaddr_in s_address, c_address;
    int addrlen = sizeof(s_address);
    // int type var for function error check 
    int check;
    // 서버에 접속한 클라이언트의 수
    int child_process_num = 0;
    // int type var for what current turn is it
    int turnCount = MAX_QNTC;
    
    // 서버 소켓 선언 (AF_INET: IPv4 인터넷 프로토콜, SOCK_STREAM: TCP/IP)
    s_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sock_fd == -1){
        perror("socket failed: ");
        exit(1);
    }
    
    // s_address의 메모리 공간을 0으로 셋팅 / memset(메모리 공간 시작점, 채우고자 하는 값, 메모리 길이)
    memset(&s_address, '0', addrlen); 
    // AF_INET: IPv4 internet protocol
    s_address.sin_family = AF_INET;
    // 서버 소켓의 주소 명시
    s_address.sin_addr.s_addr = inet_addr(sa.ip);
    // 서버 소켓의 포트번호 명시
    s_address.sin_port = htons(sa.port);
    // 소켓 정보와 서버 소켓 Bind
    check = bind(s_sock_fd, (struct sockaddr *)&s_address, sizeof(s_address)); 
    if(check == -1){
        perror("bind error: ");
        exit(1);
    }

    /*********************************************************
     2. IPC를 위한 셋팅 ****************************************
    **********************************************************/
    // two client's pid
    pid_t pid_0, pid_1;
    // the key, id for using message queue between main server and client 0, client 1 
    key_t key_p2c_0 = 20, key_c2p_0 = 19;
    key_t key_p2c_1 = 51, key_c2p_1 = 24;
    int msqid_p2c_0, msqid_c2p_0, msqid_p2c_1, msqid_c2p_1;

    //Message queue 초기화 (clear up)
    if((msqid_p2c_0=msgget(key_p2c_0, IPC_CREAT|0666))==-1){exit(-1);}
    msgctl(msqid_p2c_0, IPC_RMID, NULL);
    if((msqid_c2p_0=msgget(key_c2p_0, IPC_CREAT|0666))==-1){exit(-1);}
    msgctl(msqid_c2p_0, IPC_RMID, NULL);
    if((msqid_p2c_1=msgget(key_p2c_1, IPC_CREAT|0666))==-1){exit(-1);}
    msgctl(msqid_p2c_1, IPC_RMID, NULL);
    if((msqid_c2p_1=msgget(key_c2p_1, IPC_CREAT|0666))==-1){exit(-1);}
    msgctl(msqid_c2p_1, IPC_RMID, NULL);
    
    // Message queue 생성
    if((msqid_p2c_0=msgget(key_p2c_0, IPC_CREAT|0666))==-1){exit(-1);}
    if((msqid_c2p_0=msgget(key_c2p_0, IPC_CREAT|0666))==-1){exit(-1);}
    if((msqid_p2c_1=msgget(key_p2c_1, IPC_CREAT|0666))==-1){exit(-1);}
    if((msqid_c2p_1=msgget(key_c2p_1, IPC_CREAT|0666))==-1){exit(-1);}

/*********************************************************
     3. 각 플레이어에게 전달할 초기 게임 정보 생성 및 전송 *************
    **********************************************************/
    // 데이터 주고 받기 위한 구조체 선언 (부모 프로세스가 사용)
    struct gameInfo snd_info, rcv_info0, rcv_info1; 
    // for turning on/off the main server
    int isGameInProgress = 0;
     // for changing next question
    int curQuestion = 0;
     // pointer array for all questions
    char* questions[MAX_QNTC];
    // temp for make one question
    char temp[10]; 
    int i = 0, j = 0;

    srand(time(NULL));
    printf("Set game info - Make Question\n");
    printf("===================================\n");

    for(int i = 0; i < MAX_QNTC; i++) {
        int num[5] = { 0 };
        int index = 0;

        questions[i] = (char*)malloc(strlen(temp)+1);
        printf("%dth question: ", i+1);

        for(j = 0; j < 5; j++) {
            index = rand() % (i+j+4);
            if(index >= 10) index = 9;
            num[j] = index;
        }

        for(j = 0; j < 9; j++) {
            if(j%2 == 0) {
                //index = j/2;
                temp[j] = num[(int)j/2]+'0';
            }
            else {
                //index = rand() % symbolCount;
                temp[j] = symbol[rand() % symbolCount];
            }
        }

        temp[9] = '\n';
        temp[(rand()%4)*2+1] = '_';
        strcpy(questions[i], temp);
    }
    printf("===================================\n");

    snd_info = setPlayer(clientA);
    msgsnd(msqid_p2c_0, &snd_info, sizeof(struct gameInfo), 0);
    snd_info = setPlayer(clientB);
    msgsnd(msqid_p2c_1, &snd_info, sizeof(struct gameInfo), 0);

    while(1){
    /*********************************************************
     4. 게임 플레이어 접속 대기 ***********************************
    **********************************************************/
        // 클라이언트의 접속을 기다림
        check = listen(s_sock_fd, 16);
        if(check == -1){
            perror("listen failed: ");
            exit(1);
        }

        // 클라이언트의 접속을 허가함 -> 접속에 성공한 클라이언트와의 통신을 위해 새로운 소켓을 생성함
        new_sock_fd = accept(s_sock_fd, (struct sockaddr *) &c_address, (socklen_t*)&addrlen);
        if(new_sock_fd < 0){
            perror("accept failed: ");
            exit(1);
        }

        //게임 진행을 위해 자식 프로세스 생성
        int c_pid = fork();
        if(c_pid==0){ // 자식 프로세스
            if(child_process_num == clientA){
                printf("child(%d) joined.\n", clientA);
                struct socket_msg msg; // client와 정보를 주고 받을 구조체
                struct gameInfo playerInfo; // player의 게임정보
                struct gameInfo receivingInfo; // 부모 프로세스로에게 전달받을 게임 정보
                child_client_sock = new_sock_fd; // 시그널 사용을 위해 sock 전역변수에 저장

                signal(SIGINT, signalHandler); // 승리 시그널
                signal(SIGQUIT, signalHandler); //패배 시그널
                signal(SIGILL, signalHandler); //무승부 시그널

                // 초기 정보 수신
                msgrcv(msqid_p2c_0, &playerInfo, sizeof(struct gameInfo), 0 , 0);
                msg.flag = 0;
                sprintf(msg.text,"Current HP: %d & You got this cards: { %c, %c }\n", playerInfo.userInfo.hp, playerInfo.cardInfo.cards[0], playerInfo.cardInfo.cards[1]);
                send_sock(new_sock_fd, msg);
                sprintf(msg.text, "Game will starts soon.\n--------------------------------------------------\n");
                send_sock(new_sock_fd, msg);
                
                while(1) {
                    signal(SIGUSR1, signalHandler);
                    pause();

                    int damage = 10;
                    sprintf(msg.text,"| HP: %d\n| Cards: { %c, %c }\n", playerInfo.userInfo.hp, playerInfo.cardInfo.cards[0], playerInfo.cardInfo.cards[1]);
                    send_sock(new_sock_fd, msg);
                    sprintf(msg.text, "| Received dammage: %d\n| Question: %s\n--------------------------------------------------\n", damage, questions[curQuestion]);
                    send_sock(new_sock_fd, msg);
                    
                    char* tmp = questions[curQuestion];
                    char question[10];
                    int user_input;
                    msg.flag = 1; // input flag
                    sprintf(msg.text, "Please select a card to put in the _ section of the question\n(ex) 0 or 1: ");
                    send_sock(new_sock_fd, msg);
                    msg = receive_sock(new_sock_fd);
                    user_input = msg.flag;
                    if(!(user_input == 0 || user_input == 1)) user_input = 0;
                    strcpy(question, tmp);
                    for(int index = 0; index < 4; index++) {
                        if(question[index*2+1] == '_') question[index*2+1] = playerInfo.cardInfo.cards[user_input];   
                    }
                    
                    msg.flag = 0;
                    sprintf(msg.text, "Final question: %s\n", question);
                    send_sock(new_sock_fd, msg);

                    // one client calculates question result and that reduce damage
                    // then msgsnd player inforamtion to main server
                    // other client behave the same way
                    // main server compares two client's hp
                    // the one who loses hp under 0
                    int block = calcQuestion(question);
                    if(block < 0) block = 0;
                    damage -= block;
                    if(damage < 0) damage = 0;
                    msg.flag = 0;
                    sprintf(msg.text, "block: %d\n", block);
                    send_sock(new_sock_fd, msg);
                    playerInfo.userInfo.hp -= damage;
                    if(playerInfo.userInfo.hp <= 0) playerInfo.userInfo.hp = 0;
                    sprintf(msg.text, "You got %d damage, now your hp: %d\n--------------------------------------------------\n", damage, playerInfo.userInfo.hp);
                    send_sock(new_sock_fd, msg);

                    // send player information to main server
                    msgsnd(msqid_c2p_0, &playerInfo, sizeof(struct gameInfo), 0);

                    // change question for next turn
                    curQuestion++;
                    msg.flag = 0;
                    sprintf(msg.text, "Your turn is over. Waiting for the next turn.\n--------------------------------------------------\n");
                    send_sock(new_sock_fd, msg); // 턴 종료 후 대기
                }
            }
            else if(child_process_num == clientB) {
                printf("child(%d) joined.\n", clientB);
                struct socket_msg msg; // client와 정보를 주고 받을 구조체
                struct gameInfo playerInfo; // player의 게임정보
                struct gameInfo receivingInfo; // 부모 프로세스로에게 전달받을 게임 정보
                child_client_sock = new_sock_fd; // 시그널 사용을 위해 sock 전역변수에 저장

                signal(SIGINT, signalHandler); // 승리 시그널
                signal(SIGQUIT, signalHandler); //패배 시그널
                signal(SIGILL, signalHandler); //무승부 시그널

                // 초기 정보 수신
                msgrcv(msqid_p2c_1, &playerInfo, sizeof(struct gameInfo), 0 , 0);
                msg.flag = 0;
                sprintf(msg.text,"Current HP: %d & You got this cards: { %c, %c }\n", playerInfo.userInfo.hp, playerInfo.cardInfo.cards[0], playerInfo.cardInfo.cards[1]);
                send_sock(new_sock_fd, msg);
                sprintf(msg.text, "Game will starts soon.\n--------------------------------------------------\n");
                send_sock(new_sock_fd, msg);
                
                while(1) {
                    signal(SIGUSR1, signalHandler);
                    pause();

                    int damage = 10;
                    sprintf(msg.text,"| HP: %d\n| Cards: { %c, %c }\n", playerInfo.userInfo.hp, playerInfo.cardInfo.cards[0], playerInfo.cardInfo.cards[1]);
                    send_sock(new_sock_fd, msg);
                    sprintf(msg.text, "| Received dammage: %d\n| Question: %s\n--------------------------------------------------\n", damage, questions[curQuestion]);
                    send_sock(new_sock_fd, msg);
                    
                    char* tmp = questions[curQuestion];
                    char question[10];
                    int user_input;
                    msg.flag = 1; // input flag
                    sprintf(msg.text, "Please select a card to put in the _ section of the question\n(ex) 0 or 1: ");
                    send_sock(new_sock_fd, msg);
                    msg = receive_sock(new_sock_fd);
                    user_input = msg.flag;
                    if(!(user_input == 0 || user_input == 1)) user_input = 0;
                    strcpy(question, tmp);
                    for(int index = 0; index < 4; index++) {
                        if(question[index*2+1] == '_') question[index*2+1] = playerInfo.cardInfo.cards[user_input];   
                    }
                    
                    msg.flag = 0;
                    sprintf(msg.text, "Final question: %s\n", question);
                    send_sock(new_sock_fd, msg);

                    // one client calculates question result and that reduce damage
                    // then msgsnd player inforamtion to main server
                    // other client behave the same way
                    // main server compares two client's hp
                    // the one who loses hp under 0
                    int block = calcQuestion(question);
                    if(block < 0) block = 0;
                    damage -= block;
                    if(damage < 0) damage = 0;
                    msg.flag = 0;
                    sprintf(msg.text, "block: %d\n", block);
                    send_sock(new_sock_fd, msg);
                    playerInfo.userInfo.hp -= damage;
                    if(playerInfo.userInfo.hp <= 0) playerInfo.userInfo.hp = 0;
                    sprintf(msg.text, "You got %d damage, now your hp: %d\n--------------------------------------------------\n", damage, playerInfo.userInfo.hp);
                    send_sock(new_sock_fd, msg);

                    // send player information to main server
                    msgsnd(msqid_c2p_1, &playerInfo, sizeof(struct gameInfo), 0);

                    // change question for next turn
                    curQuestion++;
                    msg.flag = 0;
                    sprintf(msg.text, "Your turn is over. Waiting for the next turn.\n--------------------------------------------------\n");
                    send_sock(new_sock_fd, msg); // 턴 종료 후 대기
                }
            }
        }
        else{ // 부모 프로세스
            // 첫번째 자식 프로세스의 pid 저장
            if (child_process_num == clientA) pid_0 = c_pid;
            // 두번째 자식 프로세스의 pid 저장
            else if (child_process_num == clientB) pid_1 = c_pid; 

            child_process_num ++;
            isGameInProgress = 1;
            close(new_sock_fd);// 부모 프로세스는 새로운 소켓을 유지할 필요 없음.

    /*********************************************************
     5. 게임 시작 **********************************************
    **********************************************************/
            if (child_process_num >= 2) {// 2명의 Client가 접속하면 게임 시작
                while(isGameInProgress){ //반복되는 게임 동작
                    printf("current turn: %d\n", turnCount);

                    ////////////////////////////////////////////////////////////////////////
                    // Player 0의 차례 //////////////////////////////////////////////////////
                    ////////////////////////////////////////////////////////////////////////
                    kill(pid_0, SIGUSR1);
                    printf("Player 0's turn\n");
                    msgrcv(msqid_c2p_0, &rcv_info0, sizeof(struct gameInfo), 0 , 0);
                    printf("cur player %d's hp: %d\n", clientA, rcv_info0.userInfo.hp);

                    ////////////////////////////////////////////////////////////////////////
                    // Player 1의 차례 //////////////////////////////////////////////////////
                    ////////////////////////////////////////////////////////////////////////
                    kill(pid_1, SIGUSR1);
                    printf("Player 1's turn\n");
                    msgrcv(msqid_c2p_1, &rcv_info1, sizeof(struct gameInfo), 0 , 0);
                    printf("cur player %d's hp: %d\n", clientB, rcv_info1.userInfo.hp);

                    ////////////////////////////////////////////////////////////////////////
                    // Compare two client's hp & check end condition ///////////////////////
                    ////////////////////////////////////////////////////////////////////////
                    turnCount--;

                    if(rcv_info0.userInfo.hp == 0 && rcv_info1.userInfo.hp != 0) {
                        kill(pid_0, SIGQUIT);
                        kill(pid_1, SIGINT);
                        printf("Player 1 Win\n");
                        isGameInProgress = 0;
                    }
                    if(rcv_info0.userInfo.hp != 0 && rcv_info1.userInfo.hp == 0) {
                        kill(pid_0, SIGINT);
                        kill(pid_1, SIGQUIT);
                        printf("Player 0 Win\n");
                        isGameInProgress = 0;
                    }
                    if((rcv_info0.userInfo.hp == 0 && rcv_info1.userInfo.hp == 0) || turnCount == 0) {
                        kill(pid_0, SIGILL);
                        kill(pid_1, SIGILL); 
                        printf("Player 1 & 2 Draw\n");
                        isGameInProgress = 0;
                    }
                }


                for(int i = 0; i < MAX_QNTC; i++) {
                    free(questions[i]);
                }
                
                exit(0); // 게임이 끝나면 프로세스 종료
            }
        }
    }
}

struct socket_msg receive_sock(int sock) {
    struct socket_msg msg;
    recv(sock, (struct socket_msg*)&msg, sizeof(msg),0);
    return msg;
}

void send_sock(int sock, struct socket_msg msg) {
    send(sock, (struct socket_msg*)&msg, sizeof(msg), 0);
}

struct gameInfo setPlayer(int clientNum) {
    struct gameInfo player;
    
    player.cardInfo.cardCount = 0;
    //player.cardInfo.rerollCount = 1;
    player.userInfo.hp = 10;
    player.userInfo.whois = clientNum;
    srand(time(NULL)*clientNum);

    for(int i = 0; i < MAX_PCC; i++) {
        player.cardInfo.cards[i] = symbol[rand()%symbolCount];
        player.cardInfo.cardCount++;
    }

    printf("Player %dth card: {%c, %c}, hp: %d\n", player.userInfo.whois, player.cardInfo.cards[0], player.cardInfo.cards[1], player.userInfo.hp);
    printf("===================================\n");
    return player;
}

int calcQuestion(char* question) {
    int result = question[0] - '0';

    for(int i = 0; i < 4; i++) {
        char temp = question[i*2+1];
        int value = question[i*2+2] - '0';

        switch(temp) {
            case '+':
                result += value;
                break;
            case '-':
                result -= value;
                break;
            case '*':
                break;
            case '/':
                break;    
        }
    }

    return result;
}

void signalHandler(int sig) {
    if(sig == SIGUSR1) {
        struct socket_msg msg;
        msg.flag = 0;
        sprintf(msg.text, "-----------------\n| Its your turn |\n-----------------\n");
        send_sock(child_client_sock, msg);
    }
    else {
        struct socket_msg msg;
        msg.flag = -99;
        // SIGINT: win, SIGQUIT: lose, SIGILL: tie
        if(sig == SIGINT) sprintf(msg.text, "You are the winner!\n");
        else if(sig == SIGQUIT) sprintf(msg.text, "You are the loser.\n");
        else sprintf(msg.text, "Its a tie.\n");
        // 게임 결과 전송
        send_sock(child_client_sock, msg);
        // socket close
        close(child_client_sock); 
        exit(0);
    }
}
