/******************************************************************************
* FILE: radixSort.cpp
* DESCRIPTION:  
*   MPI Distributed Radix Sort
*   All numtasks processes generate their own sections of the overall array,
*   bucket sort their local subarrays using Radix, then sequentially determine prefix sum to
*   identify location/index of the value.
*   Sources: https://himnickson.medium.com/parallel-radix-sort-algorithm-using-message-passing-interface-mpi-31b9e4677fbd
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
        aryPartitionSize;    /* Size of the array an individual task computes*/
        numworkers,            /* number of worker tasks */
        source,                /* task id of message source */
        dest,                  /* task id of message destination */
        mtype,                 /* message type */
        numtasks,              /* number of tasks in partition */ 
        rc;

    int* localArrayOffset;     /*stores local array that the process is sorting*/
    arySize = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &taskid);
    MPI_Comm_rank(MPI_COMM_WORLD, &numtasks);
    if (numtasks < 2)
    {
        printf("Need at least two MPI tasks. Quitting...\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(1);
    }
    
    numworkers = numtasks - 1;

    if (argc < 3) {
	printf("Please provide the input type.\n");
        MPI_Abort(MPI_COMM_WORLD, 22);
        return 22; // EINVAL
    }

    aryPartitionSize = arySize;

    // setup local array
    CALI_MARK_BEGIN("data_init_runtime");
    local_subarray = (int *) malloc(sizeof(int) * aryPartitionSize); //allocate space to 
    local_size = setup_input(local_subarray, arySize, argv[2]);
    CALI_MARK_END("data_init_runtime");

    //TODO: perform local sort

    //TODO: compute prefix sum

    //TODO: communicate with other processes to place items in correct location


    CALI_MARK_BEGIN("correctness_check");
    rc = verify_sort(local_subarray, local_size, 0);
    CALI_MARK_END("correctness_check");
    if (rc) {
        printf("Process %d: Sort check failed.\n", local_rank);
    } else {
        printf("Process %d: Sort check succeeded.\n", local_rank);
    }

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "merge"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi"); // e.g. "mpi"
    adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", n); // The number of elements in input dataset (1000)
    adiak::value("input_type", argv[2]); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", p); // The number of processors (MPI ranks)
    // TODO: adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", 23); // The number of your group (integer, e.g., 1, 10)
    // TODO: adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

    MPI_Finalize();
    return rc;
}