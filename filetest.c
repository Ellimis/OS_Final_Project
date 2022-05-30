#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // includes write, read, and close
#include <fcntl.h> // includes open

#define BUF_SIZE 1024

int main() {
    int fd, n;
    char buf[BUF_SIZE];

    // open file
    // 0644 is allow read write row to file 
    fd = open("test.txt", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if(fd < 0) {
        printf("failed to open file\n");
        exit(1);
    }
    else printf("Opened fd(%d) file.\n", fd);

    // read & write
    // n is size of byte
    n = read(0, buf, BUF_SIZE); // read from command
    n = write(fd, buf, n); // write the string in buf into the file
    if(n < 0) {
        printf("failed to write file.\n");
        exit(1);
    }
    // set cursor at the first char(loc : SEEK_SET + 0)
    lseek(fd, 0, SEEK_SET);

    n = read(fd, buf, BUF_SIZE); // read from the file
    if(n == 0) {
        printf("the file is empty.\n");
        exit(1);
    }

    printf("==== test.txt ====\n");
    printf("%s\n", buf); // print the string in buf
    close(fd);
    return 0;
}