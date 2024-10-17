/******************************************************************************
* FILE: sample_sort.cpp
* DESCRIPTION:  
*   Sample Sort - Parallel in C++
* AUTHOR: Christiana Vancura, UIN 132003142
* LAST REVISED: 10/16/2024
******************************************************************************/

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>


#include "quick_sort.h"
#include "shared_functionality.h"

#define MASTER 0               /* taskid of first task */
#define TO_MASTER 1          /* setting a message type */
#define FROM_MASTER 2          /* setting a message type */
#define TO_OTHER 3

int main (int argc, char *argv[]) {
    CALI_CXX_MARK_FUNCTION;
    
    // Create caliper ConfigManager object
    cali::ConfigManager mgr;
    mgr.start();
    
    
    int n;
    int p;
    int s;
    MPI_Status status;

    int taskid;
    int local_size;
    int* local_subarray;

    if (argc < 2) {
        printf("Please provide the array size\n");
        return 22; // EINVAL
    }

    n = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);

    if (argc < 3) {
	    printf("Please provide the input type\n");
        MPI_Abort(MPI_COMM_WORLD, 22);
        return 22; // EINVAL
    }

    if (argc < 4) {
        printf("Please provide the sample size\n");
        MPI_Abort(MPI_COMM_WORLD, 22);
        return 22;
    }
    s = atoi(argv[3]);

    // split initial array into subarrays
    CALI_MARK_BEGIN("data_init_runtime");
    local_subarray = (int *) malloc(sizeof(int) * n/p);
    local_size = setup_input(local_subarray, n, argv[2]);
    CALI_MARK_END("data_init_runtime");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large"); // quick sort of all local subarrays is large
    // for each process
    // sort each subarray using quicksort
    quicksort(local_subarray, 0, local_size-1);
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");


    // splitter subarray declaration
    int splitter_subarray[p-1];

    // send process 0 (Master) `s` elements (MPI_Send)
    if (taskid != 0) {
        // if not in process 0 send to process 0
        int sample_send[s];
        for (int i=0; i<s; i++) {
            sample_send[i] = local_subarray[i*local_size/s];
        }

        CALI_MARK_BEGIN("comm");
        CALI_MARK_BEGIN("comm_small"); // broadcasting s elements and receiving p-1 elements

        MPI_Send(&sample_send, s, MPI_INT, 0, TO_MASTER, MPI_COMM_WORLD);

        // receive splitter array from process 0
        MPI_Recv(&splitter_subarray, p-1, MPI_INT, 0, FROM_MASTER, MPI_COMM_WORLD, &status);
        CALI_MARK_END("comm_small");
        CALI_MARK_END("comm");
    }
    else {
        // if in process 0 set up array to hold all samples - size p * s
        int* sample_subarray = new int[p*s];
        // fill in first s spots with sample
        for (int i=0; i<s; i++) {
            sample_subarray[i] = local_subarray[i*local_size/s];
        }
        CALI_MARK_BEGIN("comm");
        CALI_MARK_BEGIN("comm_small"); // receiving s elements from each process
        // process 0 receives `s` elements (MPI_Recv)
        for (int i=1; i<p; i++) {
            MPI_Recv(&sample_subarray[i*s], s, MPI_INT, i, TO_MASTER, MPI_COMM_WORLD, &status);
        }
        CALI_MARK_END("comm_small");
        CALI_MARK_END("comm");

        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_small"); // only sorts sample size : s*p
        // process 0 sorts elements with quicksort
        quicksort(sample_subarray, 0, p*s-1);
        CALI_MARK_END("comp_small");
        CALI_MARK_END("comp");

        // process 0 chooses `p-1` splitters
        // already sorted because from sorted sample array
        for (int i=1; i<p; i++) {
            splitter_subarray[i-1] = sample_subarray[i*s];
        }

        CALI_MARK_BEGIN("comm");
        CALI_MARK_BEGIN("comm_small"); // only sending splitters = p-1
        // process 0 sends splitters to all processes (MPI_Send)
        for (int i=1; i<p; i++) {
            MPI_Send(&splitter_subarray, p-1, MPI_INT, i, FROM_MASTER, MPI_COMM_WORLD);
        }
        CALI_MARK_END("comm_small");
        CALI_MARK_END("comm");
    }

    // wait until every process has received splitters
    MPI_Barrier(MPI_COMM_WORLD);

    // then for each 
    // from each subarray split on splitters into `p` buckets
    int** bucket_arrays = new int*[p];
    int bucket_counts[p]{0};
    for (int i=0; i<p; i++) {
        // fill each array with -1s to start
        bucket_arrays[i] = new int[local_size];
        for (int j=0; j<local_size; j++) {
            bucket_arrays[i][j] = -1;
        }
    }

    for (int i=0; i<local_size; i++) {
        for (int j=0; j<p-1; j++) {
            if (local_subarray[i] < splitter_subarray[j]) {
                bucket_arrays[j][bucket_counts[j]] = local_subarray[i];
                bucket_counts[j] += 1;
                break;
            }
            if (local_subarray[i] >= splitter_subarray[p-2]) {
                bucket_arrays[p-1][bucket_counts[p-1]] = local_subarray[i];
                bucket_counts[p-1] += 1;
                break;
            }
        }
    }

    // send bucket 0 to process 0, bucket 1 to process 1, ... , bucket `p-1` to process `p-1` (MPI_Scatter)
    for (int i=0; i<p; i++) {
        if (taskid != i) {
            // because weird if try to send bucket_arrays[i] through
            int bucket_singular[local_size];
            for (int j=0; j<local_size; j++) {
                bucket_singular[j] = bucket_arrays[i][j];
            }

            CALI_MARK_BEGIN("comm");
            CALI_MARK_BEGIN("comm_large"); // sending all bucket elements to other processes
            MPI_Send(&(bucket_singular), local_size, MPI_INT, i, TO_OTHER + i, MPI_COMM_WORLD);     
            CALI_MARK_END("comm_large");
            CALI_MARK_END("comm");
        }
    }

    // barrier (MPI_Barrier)
    MPI_Barrier(MPI_COMM_WORLD);

    int* received_buckets = new int[n]{-1};
        // fill each array with -1s to start
    for (int i=0; i<n; i++) {
        received_buckets[i] = -1;
    }

    int received_count = 0;

    // processes received buckets (MPI_Allgather)
    for (int i=0; i<p; i++) {
        if (taskid != i) {
            CALI_MARK_BEGIN("comm");
            CALI_MARK_BEGIN("comm_large"); // receiving buckets
            MPI_Recv(&received_buckets[received_count*local_size], local_size, MPI_INT, i, TO_OTHER + taskid, MPI_COMM_WORLD, &status);
            CALI_MARK_END("comm_large");
            CALI_MARK_END("comm");

            received_count += 1;
        }
        else {
            for (int j=0; j<local_size; j++) {
                received_buckets[received_count*local_size + j] = bucket_arrays[i][j];
            }
            received_count += 1;
        }
    }

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large"); 
    // quick sort received buckets
    quicksort(received_buckets, 0, n-1);
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    MPI_Barrier(MPI_COMM_WORLD);

    // send received buckets to process 0 (MPI_Send)
    if (taskid > 0) {
        int received_singular[n];
        for (int i=0; i<n; i++) {
            received_singular[i] = received_buckets[i];
        }
        CALI_MARK_BEGIN("comm");
        CALI_MARK_BEGIN("comm_large"); // sending entire received buckets to process 0
        MPI_Send(&received_singular, n, MPI_INT, 0, TO_MASTER, MPI_COMM_WORLD);   
        CALI_MARK_END("comm_large");
        CALI_MARK_END("comm");
    }

    int rc;

    MPI_Barrier(MPI_COMM_WORLD);

    int final_subarray[local_size];
    // to hold local final subarrays (for check)
    
    // process 0 receives and concatenates buckets (MPI_Gather) 
    if (taskid == 0) {
        // have to be not dynamically allocated
        int received_array[n];
        int final_array[n];

        for (int i=0; i<n; i++) {
            received_array[i] = -1;
            final_array[i] = -1;
        }

        int total_count = 0;

        // for process 0's array
        for (int i=0; i<n; i++) {
            if (received_buckets[i] != -1) {
                final_array[total_count] = received_buckets[i];
                total_count++;
            }
        }

        // for other processes' arrays
        for (int i=1; i<p; i++) {
            CALI_MARK_BEGIN("comm");
            CALI_MARK_BEGIN("comm_large"); // receive all from other processes
            MPI_Recv(&received_array, n, MPI_INT, i, TO_MASTER, MPI_COMM_WORLD, &status);
            CALI_MARK_END("comm_large");
            CALI_MARK_END("comm");

            for (int j=0; j<n; j++) {
                if (received_array[j] != -1) {
                    final_array[total_count] = received_array[j];
                    total_count++;
                }
            }
        }

        // send mini finalized arrays back to other processes
        for (int i=1; i<p; i++) {
            CALI_MARK_BEGIN("comm");
            CALI_MARK_BEGIN("comm_large"); // send all to other processes
            MPI_Send(&final_array[i*local_size], local_size, MPI_INT, i, FROM_MASTER, MPI_COMM_WORLD);   
            CALI_MARK_END("comm_large");
            CALI_MARK_END("comm");
        }
        // for process 0
        for (int i=0; i<local_size; i++) {
            final_subarray[i] = final_array[i];
        }

        // print final output
        // for (int i=0; i<n; i++) {
        //     printf("final[%d] : %d\n", i, final_array[i]);
        // }

        // process 0 returns sorted bucket 0, bucket 1, ..., bucket `p-1`
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (taskid != 0) {
        CALI_MARK_BEGIN("comm");
        CALI_MARK_BEGIN("comm_large"); // size n/p
        MPI_Recv(&final_subarray, local_size, MPI_INT, 0, FROM_MASTER, MPI_COMM_WORLD, &status);
        CALI_MARK_END("comm_large");
        CALI_MARK_END("comm");

    }

    // for (int i=0; i<local_size; i++) {
    //     printf("final[%d] : %d, process : %d\n", i, final_subarray[i], taskid);
    // }

    CALI_MARK_BEGIN("correctness_check");
    rc = verify_sort(final_subarray, local_size, 0);
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
    adiak::value("algorithm", "sample"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi"); // e.g. "mpi"
    adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", n); // The number of elements in input dataset (1000)
    adiak::value("input_type", argv[2]); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", p); // The number of processors (MPI ranks)
    adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", 23); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten")

    // Flush Caliper output before finalizing MPI
    mgr.stop();
    mgr.flush();


    MPI_Finalize();
    return rc;
}