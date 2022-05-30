#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>

#define SIZE 1024
#define SERVER 20195124

void signalHandler(int signum);
void writeToMem();
void readToMem();
int shmid;
key_t key = SERVER;

void printMenu() {
    int len = 7;
    char *menu[] = {
        "---------------",
        "| Select Menu |",
        "---------------",
        "| 1: sign up",
        "| 2: sign in",
        "| 3: exit",
        "---------------"
    };

    for(int i = 0; i < len; i++) {
       printf("%s\n", *(menu + i)); 
    }
}

void printSignUpMenu() {
    int len = 5;
    char *menu[] = {
        "Enter your id(more than 2 characters):\t",
        "Create a password(at least 10 English letters):\t",
        "Enter a username(more than 2 characters):\t"
    };
}

struct Player {
    char ID[20];
    char PW[20];
    char Name[20];
};

void main() {
    /*
    int menu = 0;
    while(1) {
        switch (menu) {
        case 0:
            printMenu();
            scanf("%d", &menu);
            printf("========================\n");
            printf("You select this menu: %d\n", menu);
            break;
        // sign up
        case 1:

            break;
        // sign in
        case 2:

            break;
        // exit
        case 3:
            exit(0);
            break;
        default:
            printf("NO SUCH MENU HERE\n");
            menu = 0;
            break;
        }
        

    }
    */

    //readToMem();
}

void signalHandler(int signum) {
    if(shmctl(shmid, IPC_RMID, 0) == -1) {
        printf("shmctl error\n");
        exit(0);
    }

    printf("===================================\n");
    printf("Got SIGINT, it will ends\n");
    exit(0);
}

void writeToMem() {
    void *shmaddr;

    shmid = shmget(key, SIZE, IPC_CREAT|0666);
    if(shmid == -1) {
        printf("shmget failed");
        exit(0);
    }
    shmaddr = shmat(shmid, (void *)0, 0);
    if(shmaddr == (void *)-1) {
        printf("shmat error\n");
        exit(0);
    }

    printf("Writing Hello message in the shared memory\n");
    strcpy((char*)shmaddr, "20195124");
    if(shmdt(shmaddr) == -1) {
        printf("shmdt error\n");
        exit(0);
    }

    signal(SIGINT, signalHandler);
    pause();
}

void readToMem() {
    int memID;
    void* memAddr;
    struct shmid_ds message;
    key_t key = SERVER;

    memID = shmget(key, SIZE, IPC_CREAT|0666);
    if(memID == -1) {
        printf("shmget failed\n");
        exit(0);
    }

    memAddr = shmat(memID, (void*)0, 0);
    if(memAddr == (void*)-1) {
        printf("shmat failed\n");
        exit(0);
    }

    printf("read data from shared memory as follows: %s\n", (char *)memAddr);

    if(shmctl(memID, IPC_STAT, &message) == -1) {
        printf("shmdt error\n");
        exit(0);
    }

    printf("=============================\n");
    printf("Sending SIGINT to the creator of the shared memory.\n");
    kill(message.shm_cpid, SIGINT);
}