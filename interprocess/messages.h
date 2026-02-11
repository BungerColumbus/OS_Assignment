/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
 *
 * Grading:
 * Your work will be evaluated based on the following criteria:
 * - Satisfaction of all the specifications
 * - Correctness of the program
 * - Coding style
 * - Report quality
 * - Deadlock analysis
 */

#ifndef MESSAGES_H
#define MESSAGES_H

// define the data structures for your messages here

//Request messages client -> router-dealer
typedef struct
{
    int req_id;
    int service_id;
    char data[];
} REQ_MESSAGE;

//Service message router-dealer -> services
typedef struct
{
    int req_id;
    char data[];
}SERVICE_MESSAGE;

//
typedef struct
{
    int req_id;
    char result[]; //what type of data is result
} RSP_MESSAGE;

#endif
