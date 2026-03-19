#include "common.h" 
#include <errno.h> 
/*
   Main function of Telemetry Receiver

   This process:
   - Receives telemetry data from satellites via pipe
   - Updates shared memory with latest data
   - Uses semaphore for synchronization
*/
int main(int argc, char* argv[]) {

    /*
       Check if pipe file descriptor is passed
    */
    if(argc < 2) {
        printf("Usage: %s <pipe_fd>\n", argv[0]);
        exit(1);
    }

    /*
       Get pipe read file descriptor from command line
       (passed by Mission Commander)
    */
    int pipe_fd = atoi(argv[1]);

    /*
       Access existing shared memory
       (created by Mission Commander)
    */
    int shmid = shmget(SHM_KEY, sizeof(mission_state_t), 0666);
    if(shmid < 0) {
        perror("shmget failed");
        exit(1);
    }

    /*
       Access semaphore used for synchronization
    */
    int semid = semget(SEM_KEY, 1, 0666);
    if(semid < 0) {
        perror("semget failed");
        exit(1);
    }

    /*
       Attach shared memory to this process
       'state' now points to shared memory
    */
    mission_state_t *state = shmat(shmid, NULL, 0);
    if(state == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    /*
       Structure to temporarily store incoming telemetry data
    */
    telemetry_t data;

    /*
       Infinite loop to continuously receive and update data
    */
    while(1) {

        /*
           Read telemetry data from pipe

           pipe_fd → read end of pipe
           &data   → buffer to store received data
        */
        int n = read(pipe_fd, &data, sizeof(data));

        /*
           If valid data received
        */
        if(n == sizeof(data)) {

            /*
               Semaphore operations for synchronization

               lock   → decrease semaphore (enter critical section)
               unlock → increase semaphore (exit critical section)
            */
            struct sembuf lock = {0, -1, 0};
            struct sembuf unlock = {0, 1, 0};

            /*
               Enter critical section
               Ensures only one process modifies shared memory
            */
            semop(semid, &lock, 1);

            /*
               Update shared memory with latest telemetry

               data.sat_id - 1 → index of satellite
            */
            state->satellites[data.sat_id - 1] = data;

            /*
               Exit critical section
            */
            semop(semid, &unlock, 1);
        }
    }

    return 0;
}
