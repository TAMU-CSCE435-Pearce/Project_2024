// #include "mpi.h"
// #include <stdio.h>
// #include <time.h>
// #include <math.h>
// #include <stdlib.h>
// #include <caliper/cali.h>
// #include <caliper/cali-manager.h>
// #include <adiak.hpp>

// #define MASTER 0

// int process_rank;
// int* array;
// int array_size;

// int ComparisonFunc(const void* a, const void* b) {
//     return (*(int*) a - *(int*) b);
// }

// void CompareLow(int j) {

//     //Send entire array to paired H Process
//     //Exchange with a neighbor whose (d-bit binary) processor number differs only at the jth bit.
//     int send_counter = 0;
//     int* buffer_send = (int*) malloc((array_size + 1) * sizeof(int));
//     MPI_Send(&array[array_size - 1], 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

//     //Receive new min of sorted numbers
//     int min;
//     int recv_counter;
//     int* buffer_recieve = (int*) malloc((array_size + 1) * sizeof(int));
//     MPI_Recv(&min, 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

//     //Buffers all values which are greater than min send from H Process.
//     for (int i = 0; i < array_size; i++) {
//         if (array[i] > min) {
//             buffer_send[send_counter + 1] = array[i];
//             send_counter++;
//         } else {
//             break;
//         }
//     }
//     buffer_send[0] = send_counter;

//     //Send partition to and receive it from the paired H process
//     MPI_Send(buffer_send, send_counter, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);
//     MPI_Recv(buffer_recieve, array_size, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

//     //Take received buffer of values from H Process which are smaller than current max
//     for (int i = 1; i < buffer_recieve[0] + 1; i++) {
//         if (array[array_size - 1] < buffer_recieve[i]) {
//             //Store value from message
//             array[array_size - 1] = buffer_recieve[i];
//         } else {
//             break;
//         }
//     }

//     //Sequential Sort
//     qsort(array, array_size, sizeof(int), ComparisonFunc);

//     //Reset the state of the heap from Malloc
//     free(buffer_send);
//     free(buffer_recieve);

//     return;
// }

// void CompareHigh(int j) {

//     //Receive max from L Process's entire array
//     int max;
//     int recv_counter;
//     int* buffer_recieve = (int*) malloc((array_size + 1) * sizeof(int));
//     MPI_Recv(&max, 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

//     //Send min to L Process of current process's array
//     int send_counter = 0;
//     int* buffer_send = (int*) malloc((array_size + 1) * sizeof(int));
//     MPI_Send(&array[0], 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

//     //Buffer a list of values which are smaller than max value
//     for (int i = 0; i < array_size; i++) {
//         if (array[i] < max) {
//             buffer_send[send_counter + 1] = array[i];
//             send_counter++;
//         } else {
//             break;
//         }
//     }

//     //Receive blocks greater than min from paired slave
//     MPI_Recv(buffer_recieve, array_size, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//     recv_counter = buffer_recieve[0];

//     //Send partition to paired slave
//     buffer_send[0] = send_counter;
//     MPI_Send(buffer_send, send_counter, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

//     //Take received buffer of values from L Process which are greater than current min
//     for (int i = 1; i < recv_counter + 1; i++) {
//         if (buffer_recieve[i] > array[0]) {
//             //Store value from message
//             array[0] = buffer_recieve[i];
//         } else {
//             break;
//         }
//     }

//     //Sequential Sort
//     qsort(array, array_size, sizeof(int), ComparisonFunc);

//     //Reset the state of the heap from Malloc
//     free(buffer_send);
//     free(buffer_recieve);

//     return;
// }

// int main(int argc, char * argv[]) {
//     CALI_CXX_MARK_FUNCTION;

//     //Variables for adiak and other
//     std::string algorithm = "bitonic";
//     std::string programming_model = "mpi";
//     const char* data_type = typeid(int).name();
//     int size_of_data_type = sizeof(int);
//     int input_size = atoi(argv[1]);
//     std::string input_type = "random";
//     std::string scalability = "weak";
//     int group_number = 14;
//     std::string implementation_source = "online";

//     //For timing purposes
//     double timer_start;
//     double timer_end;
//     int num_processes;

//     //Initialization, get number of processes & this PID/rank
//     MPI_Init(&argc, &argv);
//     MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
//     MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

//     //Create caliper ConfigManager object
//     cali::ConfigManager mgr;
//     mgr.start();

//     //Caliper region names
//     const char* data_init_runtime = "data_init_runtime";
//     const char* comm = "comm";
//     const char* comm_send_rcv = "comm_send_rcv";
//     const char* comm_gather = "comm_gather";
//     const char* comp = "comp";
//     const char* comp_small = "comp_small";
//     const char* comp_large = "comp_large";
//     const char* correctness_check = "correctness_check";

// //     CALI_MARK_BEGIN(data_init_runtime);

// //     CALI_MARK_END(data_init_runtime);

// //     CALI_MARK_BEGIN(comm);

// //     CALI_MARK_END(comm);

// //     CALI_MARK_BEGIN(comp);

// //     CALI_MARK_END(comp);

//     //Initialize array for random numbers
//     array_size = input_size / num_processes;
//     array = (int*) malloc(array_size * sizeof(int));

//     //Generate random numbers for sorting (within each process)
//     srand(time(NULL));  //Needed for rand()
//     for (int i = 0; i < array_size; i++) {
//         array[i] = rand() % (input_size);
//     }

//     printf("Displaying initial random array:\n");
//     for (int i = 0; i < input_size; i++) {
//         printf("%d ",array[i]);
//     }
//     printf("\n\n");

//     //Blocks until all processes have finished generating
//     MPI_Barrier(MPI_COMM_WORLD);

//     //Cube dimension
//     int dimensions = (int)(log2(num_processes));

//     //Start timer before starting first sort operation (first iteration)
//     if (process_rank == MASTER) {
//         printf("Number of Processes spawned: %d\n", num_processes);
//         printf("Size of array: %d\n", input_size);
//         timer_start = MPI_Wtime();
//     }

//     //Sequential sort
//     qsort(array, array_size, sizeof(int), ComparisonFunc);

//     //Bitonic sort follows
//     for (int i = 0; i < dimensions; i++) {
//         for (int j = i; j >= 0; j--) {
//             //(window_id is even AND jth bit of process is 0)
//             //OR (window_id is odd AND jth bit of process is 1)
//             if (((process_rank >> (i + 1)) % 2 == 0 && (process_rank >> j) % 2 == 0) || ((process_rank >> (i + 1)) % 2 != 0 && (process_rank >> j) % 2 != 0)) {
//                 CompareLow(j);
//             } else {
//                 CompareHigh(j);
//             }
//         }
//     }

//     //Blocks until all processes have finished sorting
//     MPI_Barrier(MPI_COMM_WORLD);

//     if (process_rank == MASTER) {
//         timer_end = MPI_Wtime();
//         printf("Displaying sorted array:\n");

//         //Print sorting results
//         for (int i = 0; i < array_size; i++) {
//             printf("%d ",array[i]);
//         }

//         printf("\n\n");
//         printf("Time Elapsed (Sec): %f\n", timer_end - timer_start);

//         CALI_MARK_BEGIN(correctness_check);
//         bool correct = true;
//         for (int i = 1; i < array_size; i++) {
//             if (array[i] < array[i - 1]) {
//                 correct = false;
//             }
//         }
//         if (correct) {
//             printf("Array is correctly sorted!\n");
//         } else {
//             printf("Array is not correctly sorted.\n");
//         }
//         CALI_MARK_END(correctness_check);
//     }

//     adiak::init(NULL);
//     adiak::launchdate();    // launch date of the job
//     adiak::libraries();     // Libraries used
//     adiak::cmdline();       // Command line used to launch the job
//     adiak::clustername();   // Name of the cluster
//     adiak::value("algorithm", algorithm); // The name of the algorithm you are using (e.g., "merge", "bitonic")
//     adiak::value("programming_model", programming_model); // e.g. "mpi"
//     adiak::value("data_type", data_type); // The datatype of input elements (e.g., double, int, float)
//     adiak::value("size_of_data_type", size_of_data_type); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
//     adiak::value("input_size", input_size); // The number of elements in input dataset (1000)
//     adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
//     adiak::value("num_procs", num_processes); // The number of processors (MPI ranks)
//     adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
//     adiak::value("group_num", group_number); // The number of your group (integer, e.g., 1, 10)
//     adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

//     //Reset the state of the heap from Malloc
//     free(array);

//     //Flush Caliper output before finalizing MPI
//     mgr.stop();
//     mgr.flush();
//     MPI_Finalize();
//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "mpi.h"

struct timeval startwtime, endwtime;
double seq_time;

int cmpfuncASC (const void * a, const void * b);
int cmpfuncDESC (const void * a, const void * b);
void init(int* a,int seed);

int N;
void test(int* a, int num_tasks,int rank);
void compute_top(int partner, int dir,int *a,int rank, int k);
void compute_bottom(int partner,int dir,int *a,int rank , int k);
inline void swap(int *a, int *b);


/** the main program **/
int main(int argc, char **argv) {

  if (argc != 2) {
    printf("Usage: %s q\n  where n=2^q is problem size (power of two)\n",argv[0]);
    exit(1);
  }
  int  numtasks, rank, rc,i,j, *a, k, dir;
  
  rc = MPI_Init(&argc,&argv);
  if (rc != MPI_SUCCESS) {
     printf ("Error starting MPI program. Terminating.\n");
     MPI_Abort(MPI_COMM_WORLD, rc);
  }
  MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank); 
  
  N = (1<<atoi(argv[1]))/numtasks;
  a = (int *) malloc(N * sizeof(int));  
  init(a,rank);  
  //start counting time
  gettimeofday (&startwtime, NULL);
  if ((rank%2 == 0) ) qsort(a, N, sizeof(int), cmpfuncASC); //ASCENDING
  else qsort(a, N, sizeof(int), cmpfuncDESC); //DESCENDING
  MPI_Barrier(MPI_COMM_WORLD);
  
  for (k=1;k<numtasks;k<<=1){
    dir = ((k*2 & rank) == 0);
   
    for (j=k;j>=1;j>>=1){
      int partner = rank^j;  
      if(rank<partner) compute_bottom(partner,dir,a,rank,j);
      else compute_top(partner,dir,a,rank,j);
      MPI_Barrier(MPI_COMM_WORLD);
    }
    if (dir) qsort(a, N, sizeof(int), cmpfuncASC); //ASCENDING
    else qsort(a, N, sizeof(int), cmpfuncDESC); //DESCENDING
  }
  //program finished
  gettimeofday (&endwtime, NULL);
  seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		      + endwtime.tv_sec - startwtime.tv_sec);

  if(rank == 1) printf("Parallel bitonic sort time = %f\n", seq_time);
  test(a,numtasks,rank);
}

/** function called by the smaller partner to swap the bottom part
    of both tables 
**/
void compute_bottom(int partner, int dir,int *a,int rank, int k){
  MPI_Status mpistat;
  int rc,i;
  int *swap_temp = (int *)malloc(N/2*sizeof(int));
  //tag = 1 means you need to proccess the message. 
  //since my job is to compute the top part, im sending the bottom for the other guy.
  rc = MPI_Send(a+N/2,N/2,MPI_INT,partner,1,MPI_COMM_WORLD);
  if (rc != MPI_SUCCESS) {
    printf ("Error idle. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  //i need to compare this to my bottom half.
  MPI_Recv(swap_temp,N/2,MPI_INT,partner,1,MPI_COMM_WORLD,&mpistat);
  if(dir){
    for(i=0;i<N/2;i++){
      if(a[i] > swap_temp[i]) swap(&a[i],&swap_temp[i]);  
    }
   
  }
  else{
    for(i=0;i<N/2;i++){
      if(a[i] < swap_temp[i]) swap(&a[i],&swap_temp[i]);
    }
  }
  //this guy sends the results before receiving
  rc = MPI_Send(swap_temp,N/2,MPI_INT,partner,2,MPI_COMM_WORLD);
  if (rc != MPI_SUCCESS) {
    printf ("Error idle. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  //receiving the computed top part from the partner.
  MPI_Recv(a+N/2,N/2,MPI_INT,partner,2,MPI_COMM_WORLD,&mpistat);
}  


/** function called by the bigger partner to swap the top part
    of both tables 
**/
void compute_top(int partner, int dir,int *a,int rank, int k) {
  MPI_Status mpistat;
  int rc,i;
  int *swap_temp = (int *)malloc(N/2*sizeof(int));
  //this guy will receive before sending. If they both attemp to send first it may lead to a deadlock.
  MPI_Recv(swap_temp,N/2,MPI_INT,partner,1,MPI_COMM_WORLD,&mpistat);
  rc = MPI_Send(a,N/2,MPI_INT,partner,1,MPI_COMM_WORLD);
  if (rc != MPI_SUCCESS) {
    printf ("Error idle. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  if(dir){ 
      for(i=0;i<N/2;i++){
	if(a[i+N/2] < swap_temp[i]) swap(&a[i+N/2],&swap_temp[i]);
      }   
  }
  else{
    for(i=0;i<N/2;i++){
      if(a[i+N/2] > swap_temp[i]) swap(&swap_temp[i],&a[i+N/2]);
    }
  }
  //Proccessing has ended. This guy will first receive and then send to avoid deadlock.
  MPI_Recv(a,N/2,MPI_INT,partner,2,MPI_COMM_WORLD,&mpistat);
  rc = MPI_Send(swap_temp,N/2,MPI_INT,partner,2,MPI_COMM_WORLD);
  if (rc != MPI_SUCCESS) {
    printf ("Error idle. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  
}

// the first element must be the lowest target number
inline void swap(int *a, int *b){

  int t;
  t = *a;
  *a = *b;
  *b = t;
}

void init(int* a,int seed) {
  int i;
  srand(seed+3);
  for (i = 0; i < N; i++) {
    a[i] = rand() % (4*N);
  }
}

int cmpfuncASC (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

int cmpfuncDESC (const void * a, const void * b)
{
   return ( *(int*)b - *(int*)a );
}
void test(int* a, int num_tasks,int rank)
{
  int flag=1;
  int i,rc;
  int min = a[0];
  int max = 0;

  
  for(i=0;i<N-1;i++){
    if (a[i+1] < a[i]) flag = 0;
    if (a[i] > max) max = a[i];
    if (a[i] < min) min = a[i];
  }
  if(rank == 0){
   
    MPI_Status mpistat;
    int other_flag;
    int *minimum = (int *)malloc(num_tasks*sizeof(int));
    int *maximum = (int *)malloc(num_tasks*sizeof(int));
    minimum[0] = min;
    maximum[0] = max;
    for(i=1;i<num_tasks;i++){
      //tag = 3 means local sorting flag
      MPI_Recv(&other_flag,1,MPI_INT,i,3,MPI_COMM_WORLD,&mpistat);
      flag = flag & other_flag;
      //tag = 4 means minimum
      MPI_Recv(minimum+i,1,MPI_INT,i,4,MPI_COMM_WORLD,&mpistat);
      //tag = 5 means maximum
      MPI_Recv(maximum+i,1,MPI_INT,i,5,MPI_COMM_WORLD,&mpistat);
    }
    if(flag) printf("everyone is locally sorted\n");
    else {
      printf("at least one failed to sort himself locally\n");
      exit(1);
    }
    //from now on flag has a new interpretetion
    for(i=0;i<num_tasks-1;i++){
      if(maximum[i] > minimum[i+1]) flag = 0;   
    }
    //if flag is still 1 at this point, test has been passed
    if(flag) printf("parallel bitonic sort was a success\n");
    else printf("bitonic sort has failed\n");
    
    
  } 
  else{
    //tag = 3 means local sorting flag
    rc = MPI_Send(&flag,1,MPI_INT,0,3,MPI_COMM_WORLD);
    //tag = 4 means minimum
    rc = MPI_Send(&min,1,MPI_INT,0,4,MPI_COMM_WORLD);
    //tag = 5 means maximum
    rc = MPI_Send(&max,1,MPI_INT,0,5,MPI_COMM_WORLD);
     
    
  } 
  
}