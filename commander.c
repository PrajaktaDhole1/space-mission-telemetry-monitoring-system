#include "common.h"
#include <sys/wait.h>

pid_t children[6] = {0};

/* IPC identifiers (made global so the signal handler can clean them) */
int shmid = -1;
int semid = -1;
int msgid = -1;

/* Signal handler */
void handler(int sig) {

    if(sig == SIGUSR1) {
        printf("Anomaly detected!\n");
    }

    else if(sig == SIGUSR2) {
        printf("Correction executed!\n");
    }

    else if(sig == SIGINT) {

        printf("\nShutting down system...\n");

        /* terminate all children */
        for(int i = 0; i < 6; i++) {
            if(children[i] > 0) {
                kill(children[i], SIGTERM);
            }
        }

        /* wait for all children */
        for(int i = 0; i < 6; i++) {
            if(children[i] > 0) {
                waitpid(children[i], NULL, 0);
            }
        }

        /* clean IPC resources */
        if(shmid != -1)
            shmctl(shmid, IPC_RMID, NULL);

        if(semid != -1)
            semctl(semid, 0, IPC_RMID);

        if(msgid != -1)
            msgctl(msgid, IPC_RMID, NULL);

        unlink(FIFO_NAME);

        printf("All processes terminated. Resources cleaned.\n");

        exit(0);
    }
}

int main() {

    /* Create FIFO */
    mkfifo(FIFO_NAME, 0666);

    /* Create Pipe */
    int pipefd[2];
    pipe(pipefd);

    /* Create Shared Memory */
    shmid = shmget(SHM_KEY, sizeof(mission_state_t), IPC_CREAT | 0666);

    /* Create Semaphore */
    semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);

    /* Create Message Queue */
    msgid = msgget(MSG_KEY, IPC_CREAT | 0666);

    /* Initialize semaphore */
    union semun sem_union;
    sem_union.val = 1;
    semctl(semid, 0, SETVAL, sem_union);

    /* Setup signal handlers */
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    /* pass pipe write descriptor to satellites */
    char write_fd[10];
    sprintf(write_fd, "%d", pipefd[1]);

    /* Satellite 1 */
    if((children[0] = fork()) == 0) {
        execl("./satellite", "satellite", "1", write_fd, NULL);
        exit(0);
    }

    /* Satellite 2 */
    if((children[1] = fork()) == 0) {
        execl("./satellite", "satellite", "2", write_fd, NULL);
        exit(0);
    }

    /* Telemetry Receiver */
    if((children[2] = fork()) == 0) {
        execl("./receiver", "receiver", NULL);
        exit(0);
    }

    /* Orbit Analyzer */
    if((children[3] = fork()) == 0) {
        execl("./analyzer", "analyzer", NULL);
        exit(0);
    }

    /* Fault Recovery */
    if((children[4] = fork()) == 0) {
        execl("./recovery", "recovery", NULL);
        exit(0);
    }

    /* Data Recorder */
    if((children[5] = fork()) == 0) {
        execl("./recorder", "recorder", NULL);
        exit(0);
    }

    printf("Mission Commander started. Press CTRL+C to shutdown.\n");

    /* Parent waits indefinitely */
    while(1)
        pause();

    return 0;
}
