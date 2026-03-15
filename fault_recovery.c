#include "common.h"   // Includes common structures, IPC keys, and required libraries

int main() {

    /*
       Access the existing message queue created by Mission Commander.
       This queue is used to receive alert messages from Orbit Analyzer.
    */
    int msgid = msgget(MSG_KEY, 0666);

    /*
       Structure to store the received alert message
    */
    alert_msg_t alert;

    /*
       Infinite loop to continuously listen for alerts
    */
    while(1) {

        /*
           Receive alert message from the message queue.

           msgid  → message queue ID
           &alert → structure where message will be stored
           sizeof(alert.msg_text) → size of message data
           1 → receive messages of type 1
           0 → blocking call (wait until message arrives)
        */
        msgrcv(msgid, &alert, sizeof(alert.msg_text), 1, 0);

        /*
           Print recovery message on terminal
           Example: "Recovery: Battery low in Satellite 1"
        */
        printf("Recovery: %s\n", alert.msg_text);

        /*
           Send signal to parent process (Mission Commander)
           indicating that an anomaly was detected.

           getppid() returns parent process ID.
           SIGUSR1 is used as custom alert signal.
        */
        kill(getppid(), SIGUSR1);
    }
}
