#include "common.h"
#include <errno.h>    

/*
   Main function of Fault Recovery process

   This process:
   - Receives anomaly alerts from Orbit Analyzer (via message queue)
   - Displays recovery message
   - Sends alert data to Recorder via FIFO
   - Notifies Mission Commander using signal
*/
int main() {

    /*
       Access existing message queue
       (created by Mission Commander)
    */
    int msgid = msgget(MSG_KEY, 0666);
    if(msgid < 0) {
        perror("msgget failed");
        exit(1);
    }

    /*
       Structure to store received alert message
    */
    alert_msg_t alert;

    /*
       Open FIFO (named pipe) for writing

       This FIFO sends alert data to Recorder process
    */
    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    if(fifo_fd < 0) {
        perror("FIFO open failed");
        exit(1);
    }

    /*
       Infinite loop to continuously handle alerts
    */
    while(1) {

        /*
           Receive alert message from message queue

           msgid  → message queue ID
           &alert → buffer to store message
           sizeof(alert.msg_text) → size of message data
           1 → receive only messages of type 1
           0 → blocking mode (wait until message arrives)
        */
        if(msgrcv(msgid, &alert, sizeof(alert.msg_text), 1, 0) < 0) {
            perror("msgrcv failed");
            continue;
        }

        /*
           Display recovery message on terminal
        */
        printf("Recovery: %s\n", alert.msg_text);

        /*
           Send alert message to Recorder via FIFO

           This will be logged into mission_log.txt
        */
        if(write(fifo_fd, alert.msg_text, strlen(alert.msg_text)) < 0) {
            perror("FIFO write failed");
        }

        /*
           Add newline for better readability in log file
        */
        write(fifo_fd, "\n", 1);

        /*
           Send signal to Mission Commander

           getppid() → parent process ID
           SIGUSR1 → indicates anomaly detected
        */
        kill(getppid(), SIGUSR1);
    }

    return 0;
}
