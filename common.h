/* 
 * Operating Systems <2INCO> Practical Assignment
 * Interprocess Communication
 *
 * Contains definitions which are commonly used by the farmer and the workers
 *
 */

#ifndef COMMON_H
#define COMMON_H
//#define DEBUG


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // for execlp
#include <mqueue.h> // for mq
#include "uint128.h"


// maximum size for any message in the tests
#define MAX_MESSAGE_LENGTH 6

// TODO: put your definitions of the datastructures here
typedef struct {
    int hash_index;
    uint128_t input_hash;
    char input_char;
    char alphabet_start;
    char alphabet_end;
    int max_length;
} MQ_REQUEST_MESSAGE;

typedef struct {
    int hash_index;
    char result[MAX_MESSAGE_LENGTH + 1];
} MQ_RESPONSE_MESSAGE;

#endif
