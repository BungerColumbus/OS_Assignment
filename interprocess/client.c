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
        exit (5);
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
    
    // when starting the client do it like this:
    // ./client "/mq_req_name" 
    mq_request = mq_open(argv[1], O_WRONLY);
    
    // give error message 6 if a client failed to open the mq_request

    if (mq_request == (mqd_t)-1) {
    perror("client's mq_open() failed");
    exit(6);
    }

    // prints each time a client opens the mq_request along with the process ID and the name of the queue "argv[1]"

    fprintf(stderr, "[%d] Message queue opened: %s\n", getpid(), argv[1]);
    getattr(mq_request);
    
    //  * repeatingly:
    //      - get the next job request 
    //      - send the request to the Req message queue
    //    until there are no more requests to send
    
    // used to count the requests
    int request_count = 0;
    
    while(getNextRequest(&req.job, &req.data, &req.service) == NO_ERR)
    {
        request_count++;

        // print the request i with its params and from which client it came.
        //fprintf(stderr, "[%d] Request #%d: job=%d, data=%d, service=%d\n",
        //    getpid(), request_count, req.job, req.data, req.service);


        // who is the mq_des
        // mq_request
        // who is pointer
        // the address of the message the clients wants to send
        req_message.job_id = req.job;
        req_message.data = req.data;
        req_message.service_id = req.service;

        ssize_t send_result = mq_send (mq_request, (char *) &req_message, sizeof (req_message), 0);
        
        // giving error message 7 if it failed to send message
        if (send_result == -1) {
            perror("mq_send() failed");
            fprintf(stderr, "[%d] Failed to send request #%d\n", 
            getpid(), request_count);
            exit(7);
        // says what client sent what
        } else {
         //   fprintf(stderr, "[%d] Sent %zd bytes\n", getpid(), send_result);
        }
        // delay to avoid flooding
        rsleep(100000);  // 100ms
    }
    //  * close the message queue
    // send error message 8 if it failed
    if (mq_close(mq_request) == -1) {
    perror("client's mq_close() failed");
    exit(8);
    // decide whether to exit or continue cleanup
    } else {
    //    fprintf(stderr, "[%d] mq_request closed\n", getpid());
    }
    return (0); // slay thyself
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
