#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>


void recursive_quicksort(int* sub_array, int start_idx, int end_idx) {
    if (start_idx < end_idx) {
        int pivot_value = sub_array[end_idx];
        int partition_idx = start_idx - 1;
        for (int current_idx = start_idx; current_idx < end_idx; current_idx++) {
            if (sub_array[current_idx] < pivot_value) {
                partition_idx++;
                std::swap(sub_array[partition_idx], sub_array[current_idx]);
            }
        }
        std::swap(sub_array[partition_idx + 1], sub_array[end_idx]);
        int pivot_position = partition_idx + 1;

        recursive_quicksort(sub_array, start_idx, pivot_position - 1);
        recursive_quicksort(sub_array, pivot_position + 1, end_idx);
    }
}


void initialize_data(int* data_array, int num_elements, const std::string& data_type) {
    if (data_type == "random") {
        for (int i = 0; i < num_elements; i++) {
            data_array[i] = rand() % 100000;
        }
    } else if (data_type == "sorted") {
        for (int i = 0; i < num_elements; i++) {
            data_array[i] = i;
        }
    } else if (data_type == "reverse") {
        for (int i = 0; i < num_elements; i++) {
            data_array[i] = num_elements - i;
        }
    } else if (data_type == "perturbed") {
        for (int i = 0; i < num_elements; i++) {
            data_array[i] = i;
        }
        int perturbation_count = num_elements / 100;
        for (int i = 0; i < perturbation_count; i++) {
            int idx1 = rand() % num_elements;
            int idx2 = rand() % num_elements;
            std::swap(data_array[idx1], data_array[idx2]);
        }
    }
}


bool verify_global_sort(int* local_part, int part_size, int process_rank, int total_processes, MPI_Comm mpi_comm) {
    int last_element = local_part[part_size - 1];
    int* gathered_boundaries = nullptr;

    if (process_rank == 0) {
        gathered_boundaries = new int[total_processes];
    }

    MPI_Gather(&last_element, 1, MPI_INT, gathered_boundaries, 1, MPI_INT, 0, mpi_comm);

    bool is_sorted_globally = true;
    if (process_rank == 0) {
        for (int i = 1; i < total_processes; i++) {
            if (gathered_boundaries[i - 1] > gathered_boundaries[i]) {
                is_sorted_globally = false;
                break;
            }
        }
        delete[] gathered_boundaries;
    }

    MPI_Bcast(&is_sorted_globally, 1, MPI_C_BOOL, 0, mpi_comm);

    return is_sorted_globally;
}


int main(int argc, char** argv) {
    CALI_CXX_MARK_FUNCTION;
    cali::ConfigManager cali_manager;
    cali_manager.start();

    MPI_Init(&argc, &argv);
    MPI_Comm mpi_comm = MPI_COMM_WORLD;

    int process_rank, total_processes;
    MPI_Comm_rank(mpi_comm, &process_rank);
    MPI_Comm_size(mpi_comm, &total_processes);

    if (argc != 2) {
        if (process_rank == 0) {
            printf("Usage: %s <input_size_power_of_2> <input_type (random|sorted|reverse|perturbed)>\n", argv[0]);
        }
        MPI_Finalize();
        return 0;
    }

    int exponent_size = atoi(argv[1]);  // Size as power of 2
    std::string data_type = "random";
    int total_elements = pow(2, exponent_size);  // Calculate total number of elements

    int* global_array = nullptr;  // Array on root process
    int* local_partition = new int[total_elements / total_processes];  // Partition of the array per process

    CALI_MARK_BEGIN("data_init_runtime");
    if (process_rank == 0) {
        global_array = new int[total_elements];
        initialize_data(global_array, total_elements, data_type);  // Initialize data on root
    }
    CALI_MARK_END("data_init_runtime");

    MPI_Bcast(&total_elements, 1, MPI_INT, 0, mpi_comm);

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");
    MPI_Scatter(global_array, total_elements / total_processes, MPI_INT, local_partition, total_elements / total_processes, MPI_INT, 0, mpi_comm);
    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");


    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    recursive_quicksort(local_partition, 0, (total_elements / total_processes) - 1);
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    int* merged_array = nullptr;
    if (process_rank == 0) {
        merged_array = new int[total_elements];
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Gather(local_partition, total_elements / total_processes, MPI_INT, merged_array, total_elements / total_processes, MPI_INT, 0, mpi_comm);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    if (process_rank == 0) {
        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_large");
        std::sort(merged_array, merged_array + total_elements);
        CALI_MARK_END("comp_large");
        CALI_MARK_END("comp");
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Scatter(merged_array, total_elements / total_processes, MPI_INT, local_partition, total_elements / total_processes, MPI_INT, 0, mpi_comm);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    if (process_rank == 0) {
        delete[] merged_array;
        delete[] global_array;
    }

    CALI_MARK_BEGIN("correctness_check");
    bool is_sorted = verify_global_sort(local_partition, total_elements / total_processes, process_rank, total_processes, mpi_comm);
    CALI_MARK_END("correctness_check");

    if (process_rank == 0) {
        if (is_sorted) {
            printf("Array is sorted correctly.\n");
        } else {
            printf("Array is NOT sorted correctly.\n");
        }
    }

    delete[] local_partition;

    
    adiak::init(NULL);
    adiak::launchdate();
    adiak::libraries();
    adiak::cmdline();
    adiak::clustername();
    adiak::value("algorithm", "sample sort");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", total_elements);
    adiak::value("input_type", data_type.c_str());
    adiak::value("num_procs", total_processes);
    adiak::value("scalability", "strong");

    cali_manager.stop();
    cali_manager.flush();
    MPI_Finalize();

    return 0;
}
