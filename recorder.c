#include "common.h"
#include <errno.h> 

/*
   Main function of Data Recorder

   This process:
   - Reads alert messages from FIFO (sent by Fault Recovery)
   - Writes them into a log file (mission_log.txt)
*/
int main() {

    /*
       Open (or create) log file

       O_CREAT  → create file if it doesn't exist
       O_WRONLY → open in write-only mode
       O_APPEND → append new data at the end

       0644 → file permissions
       Owner: read + write
       Others: read only
    */
    int log_fd = open("mission_log.txt", O_CREAT | O_WRONLY | O_APPEND, 0644);

    if(log_fd < 0) {
        perror("Log file open failed");
        exit(1);
    }

    /*
       Open FIFO (named pipe) for reading

       FIFO_NAME → must match the name created by commander
       O_RDONLY  → open in read-only mode

       Note:
       This call blocks until a writer (Fault Recovery) opens FIFO
    */
    int fd = open(FIFO_NAME, O_RDONLY);

    if(fd < 0) {
        perror("FIFO open failed");
        exit(1);
    }

    /*
       Buffer to store incoming data from FIFO
    */
    char buffer[200];

    /*
       Infinite loop to continuously read and log data
    */
    while(1) {

        /*
           Read data from FIFO

           fd → FIFO descriptor
           buffer → storage for incoming data
           sizeof(buffer) → max bytes to read
        */
        int n = read(fd, buffer, sizeof(buffer));

        /*
           If data received, write to log file
        */
        if(n > 0) {
            if(write(log_fd, buffer, n) != n) {
                perror("Log write failed");
            }
        }
        else if(n == 0) {
        // FIFO writer closed, reopen
        close(fd);
        fd = open(FIFO_NAME, O_RDONLY);
    }

    }

    return 0;
}
