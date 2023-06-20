# Socket-Programming
Design Document for Sync Project
1. INTRODUCTION
In this design document, we have tried to explain various design decisions and synchronizations
used in the development of the project. We would cover the following topics:
The synchronization mechanism between the threads.
The synchronization mechanism between Main program and Computation program
The communication between Server and Client
2. SYNCHRONIZATION SORTING THREADS
We have implemented the parallel version of the merge sort. The sorting function takes unsorted
list of the numbers and invokes number of threads to accomplish the task.
Each thread in turns call the recursively merge sort function to sort a certain section. The main
thread waits for all the threads to complete.
Listing 1. Pseudo Code
P a r a l l e l _ S o r t ( in tege r _A r r ay , numbe r _o f _in tege rs )
c r e a t e _ t h r e a d (M)
M _ th re ad s _p roce s s _ so r ting ( )
wait_for_M_Threads ( )
merge_M_Arrays ( )
p rin t _ S o r ted _ ou tpu t ( )
3. SYNCHRONIZATION MAIN AND SORTING
Between the main process and the forked process, we use Pipes to communicate. In order to
main the queuing structure between the two process, we use semaphore based design. A shared
semaphore is created and initialized with the value of Q. Before the parent process can write any
command to the Pipe, it needs to acquire a semaphore. Once the child process reads the data
from the pipe, it invokes the sort function. At the end of the sorting, semaphore is incremented.
Listing 2. Pseudo Code
p a ren t _p r o ce s s :
Acquire_semaphore ( )
w ri t e _ r e q u e s t _ t o _ c hil d ( )
Child _p roce s s :
re ad _ reques t _ f rom _p a ren t ( )
inv oke _p a r allel _me rge ( )
release_semaphore ( )
4. SERVER AND CLIENT
There is no unique synchronization required between the server and the client.
