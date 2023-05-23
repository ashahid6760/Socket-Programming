#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include "cal-new.h"
#include "types.h"

/* Global Variables */
uint32_t *array_to_be_sorted = NULL;
sem_t arrayReady;
sem_t firstHalfReady;
sem_t SecondHalfReady;
sem_t mergeReady;
pthread_t  th1, th2, th3;

extern sem_t *semaphore_parent_child;

// merge function for merging two parts
void merge(int low, int mid, int high)
{

    // n1 is size of left part and n2 is size of right part
    int n1 = mid - low + 1;
    int n2 = high - mid;

    int *left = malloc(n1 * sizeof(int));
    int *right = malloc(n2 * sizeof(int));

    int i;
    int j;

    // storing values in left part
    for (i = 0; i < n1; i++)
        left[i] = array_to_be_sorted[i + low];

    // storing values in right part
    for (i = 0; i < n2; i++)
        right[i] = array_to_be_sorted[i + mid + 1];

    int k = low;

    i = j = 0;

    // merge left and right in ascending order
    while (i < n1 && j < n2) {
        if (left[i] <= right[j])
            array_to_be_sorted[k++] = left[i++];
        else
            array_to_be_sorted[k++] = right[j++];
    }

    // insert remaining values from left
    while (i < n1)
        array_to_be_sorted[k++] = left[i++];

    // insert remaining values from right
    while (j < n2)
        array_to_be_sorted[k++] = right[j++];

    free(left);
    free(right);
}


// merge sort function
void merge_sort(int low, int high)
{
    // calculating mid point of array
    int mid = low + (high - low) / 2;

    if (low < high) {
        // calling first half
        merge_sort(low, mid);

        // calling second half
        merge_sort(mid + 1, high);

        // merging the two halves
        merge(low, mid, high);
    }
}

void *sortSecondHalf(void* args)
{
    sem_wait(&arrayReady);

    unsigned int arraysize = *(int*) args;
    int low = (arraysize-1)/2+1;
    int high = arraysize-1;
    // evaluating mid point
    int mid = low + (high - low) / 2;

    if (low < high) {
        merge_sort(low, mid);
        merge_sort(mid + 1, high);
        merge(low, mid, high);
    }
    printf("\n");
    sem_post(&SecondHalfReady);
    pthread_exit(0);
}

void *sortFirstHalf(void* args)
{
    sem_wait(&arrayReady);

    unsigned int arraysize = *(int*) args;
    int low = 0;
    int high = (arraysize-1)/2;
    // evaluating mid point
    int mid = low + (high - low) / 2;

    if (low < high) {
        merge_sort(low, mid);
        merge_sort(mid + 1, high);
        merge(low, mid, high);
    }
    printf("\n");

    sem_post(&firstHalfReady);
    pthread_exit(0);
}

void *mergeTwoHalves(void* args)
{
    sem_wait(&firstHalfReady);
    sem_wait(&SecondHalfReady);

    unsigned int arraysize = *(int*) args;
    int high = arraysize-1;
    merge(0, high / 2, high);

    sem_post(&mergeReady);
    pthread_exit(0);
}

void process_parallel_merge(sort_request_t *request)
{

  int *status = NULL;
  int array_size = request->number_of_integers;
  array_to_be_sorted = request->integers;

  sem_init(&arrayReady, 0, 0);
  sem_init(&firstHalfReady, 0, 0);
  sem_init(&SecondHalfReady, 0, 0);
  sem_init(&mergeReady, 0, 0);

  int rc = pthread_create(&th1, NULL, sortFirstHalf, (void *)&array_size);
  if (rc){
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }

  rc = pthread_create(&th2, NULL, sortSecondHalf, (void *)&array_size);
  if (rc){
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }

  rc = pthread_create(&th3, NULL, mergeTwoHalves, (void *)&array_size);
  if (rc){
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }


  sem_post(&arrayReady); // For the first half
  sem_post(&arrayReady); // For the second half

  pthread_join(th1, (void **) &status);
  pthread_join(th2, (void **) &status);
  pthread_join(th3, (void **) &status);
}
