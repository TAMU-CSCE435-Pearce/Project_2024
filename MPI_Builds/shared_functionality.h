/******************************************************************************
* FILE: shared_functionality.h
* DESCRIPTION:
*   Shared Functionality for Group 23
*   This header file should be included by all four algorithm implementations,
*   so that there's only one set of code to generate input and verify output.
* AUTHOR: Jeffrey Mitchell
* LAST REVISED: 10/7/2024
******************************************************************************/

#ifndef SHARED_FUNCTIONALITY_H
#define SHARED_FUNCTIONALITY_H

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mpi.h"

// Populate the specified subarray based on the input type/length and the
// current process's rank.
// Returns the number of populated values (never exceeds n/p + 1).
int setup_input(int *local_subarray, int n, char *input_type) {

    int p, local_rank, size, first_indice;
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &local_rank);

    size = n/p;
    first_indice = local_rank * size;

    // Unexpected scenario: Ensure the count remains consistent when p doesn't
    // evenly divide n:
    if (local_rank >= n % p) {
        first_indice += n % p;
    } else {
        size += 1;
        first_indice += local_rank;
    }

    // Ensure the rand() calls return different values on every run:
    srand(time(NULL) + local_rank*1024);

    // These strings are specified by the adiak::value() choices in the
    // Report.md template, section 3b:
    if (strcmp("Sorted", input_type) == 0) {
        for (int i = 0; i < size; i++)
            local_subarray[i] = first_indice + i;
    } else if (strcmp("ReverseSorted", input_type) == 0) {
        for (int i = 0; i < size; i++)
            local_subarray[i] = n - (first_indice + i);
    } else if (strcmp("Random", input_type) == 0) {
        for (int i = 0; i < size; i++)
            local_subarray[i] = rand();
    } else if (strcmp("1_perc_perturbed", input_type) == 0) {

        // Start with a normal sorted array:
        for (int i = 0; i < size; i++)
            local_subarray[i] = first_indice + i;

        // Pick size/100 indices and give them random values from 0 to n:
        for (int i = 0; i < size/100; i++)
            local_subarray[rand() % size] = rand() % n;

    } else {
        return 0;
    }
    return size;
}

// Return 0 if fully sorted or 1 if there was a problem.
// verbosity=0 means only print the first local-array error.
// verbosity=1 means print every local value until the error.
// verbosity=2 means print EVERY local value and find all errors.
int verify_sort(int *local_subarray, int size, int verbosity) {

    int rc = 0;
    int p, local_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &local_rank);

    if (verbosity)
        printf("Process %d: a[0]=%d\n", local_rank, local_subarray[0]);

    for (int i = 1; i < size; i++) {

        if (verbosity)
            printf("Process %d: a[%d]=%d\n", local_rank, i, local_subarray[i]);

        if (local_subarray[i] < local_subarray[i-1]) {
            printf("Process %d: Local array unsorted at indexes %d/%d."
                   " Values: %d/%d\n",
                   local_rank, i-1, i, local_subarray[i-1], local_subarray[i]);
            rc = 1;

            if (verbosity < 2)
                break;
        }
    }

    // Send top value to next process and bottom value to previous process:
    if (local_rank < p-1) {
        MPI_Send(&local_subarray[size-1], 1, MPI_INT,
                 local_rank+1, 0, MPI_COMM_WORLD);
    }
    if (local_rank > 0) {
        int next_lower;
        MPI_Send(&local_subarray[0], 1, MPI_INT,
                 local_rank-1, 0, MPI_COMM_WORLD);
        MPI_Recv(&next_lower, 1, MPI_INT,
                 local_rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (local_subarray[0] < next_lower) {
            printf("Process %d: First element (%d) is lower than value received"
                   " from the previous process (%d).\n",
                   local_rank, local_subarray[0], next_lower);

            rc = 1;
        }
    }
    if (local_rank < p-1) {
        int next_higher;
        MPI_Recv(&next_higher, 1, MPI_INT,
                 local_rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (local_subarray[size-1] > next_higher) {
            printf("Process %d: Last element (%d) is higher than value received"
                   " from the next process (%d).\n",
                   local_rank, local_subarray[size-1], next_higher);

            rc = 1;
        }
    }

    return rc;
}

#endif
