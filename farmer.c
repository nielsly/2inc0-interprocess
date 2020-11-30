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

#define STUDENT_NAME_1 "niels_gorter"
#define STUDENT_NAME_2 "ruben_wolters"

static char mq_name1[80];
static char mq_name2[80];

void open_queues();
void close_queues();

int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        fprintf(stderr, "%s: invalid arguments\n", argv[0]);
    }

    // TODO:
    //  * create the message queues (see message_queue_test() in interprocess_basic.c)
    static mqd_t mq_fd_request;
    static mqd_t mq_fd_response;
    static MQ_REQUEST_MESSAGE req;
    static MQ_RESPONSE_MESSAGE rsp;

    sprintf(mq_name1, "/mq_request_%s_%d", STUDENT_NAME_1, getpid());
    sprintf(mq_name2, "/mq_response_%s_%d", STUDENT_NAME_2, getpid());

    open_queues(&mq_fd_request, &mq_fd_response);

    //  * create the child processes (see process_test() and message_queue_test())
    static pid_t processID[NROF_WORKERS];
    for (int i = 0; i < NROF_WORKERS; i++)
    {
        processID[i] = fork();

        if (processID[i] < 0)
        {
            perror("fork() failed");
            exit(1);
        }
        else if (processID[i] == 0)
        {
            execlp("./worker", "./worker", mq_name1, mq_name2, NULL);
            perror("execlp() failed");
        }
    }

    //  * do the farming
    // declare vars to keep track of time
    struct timeval stop, start;
    gettimeofday(&start, NULL);
    // declare vars denoting messages currently in queue, jobs completed, index of the hash list
    // input char for current job, list of result strings
    unsigned int active_messages = 0, jobs_finished = 0, hash_index = 0;
    char input_char = ALPHABET_START_CHAR, result_list[MD5_LIST_NROF][MAX_MESSAGE_LENGTH + 1];
    // loop until done with jobs
    while (jobs_finished < JOBS_NROF)
    {
        // check if able to send a message and not done sending jobs
        if (active_messages < MQ_MAX_MESSAGES && input_char <= ALPHABET_END_CHAR)
        {
            // set request parameters
            req.hash_index = hash_index;
            req.input_hash = md5_list[hash_index];
            req.input_char = input_char;
            req.alphabet_start = ALPHABET_START_CHAR;
            req.alphabet_end = ALPHABET_END_CHAR;

            // send out job and increment active message counter
            mq_send(mq_fd_request, (char *)&req, sizeof(req), 0);
            active_messages++;
#ifdef DEBUG
            fprintf(stderr, "Farmer sent job %d%c\n", req.hash_index, req.input_char);
#endif // DEBUG

            // get next hash
            if (hash_index < MD5_LIST_NROF - 1)
            {
                // increment index if not on final hash of list
                hash_index++;
            }
            else
            {
                // else set index to 0 and go to next job input character
                hash_index = 0;
                input_char++;
            }
        }
        else
        {
            // if unable to send messages or done sending jobs
            // receive the next message, decrement active message counter and increment finished jobs counter
            mq_receive(mq_fd_response, (char *)&rsp, sizeof(rsp), NULL);
            active_messages--;
            jobs_finished++;
            fprintf(stderr, "%d finished job%s\n", jobs_finished, (jobs_finished > 1 ? "s" : ""));
#ifdef DEBUG
            fprintf(stderr, "Farmer received job %d%s\n", rsp.hash_index, rsp.result);
#endif // DEBUG

            // if the result's first byte is not the null character, correct input was found,
            // so add it to the results list at the index of the original hash
            if (rsp.result[0])
            {
                strcpy(result_list[rsp.hash_index], rsp.result);
            }
        }
    } // when done sending and receiving all jobs

    // get time of completion of all jobs
    gettimeofday(&stop, NULL);
    double diff = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
    fprintf(stderr, "Doing all jobs took %ldh %dm %ds %lfms.\n", (long)diff / 3600l, (int)((long)diff % 3600l) / 60, (int)((long)diff % 60), (diff - (int)diff) * 1000.0);

    // print the list of found inputs
    for (int i = 0; i < MD5_LIST_NROF; i++)
    {
        fprintf(stdout, "'%s'\n", result_list[i]);
    }

    // set the input char to the null character, indicating the children receiving the request should kill themselves
    req.input_char = '\0';
    for (int i = 0; i < NROF_WORKERS; i++)
    {
        // send request to kill themselves to all workers
        mq_send(mq_fd_request, (char *)&req, sizeof(req), 0);
#ifdef DEBUG
        fprintf(stderr, "Killing child %d\n", i);
#endif // DEBUG
    }

    //  * wait until the chilren have been stopped (see process_test())
    for (int j = 0; j < NROF_WORKERS; j++)
    {
        waitpid(processID[j], NULL, 0);
    }

    //  * clean up the message queues (see message_queue_test())
    close_queues(&mq_fd_request, &mq_fd_response);

    // Important notice: make sure that the names of the message queues contain your
    // student name and the process id (to ensure uniqueness during testing)

    return (0);
}

void open_queues(mqd_t *req_q, mqd_t *rsp_q)
{
    struct mq_attr attr;
    attr.mq_maxmsg = MQ_MAX_MESSAGES;
    attr.mq_msgsize = sizeof(MQ_REQUEST_MESSAGE);
    *req_q = mq_open(mq_name1, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);

    attr.mq_maxmsg = MQ_MAX_MESSAGES;
    attr.mq_msgsize = sizeof(MQ_RESPONSE_MESSAGE);
    *rsp_q = mq_open(mq_name2, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);
}

void close_queues(mqd_t *req_q, mqd_t *rsp_q)
{
    mq_close(*req_q);
    mq_close(*rsp_q);
    mq_unlink(mq_name1);
    mq_unlink(mq_name2);
}