/* 
 * Operating Systems <2INCO> Practical Assignment
 * Interprocess Communication
 *
 * Ruben Wolters (1342355)
 * Niels Gorter (1332678)
 * 
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * Extra steps can lead to higher marks because we want students to take the initiative. 
 * These extra steps must be agreed with the tutor before delivery.
 */

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "settings.h"
#include "common.h"

#define STUDENT_NAME_1 "Niels_Gorter"
#define STUDENT_NAME_2 "Ruben_Wolters"

static char mq_name1[80];
static char mq_name2[80];

int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        fprintf(stderr, "%s: invalid arguments\n", argv[0]);
    }

    // TODO:
    //  * create the message queues (see message_queue_test() in interprocess_basic.c)
    static pid_t processID[NROF_WORKERS];
    static mqd_t mq_fd_request;
    static mqd_t mq_fd_response;

    static MQ_REQUEST_MESSAGE req;
    static MQ_RESPONSE_MESSAGE rsp;
    struct mq_attr attr;

    sprintf(mq_name1, "/mq_request_%s_%d", STUDENT_NAME_1, getpid());
    sprintf(mq_name2, "/mq_response_%s_%d", STUDENT_NAME_2, getpid());

    attr.mq_maxmsg = MQ_MAX_MESSAGES;
    attr.mq_msgsize = sizeof(MQ_REQUEST_MESSAGE);
    mq_fd_request = mq_open(mq_name1, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);

    attr.mq_maxmsg = MQ_MAX_MESSAGES;
    attr.mq_msgsize = sizeof(MQ_RESPONSE_MESSAGE);
    mq_fd_response = mq_open(mq_name2, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);

    //  * create the child processes (see process_test() and message_queue_test())
    for (int i=0; i < NROF_WORKERS; i++) {
        processID[i] = fork();
        
        if (processID[i] < 0) {
            perror("fork() failed");
            exit(1);
        } else if (processID[i] == 0){
            execlp("./worker", "./worker", mq_name1, mq_name2, NULL);
                perror("execlp() failed");
        }
    }

    //  * do the farming
    int nrof_messages = 0;
    int nrof_messages_received = 0;
    char current_char = ALPHABET_START_CHAR;
    int md5_list_index = 0;
    char output[MD5_LIST_NROF][MAX_MESSAGE_LENGTH + 5];
// Wait until all messages are received
    while (nrof_messages_received < JOBS_NROF) {
        // Start new job
        if (nrof_messages < MQ_MAX_MESSAGES && current_char <= ALPHABET_END_CHAR ) {
            req.hash_index = md5_list_index;
            req.input_hash = md5_list[md5_list_index];
            req.input_char = current_char;
            req.alphabet_start = ALPHABET_START_CHAR;
            req.alphabet_end = ALPHABET_END_CHAR;
            req.max_length = MAX_MESSAGE_LENGTH;

            // Send new message
            mq_send(mq_fd_request, (char *) &req, sizeof(req), 0);
            #ifdef DEBUG
            fprintf(stderr,"Farmer sent job %d%c\n", req.hash_index, req.input_char);
            #endif // DEBUG
            nrof_messages++;

            if (md5_list_index < MD5_LIST_NROF-1) {
                md5_list_index++;
            } else {
                md5_list_index = 0;
                current_char++;
            }
        } else {
            // Read result
            mq_receive(mq_fd_response, (char *) &rsp, sizeof(rsp), NULL);
            #ifdef DEBUG
            fprintf(stderr,"Farmer received job %d%s\n", rsp.hash_index, rsp.result);
            #endif // DEBUG
            if (rsp.result[0]) {
                // Store the result in the output list
                sprintf(output[rsp.hash_index], "'%s'\n", rsp.result);
            }
            nrof_messages--;
            nrof_messages_received++;
        }
    }

    // Output
    for (int list_index = 0; list_index < MD5_LIST_NROF; list_index++) {
        fprintf(stdout, "%s", output[list_index]);
    }

    //  * wait until the children have been stopped (see process_test())
    req.max_length = 0;
    for (int i = 0; i < NROF_WORKERS; i++) {   
        mq_send(mq_fd_request, (char *) &req, sizeof(req), 0);
        #ifdef DEBUG
        fprintf(stderr,"Killing child %d\n", i);
        #endif // DEBUG
    }

    // Collect all pids of of dead children
    for (int j = 0; j < NROF_WORKERS; j++) {
        waitpid(processID[j], NULL, 0);
    }

    //  * clean up the message queues (see message_queue_test())
    mq_close(mq_fd_response);
    mq_close(mq_fd_request);
    mq_unlink(mq_name1);
    mq_unlink(mq_name2);

    // Important notice: make sure that the names of the message queues contain your
    // student name and the process id (to ensure uniqueness during testing)

    return (0);
}
