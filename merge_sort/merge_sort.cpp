#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <adiak.hpp>
#include <caliper/cali.h>  // For Caliper instrumentation

// Function to merge two sorted arrays
void merge(std::vector<int>& arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    std::vector<int> L(n1);
    std::vector<int> R(n2);

    for (int i = 0; i < n1; ++i)
        L[i] = arr[left + i];
    for (int i = 0; i < n2; ++i)
        R[i] = arr[mid + 1 + i];

    int i = 0, j = 0, k = left;

    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

// Sequential Merge Sort function
void mergeSort(std::vector<int>& arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;

        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);

        merge(arr, left, mid, right);
    }
}

int main(int argc, char* argv[]) {
    
    CALI_CXX_MARK_FUNCTION;
    
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Get current process rank
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // Get number of processes

    // Initialize Adiak for metadata collection
    adiak::init(NULL);
    adiak::value("algorithm", "merge");
    adiak::value("programming_model", "mpi");
    adiak::value("group_num", 18);

    long n = atol(argv[1]);  // Total number of elements


    std::vector<int> data(n);


   //srand(time(NULL));

    std::string input_type = argv[2];
  
        if (input_type[0] == 'r') { // Random
            for (int c = 0; c < n; ++c) {
                data[c] = rand() % n;
            }
        } else if (input_type[0] == 's') { // Sorted
            for (int c = 0; c < n; ++c) {
                data[c] = c;
            }
        } else if (input_type[0] == 'v') { // Reverse sorted
            for (int c = 0; c < n; ++c) {
                data[c] = n - c - 1;
            }
        } else if (input_type[0] == 'p') { // 1% Perturbed
            for (int c = 0; c < n; ++c) {
                data[c] = c;
            }
            // Introduce 1% perturbation
            int num_perturbations = n / 100; // 1% of the array size
            for (int i = 0; i < num_perturbations; ++i) {
                int index = rand() % n;
                data[index] = rand() % n; // Replace with random value
            }
        } else {
            std::cout << "Invalid input type. Use 'sorted', 'random', 'reverse_sorted', or 'perturbed'.\n";
            MPI_Finalize();
            return 1;
        }
    

    // Master process initializes the data
    if (rank == 0) {
        // data.resize(n);
        // Generate random numbers to sort
        for (int i = 0; i < n; ++i) {
            data[i] = rand() % n;
        }
    }

    // Broadcast the size of the array
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Determine the size of each local array
    int local_n = n / size;
    std::vector<int> local_data(local_n);

    // Scatter the data to all processes
    CALI_MARK_BEGIN("comm");
    MPI_Scatter(data.data(), local_n, MPI_INT, local_data.data(), local_n, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm");

    // Each process sorts its local array
    CALI_MARK_BEGIN("comp");
    std::sort(local_data.begin(), local_data.end());
    CALI_MARK_END("comp");

    // Gather the sorted sub-arrays back to the master process
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Gather(local_data.data(), local_n, MPI_INT, data.data(), local_n, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    // Master process merges the sorted sub-arrays
    if (rank == 0) {
        CALI_MARK_BEGIN("comp");        
        CALI_MARK_BEGIN("comp_large");
        for (int i = 1; i < size; ++i) {
            int mid = i * local_n - 1;
            int right = (i + 1) * local_n - 1;
            merge(data, 0, mid, right);
        }
        CALI_MARK_END("comp_large");
        CALI_MARK_END("comp");

        int is_sorted = 1;
        for (int i = 1; i < n - 1; i++) {
            if (data[i] > data[i + 1]) {
                std::cout << data[i] << std::endl;
                is_sorted = 0;
                break;
            }
        }

        if (is_sorted) {
            std::cout << "Array is sorted" << std::endl;
        } else {
            std::cout << "Array is not sorted" << std::endl;
        }

    }

     adiak::value("input_size", n);
    adiak::value("num_procs", size);
    adiak::value("data_type", input_type[0]);
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("scalability", "strong");
    MPI_Finalize();

    return 0;
}
