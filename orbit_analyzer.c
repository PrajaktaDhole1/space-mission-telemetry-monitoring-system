#include "common.h"   

int main() {

    /*
       Access the shared memory segment created by the Mission Commander.
       This shared memory contains telemetry data of all satellites.
    */
    int shmid = shmget(SHM_KEY, sizeof(mission_state_t), 0666);

    /*
       Access the message queue used for sending alerts.
       This queue will send anomaly alerts to the Fault Recovery process.
    */
    int msgid = msgget(MSG_KEY, 0666);

    /*
       Attach the shared memory to this process.
       'state' now points to the shared memory region.
    */
    mission_state_t *state = shmat(shmid, NULL, 0);

    /*
       Structure used to send alert messages through message queue
    */
    alert_msg_t alert;

    /*
       Infinite loop to continuously monitor satellite telemetry data
    */
    while(1) {

        /*
           Wait for 3 seconds before checking telemetry again.
           This prevents CPU overuse and repeated alerts too quickly.
        */
        sleep(3);

        /*
           Loop through all satellites stored in shared memory
        */
        for(int i = 0; i < MAX_SAT; i++) {

            /*
               Check if battery level of satellite is below threshold
               (simulate anomaly detection)
            */
            if(state->satellites[i].battery < 20) {

                /*
                   Set message type (required for message queue)
                */
                alert.msg_type = 1;

                /*
                   Create alert message describing the problem
                */
                sprintf(alert.msg_text,
                        "Battery low in Satellite %d", i + 1);

                /*
                   Send alert message to the message queue
                   This message will be received by the Fault Recovery process
                */
                msgsnd(msgid, &alert,
                       sizeof(alert.msg_text), 0);
            }
        }
    }

    
    return 0;
}
