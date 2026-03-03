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
#include <signal.h>     // for signal()

#include "messages.h"
#include "service2.h"

static void rsleep (int t);
void handle_shutdown(int sig);

char* name = "NO_NAME_DEFINED";
volatile sig_atomic_t keep_working = 1;
mqd_t dealer2worker;
mqd_t worker2dealer;


int main (int argc, char * argv[])
{
    SERVICE_MESSAGE message;
    RSP_MESSAGE response;

    ssize_t bytes_read;
    ssize_t bytes_sent;
    
    // check if the user has started this program with valid arguments
    if (argc != 3) {
        perror("Invalid number of arguments for s2.\n");
        exit(3);
    }

    // open queues
    name = argv[1];
    dealer2worker = mq_open (name, O_RDONLY);
    if (dealer2worker == (mqd_t)-1) 
    {
        perror("mq_open dealer2worker queue failed\n");
        exit(4);
    }

    name = argv[2];
    worker2dealer = mq_open (name, O_WRONLY);
    if (worker2dealer == (mqd_t)-1) 
    {
        perror("mq_open worker2dealer queue failed\n");
        exit(4);
    }

    //parent sends the shutdown signal
    signal(SIGTERM, handle_shutdown);

    while(keep_working) 
    {
    //read from the s2 message queue the new job to do
        bytes_read = mq_receive(dealer2worker, (char *)&message, sizeof(message), NULL);
        if (bytes_read == -1) 
        {
            if (bytes_read == -1)
            {
                //has received signal to exit loop
                if (errno == EINTR && !keep_working)
                {
                    break;  
                }
                perror("mq_receive failure in s2");
                exit(4);
            }
        }
    // wait a random amount of time (e.g. rsleep(10000);)
        rsleep(10000);
    // do the job 
        response.result = service(message.data);
        response.req_id = message.req_id;
    // write the results to the Response message queue
        bytes_sent = mq_send (worker2dealer, (char *) &response, sizeof (response), 0);  
        if (bytes_sent == -1)
        {
            perror("mq_send failure in worker2dealer queue, s2\n");
            exit(4);
        }  
    }          

    //  close the message queues  
    fprintf(stderr, "Closing queues \n");
    mq_close (worker2dealer);
    if (mq_close(worker2dealer) == -1) 
    {
        perror("mq_close response failed\n");
         exit(4);
    }

    mq_close (dealer2worker);  
    if (mq_close(dealer2worker) == -1) 
    {
        perror("mq_close s2 failed\n");
         exit(4);
    } 
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

/*
 * rhandle_shutdown(int sig)
 *
 * The calling process will know it has no more incoming tasks
 * 
 */
void handle_shutdown(int sig)
{
    keep_working = 0;
        mq_close(worker2dealer);
    mq_close(dealer2worker);
    //    fprintf(stderr, "Shutting Down process %d \n", getpid());
    exit(0);

}
