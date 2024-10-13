#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>
#include <vector>
#include <algorithm>
#include <string>

//Bitonic Sort (sequential on local data)
void bitonicSort(std::vector<int>& localData, int low, int count, bool direction) {
    if (count > 1) {
        int k = count / 2;
        bitonicSort(localData, low, k, true); //Sort in ascending order
        bitonicSort(localData, low + k, k, false); //Sort in descending order
        bitonicMerge(localData, low, count, direction);
    }
}

//Bitonic Merge
void bitonicMerge(std::vector<int>& localData, int low, int count, bool direction) {
    if (count > 1) {
        int k = count / 2;
        for (int i = low; i < low + k; ++i) {
            if ((localData[i] > localData[i + k]) == direction) {
                std::swap(localData[i], localData[i + k]);
            }
        }
        bitonicMerge(localData, low, k, direction);
        bitonicMerge(localData, low + k, k, direction);
    }
}

int main (int argc, char** argv) {
    CALI_CXX_MARK_FUNCTION;

    //Variables for adiak and other
    std::string algorithm = "bitonic";
    std::string programming_model = "mpi";
    const char* data_type = typeid(int).name();
    int size_of_data_type = sizeof(int);
    int input_size = 0;
    std::string input_type = "";
    int num_procs;
    std::string scalability = "";
    int group_number = 14;
    std::string implementation_source = "online";
    int rank;

    //MPI initialization
    MPI_INIT(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

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

    //TODO:Implement caliper regions
    CALI_MARK_BEGIN(data_init_runtime);
    int local_n = input_size / num_procs; //Number of elements per process
    std::vector<int> local_data(local_n);

    //Initialize local data (each process gets a portion of the array)
    srand(rank + 1); //Seed with rank for different random numbers per process
    for (int i = 0; i < local_n; ++i) {
        local_data[i] = rand() % 100;
    }
    CALI_MARK_END(data_init_runtime);

    CALI_MARK_BEGIN(comm);

    CALI_MARK_END(comm);

    CALI_MARK_BEGIN(comp);

    CALI_MARK_END(comp);

    CALI_MARK_BEGIN(correctness_check);

    CALI_MARK_END(correctness_check);

    std::cout << "Initial data for process " << rank << ": ";
    for (int i = 0; i < local_n; ++i) {
        std::cout << local_data[i] << " ";
    }
    std::cout << std::endl;

    //Sort local data using sequential bitonic sort
    bitonicSort(local_data, 0, local_n, true);

    //Perform the parallel part of bitonic sort using MPI
    for (int phase = 1; phase <= size; ++phase) {
        int partner = rank ^ (1 << (phase - 1)); //Find the process to communicate with

        //Exchange data with the partner process
        std::vector<int> recv_data(local_n);
        MPI_Sendrecv(local_data.data(), local_n, MPI_INT, partner, 0,
                     recv_data.data(), local_n, MPI_INT, partner, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //Merge the received data with the local data
        if (rank < partner) {
            //Ascending order
            local_data.insert(local_data.end(), recv_data.begin(), recv_data.end());
            bitonicMerge(local_data, 0, local_data.size(), true);
            local_data.resize(local_n); //Keep only the first half
        } else {
            //Descending order
            local_data.insert(local_data.end(), recv_data.begin(), recv_data.end());
            bitonicMerge(local_data, 0, local_data.size(), false);
            local_data.resize(local_n); //Keep only the second half
        }
    }

    //Gather the sorted subarrays back to the root process
    std::vector<int> sorted_data;
    if (rank == 0) {
        sorted_data.resize(n);
    }
    MPI_Gather(local_data.data(), local_n, MPI_INT, sorted_data.data(), local_n, MPI_INT, 0, MPI_COMM_WORLD);

    //Root process prints the sorted array
    if (rank == 0) {
        std::cout << "Sorted array: ";
        for (int i = 0; i < n; ++i) {
            std::cout << sorted_data[i] << " ";
        }
        std::cout << std::endl;
    }

    //TODO: make sure this works
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
    adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
    adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", group_number); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

    //Flush Caliper output before finalizing MPI
    mgr.stop();
    mgr.flush();
    MPI_Finalize();
}