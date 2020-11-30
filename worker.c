/* 
 * Operating Systems <2INCO> Practical Assignment
 * Interprocess Communication
 *
 * Niels Gorter (1332678)
 * Ruben Wolters (1342355)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * Extra steps can lead to higher marks because we want students to take the initiative. 
 * These extra steps must be agreed with the tutor before delivery.
 */

#include <time.h> // for time()
#include <complex.h>
#include "md5s.h"
#include "common.h"

static void rsleep(int t);

static void md5_brute_forcer(uint128_t *input_hash, char string[], int cur_length, char *alphabet_start, char *alphabet_end, bool *not_found, char *result);

void open_queues(mqd_t *req_q, mqd_t *rsp_q, char *req_q_name[80], char *rsp_q_name[80]);

void close_queues(mqd_t *req_q, mqd_t *rsp_q);

int main(int argc, char *argv[])
{
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the two message queues (whose names are provided in the arguments)
    mqd_t mq_fd_request;
    mqd_t mq_fd_response;
    MQ_REQUEST_MESSAGE req;
    MQ_RESPONSE_MESSAGE rsp;

    open_queues(&mq_fd_request, &mq_fd_response, &argv[1], &argv[2]);

    // declare vars to keep track of job computation time
    struct timeval stop, start;

    // * repeatingly:
    while (true)
    {
        // - read from a message queue the new job to do
        mq_receive(mq_fd_request, (char *)&req, sizeof(req), NULL);
#ifdef DEBUG
        fprintf(stderr, "Worker received job %d%c\n", req.hash_index, req.input_char);
#endif // DEBUG

        // - wait a random amount of time (e.g. rsleep(10000);)
        rsleep(10000);
        // - do that job
        // get start time
        gettimeofday(&start, NULL);
        // initially set the results first char to the string terminator
        rsp.result[0] = '\0';
        // until there are no more tasks to do
        if (req.input_char)
        {
            // set response hash index
            rsp.hash_index = req.hash_index;
            // initialise string containing only input char and string terminator
            // and a boolean to check if we have found the string
            char start_string[2] = {req.input_char, '\0'};
            bool not_found = true;
            //find and possibly set response string, else string remains empty
            md5_brute_forcer(&req.input_hash, start_string, 2, &req.alphabet_start, &req.alphabet_end, &not_found, rsp.result);
            // get end time
            gettimeofday(&stop, NULL);
            fprintf(stderr, "Job %d (%d,%c) computation finished in %lf seconds\n", (req.alphabet_end - req.alphabet_start + 1) * req.hash_index + req.input_char - req.alphabet_start + 1, req.hash_index, req.input_char, (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec));
        }
        else
        {
#ifdef DEBUG
            fprintf(stderr, "Worker: killing myself, bye!\n");
#endif // DEBUG
            break;
        }

        // - write the results to a message queue
        mq_send(mq_fd_response, (char *)&rsp, sizeof(rsp), 0);
#ifdef DEBUG
        fprintf(stderr, "Worker sent job %d%s\n", rsp.hash_index, rsp.result);
#endif // DEBUG
    }

    // * close the message queues
    close_queues(&mq_fd_request, &mq_fd_response);

    return 0;
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep(int t)
{
    static bool first_call = true;

    if (first_call == true)
    {
        srandom(time(NULL) % getpid());
        first_call = false;
    }
    usleep(random() % t);
}

static void md5_brute_forcer(uint128_t *input_hash, char string[], int cur_length, char *alphabet_start, char *alphabet_end, bool *not_found, char *result)
{
    // get hash from input string
    uint128_t check_hash = md5s(string, cur_length - 1);

    // if it is equal to the hash we're trying to crack
    if (check_hash == *input_hash)
    {
        // tell main to stop execution
        *not_found = false;
        // copy input string to the response result
        strcpy(result, string);
    }
    // else if we have not reached the maximum message length
    // max_length is max amount of non-\0 chars, so we check <= as cur_length includes \0
    else if (cur_length <= MAX_MESSAGE_LENGTH)
    {
        // loop over all characters in alphabet until string is found or end is reached
        for (char i = *alphabet_start; i <= *alphabet_end && *not_found; i++)
        {
            // create new, longer string and copy the current string into it
            char next_string[cur_length + 1];
            strcpy(next_string, string);

            // add character i and the string terminator
            next_string[cur_length - 1] = i;
            next_string[cur_length] = '\0';

            // call self recursively
            md5_brute_forcer(input_hash, next_string, cur_length + 1, alphabet_start, alphabet_end, not_found, result);
        }
    }
}

void open_queues(mqd_t *req_q, mqd_t *rsp_q, char *req_q_name[80], char *rsp_q_name[80])
{
    *req_q = mq_open(*req_q_name, O_RDONLY);
    *rsp_q = mq_open(*rsp_q_name, O_WRONLY);
}

void close_queues(mqd_t *req_q, mqd_t *rsp_q)
{
    mq_close(*req_q);
    mq_close(*rsp_q);
}