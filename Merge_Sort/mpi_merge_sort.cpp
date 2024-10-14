#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// Function prototypes
void merge(int *arr, int left, int mid, int right);
void mergeSort(int *arr, int left, int right);

int main(int argc, char** argv) {
    int world_rank, world_size;
    int *arr = NULL; // Original array (only in root)
    int *subArr;     // Each process's sub-array
    int size;        // Size of each sub-array

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Root process initializes the array
    int n = 20; // Size of the entire array (modify as needed)
    if (world_rank == 0) {
        arr = malloc(n * sizeof(int));
        printf("Unsorted array: ");
        for (int i = 0; i < n; i++) {
            arr[i] = rand() % n;
            printf("%d ", arr[i]);
        }
        printf("\n\n");
    }

    // Calculate size of each sub-array and allocate memory
    size = n / world_size;
    subArr = malloc(size * sizeof(int));

    // Scatter the array among all processes
    MPI_Scatter(arr, size, MPI_INT, subArr, size, MPI_INT, 0, MPI_COMM_WORLD);

    // Perform merge sort on each process's sub-array
    mergeSort(subArr, 0, size - 1);

    // Gather sorted sub-arrays at the root process
    if (world_rank == 0) {
        int *sorted_arr = malloc(n * sizeof(int));
        MPI_Gather(subArr, size, MPI_INT, sorted_arr, size, MPI_INT, 0, MPI_COMM_WORLD);

        // Final merge of all sorted sub-arrays in root
        int *temp = malloc(n * sizeof(int));
        mergeSort(sorted_arr, 0, n - 1);

        // Display the sorted array
        printf("Sorted array: ");
        for (int i = 0; i < n; i++) {
            printf("%d ", sorted_arr[i]);
        }
        printf("\n\n");

        // Free root process memory
        free(sorted_arr);
        free(temp);
    } else {
        // Non-root processes participate in gather only
        MPI_Gather(subArr, size, MPI_INT, NULL, size, MPI_INT, 0, MPI_COMM_WORLD);
    }

    // Free allocated memory and finalize MPI
    free(subArr);
    if (world_rank == 0) {
        free(arr);
    }
    MPI_Finalize();

    return 0;
}

// Merge two halves of the array
void merge(int *arr, int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    int *L = malloc(n1 * sizeof(int));
    int *R = malloc(n2 * sizeof(int));

    for (i = 0; i < n1; i++) {
        L[i] = arr[left + i];
    }
    for (j = 0; j < n2; j++) {
        R[j] = arr[mid + 1 + j];
    }

    i = 0; j = 0; k = left;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }

    while (i < n1) {
        arr[k++] = L[i++];
    }
    while (j < n2) {
        arr[k++] = R[j++];
    }

    free(L);
    free(R);
}

// Recursive merge sort function
void mergeSort(int *arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}