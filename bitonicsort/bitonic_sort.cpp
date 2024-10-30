// Code with Caliper Time Implementation 
// *********************************************************************************
// *********************************************************************************
// *********************************************************************************
// *********************************************************************************
// *********************************************************************************

#include <iostream>
#include <vector>
#include <algorithm>
#include <stdlib.h> // For rand() and srand()
#include <ctime>   // For time()
#include "mpi.h"
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>
#include <cmath>

// Create input for bitonic sort
void generateInput(std::vector<int>& arr, int size, const std::string& input_type) {
    if (input_type == "sorted") {
        for (int i = 0; i < size; i++) {
            arr[i] = i;
        }
    } else if (input_type == "reverse_sorted") {
        for (int i = 0; i < size; i++) {
            arr[i] = size - i;
        }
    } else if (input_type == "random") {
        for (int i = 0; i < size; i++) {
            arr[i] = rand() % 100000; // Random numbers between 0 and 100000
        }
    } else if (input_type == "perturbed") {
        for (int i = 0; i < size; i++) {
            arr[i] = i;
        }
        int swaps = size * 0.01; // 1% perturbed
        for (int i = 0; i < swaps; i++) {
            int idx1 = rand() % size;
            int idx2 = rand() % size;
            std::swap(arr[idx1], arr[idx2]);
        }
    }
}

void bitonicMerge(std::vector<int>& arr, int low, int count, bool dir) {
    if (count > 1) {
        int k = count / 2;
        for (int i = low; i < low + k; i++) {
            if (dir == (arr[i] > arr[i + k])) {
                std::swap(arr[i], arr[i + k]);
            }
        }
        bitonicMerge(arr, low, k, dir);
        bitonicMerge(arr, low + k, k, dir);
    }

}

void bitonicSort(std::vector<int>& arr, int low, int count, bool dir) {
    if (count > 1) {
        int k = count / 2;
        bitonicSort(arr, low, k, true);  // Sort in ascending order
        bitonicSort(arr, low + k, k, false); // Sort in descending order

        
        bitonicMerge(arr, low, count, dir); // Merge the result
        
    }

}

void parallelBitonicSort(std::vector<int>& arr, int size, int rank, int numProcs) {
    // Calculate local array size
    int localSize = size / numProcs;
    std::vector<int> localArr(localSize);

    // Start communication timing for scatter
    CALI_MARK_BEGIN("comm_scatter");
    double comm_start = MPI_Wtime();

    // Scatter the array to all processes
    MPI_Scatter(arr.data(), localSize, MPI_INT, localArr.data(), localSize, MPI_INT, 0, MPI_COMM_WORLD);

    // End communication timing for scatter
    double comm_end = MPI_Wtime();
    CALI_MARK_END("comm_scatter");
    double comm_time = comm_end - comm_start;

    // Sort the local array
    bitonicSort(localArr, 0, localSize, true);

    // Start communication timing for gather
    CALI_MARK_BEGIN("comm_gather");
    comm_start = MPI_Wtime();

    // Gather sorted local arrays back to the root process
    MPI_Gather(localArr.data(), localSize, MPI_INT, arr.data(), localSize, MPI_INT, 0, MPI_COMM_WORLD);

    comm_end = MPI_Wtime();
    CALI_MARK_END("comm_gather");
    comm_time += (comm_end - comm_start);

    // You can also reduce the communication times if needed
    double total_comm_time;
    MPI_Reduce(&comm_time, &total_comm_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "Total communication time: " << total_comm_time << " seconds" << std::endl;
    }

    // Merge the sorted arrays in the root process
    if (rank == 0) {
        CALI_MARK_BEGIN("comp");
        bitonicSort(arr, 0, size, true); // Sort the gathered array
        CALI_MARK_END("comp");

    }
}

// Function to check if the array is sorted
bool isSorted(const std::vector<int>& arr) {
    for (size_t i = 1; i < arr.size(); ++i) {
        if (arr[i] < arr[i - 1]) {
            return false; // Found an element that is out of order
        }
    }
    return true; // Array is sorted
}

int main(int argc, char** argv) {
    // Create caliper ConfigManager object
    cali::ConfigManager mgr;
    mgr.start();

    CALI_MARK_BEGIN("main");
    MPI_Init(&argc, &argv);

    int rank, numProcs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    if (argc < 4) {
        if (rank == 0) {
            std::cerr << "Usage: " << argv[0] << " <size exponent> <number of processes> <input_type>" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    std::string input_type = argv[3];
    int exponent = std::atoi(argv[1]);  // Get the exponent (size)
    int size = std::pow(2, exponent);   // Calculate size as 2^exponent
    int requestedProcs = std::atoi(argv[2]);

    if (numProcs != requestedProcs) {
        if (rank == 0) {
            std::cerr << "Error: The number of processes specified (" << requestedProcs 
                      << ") does not match the number of processes started (" << numProcs << ")." << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    std::vector<int> arr;
    if (rank == 0) {
        srand(static_cast<unsigned int>(time(nullptr)));
        arr.resize(size);
        generateInput(arr, size, input_type);

        adiak::init(NULL);
        adiak::user();
        adiak::launchdate();
        adiak::libraries();
        adiak::cmdline();
        adiak::clustername();
        adiak::value("algorithm", "bitonic");
        adiak::value("programming_model", "mpi");
        adiak::value("data_type", "int");
        adiak::value("size_of_data_type", sizeof(int));
        adiak::value("input_size", size);
        adiak::value("input_type", input_type);
        adiak::value("num_procs", numProcs);
    }

    double start_time = MPI_Wtime();
    parallelBitonicSort(arr, size, rank, numProcs);
    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;

    double min_time, max_time, avg_time, total_time;
    MPI_Reduce(&local_time, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_time, &total_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        avg_time = total_time / numProcs;
        std::cout << "Min time: " << min_time << ", Max time: " << max_time 
                  << ", Avg time: " << avg_time << ", Total time: " << total_time << std::endl;
    }

    if (rank == 0) {
        // std::cout << "Sorted array: ";
        // for (int i : arr) std::cout << i << " ";
        // std::cout << std::endl;

        CALI_MARK_BEGIN("correctness_check");
        if (isSorted(arr)) {
            std::cout << "Array is sorted correctly." << std::endl;
        } else {
            std::cout << "Array is NOT sorted correctly!" << std::endl;
        }
        CALI_MARK_END("correctness_check");
    }

    CALI_MARK_END("main");
    // Flush Caliper output before finalizing MPI
    mgr.stop();
    mgr.flush();
    MPI_Finalize();
    return 0;
}

