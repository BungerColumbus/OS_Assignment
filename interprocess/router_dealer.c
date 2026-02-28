/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * Vasilescu Dan Gabriel SN:2155699
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
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>    
#include <unistd.h>    // for execlp
#include <mqueue.h>    // for mq


#include "settings.h"  
#include "messages.h"

char client2dealer_name[30];
char dealer2worker1_name[30];
char dealer2worker2_name[30];
char worker2dealer_name[30];

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
  if (argc != 1)
  {
    fprintf (stderr, "%s: invalid arguments\n", argv[0]);
  }
  

  // TODO:
    //  * create the message queues (see message_queue_test() in
    //    interprocess_basic.c)
  mqd_t mq_fd_req;  //Defining the file descriptors for request, response, and worker queues
  mqd_t mq_fd_rep;
  mqd_t mq_fd_S1;
  mqd_t mq_fd_S2;

  REQ_MESSAGE req; //Instantiating one type of every message type, might be useful later
  RSP_MESSAGE rsp;
  SERVICE_MESSAGE ser;

  struct mq_attr attr; //Instantiating structure of attributes for message queues

  sprintf (client2dealer_name, "/mq_request_%s_%d", "Group44", getpid()); //Naming the file paths to the queues according to the 
  sprintf (dealer2worker1_name, "/mq_worker1_%s_%d", "Group44", getpid()); //assignment specificiation. Our group number is included
  sprintf (dealer2worker2_name, "/mq_worker2_%s_%d", "Group44", getpid());
  sprintf (worker2dealer_name, "/mq_response_%s_%d", "Group44", getpid());

  attr.mq_maxmsg = MQ_MAX_MESSAGES; //Number of messages in a queue is constant across the board

  //Dealer can only read from the queue in which the client sends
  attr.mq_msgsize = sizeof(REQ_MESSAGE);
  mq_fd_req = mq_open (client2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr); //Opening a queue for requests
  
  //Dealer can only read from the response queue in which the workers send
  attr.mq_msgsize = sizeof(RSP_MESSAGE);
  mq_fd_rep = mq_open (worker2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr); //Opening a queue for worker responses

  //Dealer can only write in the queue towards the workers, sending services their way
  attr.mq_msgsize = sizeof(SERVICE_MESSAGE);
  mq_fd_S1 = mq_open(dealer2worker1_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);  //Opening queue for service 1 tasks
  mq_fd_S2 = mq_open(dealer2worker2_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);  //Opening queue for service 2 tasks

  //Debugging function, making sure all queues are defined as we want them
  getattr(mq_fd_rep);
  getattr(mq_fd_req);
  getattr(mq_fd_S1);
  getattr(mq_fd_S2);





    //  * create the child processes (see process_test() and
    //    message_queue_test())
    //  * read requests from the Req queue and transfer them to the workers
    //    with the Sx queues
    //  * read answers from workers in the Rep queue and print them
    //  * wait until the client has been stopped (see process_test())
    //  * clean up the message queues (see message_queue_test())

    // Important notice: make sure that the names of the message queues
    // contain your goup number (to ensure uniqueness during testing)
  
  return (0);
}
