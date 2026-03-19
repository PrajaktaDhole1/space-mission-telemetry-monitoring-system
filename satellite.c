#include "common.h"   
/*
   Global variables
*/

/* File descriptor of pipe (used to send telemetry data to receiver) */
int pipe_fd;

/* Satellite ID (passed from commander: 1 or 2) */
int sat_id;

/* Mutex to protect shared telemetry data between threads */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* Structure to hold telemetry data */
telemetry_t data;


/*
   Thread function: Generates telemetry data continuously

   This simulates:
   - Temperature sensor
   - Battery drain
   - Position changes
*/
void* telemetry_thread(void* arg) {

    while(1) {

        /* Simulate sensor update every 2 seconds */
        sleep(2);

        /* Lock mutex before modifying shared data */
        pthread_mutex_lock(&lock);

        /* Generate random temperature (0–99) */
        data.temperature = rand() % 100;

        /* Simulate battery drain */
        data.battery -= (rand() % 5);

        /* Prevent battery from going below 0 */
        if(data.battery < 0)
            data.battery = 0;

        /* Simulate satellite movement */
        data.pos_x += rand() % 10;
        data.pos_y += rand() % 10;

        /* Display telemetry data on terminal */
        printf("Satellite %d -> Temp: %.2f Battery: %.2f Position:(%.2f, %.2f)\n",
               sat_id,
               data.temperature,
               data.battery,
               data.pos_x,
               data.pos_y);

        fflush(stdout);
        /*
           Send telemetry data to receiver process via pipe

           pipe_fd → write end of pipe
           &data   → pointer to telemetry structure
           sizeof(data) → size of structure
        */
        if(write(pipe_fd, &data, sizeof(data)) != sizeof(data)) {
            perror("Pipe write failed");
        }

        /* Unlock mutex after updating data */
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}


/*
   Main function of satellite process

   Arguments received from commander:
   argv[1] → satellite ID
   argv[2] → pipe write file descriptor
*/
int main(int argc, char* argv[]) {

    /* Validate arguments */
    if(argc < 3) {
        printf("Usage: %s <sat_id> <pipe_fd>\n", argv[0]);
        exit(1);
    }

    /* Get satellite ID */
    sat_id = atoi(argv[1]);

    /* Get pipe file descriptor */
    pipe_fd = atoi(argv[2]);

    /* Initialize random number generator */
    srand(time(NULL) + sat_id);

    /* Initialize telemetry data */
    data.sat_id = sat_id;
    data.battery = 100;   // start with full battery
    data.pos_x = 0;
    data.pos_y = 0;

    /* Thread variable */
    pthread_t t;

    /* Create telemetry generation thread */
    if(pthread_create(&t, NULL, telemetry_thread, NULL) != 0) {
        perror("Thread creation failed");
        exit(1);
    }

    /* Wait for thread (runs infinitely) */
    pthread_join(t, NULL);

    return 0;
}
