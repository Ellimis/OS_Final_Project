#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // includes write, read, and close
#include <fcntl.h> // includes open
#include <string.h>

#define BUF_SIZE 1024

void createCard();

char buf[BUF_SIZE];

int main() {
    int fd, n;

    createCard();

    fd = open("CardInfo.txt", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if(fd < 0) {
        printf("failed to open file\n");
        exit(1);
    }

    // 251: total card form size
    n = write(fd, buf, 251);
    if(n < 0) {
        printf("failed to write file.\n");
        exit(1);
    }
    
    close(fd);
    return 0;
}

void createCard() {
    char symbol[14];
    char form[3][10];

    for(int i = 0; i < 10; i++) {
        symbol[i] = i + '0';
    }
    symbol[10] = '+';
    symbol[11] = '-';
    symbol[12] = '*';
    symbol[13] = '/';
    strcpy(form[0], "-----\n");
    strcpy(form[1], "|   |\n");
    strcpy(form[2], "-----\n");

    for(int i = 0; i < 14; i++) {
        form[1][2] = symbol[i];

        for(int j = 0; j < 3; j++) {
            strcat(buf, form[j]);
        }
    }
}