#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#define DEF_MODE S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH

int main() {
    int fd;
    char fileName[] = "test.txt";
    char processA[] = "Hello! I'm process A\n";
    char processB[] = "Hello! I'm process B\n";

    int proj_id = 3;
    int semid;
    struct sembuf sembuf[2];
    key_t key;

    fd = open(fileName, O_CREAT | O_APPEND | O_TRUNC | O_WRONLY, DEF_MODE);
    if (fd < 0) {
        perror("Error in opening");
        return EXIT_FAILURE;
    }

    key = ftok(fileName, proj_id);
    if (key == -1) {
        perror("Cannot generate a key");
        exit(EXIT_FAILURE);
    }

    semid = semget(key, 2, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("Cannot create a semaphore (A)");
        exit(EXIT_FAILURE);
    }

    sembuf[0].sem_num = 0;
    sembuf[0].sem_flg = SEM_UNDO;
    sembuf[1].sem_num = 1;
    sembuf[1].sem_flg = SEM_UNDO;

    if (fork() == 0) {
/*----------------------------------------------------------
    Process A
----------------------------------------------------------*/
        semctl(semid, 0, SETVAL, 1);
        semctl(semid, 1, SETVAL, 0);

        for (int i = 0; i < 3; ++i) {
            sembuf[0].sem_op = -1;
            semop(semid, &sembuf[0], 1);

            printf("Process A: writing to %s file\n", fileName);
            write(fd, processA, strlen(processA));
            sleep(1);

            sembuf[1].sem_op = 1;
            semop(semid, &sembuf[1], 1);
        }
        semctl(semid, 2, IPC_RMID);

    } else {
/*----------------------------------------------------------
    Process B
----------------------------------------------------------*/
        for (int i = 0; i < 3; ++i) {
            sembuf[1].sem_op = -1;
            semop(semid, &sembuf[1], 1);

            printf("Process B: writing to %s file\n", fileName);
            write(fd, processB, strlen(processB));
            sleep(1);

            sembuf[0].sem_op = 1;
            semop(semid, &sembuf[0], 1);
        }
        semctl(semid, 2, IPC_RMID);
    }

    close(fd);
    return 0;
}