#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define MASTER 0

// Function to compare and swap elements based on direction (0 = ascending, 1 = descending)
void compare_swap(int &a, int &b, bool dir) {
    if (dir == (a > b)) std::swap(a, b);
}

// Perform bitonic merge
void bitonic_merge(std::vector<int>& arr, int low, int cnt, bool dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++)
            compare_swap(arr[i], arr[i + k], dir);
        bitonic_merge(arr, low, k, dir);
        bitonic_merge(arr, low + k, k, dir);
    }
}

// Recursive bitonic sort function
void bitonic_sort(std::vector<int>& arr, int low, int cnt, bool dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        bitonic_sort(arr, low, k, true);  // Sort in ascending order
        bitonic_sort(arr, low + k, k, false);  // Sort in descending order
        bitonic_merge(arr, low, cnt, dir);  // Merge whole sequence in ascending/descending order
    }
}

// Function to merge data received from partner processes
void process_merge(std::vector<int>& local_arr, std::vector<int>& temp, int rank, int partner, bool direction) {
    std::vector<int> combined(local_arr.size() * 2);
    std::merge(local_arr.begin(), local_arr.end(), temp.begin(), temp.end(), combined.begin());
    local_arr.resize(combined.size());
    std::copy(combined.begin(), combined.end(), local_arr.begin());
}

// Function to check the correctness of the sorted array
bool check_correctness(const std::vector<int>& arr) {
    return std::is_sorted(arr.begin(), arr.end());
}

int main(int argc, char** argv) {
    CALI_CXX_MARK_FUNCTION;

    //Variables for adiak and other
    std::string algorithm = "bitonic";
    std::string programming_model = "mpi";
    const char* data_type = typeid(int).name();
    int size_of_data_type = sizeof(int);
    int input_size = atoi(argv[1]);
    std::string input_type = "random";
    std::string scalability = "weak";
    int group_number = 14;
    std::string implementation_source = "online";

    MPI_Init(&argc, &argv);

    int rank, num_processes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    //Create caliper ConfigManager object
    cali::ConfigManager mgr;
    mgr.start();

    //Caliper region names
    const char* data_init_runtime = "data_init_runtime";
    const char* comm = "comm";
    const char* comm_small = "comm_small";
    const char* comm_large = "comm_large";
    const char* comp = "comp";
    const char* comp_small = "comp_small";
    const char* comp_large = "comp_large";
    const char* correctness_check = correctness_check;

    std::vector<int> arr;
    if (rank == 0) {
        // Track data initialization
        CALI_MARK_BEGIN(data_init_runtime);
        srand(time(nullptr));
        arr.resize(input_size);
        for (int i = 0; i < input_size; i++) {
            arr[i] = rand() % 10000;  // Random values
        }
        CALI_MARK_END(data_init_runtime);
    }

    int local_n = input_size / num_processes;  // Number of elements per process
    std::vector<int> local_arr(local_n);

    // Scatter the array among processes
    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    MPI_Scatter(arr.data(), local_n, MPI_INT, local_arr.data(), local_n, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_large);

    CALI_MARK_BEGIN(comp);

    // Perform local bitonic sort
    CALI_MARK_BEGIN(comp_large);
    bitonic_sort(local_arr, 0, local_n, true);
    CALI_MARK_END(comp_large);

    // Perform bitonic merging across processes
    for (int stage = 2; stage <= num_processes; stage <<= 1) {
        for (int step = stage >> 1; step > 0; step >>= 1) {
            int partner = rank ^ step;
            if (partner < num_processes) {
                std::vector<int> temp(local_n);
                CALI_MARK_BEGIN(comm_small);
                MPI_Sendrecv(local_arr.data(), local_n, MPI_INT, partner, 0,
                             temp.data(), local_n, MPI_INT, partner, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                CALI_MARK_END(comm_small);             

                bool direction = ((rank & stage) == 0);  // Ascending or descending depending on stage
                process_merge(local_arr, temp, rank, partner, direction);
            }
        }
    }

    if (rank == MASTER) {
        arr.resize(local_arr.size());
        std::copy(local_arr.begin(), local_arr.end(), arr.begin());
    }

    CALI_MARK_END(comp);
    CALI_MARK_END(comm);

    if (rank == 0) {
        // Track correctness check
        CALI_MARK_BEGIN(correctness_check);
        bool sorted = check_correctness(arr);
        CALI_MARK_END(correctness_check);

        if (sorted) {
            std::cout << "Array is sorted correctly." << std::endl;
        } else {
            std::cout << "Array is not sorted correctly!" << std::endl;
        }
    }

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", algorithm); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", programming_model); // e.g. "mpi"
    adiak::value("data_type", data_type); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", size_of_data_type); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", input_size); // The number of elements in input dataset (1000)
    adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", num_processes); // The number of processors (MPI ranks)
    adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", group_number); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

    //Flush Caliper output before finalizing MPI
    mgr.stop();
    mgr.flush();
    MPI_Finalize();
    return 0;
}

