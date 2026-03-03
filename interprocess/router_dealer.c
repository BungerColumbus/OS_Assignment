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
#include <unistd.h>   // for execlp
#include <mqueue.h>   // for mq
#include <poll.h>     //for efficient multi-queue checks
#include <signal.h>   //For signaling

#include "settings.h"  
#include "messages.h"

char client2dealer_name[30];
char dealer2worker1_name[30];
char dealer2worker2_name[30];
char worker2dealer_name[30];

//Helper function for printing the attributes of a function
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



void cleanup(void){ //Generic cleanup function for exit
    kill(0, SIGKILL); //Kill all children
    
    if(mq_unlink(worker2dealer_name) == -1){  //Destroy all queues
      if(errno == ENOENT){
        perror("Queue nonexistent");
      }
      perror("Error unlinking w2d queue");
    };  
    if(mq_unlink(client2dealer_name) == -1){
      perror("Error unlinking c2d queue");
      if(errno == ENOENT){
        perror("Queue nonexistent");
      }
    };
    if(mq_unlink(dealer2worker1_name) == -1){
      perror("Error unlinking d2w1 queue");
      if(errno == ENOENT){
        perror("Queue nonexistent");
      }
    };
    if(mq_unlink(dealer2worker2_name) == -1){
      perror("Error unlinking d2w2 queue");
      if(errno == ENOENT){
        perror("Queue nonexistent");
      }
    };

}
void cleanup_and_exit(int sig) {  //Cleanup function in case of input during program run
    cleanup();
    _exit(0);
}


int main (int argc, char * argv[])
{
  if (argc != 1)
  {
    fprintf (stderr, "%s: invalid arguments\n", argv[0]);
  }

  signal(SIGINT, cleanup_and_exit);
  atexit(cleanup);
  

  // TODO:
    //  * create the message queues (see message_queue_test() in
    //    interprocess_basic.c)


  //Pointers towards the filenames of the programs that will be run by the child processes
  char* worker1Path = "/home/student/Documents/OS_Assignment/interprocess/worker_s1"; 
  char* clientPath = "/home/student/Documents/OS_Assignment/interprocess/client";
  char* worker2Path = "/home/student/Documents/OS_Assignment/interprocess/worker_s2";

  pid_t processID;  //Defining process ID for the router dealer
  pid_t clientPID;  //Defining process ID for the client
  pid_t workers[N_SERV1 + N_SERV2];
  mqd_t mq_fd_req;  //Defining the file descriptors for request, response, and worker queues
  mqd_t mq_fd_rep;
  mqd_t mq_fd_S1;
  mqd_t mq_fd_S2;

  REQ_MESSAGE req; //Instantiating one type of every message type, might be useful later
  RSP_MESSAGE rsp;
  SERVICE_MESSAGE ser;

  int status;
  int reqCounter = 0;
  struct mq_attr attr; 
  attr.mq_flags = 0;//Instantiating structure of attributes for message queues

  sprintf (client2dealer_name, "/mq_request_%s_%d", "Group44", getpid()); //Naming the file paths to the queues according to the 
  sprintf (dealer2worker1_name, "/mq_worker1_%s_%d", "Group44", getpid()); //assignment specificiation. Our group number is included
  sprintf (dealer2worker2_name, "/mq_worker2_%s_%d", "Group44", getpid());
  sprintf (worker2dealer_name, "/mq_response_%s_%d", "Group44", getpid());

  attr.mq_maxmsg = MQ_MAX_MESSAGES; //Number of messages in a queue is constant across the board

  //Dealer can only read from the queue in which the client sends
  attr.mq_msgsize = sizeof(REQ_MESSAGE);
  mq_fd_req = mq_open (client2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr); //Opening a queue for requests
  if (mq_fd_req == -1){
    perror("Cannot create client queue");
  }
  
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

  processID = fork();
  if (processID < 0) {
    perror("client fork() failed");  //Check if the fork failed
    exit(1);
  } else
    {   
      //Assign client process its main function
      if (processID == 0)
      {   
          char rqfd_str[12]; // Buffer to hold the number
          sprintf(rqfd_str, "%d", mq_fd_req); // Convert int (e.g., 3) to string ("3")
          execlp(clientPath, clientPath, client2dealer_name, NULL);

          perror("execlp failed client"); //The program should never reach here
          exit(2);
      }
      clientPID = processID;

      //Assign worker 1 processes their main function. There should be NSERV1 of them
      //First we create NSERV1 of them. Only the parent process can do that
      for (int i=0; i < N_SERV1; i++){
        if (processID > 0){
          processID = fork();
          workers[i]=processID;
        }
      }
      if(processID < 0){
        perror("worker fork() failed");  //Check if the fork failed
        exit(1);
      }

      if(processID == 0){
        execlp(worker1Path, worker1Path, dealer2worker1_name, worker2dealer_name, NULL);
        
        perror("execlp failed w1");  //The program should never reach here
        exit(2);
      }


      //Assign worker 2 processes their main function. There should be NSERV2 of them
      //First we create NSERV2 of them. Only the parent process can do that
      for (int i=0; i < N_SERV2; i++){
        if (processID > 0){
          processID = fork();
          workers[i + N_SERV1] = processID;
        }
      }
      if(processID < 0){
        perror("worker fork() failed");  //Check if the fork failed
        exit(1);
      }

      if(processID == 0){
        char w2fd_str[12]; // Buffer to hold  number
        sprintf(w2fd_str, "%d", mq_fd_S2); // Convert int to string ("3")
        char rsfd_str[12]; // Buffer to hold number
        sprintf(rsfd_str, "%d", mq_fd_rep); // Convert int to string 
        execlp(worker2Path, worker2Path, dealer2worker2_name, worker2dealer_name, NULL);
        
        perror("execlp failed w2");  //The program should never reach here
        exit(2);
      }

    //  * read requests from the Req queue and transfer them to the workers
    //    with the Sx queues
    //  * read answers from workers in the Rep queue and print them
    //  * wait until the client has been stopped (see process_test())
    
    //We will try implementing the polling method. 

    struct pollfd fds[2];
    int ret;

    fds[0].fd = mq_fd_req;
    fds[0].events = POLLIN;
    
    fds[1].fd = mq_fd_rep;
    fds[1].events = POLLIN;

    ret = poll(fds, 2, -1);

    if (ret  == -1){
      perror("Poll failed");
      exit(4);
    }

    if (ret == 0){
    //  perror("Timeout on poll");
      //exit(5);    
    }



    while(waitpid(clientPID, &status, WNOHANG) == 0 || attr.mq_curmsgs > 0){ //Handle request and response messages while client is active

    ret = poll(fds, 2, 10000);

    if (ret  == -1){
      perror("Poll failed");
      exit(4);
    }

    if (ret == 0){
    //  perror("Timeout on poll");
      //exit(5);   
    } 
    

    if(fds[0].revents & POLLIN){    //Handle a message in the request queue
      //fprintf (stderr, "                                   request: receiving...\n");
      ssize_t bytes_read = mq_receive (mq_fd_req, (char *) &req, sizeof (req), NULL);

      if (bytes_read == -1) {
          if (errno == EAGAIN) {
              fprintf(stderr, "Nothing to read from client");
          } else if (errno == EINTR) {
              fprintf(stderr, "Receive from client was interrupted by arbitrary signal.\n");
          } else {
              perror("mq_receive failed");
          }
      }

      //fprintf (stderr, "                                   request: received: %d, %d, '%d'\n",
      //    req.job_id, req.service_id, req.data);
      ser.data = req.data;
      ser.req_id = req.job_id;
      //fprintf(stderr, "Sending to workers...");
      if(req.service_id == 1){
        if (mq_send(mq_fd_S1, (char *) &ser, sizeof(SERVICE_MESSAGE), 0) == -1) {
          if (errno == EAGAIN) {
              perror("Queue full");
          } else if (errno == EMSGSIZE) {
              perror("Message too large");
          } else {
              perror("mq_send failed");
          }
}
        
      } else {
        if (mq_send(mq_fd_S2, (char *) &ser, sizeof(SERVICE_MESSAGE), 0) == -1) {
          if (errno == EAGAIN) {
              perror("Queue full");
          } else if (errno == EMSGSIZE) {
              perror("Message too large");
          } else {
              perror("mq_send failed");
          }
}
      }
      reqCounter++;
      //fprintf(stderr, "reqCounter: %d \n", reqCounter);
    }

    if (fds[1].revents & POLLIN){ //Handle a message in the response queue
      ssize_t bytes_read = mq_receive(mq_fd_rep, (char *) &rsp, sizeof(RSP_MESSAGE), NULL);

      if (bytes_read == -1) {
          if (errno == EAGAIN) {
              fprintf(stderr,"Nothing to read at worker2dealer queue");
          } else if (errno == EINTR) {
              fprintf(stderr, "Receive from worker was interrupted by arbitrary signal.\n");
          } else {
              perror("mq_receive failed");
          }
      }
      //fprintf(stderr, "%d --> %d\n", rsp.req_id, rsp.result);
      fprintf(stdout, "%d --> %d\n", rsp.req_id, rsp.result);
      fflush(stdout);
      reqCounter--;
      //fprintf(stderr, "Req counter %d\n", reqCounter);
    }

    if(mq_getattr(mq_fd_req, &attr) == -1){
      perror("Response queue attribute detection error");
    } 
      
    //fprintf(stderr, "GOT TO END OF WHILE \n");
    }

    //fprintf(stderr, "RIGHT AFTER WHILE \n");

    while(reqCounter > 0){  //Finish up the requests left after the client finished
      fprintf(stderr, "Got to the sending responses\n");
      mq_receive(mq_fd_rep, (char *) &rsp, sizeof(RSP_MESSAGE), NULL);
      fprintf(stderr, "Sending responses\n");
      fprintf(stderr, "ERROR %d --> %d\n", rsp.req_id, rsp.result);

      fprintf(stdout, "%d --> %d\n", rsp.req_id, rsp.result);
      fflush(stdout);

      reqCounter--;
    }

    //fprintf(stderr, "REQ COUNTER %d \n", reqCounter);

    //Having finished all operations with the workers, we choose to terminate them
    for (int i = 0; i < sizeof(workers); i++){
      if(workers[i] > 0){
        kill(workers[i], SIGTERM);
      }
    }

    //We wait for the processes to close their queues and exit
    for (int i = 0; i < sizeof(workers); i++) {
      waitpid(workers[i], NULL, 0);
    }


    //  * clean up the message queues (see message_queue_test())
    //We close the message queues. The unlinking happens atexit

    fprintf(stderr, "about to close the queues");
    if (mq_close(mq_fd_rep) == -1) {
      if (errno == EBADF) {
          fprintf(stderr,"Error: Attempted to close an invalid or already-closed descriptor d2w1.\n");
      } else {
          perror("mq_close failed");
      }
  }
    if (mq_close(mq_fd_req) == -1) {
      if (errno == EBADF) {
          fprintf(stderr,"Error: Attempted to close an invalid or already-closed descriptor c2d.\n");
      } else {
          perror("mq_close failed");
      }
  }
    if (mq_close(mq_fd_S1) == -1) {
    if (errno == EBADF) {
        fprintf(stderr,"Error: Attempted to close an invalid or already-closed descriptor d2w1.\n");
    } else {
        perror("mq_close failed");
    }
  }
    if (mq_close(mq_fd_S2) == -1) {
    if (errno == EBADF) {
        fprintf(stderr,"Error: Attempted to close an invalid or already-closed descriptor d2w2.\n");
    } else {
        perror("mq_close failed");
    }
  }
    }

    // Important notice: make sure that the names of the message queues
    // contain your goup number (to ensure uniqueness during testing)
  
  return (0);
}
