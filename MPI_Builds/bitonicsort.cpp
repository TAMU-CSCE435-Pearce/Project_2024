/******************************************************************************
* FILE: bitonicsort.cpp
* DESCRIPTION:
*   MPI Distributed Bitonic Sort
*   All processes generate their sections of the array, order it
*   into a bitonic sequence recursively, and communicate to order
*   on the whole array scale
* SOURCES:
*   Adapted
*   https://www.geeksforgeeks.org/bitonic-sort/
*   https://en.wikipedia.org/wiki/Bitonic_sorter
* AUTHOR: Brandon Cisneros
* LAST REVISED: 10/14/2024
******************************************************************************/

#include <stdio.h>

#include "mpi.h"
// #include "caliper/cali.h"
// #include "adiak.hpp"

#include "shared_functionality.h"

void compare_and_swap(int *array, int i, int j, int order) {
    if ((order == 1 && array[i] > array[j]) || (order == 2 && array[i] < array[j])) {
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

void local_bitonic_merge(int* array, int size, int sort) {
        if (size > 1) {
                for (int i = 0; i < size/2; i ++) {
                    compare_and_swap(array, i, i+size/2, sort);
                }
                local_bitonic_merge(array, size/2, sort);
                local_bitonic_merge(array + size/2, size-size/2, sort);
        }
}

// Makes local subarray a bitonic series
void make_local_bitonic(int *array, int size, int sort) {
    if (size > 1) {
        make_local_bitonic(array, size/2, 1);
        make_local_bitonic(array + size/2, size - size/2, 2);
        if (sort != 0) {
                local_bitonic_merge(array, size, sort);
        }
    }
}

void bitonic_recurse_fix(int* local_subarray, int num_procs, int local_rank, int local_size, int* recv_buffer, int level) {
    for (int new_level = level; new_level >= 2; new_level /=2) {
        int partner;
        // CHECK
        if ((local_rank % new_level) >= (new_level/2)) {
            partner = local_rank - new_level/2;
            // printf("FIX: I'm %d with %d getting descending at level %d at depth %d with caller %d\n", local_rank, partner, level, depth, caller);
            make_local_bitonic(local_subarray, local_size, 2);
        }
        else {
            partner = local_rank + new_level/2;
            // printf("FIX: I'm %d with %d getting ascending at level %d at depth %d with caller %d\n", local_rank, partner, level, depth, caller);
            make_local_bitonic(local_subarray, local_size, 1);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Sendrecv(local_subarray, local_size, MPI_INT, partner, 0,
                    recv_buffer, local_size, MPI_INT, partner, 0, 
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        bool minSide;
        minSide = (local_rank < partner);

        if (minSide) {
            for (int i = 0; i < local_size; i++) {
                if (local_subarray[i] > recv_buffer[i]) {
                    local_subarray[i] = recv_buffer[i];
                }
            }
        }
        else {
            for (int i = 0; i < local_size; i++) {
                if (local_subarray[i] < recv_buffer[i]) {
                    local_subarray[i] = recv_buffer[i];
                }
            }
        }
    }
}

void bitonic_recurse(int* local_subarray, int num_procs, int local_rank, int local_size, int* recv_buffer, int level, int depth, int caller) {
    int new_level = level;
    while (new_level != 2) {
        new_level /= 2;
        bitonic_recurse(local_subarray, num_procs, local_rank, local_size, recv_buffer, new_level, depth+1, level);
    }

    int partner;
    if ((local_rank % level) >= (level/2)) {
        partner = local_rank - level/2;
        // printf("I'm %d with %d getting descending at level %d at depth %d with caller %d\n", local_rank, partner, level, depth, caller);
        make_local_bitonic(local_subarray, local_size, 2);
    }
    else {
        partner = local_rank + level/2;
        // printf("I'm %d with %d getting ascending at level %d at depth %d with caller %d\n", local_rank, partner, level, depth, caller);
        make_local_bitonic(local_subarray, local_size, 1);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Sendrecv(local_subarray, local_size, MPI_INT, partner, 0,
                recv_buffer, local_size, MPI_INT, partner, 0, 
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // NEED ALTERNATING MIN
    bool minSide;
    minSide = (local_rank < partner);

    // TODO: ADIAK, CALLIPER, SCRIPT, RUN

    // Flips based on caller level splits
    if (caller != level) {
        if ((local_rank % caller) >= (caller/2)) {
            minSide = !minSide;
        }
    }
    // if (level == 2 && depth == 1 && caller == 4) {
    //     if ((local_rank == 2) || (local_rank == 3))
    //         minSide = !minSide;
    // }
    // if (level == 2 && depth == 2 && caller == 4) {
    //     if ((local_rank == 2) || (local_rank == 3) || (local_rank == 6) || (local_rank == 7))
    //         minSide = !minSide;
    // }
    // if (level == 4 && depth == 1 && caller == 8) {
    //     if ((local_rank == 4) || (local_rank == 5) || (local_rank == 6) || (local_rank == 7))
    //         minSide = !minSide;
    // }
    // if (level == 2 && depth == 1 && caller == 8) {
    //     if ((local_rank == 4) || (local_rank == 5) || (local_rank == 6) || (local_rank == 7))
    //         minSide = !minSide;
    // }
    // If called directly, no change in minside
    // If first level down from caller, right half of caller size splits flipped

    if (minSide) {
        for (int i = 0; i < local_size; i++) {
            if (local_subarray[i] > recv_buffer[i]) {
                local_subarray[i] = recv_buffer[i];
            }
        }
    }
    else {
        for (int i = 0; i < local_size; i++) {
            if (local_subarray[i] < recv_buffer[i]) {
                local_subarray[i] = recv_buffer[i];
            }
        }
    }

    if ((level == num_procs) && (num_procs != 2))
        bitonic_recurse_fix(local_subarray, num_procs, local_rank, local_size, recv_buffer, num_procs/2);

}

void full_bitonic_merge(int* local_subarray, int num_procs, int local_rank, int local_size) {
    int partner;
    int *recv_buffer;

    // Allocate buffer for receiving data
    recv_buffer = (int *) malloc(sizeof(int) * local_size);

    bitonic_recurse(local_subarray, num_procs, local_rank, local_size, recv_buffer, num_procs, 0, num_procs);

    make_local_bitonic(local_subarray, local_size, 1);
    free(recv_buffer);

}

int main(int argc, char* argv[]) {
    // CALI_CXX_MARK_FUNCTION;

    int rc = 0;
    int num_procs;
    int input_size;
    int *local_subarray;
    int local_rank;
    int local_size;

    if (argc < 2) {
        printf("Please provide the array size\n");
        return 22; // EINVAL
    }
    input_size = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &local_rank);

    if (argc < 3) {
	printf("Please provide the input type\n");
        MPI_Abort(MPI_COMM_WORLD, 22);
        return 22; // EINVAL
    }

    // CALI_MARK_BEGIN("data_init_runtime");
    local_subarray = (int *) malloc(sizeof(int) * input_size/num_procs);
    local_size = setup_input(local_subarray, input_size, argv[2]);
    // CALI_MARK_END("data_init_runtime");

    // CALI_MARK_BEGIN("comp");
    // CALI_MARK_BEGIN("comp_large");
    if (num_procs == 1) {
        make_local_bitonic(local_subarray, local_size, 1);
    }
    else {
        make_local_bitonic(local_subarray, local_size, 0);
    }
    // CALI_MARK_END("comp_large");
    // CALI_MARK_END("comp");
    // printf("Process %d: Local sort complete.\n", local_rank);

    // CALI_MARK_BEGIN("comm");
    // CALI_MARK_BEGIN("comm_large");
    if (num_procs != 1)
        full_bitonic_merge(local_subarray, num_procs, local_rank, local_size);
    // CALI_MARK_END("comm_large");
    // CALI_MARK_END("comm");


    // CALI_MARK_BEGIN("correctness_check");
    rc = verify_sort(local_subarray, local_size, 0);
    // CALI_MARK_END("correctness_check");
    if (rc) {
        printf("Process %d: Sort check failed.\n", local_rank);
    } else {
        printf("Process %d: Sort check succeeded.\n", local_rank);
    }


    // adiak::init(NULL);
    // adiak::launchdate();    // launch date of the job
    // adiak::libraries();     // Libraries used
    // adiak::cmdline();       // Command line used to launch the job
    // adiak::clustername();   // Name of the cluster
    // adiak::value("algorithm", "bitonic"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    // adiak::value("programming_model", "mpi"); // e.g. "mpi"
    // adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
    // adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    // adiak::value("input_size", input_size); // The number of elements in input dataset (1000)
    // adiak::value("input_type", argv[2]); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    // adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
    // adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
    // adiak::value("group_num", 23); // The number of your group (integer, e.g., 1, 10)
    // adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

    MPI_Finalize();
    return rc;
}