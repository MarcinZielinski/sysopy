//
// Created by Mrz355 on 19.04.17.
//

#include "communication.h"

int main() {
    key_t server_key = ftok(getenv(SERVER_PATH), SERVER_ID); // create the special server key
    int server_qid = msgget(server_key, IPC_CREAT | 0600); // create the brand-new server queue

    while(1) {
        struct msqid_ds stats;
        struct message msg;
        msgctl(server_qid, IPC_STAT, &stats);

        printf("Number of messages in queue now: %zu\nPid of process who last sent: %d\n",stats.msg_qnum,stats.msg_lspid);
        if(msgrcv(server_qid, &msg, MAX_MSG_SIZE, 0, 0) < 0) break;
        printf("Message: \"%s\"\n\n",msg.value);
        sleep(1);
    }

    msgctl(server_qid, IPC_RMID, NULL);


    return 0;
}