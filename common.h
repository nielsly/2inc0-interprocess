/* 
 * Operating Systems <2INCO> Practical Assignment
 * Interprocess Communication
 *
 * Contains definitions which are commonly used by the farmer and the workers
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include "uint128.h"

// maximum size for any message in the tests
#define MAX_MESSAGE_LENGTH 6

// TODO: put your definitions of the datastructures here
typedef struct {
    char start_char;
    char end_char;
    int length;
    char first_char;
    uint128_t md5;
    int list_index;
} MQ_REQUEST_MESSAGE;

typedef struct {
    char word[MAX_MESSAGE_LENGTH];
    int length;
    int index;
} MQ_RESPONSE_MESSAGE;

#endif
