#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#define DEF_MODE S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH

void release(char buf[]);

int main(int argc, char *argv[]) {
    int fd;
    int pipeFds[2];
    char fileName1[50];
    char fileName2[50];
    unsigned int buf_size;
    char readBuf[1024];

    if (argc <= 2) {
        printf("Two file names are required from command line\n");
        return 1;
    }
    strcpy(fileName1, argv[1]);
    strcpy(fileName2, argv[2]);

    // Create a pipe (for sharing file contents)
    if (pipe(pipeFds) == -1) {
        perror("Pipe error");
        return 1;
    }

    if (fork() == 0) {
/*----------------------------------------------------------
    Child Process
----------------------------------------------------------*/
        fd = open(fileName1, O_CREAT | O_RDONLY, DEF_MODE);
        buf_size = sizeof(readBuf) / sizeof(readBuf[0]); //* n=1024 as readBuf is declared readBuf[1024]
        release(readBuf);
        int nbytes = read(fd, readBuf, buf_size);
        if (nbytes < 0) {
            perror("Error occurred while reading");
            exit(1);
        }
        readBuf[nbytes] = '\0';

        // pass data to main process
        write(pipeFds[1], readBuf, sizeof(readBuf));
        close(fd);

    } else {
/*----------------------------------------------------------
    Parent Process
----------------------------------------------------------*/
        char originalData[1024];
        char upperCaseData[1024];

        // get passed data from child process
        read(pipeFds[0], originalData, sizeof(originalData));

        // Convert to uppercase
        strcpy(upperCaseData, originalData);
        for (int i = 0; upperCaseData[i] != '\0'; i++) {
            if (upperCaseData[i] >= 'a' && upperCaseData[i] <= 'z') {
                upperCaseData[i] = upperCaseData[i] - 32;
            }
        }

        fd = open(fileName2, O_CREAT | O_WRONLY, DEF_MODE);

        if (fd < 0) {
            perror("Error in opening");
            return EXIT_FAILURE;
        }
        if (write(fd, upperCaseData, strlen(upperCaseData)) < 0) {
            perror("Error occurred while writing");
            return EXIT_FAILURE;
        }
        close(fd);
    }


    return 0;
}


void release(char buf[]) {
    for (size_t i = 0; i < 1024; i++) {
        buf[i] = '\0';
    }
}