/* 
 * Operating Systems  (2INC0)  Practical Assignment.
 * Condition Variables Application.
 *
 * Dan Gabriel Vasilescu (2155699)
 * Vlad Erceanu (2115581)
 * Alexia Constantinof (2130793)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * Extra steps can lead to higher marks because we want students to take the initiative.
 */
 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>  
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "prodcons.h"

static ITEM buffer[BUFFER_SIZE];

static void rsleep (int t);	        // already implemented (see below)
static ITEM get_next_item (void);   // already implemented (see below)

// Globals
static int buffer_count = 0;
static bool production_done = false;
static int expected_value = 0;
static bool received[NROF_ITEMS] = {false};  // Track which items arrived
static int signal_calls = 0;
static int broadcast_calls = 0;
static int wake_on_broadcast = 0;

static pthread_mutex_t      buffer_mutex          = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       consumer_state        = PTHREAD_COND_INITIALIZER;
static pthread_cond_t       producer_state        = PTHREAD_COND_INITIALIZER;

/* producer thread */
static void * 
producer (void * arg)
{
	int id = *(int *)arg;
    while (1)
    {
        // put item in the buffer
        ITEM item = get_next_item();
        if (item == NROF_ITEMS)  
            break;
		
        rsleep (100);	
		
        // lock mutex-lock;
        pthread_mutex_lock(&buffer_mutex);
        
        // wait while condition is not TRUE
        while (item != expected_value || buffer_count + 1 == BUFFER_SIZE)
        {
            pthread_cond_wait(&producer_state, &buffer_mutex);

			wake_on_broadcast++;
			fprintf(stderr,"broascast %d wakes up producer %d\n",broadcast_calls, id);
        }
		
        // critical-section;
        buffer[buffer_count] = item;  // Insert at next available position
        buffer_count++;
        expected_value++;    
        
		//signal consumer thread
        pthread_cond_signal(&consumer_state);
        
        // unlock mutex
        pthread_mutex_unlock(&buffer_mutex);
    }
    return (NULL);
}

/* consumer thread */
static void * 
consumer (void * arg)
{
    while (1)
    {
        // lock mutex
        pthread_mutex_lock(&buffer_mutex);
        
        // while condiiton is false
        while (buffer_count == 0 && !production_done) {
            pthread_cond_wait(&consumer_state, &buffer_mutex);
        }
        
        // exit if production done AND buffer empty
        if (buffer_count == 0 && production_done) {
            pthread_mutex_unlock(&buffer_mutex);
            break;
        }
        
        //output FIFO
        printf("%d\n", buffer[0]);

        //remove it from the buffer
        for (int i = 0; i < buffer_count - 1; i++) {
            buffer[i] = buffer[i+1];  
        }

        buffer_count--;

		// // get the next item from buffer[]
        // ITEM buffer_item = buffer[0];
        // buffer_count--;
        // for (int i = 0; i < BUFFER_SIZE - 1; i++) {
        //     buffer[i] = buffer[i+1];  // Shift elements left
        // }

		// received[buffer_item] = true;

		// // Print all consecutive items we now have (reordering logic)
		// while (expected_value < NROF_ITEMS && received[expected_value]) {
    	// 	printf("%d\n", expected_value);
    	// 	received[expected_value] = false;  // Optional: clean up
    	// 	expected_value++;
		// }
        
        // possible-cv-signals
		broadcast_calls++;
		fprintf(stderr,"broadcast call = %d (consumer to producers)\n", broadcast_calls);
        pthread_cond_broadcast(&producer_state);
        
        // unlock mutex
        pthread_mutex_unlock(&buffer_mutex);
                
        rsleep (100);
    }
    return (NULL);
}

int main (void)
{
	clock_t start = clock();
    pthread_t prod[NROF_PRODUCERS];
    pthread_t cons;
	int ids[NROF_PRODUCERS];

    //create producer threads
    for (int i = 0; i < NROF_PRODUCERS; i++)
    {
		ids[i] = i;
        if (pthread_create(&prod[i], NULL, producer, &ids[i]) != 0)
        {
            fprintf(stderr, "Failed to create thread for producer no. %d\n", i);
            return 1;
        }
    }

    //create consumer thread
    if (pthread_create(&cons, NULL, consumer, NULL) != 0)
    {
        fprintf(stderr, "Failed to create thread for consumer\n");
        return 1;
    }

    //wait for producer threads to finish
    for (int i = 0; i < NROF_PRODUCERS; i++) {
        pthread_join(prod[i], NULL);
    }

    //Signal that production is complete
    pthread_mutex_lock(&buffer_mutex);
    production_done = true;
    pthread_cond_signal(&consumer_state);  // Wake consumer if it's waiting
    pthread_mutex_unlock(&buffer_mutex);

    //wait for consumer thread to finish
    pthread_join(cons, NULL);

	fprintf(stderr, "Number of signal calls = %d\n", signal_calls);
	fprintf(stderr, "Number of broadcast calls = %d\n", broadcast_calls);
	fprintf(stderr, "Number of times producers have been woken up = %d\n", wake_on_broadcast);

	//compute running_time
	clock_t end = clock();
    double running_time = (double)(end - start) / CLOCKS_PER_SEC;
    fprintf(stderr, "Running time = %.6f seconds\n", running_time);
    return (0);
}


/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void 
rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time(NULL));
        first_call = false;
    }
    usleep (random () % t);
}


/* 
 * get_next_item()
 *
 * description:
 *	thread-safe function to get a next job to be executed
 *	subsequent calls of get_next_item() yields the values 0..NROF_ITEMS-1 
 *	in arbitrary order 
 *	return value NROF_ITEMS indicates that all jobs have already been given
 * 
 * parameters:
 *	none
 *
 * return value:
 *	0..NROF_ITEMS-1: job number to be executed
 *	NROF_ITEMS:	 ready
 */
static ITEM
get_next_item(void)
{
    static pthread_mutex_t  job_mutex   = PTHREAD_MUTEX_INITIALIZER;
    static bool    jobs[NROF_ITEMS+1] = { false }; // keep track of issued jobs
    static int     counter = 0;    // seq.nr. of job to be handled
    ITEM           found;          // item to be returned
	
	/* avoid deadlock: when all producers are busy but none has the next expected item for the consumer 
	 * so requirement for get_next_item: when giving the (i+n)'th item, make sure that item (i) is going to be handled (with n=nrof-producers)
	 */
	pthread_mutex_lock (&job_mutex);

	counter++;
	if (counter > NROF_ITEMS)
	{
	    // we're ready
	    found = NROF_ITEMS;
	}
	else
	{
	    if (counter < NROF_PRODUCERS)
	    {
	        // for the first n-1 items: any job can be given
	        // e.g. "random() % NROF_ITEMS", but here we bias the lower items
	        found = (random() % (2*NROF_PRODUCERS)) % NROF_ITEMS;
	    }
	    else
	    {
	        // deadlock-avoidance: item 'counter - NROF_PRODUCERS' must be given now
	        found = counter - NROF_PRODUCERS;
	        if (jobs[found] == true)
	        {
	            // already handled, find a random one, with a bias for lower items
	            found = (counter + (random() % NROF_PRODUCERS)) % NROF_ITEMS;
	        }    
	    }
	    
	    // check if 'found' is really an unhandled item; 
	    // if not: find another one
	    if (jobs[found] == true)
	    {
	        // already handled, do linear search for the oldest
	        found = 0;
	        while (jobs[found] == true)
            {
                found++;
            }
	    }
	}
    	jobs[found] = true;
			
	pthread_mutex_unlock (&job_mutex);
	return (found);
}



