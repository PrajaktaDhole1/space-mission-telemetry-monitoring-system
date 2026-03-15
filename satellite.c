#include "common.h"     // Contains shared structures and constants used in project

// File descriptor for pipe (used to send telemetry data to receiver)
int pipe_fd;

// ID of the satellite (1 or 2)
int sat_id;

// Mutex for protecting shared telemetry data between threads
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Structure that stores telemetry data
telemetry_t data;


/*
   Thread function that simulates telemetry generation
   It runs continuously and sends sensor data through pipe
*/
void* telemetry_thread(void* arg) {

    while(1) {

        // simulate sensor update every 2 seconds
        sleep(2);

        // lock mutex before modifying shared telemetry data
        pthread_mutex_lock(&lock);

        // generate random temperature (0–99)
        data.temperature = rand() % 100;

        // decrease battery randomly (simulate battery drain)
        data.battery -= (rand() % 5);

        // prevent battery from going below 0
        if(data.battery < 0)
            data.battery = 0;

        // simulate satellite movement in orbit
        data.pos_x += rand() % 10;
        data.pos_y += rand() % 10;

        // print telemetry values for monitoring (useful in demo)
        printf("Satellite %d -> Temp:%.2f Battery:%.2f Position:(%.2f,%.2f)\n",
                sat_id,
                data.temperature,
                data.battery,
                data.pos_x,
                data.pos_y);

        // send telemetry data to receiver process through pipe
        write(pipe_fd, &data, sizeof(data));

        // unlock mutex after updating shared data
        pthread_mutex_unlock(&lock);
    }
}


/*
   Main function of satellite process
   It receives arguments from Mission Commander:
   argv[1] → satellite ID
   argv[2] → pipe file descriptor
*/
int main(int argc, char* argv[]) {

    // initialize random number generator
    srand(time(NULL));

    // get satellite ID from command line argument
    sat_id = atoi(argv[1]);

    // get pipe write file descriptor from command line
    pipe_fd = atoi(argv[2]);

    // initialize telemetry data
    data.sat_id = sat_id;
    data.battery = 100;   // satellites start with full battery
    data.pos_x = 0;
    data.pos_y = 0;

    // thread variable
    pthread_t t1;

    // create telemetry generation thread
    pthread_create(&t1, NULL, telemetry_thread, NULL);

    // wait for thread to finish (it never ends)
    pthread_join(t1, NULL);

    return 0;
}
