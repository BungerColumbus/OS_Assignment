/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * Dan Gabriel Vasilescu (2155699)
 * Vlad Erceanu (2115581)
 * Alexia Constantinof (2130793)
 *
 * Grading:
 * Your work will be evaluated based on the following criteria:
 * - Satisfaction of all the specifications
 * - Correctness of the program
 * - Coding style
 * - Report quality
 * - Deadlock analysis
 */
#define _POSIX_C_SOURCE 200112L  // Unlocks clock_gettime and CLOCK_REALTIME for mq_timed receive
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
#include <time.h>
#include <sys/time.h>


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
  //  kill(0, SIGKILL); //Kill all children
    
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

  //signal(SIGINT, cleanup_and_exit);
  atexit(cleanup);
  

  // TODO:
    //  * create the message queues (see message_queue_test() in
    //    interprocess_basic.c)


  //Pointers towards the filenames of the programs that will be run by the child processes
  char* worker1Path = "./worker_s1"; 
  char* clientPath = "./client";
  char* worker2Path = "./worker_s2";

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
  bool client_alive = true;
  struct mq_attr attr; 
  attr.mq_flags = 0;//Instantiating structure of attributes for message queues

  sprintf (client2dealer_name, "/mq_request_%s_%d", "Group44", getpid()); //Naming the file paths to the queues according to the 
  sprintf (dealer2worker1_name, "/mq_worker1_%s_%d", "Group44", getpid()); //assignment specificiation. Our group number is included
  sprintf (dealer2worker2_name, "/mq_worker2_%s_%d", "Group44", getpid());
  sprintf (worker2dealer_name, "/mq_response_%s_%d", "Group44", getpid());

  attr.mq_maxmsg = MQ_MAX_MESSAGES; //Number of messages in a queue is constant across the board

  //Dealer can only read from the queue in which the client sends
  attr.mq_msgsize = sizeof(REQ_MESSAGE);
  mq_fd_req = mq_open (client2dealer_name, O_RDONLY | O_CREAT | O_EXCL| O_NONBLOCK, 0600, &attr); //Opening a queue for requests
  if (mq_fd_req == -1){
    perror("Cannot create client queue");
  }
  
  //Dealer can only read from the response queue in which the workers send
  attr.mq_msgsize = sizeof(RSP_MESSAGE);
  mq_fd_rep = mq_open (worker2dealer_name, O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr); //Opening a queue for worker responses

  //Dealer can only write in the queue towards the workers, sending services their way
  attr.mq_msgsize = sizeof(SERVICE_MESSAGE);
  mq_fd_S1 = mq_open(dealer2worker1_name, O_WRONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr);  //Opening queue for service 1 tasks
  mq_fd_S2 = mq_open(dealer2worker2_name, O_WRONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr);  //Opening queue for service 2 tasks

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
      // for (int i=0; i < N_SERV1; i++){
      //   if (processID > 0){
      //     processID = fork();
      //     workers[i]=processID;
      //   }
      // }
      // if(processID < 0){
      //   perror("worker fork() failed");  //Check if the fork failed
      //   exit(1);
      // }

      // if(processID == 0){
      //   execlp(worker1Path, worker1Path, dealer2worker1_name, worker2dealer_name, NULL);
        
      //   perror("execlp failed w1");  //The program should never reach here
      //   exit(2);
      // }

      for (int i=0; i < N_SERV1; i++) {
    if (processID > 0) {
        processID = fork();
        if(processID < 0){
        perror("worker fork() failed");  //Check if the fork failed
        exit(1);
      }
        if (processID == 0) { 
            // Child logic MUST be here to prevent loop fall-through
        execlp(worker1Path, worker1Path, dealer2worker1_name, worker2dealer_name, NULL);
        perror("execlp failed w1");  //The program should never reach here
        exit(2);
        }
        workers[i] = processID; // Parent records PID
    }
}


      //Assign worker 2 processes their main function. There should be NSERV2 of them
      //First we create NSERV2 of them. Only the parent process can do that
      // for (int i=0; i < N_SERV2; i++){
      //   if (processID > 0){
      //     processID = fork();
      //     workers[i + N_SERV1] = processID;
      //   }
      // }
      // if(processID < 0){
      //   perror("worker fork() failed");  //Check if the fork failed
      //   exit(1);
      // }

      // if(processID == 0){

        
      //   char w2fd_str[12]; // Buffer to hold  number
      //   sprintf(w2fd_str, "%d", mq_fd_S2); // Convert int to string ("3")
      //   char rsfd_str[12]; // Buffer to hold number
      //   sprintf(rsfd_str, "%d", mq_fd_rep); // Convert int to string 
      //   execlp(worker2Path, worker2Path, dealer2worker2_name, worker2dealer_name, NULL);
        
      //   perror("execlp failed w2");  //The program should never reach here
      //   exit(2);
      // }

      for (int i = 0; i < N_SERV2; i++) 
      { // Start for
          if (processID > 0) 
          { // Start if parent
              processID = fork();
              
              if (processID < 0) 
              {
                  perror("worker2 fork() failed");
                  exit(1);
              }

              if (processID == 0) 
              {
                  // Child logic
                  execlp(worker2Path, worker2Path, dealer2worker2_name, worker2dealer_name, NULL);
                  perror("execlp failed w2");
                  exit(2);
              }

              // Parent records PID
              workers[i + N_SERV1] = processID; 
          } // End if parent
      } // End for

    //  * read requests from the Req queue and transfer them to the workers
    //    with the Sx queues
    //  * read answers from workers in the Rep queue and print them
    //  * wait until the client has been stopped (see process_test())
    
    //We will try implementing the polling method. 

    struct pollfd fds[4];
    int ret;
    bool has_pending_S1 = false;
    bool has_pending_S2 = false;
    SERVICE_MESSAGE pending_S1, pending_S2; //buffer in case message doesn't work


    fds[0].fd = mq_fd_req; fds[0].events = (!has_pending_S1 && !has_pending_S2) ? POLLIN : 0; // Only read if sending to worker queue will work
    fds[1].fd = mq_fd_rep; fds[1].events = POLLIN;                 // Always listen for responses
    fds[2].fd = mq_fd_S1;  fds[2].events = has_pending_S1 ? POLLOUT : 0; // Only check space if a message needs to be resent
    fds[3].fd = mq_fd_S2;  fds[3].events = has_pending_S2 ? POLLOUT : 0; 

    ret = poll(fds, 4, 1000);

    if (ret  == -1){
      perror("Poll failed");
      exit(4);
    }

    if (ret == 0){
    //  perror("Timeout on poll");
      //exit(5);    
    }

    mq_getattr(mq_fd_req, &attr);
    long messages_in_queue = attr.mq_curmsgs;

    while(client_alive|| reqCounter > 0|| has_pending_S1 || has_pending_S2 || messages_in_queue > 0){ //Handle request and response messages while client is active

    if (client_alive) {
        int r = waitpid(clientPID, &status, WNOHANG);
        if (r > 0 || (r == -1 && errno == ECHILD)) {
            client_alive = false;
            fprintf(stderr, "Dealer: Client finished. Counter: %d\n", reqCounter);
        }
    }

    // 2. THE CRITICAL CHECK: Exit BEFORE polling if everything is done
    if (!client_alive && reqCounter <= 0 && !has_pending_S1 && !has_pending_S2) {
        fprintf(stderr, "Dealer: All work complete. Exiting loop.\n");
        break; 
    }

    // Check if client is still running
if (client_alive) {
        int r = waitpid(clientPID, &status, WNOHANG);
        if (r > 0 || (r == -1 && errno == ECHILD)) {
            client_alive = false;
            fprintf(stderr, "Dealer: Client finished. Draining %d jobs...\n", reqCounter);
            // If no jobs are left, exit the loop immediately
            if (reqCounter <= 0 && !has_pending_S1 && !has_pending_S2) {
                break; 
            }
        }
    }
    fds[0].events = (!has_pending_S1 && !has_pending_S2) ? POLLIN : 0; // Reinitialize every iteration
    fds[1].events = POLLIN; 
    fds[2].events = has_pending_S1 ? POLLOUT : 0; 
    fds[3].events = has_pending_S2 ? POLLOUT : 0; 

    ret = poll(fds, 4, 1000);

    if (ret  == -1){
      if (errno == EINTR) continue; // Restart if interrupted by signal
      perror("Poll failed");
      exit(4);
    }

if (ret == 0) {
        // If there is a timeout, send the program back to wait for another 10 seconds before checking the loop again
        //continue;
        perror("Timeout");
    }
  if(ret > 0){  
    // fprintf(stderr, "entered ret > 0 \n");
    // if (fds[1].revents & POLLIN){ //Handle a message in the response queue
    // fprintf(stderr, "DEBUG: Attempting to receive from workers...\n"); // Add this
    // ssize_t bytes_read = mq_receive(mq_fd_rep, (char *) &rsp, sizeof(RSP_MESSAGE), NULL);
    // fprintf(stderr, "DEBUG: Receive successful!\n"); // Add this

    // if (bytes_read > 0) { // <--- ADD THIS GUARD
    //     fprintf(stdout, "%d --> %d\n", rsp.req_id, rsp.result);
    //     fflush(stdout);
    //     reqCounter--;
    // } else if (bytes_read == -1) {
    //     if (errno != EAGAIN && errno != EINTR) {
    //         perror("mq_receive failed");
    //     }
    // }
    if (fds[1].revents & POLLIN) {
    // Loop until the queue is empty (mq_receive returns -1 with EAGAIN)
    while (true) {
        ssize_t bytes_read = mq_receive(mq_fd_rep, (char *) &rsp, sizeof(RSP_MESSAGE), NULL);
        if (bytes_read > 0) {
            fprintf(stdout, "%d -> %d\n", rsp.req_id, rsp.result);
            fflush(stdout);
            reqCounter--;
        } else {
            break; // Queue is empty or interrupted
        }
    }
}
  
}

    // Retry message sending to S1
    if (has_pending_S1 && (fds[2].revents & POLLOUT)) {
        if (mq_send(mq_fd_S1, (char *)&pending_S1, sizeof(pending_S1), 0) == 0) {
            has_pending_S1 = false; 
        }
    }

    // Retry message sending to S2
    if (has_pending_S2 && (fds[3].revents & POLLOUT)) {
        if (mq_send(mq_fd_S2, (char *)&pending_S2, sizeof(pending_S2), 0) == 0) {
            has_pending_S2 = false;
        }
    }

    if (!has_pending_S1 && !has_pending_S2 && (fds[0].revents & POLLIN)) {    
    // INNER LOOP: Drain all available requests from the client queue
    while (true) {
        ssize_t bytes_read = mq_receive(mq_fd_req, (char *)&req, sizeof(req), NULL);

        if (bytes_read == -1) {
            if (errno == EAGAIN) {
                // Queue is finally empty, break the inner while loop
                break; 
            } else if (errno == EINTR) {
                continue; // Interrupted, try again
            } else {
                perror("mq_receive failed");
                break;
            }
        }

        if (bytes_read > 0) {
            ser.data = req.data;
            ser.req_id = req.job_id;
            bool sent_or_buffered = false;

            if (req.service_id == 1) {
                if (mq_send(mq_fd_S1, (char *)&ser, sizeof(ser), 0) == 0) {
                    sent_or_buffered = true;
                } else if (errno == EAGAIN) {
                    has_pending_S1 = true;
                    pending_S1 = ser;
                    sent_or_buffered = true;
                    // If worker queue is full, we MUST stop draining requests
                    // or we will overwrite our buffer/lose data.
                    break; 
                }
            } else {
                if (mq_send(mq_fd_S2, (char *)&ser, sizeof(ser), 0) == 0) {
                    sent_or_buffered = true;
                } else if (errno == EAGAIN) {
                    has_pending_S2 = true;
                    pending_S2 = ser;
                    sent_or_buffered = true;
                    break; 
                }
            }

            if (sent_or_buffered) {
                reqCounter++;
            }
            
            // Critical check: If we just buffered a message, we can't read any more 
            // until poll() tells us the worker queue has space (POLLOUT).
            if (has_pending_S1 || has_pending_S2) {
                break;
            }
        }
    } // End of inner while
    

} 
// 1. Check for exit every iteration, regardless of poll outcome
    if (!client_alive && reqCounter <= 0 && !has_pending_S1 && !has_pending_S2) {
        fprintf(stderr, "Dealer: All conditions met. Exiting loop.\n");
        break; 
    }

    // 2. Debug state (this will now actually print during the hang)
    if (!client_alive) {
        fprintf(stderr, "[DEBUG] Loop state: reqCounter=%d, S1_pend=%d, S2_pend=%d\n", 
                reqCounter, has_pending_S1, has_pending_S2);
    }

    if(mq_getattr(mq_fd_req, &attr) == -1){
      perror("mq_getattr() function failed");
      exit(2);
    }
    messages_in_queue = attr.mq_curmsgs;

    }

    //fprintf(stderr, "RIGHT AFTER WHILE \n");

    //fprintf(stderr, "REQ COUNTER %d \n", reqCounter);
    // 1. Send SIGTERM to everyone first
    int total_workers = N_SERV1 + N_SERV2;
    for (int i = 0; i < total_workers; i++) {
        if (workers[i] > 0) {
            kill(workers[i], SIGTERM);
        }
    }

    // 2. NOW wait for them to actually finish
    for (int i = 0; i < total_workers; i++) {
        if (workers[i] > 0) {
            fprintf(stderr, "Dealer: Waiting for worker %d (PID %d)...\n", i, workers[i]);
            waitpid(workers[i], NULL, 0); 
            fprintf(stderr, "Dealer: Worker %d (PID %d) has FULLY exited.\n", i, workers[i]);
        }
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
    

    // Important notice: make sure that the names of the message queues
    // contain your goup number (to ensure uniqueness during testing)
  
  
  return (0);
    }
}
