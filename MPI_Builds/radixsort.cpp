#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string>
#include <vector>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define BASE 10

int find_max(int* arr, int size) {
    int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

void counting_sort(int* arr, int size, int exp) {
    int output[size]; 
    int count[BASE] = {0};

    // Count occurrences of digits based on current exponent
    for (int i = 0; i < size; i++) {
        count[(arr[i] / exp) % BASE]++;
    }

    // Transform count array to represent positions in the output array
    for (int i = 1; i < BASE; i++) {
        count[i] += count[i - 1];
    }

    // Build the output array
    for (int i = size - 1; i >= 0; i--) {
        output[count[(arr[i] / exp) % BASE] - 1] = arr[i];
        count[(arr[i] / exp) % BASE]--;
    }

    // Copy the output array back to arr[]
    for (int i = 0; i < size; i++) {
        arr[i] = output[i];
    }
}

void generate_data(int* data, int N, const std::string& input_type) {
    if (input_type == "random") {
        for (int i = 0; i < N; i++) {
            data[i] = rand() % 100000;
        }
    } else if (input_type == "sorted") {
        for (int i = 0; i < N; i++) {
            data[i] = i;
        }
    } else if (input_type == "reverse") {
        for (int i = 0; i < N; i++) {
            data[i] = N - i;
        }
    } else if (input_type == "perturbed") {
        for (int i = 0; i < N; i++) {
            data[i] = i;
        }
        int perturb_count = N / 100;
        for (int i = 0; i < perturb_count; i++) {
            int idx1 = rand() % N;
            int idx2 = rand() % N;
            std::swap(data[idx1], data[idx2]);
        }
    }
}

bool check_sorted_correctly(int* local_data, int local_size, int rank, int size, MPI_Comm comm) {
    int local_last = local_data[local_size - 1];
    int* boundary_values = nullptr;

    if (rank == 0) {
        boundary_values = new int[size];
    }

    MPI_Gather(&local_last, 1, MPI_INT, boundary_values, 1, MPI_INT, 0, comm);

    bool global_sorted = true;
    if (rank == 0) {
        for (int i = 1; i < size; i++) {
            if (boundary_values[i - 1] > boundary_values[i]) {
                global_sorted = false;
                break;
            }
        }
        delete[] boundary_values;
    }

    MPI_Bcast(&global_sorted, 1, MPI_C_BOOL, 0, comm);

    return global_sorted;
}

int main(int argc, char** argv) {
    CALI_CXX_MARK_FUNCTION;
    cali::ConfigManager mgr;
    mgr.start();

    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;

    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if (argc != 2) {
        if (rank == 0) {
            printf("Usage: %s <input_size_power_of_2> <input_type (random|sorted|reverse|perturbed)>\n", argv[0]);
        }
        MPI_Finalize();
        return 0;
    }

    int N_power = atoi(argv[1]);      // Input size as power of 2 (e.g., 16 for 2^16)
    std::string input_type = "random";
    int N = pow(2, N_power);          // Calculate 2^N

    int fullArray[N];  
    int subArray[N / size];  

    CALI_MARK_BEGIN("data_init_runtime");
    if (rank == 0) {
        generate_data(fullArray, N, input_type); 
    }
    CALI_MARK_END("data_init_runtime");

    MPI_Bcast(&N, 1, MPI_INT, 0, comm);

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");
    MPI_Scatter(fullArray, N / size, MPI_INT, subArray, N / size, MPI_INT, 0, comm);
    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");
    int local_max = find_max(subArray, N / size);
    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    CALI_MARK_BEGIN("comm");
    int global_max;
    MPI_Allreduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, comm);
    CALI_MARK_END("comm");

    for (int exp = 1; global_max / exp > 0; exp *= BASE) {
        CALI_CXX_MARK_LOOP_BEGIN(sort_loop, "Sorting loop");

        CALI_MARK_BEGIN("comp");
        counting_sort(subArray, N / size, exp);
        CALI_MARK_END("comp");

        CALI_MARK_BEGIN("comm");
        CALI_MARK_BEGIN("comm_large");

        // Redistribute data using bucketing based on current digit
        std::vector<int> buckets[size];
        for (int i = 0; i < N / size; i++) {
            int bucket_idx = (subArray[i] / exp) % size;
            buckets[bucket_idx].push_back(subArray[i]);
        }

        // Prepare for MPI_Alltoallv
        int send_counts[size];
        int recv_counts[size];
        int send_displs[size];
        int recv_displs[size];

        // Calculate send_counts
        for (int i = 0; i < size; i++) {
            send_counts[i] = buckets[i].size();
        }

        MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, comm);

        // Calculate displacements for MPI_Alltoallv
        send_displs[0] = 0;
        recv_displs[0] = 0;
        for (int i = 1; i < size; i++) {
            send_displs[i] = send_displs[i - 1] + send_counts[i - 1];
            recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
        }

        // Flatten the buckets into send buffer
        std::vector<int> send_buffer;
        for (int i = 0; i < size; i++) {
            send_buffer.insert(send_buffer.end(), buckets[i].begin(), buckets[i].end());
        }

        std::vector<int> recv_buffer(recv_displs[size - 1] + recv_counts[size - 1]);
        MPI_Alltoallv(send_buffer.data(), send_counts, send_displs, MPI_INT,
                      recv_buffer.data(), recv_counts, recv_displs, MPI_INT, comm);

        // Replace the subArray with the received data
        std::copy(recv_buffer.begin(), recv_buffer.end(), subArray);

        CALI_MARK_END("comm_large");
        CALI_MARK_END("comm");

        CALI_CXX_MARK_LOOP_END(sort_loop);
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Gather(subArray, N / size, MPI_INT, (rank == 0) ? fullArray : NULL, N / size, MPI_INT, 0, comm);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    if (rank == 0) {
        printf("Sorting completed.\n");
    }

    CALI_MARK_BEGIN("correctness_check");
    bool sorted_correctly = check_sorted_correctly(subArray, N / size, rank, size, comm);
    CALI_MARK_END("correctness_check");

    if (rank == 0) {
        if (sorted_correctly) {
            printf("Array is sorted correctly.\n");
        } else {
            printf("Array is NOT sorted correctly.\n");
        }
    }

    adiak::init(NULL);
    adiak::launchdate();
    adiak::libraries();
    adiak::cmdline();
    adiak::clustername();
    adiak::value("algorithm", "radix");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", N);
    adiak::value("input_type", input_type.c_str());
    adiak::value("num_procs", size);
    adiak::value("scalability", "strong");
    adiak::value("group_num", "5");
    adiak::value("implementation_source", "online and ai");

    mgr.stop();
    mgr.flush();
    MPI_Finalize();

    return 0;
}
