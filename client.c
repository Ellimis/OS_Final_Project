#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>

#define SIZE 1024
// Server Key for communication with client 1 & client 2
#define K_C1 2019
#define K_C2 5124
#define DEFAULTKEY 20195124

void signalHandler(int signum);
void CS(int mode);
int memID;
// intial key for sign in before connect server
key_t key = DEFAULTKEY;

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

// +, -, *, /
// 0, 1, 2, 3, 4, 5, 6, 7, 8, 9

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

void main(int argc, char* argv[]) {
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

    printf("-----\n");
    printf("| 1 |\n");
    printf("-----\n");
    printf("-----\n");
    printf("| + |\n");
    printf("-----\n");


    //CS(0);
}

void signalHandler(int signum) {
    if(shmctl(memID, IPC_RMID, 0) == -1) {
        printf("shmctl error\n");
        exit(0);
    }

    printf("===================================\n");
    printf("Got SIGINT, it will ends\n");
    exit(0);
}

// Communication with Server
void CS(int mode) {
    if(key == DEFAULTKEY) {
        // check user id, pw with db
    }
    else if((key == K_C1) || (key == K_C2)) {
        void* memAddr;
        struct shmid_ds message;

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

        switch(mode) {
        // read 
        case 0:
            printf("read data from shared memory as follows: %s\n", (char *)memAddr);

            if(shmctl(memID, IPC_STAT, &message) == -1) {
                printf("shmdt error\n");
                exit(0);
            }

            printf("=============================\n");
            printf("Sending SIGINT to the creator of the shared memory.\n");
            kill(message.shm_cpid, SIGINT);        
            break;
        // write
        case 1:
            printf("Writing 20195124 message\n");
            strcpy((char*)memAddr, "20195124");

            if(shmdt(memAddr) == -1) {
                printf("shmdt error\n");
                exit(0);
            }

            signal(SIGINT, signalHandler);
            pause();
            break;
        }
    }
    else { exit(1); }
}
