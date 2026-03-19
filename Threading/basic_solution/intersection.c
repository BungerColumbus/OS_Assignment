#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include "arrivals.h"
#include "intersection_time.h"
#include "input.h"

// declare a mutex
static pthread_mutex_t      mutex          = PTHREAD_MUTEX_INITIALIZER;

/* 
 * curr_arrivals[][][]
 *
 * A 3D array that stores the arrivals that have occurred
 * The first two indices determine the entry lane: first index is Side, second index is Direction
 * curr_arrivals[s][d] returns an array of all arrivals for the entry lane on side s for direction d,
 *   ordered in the same order as they arrived
 */
static Arrival curr_arrivals[4][3][20];

/*
 * semaphores[][]
 *
 * A 2D array that defines a semaphore for each entry lane,
 *   which are used to signal the corresponding traffic light that a car has arrived
 * The two indices determine the entry lane: first index is Side, second index is Direction
 */
static sem_t semaphores[4][3];

/*
 * supply_arrivals()
 *
 * A function for supplying arrivals to the intersection
 * This should be executed by a separate thread
 */

typedef struct 
{
  int side;
  int direction;
}args;

static void* supply_arrivals()
{
  int num_curr_arrivals[4][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

  // for every arrival in the list
  for (int i = 0; i < sizeof(input_arrivals)/sizeof(Arrival); i++)
  {
    // get the next arrival in the list
    Arrival arrival = input_arrivals[i];
    // wait until this arrival is supposed to arrive
    sleep_until_arrival(arrival.time);
    // store the new arrival in curr_arrivals
    curr_arrivals[arrival.side][arrival.direction][num_curr_arrivals[arrival.side][arrival.direction]] = arrival;
    num_curr_arrivals[arrival.side][arrival.direction] += 1;
    // increment the semaphore for the traffic light that the arrival is for
    sem_post(&semaphores[arrival.side][arrival.direction]);
  }

  return(0);
}


/*
 * manage_light(void* arg)
 *
 * A function that implements the behaviour of a traffic light
 */
static void* manage_light(void* arg)
{
  args *coords = (args*) arg;  // cast void* to your struct
  

  int i = coords->side;          // retrieve side
  int j = coords->direction;          // retrieve direction

  struct timespec ts;
  int num_arrivals = 0;

  while(get_time_passed() < END_TIME)
  {
    int timeout = END_TIME - get_time_passed();
    if(timeout < 0) 
    {
      return 0;
    }

    clock_gettime(0, &ts);
    ts.tv_sec += timeout;

    int s = sem_timedwait(&semaphores[i][j], &ts);
    if(s == 0)
    {
      pthread_mutex_lock (&mutex);

      //critical section 
      printf("traffic light %d %d turns green at time %d for car %d\n",i, j, get_time_passed(), curr_arrivals[i][j][num_arrivals].id);
      sleep (CROSS_TIME);
      printf("traffic light %d %d turns red at time %d\n", i, j, get_time_passed());
      
      pthread_mutex_unlock (&mutex);

    num_arrivals++;
    } else {
      return(0);
    }
  }
  return(0);
}


int main(int argc, char * argv[])
{
  // create semaphores to wait/signal for arrivals
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      sem_init(&semaphores[i][j], 0, 0);
    }
  }

  // start the timer
  start_time();

  // TODO: create a thread per traffic light that executes manage_light
  // 4 threads for traffic lights (each side)
  pthread_t traffic_light[4][3];
  args arg_sem[12];
  int counter = 0;

  for(int i = 0; i < 4; i++) 
  {
    for(int j = 0; j < 3; j++)
    {
      arg_sem[counter].side = i;
      arg_sem[counter].direction = j;
      if (pthread_create(&traffic_light[i][j], NULL, manage_light, &arg_sem[counter]) != 0) 
      {
          fprintf(stderr, "Failed to create thread for traffic light %d\n", i);
          return 1;
      }
      counter ++;
    }
  }

  // TODO: create a thread that executes supply_arrivals
  // 1 thread for supply_arrivals
  pthread_t arrival_supplier;
  if (pthread_create(&arrival_supplier, NULL, supply_arrivals, NULL) != 0) {
        fprintf(stderr, "Failed to create thread for supply arrival\n");
        return 1;
  }

  // TODO: wait for all threads to finish
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 3; j++) {
      pthread_join(traffic_light[i][j], NULL);
    }
  }
  pthread_join(arrival_supplier, NULL);

  // destroy semaphores
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      sem_destroy(&semaphores[i][j]);
    }
  }
}
