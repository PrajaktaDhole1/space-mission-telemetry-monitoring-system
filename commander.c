#include "common.h"          
#include <sys/wait.h>      
#include <errno.h>         

/*
   Array to store process IDs of all child processes
   Total processes: 6 (2 satellites + 4 subsystems)
*/
pid_t children[6] = {0};

/*
   IPC identifiers (global so signal handler can access them)
*/
int shmid = -1;   // Shared Memory ID
int semid = -1;   // Semaphore ID
int msgid = -1;   // Message Queue ID


/*
   Signal Handler Function

   Handles:
   SIGUSR1 → Anomaly detected
   SIGUSR2 → Correction executed
   SIGINT  → Graceful shutdown (Ctrl + C)
*/
void handler(int sig) {

    /* Custom signal for anomaly detection */
    if(sig == SIGUSR1) {
        write(STDOUT_FILENO, "Anomaly detected!\n", 18);
    }

    /* Custom signal for correction completion */
    else if(sig == SIGUSR2) {
        write(STDOUT_FILENO, "Correction executed!\n", 21);
    }

    /* CTRL + C → graceful shutdown */
    else if(sig == SIGINT) {

        write(STDOUT_FILENO, "\nShutting down...\n", 18);

        /*
           Terminate all child processes
        */
        for(int i = 0; i < 6; i++) {
            if(children[i] > 0)
                kill(children[i], SIGTERM);
        }

        /*
           Wait for all child processes to exit
        */
        for(int i = 0; i < 6; i++) {
            if(children[i] > 0)
                waitpid(children[i], NULL, 0);
        }

        /*
           Cleanup IPC resources
        */

        /* Remove shared memory */
        if(shmid != -1)
            shmctl(shmid, IPC_RMID, NULL);

        /* Remove semaphore */
        if(semid != -1)
            semctl(semid, 0, IPC_RMID);

        /* Remove message queue */
        if(msgid != -1)
            msgctl(msgid, IPC_RMID, NULL);

        /* Remove FIFO (named pipe) */
        unlink(FIFO_NAME);

        write(STDOUT_FILENO, "Cleanup complete.\n", 18);

        exit(0);
    }
}


int main() {

    /*
       Create FIFO (named pipe) for logging system

       If FIFO already exists, ignore error
    */
    if(mkfifo(FIFO_NAME, 0666) == -1) {
        if(errno != EEXIST) {
            perror("mkfifo failed");
            exit(1);
        }
    }

    /*
       Create pipe for communication between satellites and receiver

       pipefd[0] → read end
       pipefd[1] → write end
    */
    int pipefd[2];
    if(pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    /*
       Create Shared Memory
       Used to store telemetry data of satellites
    */
    shmid = shmget(SHM_KEY, sizeof(mission_state_t), IPC_CREAT | 0666);
    if(shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    /* Initialize shared memory */
    mission_state_t *state = shmat(shmid, NULL, 0);
    memset(state, 0, sizeof(mission_state_t));
    shmdt(state);

    /*
       Create Semaphore
       Used for synchronization while accessing shared memory
    */
    semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if(semid == -1) {
        perror("semget failed");
        exit(1);
    }

    /*
       Create Message Queue
       Used for sending alerts from analyzer to recovery
    */
    msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if(msgid == -1) {
        perror("msgget failed");
        exit(1);
    }

    /*
       Initialize semaphore value to 1 (binary semaphore)
    */
    union semun sem_union;
    sem_union.val = 1;

    if(semctl(semid, 0, SETVAL, sem_union) == -1) {
        perror("semctl failed");
        exit(1);
    }

    /*
       Setup signal handlers using sigaction()
       (more reliable than signal())
    */
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGUSR1, &sa, NULL) == -1 ||
       sigaction(SIGUSR2, &sa, NULL) == -1 ||
       sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(1);
    }

    /*
       Convert pipe file descriptors to string
       These are passed as arguments to child processes
    */
    char write_fd[10], read_fd[10];
    sprintf(write_fd, "%d", pipefd[1]);
    sprintf(read_fd, "%d", pipefd[0]);

    /*
       Create child processes
    */

    /* Satellite 1 */
    if((children[0] = fork()) == 0) {
        close(pipefd[0]);  //close read end
        execl("./satellite", "satellite", "1", write_fd, NULL);
        perror("execl satellite 1 failed");
        exit(1);
    }

    /* Satellite 2 */
    if((children[1] = fork()) == 0) {
        close(pipefd[0]);  //close read end
        execl("./satellite", "satellite", "2", write_fd, NULL);
        perror("execl satellite 2 failed");
        exit(1);
    }

    /* Telemetry Receiver */
    if((children[2] = fork()) == 0) {
        close(pipefd[1]);  //close write end
        execl("./receiver", "receiver", read_fd, NULL);
        perror("execl receiver failed");
        exit(1);
    }

    /* Orbit Analyzer */
    if((children[3] = fork()) == 0) {
        execl("./analyzer", "analyzer", NULL);
        perror("execl analyzer failed");
        exit(1);
    }

    /* Fault Recovery */
    if((children[4] = fork()) == 0) {
        execl("./recovery", "recovery", NULL);
        perror("execl recovery failed");
        exit(1);
    }

    /* Data Recorder */
    if((children[5] = fork()) == 0) {
        execl("./recorder", "recorder", NULL);
        perror("execl recorder failed");
        exit(1);
    }

    /*
       Parent process (Mission Commander)
    */
    close(pipefd[0]);
    close(pipefd[1]);
    printf("Mission Commander started. Press CTRL+C to stop.\n");

    /*
       Wait indefinitely for signals
    */
    while(1)
        pause();

    return 0;
}                                                              
