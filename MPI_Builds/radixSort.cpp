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
#include "caliper/cali.h"
#include <caliper/cali-manager.h>
#include "adiak.hpp"

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

int main(int argc, char* argv[]){
    CALI_CXX_MARK_FUNCTION;

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
    cali::ConfigManager mgr;
    mgr.start();

    // setup local array
    CALI_MARK_BEGIN("data_init_runtime");
    local_subarray = (int *) malloc(sizeof(int) * arySize/numtasks); //allocate space to 
    local_size = setup_input(local_subarray, arySize, argv[2]);
    CALI_MARK_END("data_init_runtime");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");
    //start by calculating max, and performing a mpi allreduce to get the max value and then broadcast
    int max = getMax(local_subarray, local_size);
    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    // reducing and broadcasting a single value
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");
    //gather the results, and distribute to all processes
    MPI_Allreduce(&max, &globalMax, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD); //wait for all processors to get the max value
    
    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");


    // large computation
    //global max tells us how many iterations we need to perform for local sort, and rearranging.
    int* localBufferRearrange = (int*) malloc(sizeof(int) * local_size); //array to hold the transitionary results, as we need to retain the original array for sending
    int* combinedCountHistogram = (int*) malloc(sizeof(int) * 10 * numtasks); //array to hold all the count histograms so we can calculate for index

    //sum, prefix sum, and left sum arrays
    for (int exponent = 1;  globalMax/exponent > 0; exponent*= 10)
    {
        CALI_MARK_BEGIN("comp"); //large computation because sorting values inarray
        CALI_MARK_BEGIN("comp_large");
        
        int countHistogram[10] = {0}; // histogram for current digit place
        int allCountsSum[10] = {0}; // histogram for aggregate array
        int allCountsPrefixSum[10] = {0}; // cumulative sum array to be built from allCountsSum
        int allCountsSumLeft[10] = {0}; /* histogram for elements left of current proc */

        countSort(local_subarray, countHistogram, local_size, exponent); //sort by current digit
        CALI_MARK_END("comp_large");
        CALI_MARK_END("comp");
        
        CALI_MARK_BEGIN("comm");
        CALI_MARK_BEGIN("comm_small");        
        MPI_Barrier(MPI_COMM_WORLD); //wait for all processors to sort local arrays first

        // MPI_Gather the histograms to get a cumulative one.
        MPI_Allgather(countHistogram, 10, MPI_INTEGER, combinedCountHistogram, 10, MPI_INTEGER, MPI_COMM_WORLD);
        //use the combinedCountHistogram to determine global location. but first, we need to compress it into prefixsum, sum, and left sum
        CALI_MARK_END("comm_small");
        CALI_MARK_END("comm");

        // THIS CAN BE COMPUTATION LARGE DEPENDING ON NUMBER OF TASKS
            
        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_large");
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
        CALI_MARK_END("comp_large");
        CALI_MARK_END("comp");

        CALI_MARK_BEGIN("comm");
        CALI_MARK_BEGIN("comm_small");
        MPI_Barrier(MPI_COMM_WORLD); //wait for sequential
        CALI_MARK_END("comm_small");
        CALI_MARK_END("comm");

        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_large");
        MPI_Request request;
        MPI_Status status;      

        //keep track of the elements we sent from this processor
        int lsdSent[10] = {0}; 

        // communicate with other processes to place items in correct location using MPI_Send and MPI_Recv
        // data rearrangement
        // we do this with an int[2] array, where the first element is the value and the second element is the index
        int valueAndIndex[2];
        int value, lsd, destIndex, destProcess, localDestIndex;
        for (int i = 0; i < local_size; i++)
        {
            value = local_subarray[i];
            lsd = (local_subarray[i] / exponent) % 10;

            // calculate the destination index
            destIndex = allCountsPrefixSum[lsd] - allCountsSum[lsd] + allCountsSumLeft[lsd] + lsdSent[lsd];
            
            // increment count of elements with key lsd is to be sent
            lsdSent[lsd]++;
            destProcess = destIndex / local_size; 

            // then Isend (nonblocking for speed) and recv, and place into the correct location
            // this is any processor
            
            valueAndIndex[0] = value;
            valueAndIndex[1] = destIndex; 
            CALI_MARK_BEGIN("comm");
            CALI_MARK_BEGIN("comm_large");
            MPI_Isend(&valueAndIndex, 2, MPI_INT, destProcess, 0, MPI_COMM_WORLD, &request);
            MPI_Recv(valueAndIndex, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            CALI_MARK_END("comm_large");
            CALI_MARK_END("comm");
            localDestIndex = valueAndIndex[1] % local_size; //interpret the global index as a local one 
            localBufferRearrange[localDestIndex] = valueAndIndex[0]; //assign to the right location
        
        }
        CALI_MARK_END("comp_large");
        CALI_MARK_END("comp");

        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_small");
        // set localBufferFinal to the right location
        for (int i = 0; i < local_size; i++)
        {
            local_subarray[i] = localBufferRearrange[i];
        }

        CALI_MARK_END("comp_small");
        CALI_MARK_END("comp");
        
        CALI_MARK_BEGIN("comm");
        CALI_MARK_BEGIN("comm_small");
        MPI_Barrier(MPI_COMM_WORLD); //wait for all processors to finish rearranging
        CALI_MARK_END("comm_small");
        CALI_MARK_END("comm");

    }
        
    //after iterating through every digit of max digit, and resending, and reorganizing, we have completed radix
    
    
    CALI_MARK_BEGIN("correctness_check");
    rc = verify_sort(local_subarray, local_size, 0);
    CALI_MARK_END("correctness_check");
    if (rc) {
        printf("Process %d: Sort check failed.\n", taskid);
    } else {
        printf("Process %d: Sort check succeeded.\n", taskid);
    }

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "radix"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi"); // e.g. "mpi"
    adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", arySize); // The number of elements in input dataset (1000)
    adiak::value("input_type", argv[2]); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", numtasks); // The number of processors (MPI ranks)
    adiak::value("scalability", "weak"); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", 23); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "online"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

   // Flush Caliper output before finalizing MPI
    mgr.stop();
    mgr.flush();
    MPI_Finalize();
    return rc;
}