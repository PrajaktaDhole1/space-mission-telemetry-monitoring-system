#include "common.h"   

int main() {

    /*
       Open the FIFO (named pipe) for reading.

       FIFO_NAME → name of the named pipe created by Mission Commander
       O_RDONLY  → open in read-only mode

       This FIFO receives mission data or alerts that need to be logged.
    */
    int fd = open(FIFO_NAME, O_RDONLY);

    /*
       Open a log file where all mission data will be stored.

       "mission_log.txt" → log file name

       Flags used:
       O_CREAT  → create file if it does not exist
       O_WRONLY → open file in write-only mode
       O_APPEND → append data at end of file

       0644 → file permissions
       Owner: read + write
       Others: read
    */
    int log_fd = open("mission_log.txt", O_CREAT | O_WRONLY | O_APPEND, 0644);

    /*
       Buffer to store incoming data from FIFO
    */
    char buffer[200];

    /*
       Infinite loop to continuously record incoming data
    */
    while(1) {

        /*
           Read data from FIFO

           fd → FIFO file descriptor
           buffer → where incoming data will be stored
           sizeof(buffer) → maximum bytes to read
        */
        int n = read(fd, buffer, sizeof(buffer));

        /*
           If data is received (n > 0),
           write that data to the mission log file.
        */
        if(n > 0)
            write(log_fd, buffer, n);
    }
}
