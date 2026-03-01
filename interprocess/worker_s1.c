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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>      // for perror()
#include <unistd.h>     // for getpid()
#include <mqueue.h>     // for mq-stuff
#include <time.h>       // for time()

#include "messages.h"
#include "service1.h"

static void rsleep (int t);


int main (int argc, char * argv[])
{
    // check if the user has started this program with valid arguments
    if (argc != 3)
    {
    fprintf (stderr, "%s: %d arguments for Service 1:\n", argv[0], argc);
        for (int i = 1; i < argc; i++)
        {
        fprintf (stderr, " '%s'\n", argv[i]);
        }
    exit (3);
    }
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    char* s1 = argv[1];
    char* rsp = argv[2];

    mqd_t mq_fd_s1;
    mqd_t mq_fd_response;

    SERVICE_MESSAGE message;
    RSP_MESSAGE response;
    //  * open the two message queues (whose names are provided in the
    //    arguments)
    mq_fd_s1 = mq_open (s1, O_RDONLY);
    mq_fd_response = mq_open (rsp, O_WRONLY);
    //  * repeatedly:
    do {
    //      - read from the S1 message queue the new job to do
        mq_receive (mq_fd_s1, (char *) &message, sizeof (message), NULL);
    //      - wait a random amount of time (e.g. rsleep(10000);)
        rsleep(10000);
    //      - do the job 
        response.result = service(message.data);
        response.req_id = message.req_id;
    //      - write the results to the Rsp message queue
        mq_send (mq_fd_response, (char *) &response, sizeof (response), 0);    
    //    until there are no more tasks to do
    //??
    } while (1);           
    //  * close the message queues
    mq_close (mq_fd_response);
    mq_close (mq_fd_s1);   
    return(0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time (NULL) % getpid ());
        first_call = false;
    }
    usleep (random() % t);
}
