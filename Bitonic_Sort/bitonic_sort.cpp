#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define MASTER 0

//Function to swap a[i] and a[j] based on if they match the direction
void compAndSwap(int a[], int i, int j, int direction)
{
    if (direction == (a[i] > a[j])) {
        int temp = a[i];
        a[i] = a[j];
        a[j] = temp;
    }
}

//Bitonic merge to be done on every bitonic sequence, merges each sequence into
// one order depending on direction 
void bitonicMerge(int a[], int low, int count, int direction)
{
    if (count > 1)
    {
        int k = count / 2;
        for (int i = low; i < low + k; ++i) {
            compAndSwap(a, i, i + k, direction);
        }
        bitonicMerge(a, low, k, direction);
        bitonicMerge(a, low + k, k, direction);
    }
}

//Changes boolean direction of bitonic sequence
bool changeDirection(bool curr) {
    if (curr) {
        return false;
    }
    return true;
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
    const char* correctness_check = "correctness_check";

    int localArraySize = input_size / num_processes;
    int localArray[localArraySize]; //Local array for each process seeding

    // Track data initialization
    CALI_MARK_BEGIN(data_init_runtime);
    for (int i = 0; i < localArraySize; ++i) {
        localArray[i] = rand() % 10000;  // Random values
    }
    CALI_MARK_END(data_init_runtime);

    std::cout << "Array for process " << rank << ": ";
    for (int i = 0; i < localArraySize; ++i) {
        std::cout << localArray[i] << " ";
    }
    std::cout << std::endl << std::endl;

    //Creating the Bitonic Sequence most cases should work on powers of 2,
    // there is a special case where each process only has one element
    int stage = 0; //To determine the partner processes
    bool direction = true; //True is increasing, false is decreasing
    for (int i = 0; i < std::log2(num_processes); i++) {
        for (int j = 0; j < std::log2(localArraySize); j++) {
            bitonicMerge(localArray, 0, localArraySize, direction);
            direction = changeDirection(direction);
        }

        //Special case where each process has one element
        if (num_processes == input_size) {

        }
    }

    std::cout << "Array bitonic sequence for process " << rank << ": ";
    for (int i = 0; i < localArraySize; ++i) {
        std::cout << localArray[i] << " ";
    }
    std::cout << std::endl << std::endl;

    // // Perform local bitonic sort
    // CALI_MARK_BEGIN(comp_large);
    // bitonic_sort(local_arr, 0, local_n, true);
    // CALI_MARK_END(comp_large);

    // // Perform bitonic merging across processes
    // for (int stage = 2; stage <= num_processes; stage <<= 1) {
    //     for (int step = stage >> 1; step > 0; step >>= 1) {
    //         int partner = rank ^ step;
    //         if (partner < num_processes) {
    //             std::vector<int> temp(local_n);
    //             CALI_MARK_BEGIN(comm_small);
    //             MPI_Sendrecv(local_arr.data(), local_n, MPI_INT, partner, 0,
    //                          temp.data(), local_n, MPI_INT, partner, 0,
    //                          MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //             CALI_MARK_END(comm_small);             

    //             bool direction = ((rank & stage) == 0);  // Ascending or descending depending on stage
    //             process_merge(local_arr, temp, rank, partner, direction);
    //         }
    //     }
    // }

    // if (rank == MASTER) {
    //     arr.resize(local_arr.size());
    //     std::copy(local_arr.begin(), local_arr.end(), arr.begin());
    // }

    // CALI_MARK_END(comp);
    // CALI_MARK_END(comm);

    // if (rank == 0) {
    //     // Track correctness check
    //     CALI_MARK_BEGIN(correctness_check);
    //     bool sorted = check_correctness(arr);
    //     CALI_MARK_END(correctness_check);

    //     if (sorted) {
    //         std::cout << "Array is sorted correctly." << std::endl;
    //     } else {
    //         std::cout << "Array is not sorted correctly!" << std::endl;
    //     }
    // }

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