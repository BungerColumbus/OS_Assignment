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
 * alexias personal token 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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
 */

#ifndef MESSAGES_H
#define MESSAGES_H

// define the data structures for your messages here

//Request messages client -> router-dealer
typedef struct
{
    int job_id;
    int service_id;
    int data;
} REQ_MESSAGE;

//Service message router-dealer -> services
typedef struct
{
    int req_id;
    int data;
} SERVICE_MESSAGE;

//Lmao comment 
typedef struct
{
    int req_id;
    int result;
} RSP_MESSAGE;

#endif
