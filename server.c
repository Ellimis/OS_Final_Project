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
#define Client1 0
#define Client2 1

void signalHandler(int signum);
void CC(int clientNum, int mode);
int memID;
key_t key[2] = { K_C1, K_C2 };

struct Player {
    char ID[20];
    char PW[20];
    char Name[20];
};

void main() {
    CC(Client1, 1);
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

// Communicate with Client 
void CC(int clientNum, int mode) {
    void* memAddr;
    struct shmid_ds message;

    memID = shmget(key[clientNum], SIZE, IPC_CREAT|0666);
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
