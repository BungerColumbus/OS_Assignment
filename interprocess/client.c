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
#include "request.h"

static void rsleep (int t);

static void 
getattr (mqd_t mq_fd)
{
    struct mq_attr      attr;
    int                 rtnval;
    
    rtnval = mq_getattr (mq_fd, &attr);
    if (rtnval == -1)
    {
        perror ("mq_getattr() failed");
        exit (1);
    }
    fprintf (stderr, "%d: mqdes=%d max=%ld size=%ld nrof=%ld\n",
                getpid(), 
                mq_fd, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
}

int main (int argc, char * argv[])
{
    REQ_MESSAGE	req_message;
	Request 	req;
	mqd_t		mq_request;
    
    
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the message queue (whose name is provided in the
    //    arguments)
    
    // When starting the client do it like this:
    // ./client "/mq_req_name" 
    mq_request = mq_open(argv[1], O_WRONLY);
    
    //  * repeatingly:
    //      - get the next job request 
    //      - send the request to the Req message queue
    //    until there are no more requests to send
    while(getNextRequest(&req.job, &req.data, &req.service) == NO_ERR)
    {
        // who is the mq_des
        // mq_request
        // who is pointer
        // THE FUCKING ADDRESS OF THE MESSAGE I WANT TO SEND OFC
        req_message.job_id = req.job;
        req_message.data = req.data;
        req_message.service_id = req.service;

        mq_send (mq_request, (char *) &req_message, sizeof (req_message), NULL);


        // delay to avoid flooding
        usleep(100000);  // 100ms
    }
    //  * close the message queue
    mq_close (mq_request);
    return (0);
}
