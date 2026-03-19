#include "common.h"
#include <errno.h>    

/*
   Main function of Orbit Analyzer

   This process:
   - Reads telemetry data from shared memory
   - Detects anomalies (low battery)
   - Sends alerts to Fault Recovery via message queue
*/
int main() {

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
       Access message queue used to send alerts
    */
    int msgid = msgget(MSG_KEY, 0666);
    if(msgid < 0) {
        perror("msgget failed");
        exit(1);
    }

    /*
       Attach shared memory to this process
       'state' now points to telemetry data
    */
    mission_state_t *state = shmat(shmid, NULL, 0);
    if(state == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    /*
       Structure used to send alert messages
    */
    alert_msg_t alert;

    /*
       Array to track whether alert already sent
       (prevents repeated alerts for same issue)
    */
    int alert_sent[MAX_SAT] = {0};

    /*
       Infinite loop to continuously monitor telemetry
    */
    while(1) {

        /*
           Wait for 3 seconds before next check
           (reduces CPU usage and avoids rapid alerts)
        */
        sleep(3);

        /*
           Check all satellites
        */
        for(int i = 0; i < MAX_SAT; i++) {

            /*
               If battery is below threshold AND
               alert not already sent
            */
            if(state->satellites[i].battery < 20 && !alert_sent[i]) {

                /*
                   Set message type (required for message queue)
                */
                alert.msg_type = 1;

                /*
                   Create alert message
                */
                sprintf(alert.msg_text, "Battery low in Satellite %d", i + 1);

                /*
                   Send alert to Fault Recovery process
                */
                if(msgsnd(msgid, &alert, sizeof(alert.msg_text), 0) < 0) {
                    perror("msgsnd failed");
                }

                /*
                   Mark alert as sent to avoid duplicates
                */
                alert_sent[i] = 1;
            }

            /*
               Reset alert flag if battery recovers
               (so future alerts can be sent again)
            */
            if(state->satellites[i].battery >= 20)
                alert_sent[i] = 0;
        }
    }

    return 0;
}
