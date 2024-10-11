/******************************************************************************
* FILE: radixSort.cpp
* DESCRIPTION:  
*   MPI Distributed Radix Sort
*   All numtasks processes generate their own sections of the overall array,
*   bucket sort their local subarrays using Radix, then sequentially determine prefix sum to
*   identify location/index of the value.
*   Sources: 
*   https://himnickson.medium.com/parallel-radix-sort-algorithm-using-message-passing-interface-mpi-31b9e4677fbd
*   https://www.geeksforgeeks.org/radix-sort/
* AUTHOR: Ren Mai
* LAST REVISED: 10/8/2024
******************************************************************************/
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "shared_functionality.h"
// #include "caliper/cali.h"
// #include <caliper/cali-manager.h>
// #include "adiak.hpp"

/// <summary>
/// Utility function to get maximum value in the array.
/// </summary>
/// <param name="ary"></param>
/// <param name="n"></param>
/// <returns></returns>
int getMax(int* ary, int n)
{
    int max = ary[0];
    for (int i = 1; i < n; i++)
        if (ary[i] > max)
            max = ary[i];
    return max;
}

/// <summary>
/// Function to do counting sort of the array according to the digit
/// represented by exp.
/// Typically, we are doing binary instead of decimal for digit extraction.
/// </summary>
/// <param name="ary"></param>
/// <param name="n"></param>
/// <param name="exp"></param>
void countSort(int* ary, int* countHistogram, int n, int exp)
{
    // Output array, temporary storage that will be written to ary afterwards
    int* output = (int*) malloc(sizeof(int) * n); //allocate space to store output array
    int i, count[10] = { 0 }; //initialize count array

    // Store count of occurrences
    // in count[]
    for (i = 0; i < n; i++)
        count[(ary[i] / exp) % 10]++;


    //copy count to histogramRet so we can use it for prefix
    for (i = 0; i < 10; i++)
		countHistogram[i] = count[i];
    
    // Change count[i] so that count[i]
    // now contains actual position
    // of this digit in output[]
    for (i = 1; i < 10; i++)
        count[i] += count[i - 1];

    // Build the output array
    for (i = n - 1; i >= 0; i--) {
        output[count[(ary[i] / exp) % 10] - 1] = ary[i];
        count[(ary[i] / exp) % 10]--;
    }

    // Copy the output array to arr[],
    // so that arr[] now contains sorted
    // numbers according to current digit
    for (i = 0; i < n; i++)
        ary[i] = output[i];

    free(output);
}

void printOutput(int* ary, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", ary[i]);
    }
    printf("\n");
}
/// <summary>
/// Sorts the local array using radix sort, and then communicates with other processes to place items in correct location.
/// We will not be using this function, as it sorts the entire array. For parallel computation, we want to sort by digit, rearrange, sort by digit, rearrange, and so on.
/// </summary>
/// <param name="array"></param>
/// <param name="size"></param>
// void local_radix_sort(int* array, int size){

//     //get maximum value in the array in order to know the number of digits
//     int max = getMax(array, size);

//     // perform counting sort for every digit, here we are doing decimal digit
//     for (int exp = 1; max / exp > 0; exp *= 10)
//         countSort(array, size, exp);

// }

int main(int argc, char* argv[]){
    // CALI_CXX_MARK_FUNCTION;

    //check arguments:    
    if (argc < 2) {
        printf("Please provide the array size.\n");
        return 22; // EINVAL
    }

    
    //declare variables used:
    int arySize,
        taskid,                /* a task identifier */
        local_size,            /* Size of the array an individual task computes*/
        numworkers,            /* number of worker tasks */
        source,                /* task id of message source */
        dest,                  /* task id of message destination */
        mtype,                 /* message type */
        numtasks,              /* number of tasks in partition */ 
        globalMax,             /* Maximum value in the array */
        rc;        			   /* return code */
    MPI_Status status;

    int* local_subarray;     /*stores local array that the process is sorting*/
    arySize = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    
    numworkers = numtasks - 1;

    if (argc < 3) {
	printf("Please provide the input type.\n");
        MPI_Abort(MPI_COMM_WORLD, 22);
        return 22; // EINVAL
    }
    // Create caliper ConfigManager object
    // cali::ConfigManager mgr;
    // mgr.start();

    // setup local array
    // CALI_MARK_BEGIN("data_init_runtime");
    local_subarray = (int *) malloc(sizeof(int) * arySize/numtasks); //allocate space to 
    local_size = setup_input(local_subarray, arySize, argv[2]);
    // CALI_MARK_END("data_init_runtime");

    //start by calculating max, and performing a mpi allreduce to get the max value and then broadcast
    int max = getMax(local_subarray, local_size);

    //gather the results, and distribute to all processes
    MPI_Allreduce(&max, &globalMax, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD); //wait for all processors to get the max value


    //global max tells us how many iterations we need to perform for local sort, and rearranging.
    int* localBufferRearrange = (int*) malloc(sizeof(int) * local_size); //array to hold the transitionary results, as we need to retain the original array for sending
    int* combinedCountHistogram = (int*) malloc(sizeof(int) * 10 * numtasks); //array to hold all the count histograms so we can calculate for index

    //sum, prefix sum, and left sum arrays
    // printf("Process %d: Global max is %d\n", taskid, globalMax);
    for (int exponent = 1;  globalMax/exponent > 0; exponent*= 10)
    {
        int countHistogram[10] = {0}; // histogram for current digit place
        int allCountsSum[10] = {0}; // histogram for aggregate array
        int allCountsPrefixSum[10] = {0}; // cumulative sum array to be built from allCountsSum
        int allCountsSumLeft[10] = {0}; /* histogram for elements left of current proc */

        countSort(local_subarray, countHistogram, local_size, exponent); //sort by current digit
        MPI_Barrier(MPI_COMM_WORLD); //wait for all processors to sort local arrays first

        // printf("Process %d: After sorting by digit %d\n", taskid, exponent);
        // printOutput(local_subarray, local_size);

        //TODO: compute prefix sum to determine location of each value
        // MPI_Gather the histograms to get a cumulative one.
        MPI_Allgather(countHistogram, 10, MPI_INTEGER, combinedCountHistogram, 10, MPI_INTEGER, MPI_COMM_WORLD);
        //use the combinedCountHistogram to determine global location. but first, we need to compress it into prefixsum, sum, and left sum
        // printf("Process %d: Combined count histogram\n", taskid);
        // printOutput(combinedCountHistogram, 10 * numtasks);

        for (int i = 0; i < 10 * numtasks; i++) {
          int lsd = i % 10;
          int p = i / 10; //processor rank
          int val = combinedCountHistogram[i];

          // add histogram values to allCountsSumLeft for all processors "left" of current processor
          if (p < taskid) {
            allCountsSumLeft[lsd] += val;
          }
          allCountsSum[lsd] += val;
          allCountsPrefixSum[lsd] += val;
        }        

        // build cumulative sum array
        for (int i = 1; i < 10; i++) {
          allCountsPrefixSum[i] += allCountsPrefixSum[i - 1];
        }

        MPI_Barrier(MPI_COMM_WORLD); //wait for sequential
        // request and status variables to be passed to MPI send and receive functions
        // printf("Process %d: All counts sum\n", taskid);
        // printOutput(allCountsSum, 10);
        // printf("Process %d: All counts prefix sum\n", taskid);
        // printOutput(allCountsPrefixSum, 10);
        // printf("Process %d: All counts sum left\n", taskid);
        // printOutput(allCountsSumLeft, 10);
        MPI_Request request;
        MPI_Status status;      

        //keep track of the elements we sent from this processor
        int lsdSent[10] = {0}; 

        //TODO: communicate with other processes to place items in correct location using MPI_Send and MPI_Recv
        // data rearrangement
        // we do this with an int[2] array, where the first element is the value and the second element is the index
        int valueAndIndex[2];
        int value, lsd, destIndex, destProcess, localDestIndex;
        for (int i = 0; i < local_size; i++)
        {
            value = local_subarray[i];
            lsd = (local_subarray[i] / exponent) % 10;

            //FIXME: destIndex is not being calculated correctly
            //printf the allCountsPrefixSum, allCountsSum, allCountsSumLeft, and lsdSent arrays to debug
            // determine which process to send to based on the value, and the according index
            // destIndex = allCountsPrefixSum[lsd] - allCountsSum[lsd] + allCountsSumLeft[lsd] + lsdSent[lsd];
            destIndex = allCountsPrefixSum[lsd] - allCountsSum[lsd] + allCountsSumLeft[lsd] + lsdSent[lsd];
            
            // printf("Process %d: lsd %d, allCountsPrefixSum %d, allCountsSum %d, allCountsSumLeft %d, lsdSent %d\n", taskid, lsd, allCountsPrefixSum[lsd], allCountsSum[lsd], allCountsSumLeft[lsd], lsdSent[lsd]);
            // printf("destIndex %d\n", destIndex);
            // increment count of elements with key lsd is to be sent
            lsdSent[lsd]++;
            destProcess = destIndex / local_size; 
            // printf("value %d, lsd %d, destIndex %d, destProcess %d\n", value, lsd, destIndex, destProcess);
            //set the array to be sent and send!

            // TODO: make sure that you are not sending to yourself
            // then Isend (nonblocking for speed) and recv, and place into the correct location
            // this is any processor
            // printf("Process %d: Sending %d to process %d\n", taskid, value, destProcess);
            //check if invalid destination and abort
            
            valueAndIndex[0] = value;
            valueAndIndex[1] = destIndex; 

            MPI_Isend(&valueAndIndex, 2, MPI_INT, destProcess, 0, MPI_COMM_WORLD, &request);
            MPI_Recv(valueAndIndex, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            localDestIndex = valueAndIndex[1] % local_size; //interpret the global index as a local one 
            localBufferRearrange[localDestIndex] = valueAndIndex[0]; //assign to the right location
            //print array
        }


        // set localBufferFinal to the right location
        for (int i = 0; i < local_size; i++)
        {
            local_subarray[i] = localBufferRearrange[i];
        }
        MPI_Barrier(MPI_COMM_WORLD); //wait for all processors to finish rearranging
    }
        
    //after iterating through every digit of max digit, and resending, and reorganizing, we have completed radix
    
    
    // CALI_MARK_BEGIN("correctness_check");
    rc = verify_sort(local_subarray, local_size, 0);
    // CALI_MARK_END("correctness_check");
    if (rc) {
        printf("Process %d: Sort check failed.\n", taskid);
    } else {
        printf("Process %d: Sort check succeeded.\n", taskid);
    }

//     adiak::init(NULL);
//     adiak::launchdate();    // launch date of the job
//     adiak::libraries();     // Libraries used
//     adiak::cmdline();       // Command line used to launch the job
//     adiak::clustername();   // Name of the cluster
//     adiak::value("algorithm", "radix"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
//     adiak::value("programming_model", "mpi"); // e.g. "mpi"
//     adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
//     adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
//     adiak::value("input_size", n); // The number of elements in input dataset (1000)
//     adiak::value("input_type", argv[2]); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
//     adiak::value("num_procs", p); // The number of processors (MPI ranks)
//     // TODO: adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
//     adiak::value("group_num", 23); // The number of your group (integer, e.g., 1, 10)
//     // TODO: adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

//    // Flush Caliper output before finalizing MPI
//     mgr.stop();
//     mgr.flush();
    MPI_Finalize();
    return rc;
}