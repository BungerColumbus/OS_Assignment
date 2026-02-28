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

int main (int argc, char * argv[])
{
    REQ_MESSAGE	req_message;
	Request 	req;
	mqd_t		mq_client;
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the message queue (whose name is provided in the
    //    arguments)
    
    // When starting the client do it like this:
    // ./client "/mq_req_name" 
    mq_fd_request = mq_open(argv[1], O_WRONLY);
    
    //  * repeatingly:
    //      - get the next job request 
    //      - send the request to the Req message queue
    //    until there are no more requests to send
    while(getNextRequest(&, &, &) == NO_ERR)
    {
        getNextRequest(&, &, &);
        mq_send (mq_fd_request, );


        // delay to avoid flooding
        usleep(100000);  // 100ms
    }
    //  * close the message queue
    mq_close (mq_fd_request);
    return (0);
}
