/******************************************************************************
* FILE: merge_sort.cpp
* DESCRIPTION:  
*   Merge Sort Algorithm implementation
*   In this code, tasks spawn 0-2 child processes to process half 
*   of an array using merge sort, and then combine the result all the
*   wau up to output an array sorted least to greatest
* AUTHOR: Ariela Mitrani
* LAST REVISED: 10/04/2024
******************************************************************************/
#include "mpi.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <caliper/cali.h> //for performance analysis
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define MASTER 0               /* taskid of first task */
#define FROM_PARENT 1          /* setting a message type */
#define FROM_CHILD 2          /* setting a message type */

const char* main_cali = "main";
const char* data_init_runtime = "data_init_runtime";
const char* correctness_check = "correctness_check";
const char* comm = "comm";
const char* comm_small = "comm_small";
const char* comm_large = "comm_large";
const char* comp = "comp";
const char* comp_small = "comp_small";
const char* comp_large = "comp_large";

double* combineSortedArrays(double* left, double* right, int leftSize, int rightSize) {
   double* returnArray = new double[leftSize+rightSize];
   int r = 0;
   int l = 0;
   for(int i = 0; i < leftSize+rightSize; i++){
    if(r == rightSize) {
      returnArray[i] = left[l];
      l++;
    }
    else if(l == leftSize) {
      returnArray[i] = right[r];
      r++;
    }
    else if(right[r] < left[l]) {
      returnArray[i] = right[r];
      r++;
    }
    else {
      returnArray[i] = left[l];
      l++;
    }
  }
  return returnArray;
}

int getChildCount(int id, int numtasks) {
   int leftChild = 2*id+1;
   if(leftChild + 1 < numtasks) {
      return 2;
   } else if(leftChild < numtasks) {
      return 1;
   } else {
      return 0;
   }
}

double* mergeSort(double* arr, int size) {
   if(size == 1) {
      return arr;
   } else {
      int leftSize = size/2;
      int rightSize = size - leftSize;
      double* leftSorted = mergeSort(arr, leftSize);
      double* rightSorted = mergeSort(arr + leftSize, rightSize);
      return combineSortedArrays(leftSorted, rightSorted, leftSize, rightSize);
   }
}

double* startChildProcesses(int id, int childCount, double* arr, int arrSize) {
   //printf("process %d, children %d\n", id, childCount);
   CALI_MARK_BEGIN(comp);
   CALI_MARK_BEGIN(comp_small);
   int leftSize = arrSize/2;
   int rightSize = arrSize - leftSize;
   MPI_Status status;
   double* returnArr;
   int firstChildId = 2*id+1;
   if(childCount == 0) {
      CALI_MARK_END(comp_small);
      CALI_MARK_BEGIN(comp_large);
      returnArr = mergeSort(arr, arrSize);
      CALI_MARK_END(comp_large);
      CALI_MARK_END(comp);
      //printf("Process %d sorted array: ", id);
      //printArray(returnArr, arrSize);
      return returnArr;
   } else if(childCount == 1) {
      CALI_MARK_END(comp_small);
      //send the left half of the array and its size to child for sorting
      CALI_MARK_BEGIN(comm);
      CALI_MARK_BEGIN(comm_small);
      MPI_Send(&leftSize, 1, MPI_INT, firstChildId, FROM_PARENT, MPI_COMM_WORLD);
      CALI_MARK_END(comm_small);
      CALI_MARK_BEGIN(comm_large);
      MPI_Send(arr, leftSize, MPI_DOUBLE, firstChildId, FROM_PARENT, MPI_COMM_WORLD);
      CALI_MARK_END(comm_large);
      CALI_MARK_END(comm);

      //sort the right half of the array ourselves
      CALI_MARK_BEGIN(comp_large);
      double* rightSorted = mergeSort(arr+leftSize, rightSize);
      CALI_MARK_END(comp_large);
      CALI_MARK_END(comp);

      //receive the sorted left half of the array from child
      double leftSorted[leftSize];
      CALI_MARK_BEGIN(comm);
      CALI_MARK_BEGIN(comm_large);
      MPI_Recv(&leftSorted, leftSize, MPI_DOUBLE, firstChildId, FROM_CHILD, MPI_COMM_WORLD, &status);
      CALI_MARK_END(comm_large);
      CALI_MARK_END(comm);

      //combine and return the arrays
      CALI_MARK_BEGIN(comp);
      CALI_MARK_BEGIN(comp_large);
      returnArr = combineSortedArrays(leftSorted, rightSorted, leftSize, rightSize);
      CALI_MARK_END(comp_large);
      CALI_MARK_END(comp);
      //printf("Process %d sorted array: ", id);
      //printArray(returnArr, arrSize);

      delete[] rightSorted;
      return returnArr;
   } else { //2 children
      CALI_MARK_END(comp_small);
      CALI_MARK_END(comp);

      //send the left half of the array and its size to child for sorting
      CALI_MARK_BEGIN(comm);
      CALI_MARK_BEGIN(comm_small);
      MPI_Send(&leftSize, 1, MPI_INT, firstChildId, FROM_PARENT, MPI_COMM_WORLD);
      CALI_MARK_END(comm_small);
      CALI_MARK_BEGIN(comm_large);
      MPI_Send(arr, leftSize, MPI_DOUBLE, firstChildId, FROM_PARENT, MPI_COMM_WORLD);
      CALI_MARK_END(comm_large);

      //send the right half of the array and its size to child for sorting
      CALI_MARK_BEGIN(comm_small);
      MPI_Send(&rightSize, 1, MPI_INT, firstChildId+1, FROM_PARENT, MPI_COMM_WORLD);
      CALI_MARK_END(comm_small);
      CALI_MARK_BEGIN(comm_large);
      MPI_Send(arr + leftSize, rightSize, MPI_DOUBLE, firstChildId+1, FROM_PARENT, MPI_COMM_WORLD);
      CALI_MARK_END(comm_large);
      CALI_MARK_END(comm);

      //receive the sorted arrays
      CALI_MARK_BEGIN(comm);
      CALI_MARK_BEGIN(comm_large);
      double leftSorted[leftSize];
      double rightSorted[rightSize];
      MPI_Recv(&leftSorted, leftSize, MPI_DOUBLE, id+1, FROM_CHILD, MPI_COMM_WORLD, &status);
      MPI_Recv(&rightSorted, leftSize, MPI_DOUBLE, id+2, FROM_CHILD, MPI_COMM_WORLD, &status);
      CALI_MARK_END(comm_large);
      CALI_MARK_END(comm);

      //combine and return the arrays
      CALI_MARK_BEGIN(comp);
      CALI_MARK_BEGIN(comp_large);
      returnArr = combineSortedArrays(leftSorted, rightSorted, leftSize, rightSize);
      CALI_MARK_END(comp_large);
      CALI_MARK_END(comp);
      //printf("Process %d sorted array: ", id);
      //printArray(returnArr, arrSize);
      
      return returnArr;
   }
}

void printIntro(int numtasks, int sizeOfArray, int inputType) {
   if(inputType == RANDOM_INPUT)
      printf("Starting merge sort with %d processes, array size %d, random initial data\n", numtasks, sizeOfArray);
   if(inputType == SORTED_INPUT)
      printf("Starting merge sort with %d processes, array size %d, sorted initial data\n", numtasks, sizeOfArray);
   if(inputType == REVERSE_SORTED_INPUT)
      printf("Starting merge sort with %d processes, array size %d, reverse sorted initial data\n", numtasks, sizeOfArray);
   if(inputType == ONE_PERC_PERMUTED)
      printf("Starting merge sort with %d processes, array size %d, 1%% permuted initial data\n", numtasks, sizeOfArray);
}

int main (int argc, char *argv[])
{
   int sizeOfArray; //this is an input to the code
   int inputType;
   std::string inputString;
   if (argc == 3)
   {
       sizeOfArray = atoi(argv[1]);
       inputType = atoi(argv[2]);
       if(inputType < 0 || inputType > 3) {
         printf("\n Please provide a valid input type [0-3]");
       }
   }
   else
   {
       printf("\n Please provide arguments [sizeOfArray] [input type]");
       return 0;
   }
   int	numtasks,              /* number of tasks in partition */
   	taskid,                /* a task identifier */
   	source,                /* task id of message source */
   	dest,                  /* task id of message destination */
   	mtype;                 /* message type */
   MPI_Status status;
   int boredProcesses;
   double* sortedArr;
   double whole_computation_time;

   if(inputType == RANDOM_INPUT)
      inputString = "random";
   if(inputType == SORTED_INPUT)
       inputString = "sorted";
   if(inputType == REVERSE_SORTED_INPUT)
       inputString = "reversed";
   if(inputType == ONE_PERC_PERMUTED)
       inputString = "permuted";

   // Create caliper ConfigManager object
   cali::ConfigManager mgr;
   mgr.start();

   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
   MPI_Comm_size(MPI_COMM_WORLD,&numtasks);

   CALI_MARK_BEGIN(main_cali);
   double wholeMasterStart = MPI_Wtime();

   for(int n = 0; n < numtasks; n++){
      if(taskid == n) {
         if(taskid == MASTER) { //only our first array initializes the array to be sorted
            CALI_MARK_END(main_cali);
            printIntro(numtasks, sizeOfArray, inputType);
            CALI_MARK_BEGIN(main_cali);

            CALI_MARK_BEGIN(comp);
            CALI_MARK_BEGIN(comp_small);
            int children = getChildCount(n, numtasks);
            CALI_MARK_END(comp_small);
            CALI_MARK_END(comp);

            CALI_MARK_BEGIN(data_init_runtime);
            double* toSort = generateArray(sizeOfArray, inputType);
            CALI_MARK_END(data_init_runtime);
            //printf("****************************\nINITIAL ARRAY: \n");
            //printArray(toSort, sizeOfArray);
            //printf("****************************\n\n");
            CALI_MARK_END(main_cali);
            if(children == 2)
               printf("Process %d sending data to processes %d, %d\n", n, n+1, n+2);
            else if(children == 1)
               printf("Process %d sending data to process %d\n", n, n+1);
            else if(children == 0)
               printf("Process %d is leaf node; Calculating...\n", n);

            CALI_MARK_BEGIN(main_cali);
            sortedArr = startChildProcesses(n, children, toSort, sizeOfArray);
         } else {
            CALI_MARK_BEGIN(comp);
            CALI_MARK_BEGIN(comp_small);
            int children = getChildCount(n, numtasks);
            int leftChild = 2*n+1;
            
            int parentId = (n + n%2 - 2)/2; //(n-1)/2 if odd, (n-2)/2 if even
            int toSortSize;
            CALI_MARK_END(comp_small);
            CALI_MARK_END(comp);

            //get array and its size from parent process
            CALI_MARK_BEGIN(comm);
            CALI_MARK_BEGIN(comm_small);
            MPI_Recv(&toSortSize, 1, MPI_INT, parentId, FROM_PARENT, MPI_COMM_WORLD, &status);
            CALI_MARK_END(comm_small);
            CALI_MARK_END(comm);

            double toSort[toSortSize];

            CALI_MARK_BEGIN(comm);
            CALI_MARK_BEGIN(comm_large);
            MPI_Recv(&toSort, toSortSize, MPI_DOUBLE, parentId, FROM_PARENT, MPI_COMM_WORLD, &status);
            CALI_MARK_END(comm_large);
            CALI_MARK_END(comm);

            CALI_MARK_END(main_cali);
            //print some info
            if(children == 2)
               printf("Process %d sending data to processes %d, %d\n", n, leftChild, leftChild+1);
            else if(children == 1)
               printf("Process %d sending data to process %d\n", n, leftChild);
            else if(children == 0)
               printf("Process %d is leaf node; Calculating...\n", n);

            CALI_MARK_BEGIN(main_cali);
            //start child processes and receive sorted array
            double* sorted = startChildProcesses(n, children, toSort, toSortSize);

            //send sorted array back to parent process; they already know the size
            CALI_MARK_BEGIN(comm);
            CALI_MARK_BEGIN(comm_large);
            MPI_Send(sorted, toSortSize, MPI_DOUBLE, parentId, FROM_CHILD, MPI_COMM_WORLD);
            CALI_MARK_END(comm_large);
            CALI_MARK_END(comm);
         }
      }
   }

   if(taskid == MASTER) {
      CALI_MARK_BEGIN(correctness_check);
      if(!verifySorted(sortedArr, sizeOfArray)) {
         CALI_MARK_END(correctness_check);
         CALI_MARK_END(main_cali);
         printf("There is a bug! This final array is not sorted.\n");
         exit(-1);
      }
      CALI_MARK_END(correctness_check);
   }

   CALI_MARK_END(main_cali);
   double wholeMasterEnd = MPI_Wtime();

   if(taskid == MASTER) {
      printf("Total time taken: %f\n", wholeMasterEnd-wholeMasterStart);
   }

   adiak::init(NULL);
   adiak::launchdate();    // launch date of the job
   adiak::libraries();     // Libraries used
   adiak::cmdline();       // Command line used to launch the job
   adiak::clustername();   // Name of the cluster
   adiak::value("algorithm", "merge"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
   adiak::value("programming_model", "mpi"); // e.g. "mpi"
   adiak::value("data_type", "double"); // The datatype of input elements (e.g., double, int, float)
   adiak::value("size_of_data_type", sizeof(double)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
   adiak::value("input_size", sizeOfArray); // The number of elements in input dataset (1000)
   adiak::value("input_type", inputString); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
   adiak::value("num_procs", numtasks); // The number of processors (MPI ranks)
   adiak::value("scalability", "todo"); // The scalability of your algorithm. choices: ("strong", "weak")
   adiak::value("group_num", 1); // The number of your group (integer, e.g., 1, 10)
   adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").


   mgr.stop();
   mgr.flush();

   MPI_Finalize();
}