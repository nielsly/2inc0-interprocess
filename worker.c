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

#include <time.h>   // for time()
#include <complex.h>
#include "md5s.h"
#include "common.h"

static void rsleep(int t);
static void md5_brute_forcer(uint128_t *input_hash, char string[], int cur_length, int *max_length, char *alphabet_start, char *alphabet_end, bool *not_found, char *result);

int main(int argc, char *argv[])
{
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the two message queues (whose names are provided in the arguments)
    mqd_t               mq_fd_request;
    mqd_t               mq_fd_response;
    MQ_REQUEST_MESSAGE  req;
    MQ_RESPONSE_MESSAGE rsp;

    mq_fd_request = mq_open (argv[1], O_RDONLY);
	mq_fd_response = mq_open (argv[2], O_WRONLY);

    //  * repeatingly:
    //      - read from a message queue the new job to do
    while(true) {
        mq_receive (mq_fd_request, (char*) &req, sizeof(req), NULL);
        #ifdef DEBUG
        fprintf(stderr,"Worker received job %d%c\n", req.hash_index, req.input_char);
        #endif // DEBUG
        //      - wait a random amount of time (e.g. rsleep(10000);)
        rsleep(10000);
        //      - do that job
        strcpy(rsp.result, "");

        if (req.max_length > 0) {
            char start_string[2] = {req.input_char, '\0'};
            rsp.hash_index = req.hash_index;
            bool not_found = true;
            md5_brute_forcer(&req.input_hash, start_string, 2, &req.max_length, &req.alphabet_start, &req.alphabet_end, &not_found, rsp.result);
        } else {
            #ifdef DEBUG
            fprintf(stderr,"Worker: killing myself, bye!\n");
            #endif // DEBUG
            break;
        }

        //      - write the results to a message queue

        mq_send (mq_fd_response, (char*) &rsp, sizeof(rsp), 0);
        #ifdef DEBUG
        fprintf(stderr,"Worker sent job %d%s\n", rsp.hash_index, rsp.result);
        #endif // DEBUG
    }
            
    //  * close the message queues        
    mq_close (mq_fd_response);
    mq_close (mq_fd_request);

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

static void md5_brute_forcer(uint128_t *input_hash, char string[], int cur_length, int *max_length, char *alphabet_start, char *alphabet_end, bool *not_found, char *result) {
    uint128_t check_hash;
    check_hash = md5s(string, cur_length - 1);

    if (check_hash == *input_hash) {
        *not_found = false;
        strcpy(result, string);
    } else if (cur_length <= *max_length) {
        // max_length is max amount of non-\0 chars, so we check <= as cur_length includes \0
        for (char i = *alphabet_start; i <= *alphabet_end && *not_found; i++) {
            char next_string[cur_length + 1];
            strcpy(next_string, string);
            next_string[cur_length - 1] = i;
            next_string[cur_length] = '\0';
            md5_brute_forcer(input_hash, next_string, cur_length + 1, max_length, alphabet_start, alphabet_end, not_found, result);
        }
    }
}