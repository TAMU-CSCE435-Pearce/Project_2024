/******************************************************************************
* FILE: mergesort.cpp
* DESCRIPTION:  
*   MPI Distributed Merge Sort
*   All numtasks processes generate their own sections of the overall array,
*   sort their local subarrays, and then sort-merge their subarrays across
*   process boundaries while keeping the array decentralized.
* AUTHOR: Jeffrey Mitchell
* LAST REVISED: 10/7/2024
******************************************************************************/

#include <stdio.h>

#include "mpi.h"

#include "shared_functionality.h"

int *local_subarray;
int local_rank;
int local_size;

// Sort a fully-local array with standard merge sort:
void local_merge_sort(int *array, int size) {
    if (size < 2)
        return;

    local_merge_sort(array, size/2);
    local_merge_sort(&array[size/2], size - size/2);

    int *tmp_array_i = (int *) malloc(sizeof(int) * size/2);
    int *tmp_array_j = (int *) malloc(sizeof(int) * (size - size/2));
    for (int i = 0; i < size/2; i++) {
        tmp_array_i[i] = array[i];
        tmp_array_j[i] = array[size/2 + i];
    }
    if (size % 2 == 1)
        tmp_array_j[size/2] = array[size-1];

    int i = 0;
    int j = 0;
    while (i + j < size) {
        if (i == size/2) {
            array[i+j] = tmp_array_j[j];
            j++;
        } else if (j == size - size/2) {
            array[i+j] = tmp_array_i[i];
            i++;
        } else if (tmp_array_i[i] <= tmp_array_j[j]) {
            array[i+j] = tmp_array_i[i];
            i++;
        } else {
            array[i+j] = tmp_array_j[j];
            j++;
        }
    }

    free(tmp_array_i);
    free(tmp_array_j);
}

// Take only the bottom half of the provided values and merge them:
void merge_bottom_half(int *array_i, int *array_j, int *array_out, int size) {
    int i = 0;
    int j = 0;
    while (i + j < size) {
        if (array_i[i] <= array_j[j]) {
            array_out[i+j] = array_i[i];
            i++;
        } else {
            array_out[i+j] = array_j[j];
            j++;
        }
    }
}

// Take only the top half of the provided values and merge them:
void merge_top_half(int *array_i, int *array_j, int *array_out, int size) {
    int i = size-1;
    int j = size-1;
    int out_place = size-1;
    while (out_place >= 0) {
        if (array_i[i] <= array_j[j]) {
            array_out[out_place] = array_j[j];
            j--;
        } else {
            array_out[out_place] = array_i[i];
            i--;
        }
        out_place--;
    }
}

// Merge with one specific neighbor. Relative ranks determine which one gets the
// higher or lower array elements.
void merge_2_way(int neighbor_id) {

    // Via local experimentation, it appears that MPI has a < 1024-int limit on
    // how much data can fully-asynchronously transfer.

    static int *remote_subarray = (int *) malloc(sizeof(int) * local_size);

    if (local_size % 512) {
        MPI_Send(local_subarray, local_size % 512, MPI_INT,
                 neighbor_id, 0, MPI_COMM_WORLD);
        MPI_Recv(remote_subarray, local_size % 512, MPI_INT,
                 neighbor_id, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    for (int amount = local_size % 512; amount < local_size; amount += 512) {
        MPI_Send(local_subarray + amount, 512, MPI_INT,
                 neighbor_id, 0, MPI_COMM_WORLD);
        MPI_Recv(remote_subarray + amount, 512, MPI_INT,
                 neighbor_id, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }


    static int *new_subarray = (int *) malloc(sizeof(int) * local_size);
    if (neighbor_id > local_rank)
        // Neighbor has a higher rank, meaning this process will get the lower
        // values.
        merge_bottom_half(local_subarray, remote_subarray, new_subarray,
                          local_size);
    else
        // Neighbor has a lower rank, meaning this process will get the higher
        // values.
        merge_top_half(local_subarray, remote_subarray, new_subarray,
                       local_size);

    // Rather than repeatedly free/reallocate blocks of the same size, use a
    // static one and just swap back and forth between the two:
    //free(local_subarray);
    int *tmp = local_subarray;
    local_subarray = new_subarray;
    new_subarray = tmp;
}

// Use a combination of 2-way merges to combine two sorted chunk_size/2 sized
// chunks into one sorted chunk_size sized chunk. Each offset=x iteration
// isolates another process at the top and bottom of the chunk as definitely
// being sorted within the new chunk. At the last iteration, only the middle two
// processes merge with each other, because they're the only two not confirmed
// to be sorted yet.
void merge_n_way(int chunk_size) {
    int rank_within_chunk = local_rank % chunk_size;

    int offset = chunk_size / 2;
    int min_offset = chunk_size/2 - rank_within_chunk;
    if (rank_within_chunk >= chunk_size / 2)
        min_offset = -min_offset + 1;

    while (offset >= min_offset) {

        // Choose offset direction based on whether this process is in the top
        // or bottom half of the chunk:
        if (rank_within_chunk < chunk_size/2)
            merge_2_way(local_rank + offset);
        else
            merge_2_way(local_rank - offset);
        offset--;
    }
}

int main(int argc, char *argv[]) {

    int rc = 0;
    int p;
    int n;
    if (argc < 2) {
        printf("Please provide the array size\n");
        return 22; // EINVAL
    }
    n = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &local_rank);

    if (argc < 3) {
	printf("Please provide the input type\n");
        MPI_Abort(MPI_COMM_WORLD, 22);
        return 22; // EINVAL
    }
    local_subarray = (int *) malloc(sizeof(int) * n/p);
    local_size = setup_input(local_subarray, n, argv[2]);


    local_merge_sort(local_subarray, local_size);
    printf("Process %d: Local sort complete.\n", local_rank);

    int k = 1;
    while (k < p) {
        k *= 2;
        merge_n_way(k);
    }


    if (verify_sort(local_subarray, local_size, 0)) {
        printf("Process %d: Sort check failed.\n", local_rank);
        rc = 1;
    } else {
        printf("Process %d: Sort check succeeded.\n", local_rank);
    }

    MPI_Finalize();
    return rc;
}
