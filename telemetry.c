#include "common.h"   

int main() {

    /*
       Access existing shared memory created by Mission Commander.
       This shared memory stores telemetry data of all satellites.
    */
    int shmid = shmget(SHM_KEY, sizeof(mission_state_t), 0666);

    /*
       Access the semaphore used for synchronization.
       Semaphore ensures that only one process updates shared memory at a time.
    */
    int semid = semget(SEM_KEY, 1, 0666);

    /*
       Attach shared memory to this process.
       After attaching, 'state' points to shared memory where telemetry is stored.
    */
    mission_state_t *state = shmat(shmid, NULL, 0);

    /*
       Create a pipe for receiving telemetry data from satellite processes.
       pipefd[0] -> read end
       pipefd[1] -> write end
    */
    int pipefd[2];
    pipe(pipefd);

    /*
       Structure to temporarily store telemetry received from satellites.
    */
    telemetry_t data;

    /*
       Infinite loop to continuously receive telemetry data.
    */
    while(1) {

        /*
           Read telemetry data from pipe.
           Satellite processes write telemetry data to this pipe.
        */
        read(pipefd[0], &data, sizeof(data));

        /*
           Semaphore operations for synchronization
        */

        /*
           Lock operation (P operation)
           Decrease semaphore value by 1.
           If semaphore is 0, process will wait.
        */
        struct sembuf lock = {0, -1, 0};

        /*
           Unlock operation (V operation)
           Increase semaphore value by 1.
        */
        struct sembuf unlock = {0, 1, 0};

        /*
           Enter critical section
        */
        semop(semid, &lock, 1);

        /*
           Update shared memory with latest telemetry
           Store satellite data in its corresponding index
        */
        state->satellites[data.sat_id - 1] = data;

        /*
           Exit critical section
        */
        semop(semid, &unlock, 1);
    }
}
