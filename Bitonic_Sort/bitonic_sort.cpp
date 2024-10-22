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

void bitonicSort(int a[],int low, int cnt, int dir)
{
    if (cnt>1)
    {
        int k = cnt/2;
 
        // sort in ascending order since dir here is 1
        bitonicSort(a, low, k, 1);
 
        // sort in descending order since dir here is 0
        bitonicSort(a, low+k, k, 0);
 
        // Will merge whole sequence in ascending order
        // since dir=1.
        bitonicMerge(a,low, cnt, dir);
    }
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

    // std::cout << "Array for process " << rank << ": ";
    // for (int i = 0; i < localArraySize; ++i) {
    //     std::cout << localArray[i] << " ";
    // }
    // std::cout << std::endl << std::endl;

    int direction[2] = {1, 0}; //True is increasing, false is decreasing

    //Initial sorting of sequence on each processor to get a bitonic sequence across all processors
    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_large);
    bitonicSort(localArray, 0, localArraySize, direction[0]);
    CALI_MARK_END(comp_large);
    CALI_MARK_END(comp);

    //Waiting for each process to sort before merging them
    MPI_Barrier(MPI_COMM_WORLD);

    int mergedArraySize = localArraySize;
    int l;
    int k;
    for (int i = 0; i < (int) std::log2(num_processes); ++i) {
        for (int j = i; j >= 0; --j) {
            int iBit = (rank >> (i + 1)) & 1;
            int jBit = (rank >> j) & 1;
            int partner[localArraySize];
            int copy[localArraySize];

            CALI_MARK_BEGIN(comm);
            CALI_MARK_BEGIN(comm_large);
            MPI_Send(&localArray, localArraySize, MPI_INT, rank ^ (1 << j), 0, MPI_COMM_WORLD);
            MPI_Recv(&partner, localArraySize, MPI_INT, rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            CALI_MARK_END(comm_large);
            CALI_MARK_END(comm);

            for (l = 0; l < localArraySize; l++) {
                copy[l] = localArray[l];
            }

            //Merge smaller elements
            if (iBit == jBit) {
                l = 0;
                k = 0;
                while (l + k < localArraySize) {
                    if (copy[l] > partner[k]) {
                        localArray[l + k] = partner[k];
                        k++;
                    }
                    else {
                        localArray[l + k] = copy[l];
                        l++;
                    }
                }
            }

            //Merge larger elements
            else {
                l = localArraySize;
                k = localArraySize;
                while (l + k > localArraySize) {
                    if (copy[l - 1] < partner[k - 1]) {
                        localArray[l + k - localArraySize - 1] = partner[k - 1];
                        k--;
                    }
                    else {
                        localArray[l + k - localArraySize - 1] = copy[l - 1];
                        l--;
                    }
                } 
            }

            // std::cout << "Array merged sequence for process " << rank << ": ";
            // for (int i = 0; i < mergedArraySize; ++i) {
            //     std::cout << mergedArray[i] << " ";
            // }
            // std::cout << std::endl << std::endl;

            // std::cout << "Partner sequence for process " << rank << ": ";
            // for (int i = 0; i < localArraySize; ++i) {
            //     std::cout << partnerArray[i] << " ";
            // }
            // std::cout << std::endl << std::endl;
        }
    }

    //Waiting for final sort before checking correctness
    MPI_Barrier(MPI_COMM_WORLD);

    // std::cout << "Array bitonic sequence for process " << rank << ": ";
    // for (int i = 0; i < localArraySize; ++i) {
    //     std::cout << localArray[i] << " ";
    // }
    // std::cout << std::endl << std::endl;

    //Variables for correctness check
    int finalArray[input_size];
    bool sorted = true;

    //Grabbing all the values and collecting to the master
    MPI_Gather(localArray, localArraySize, MPI_INT, finalArray, localArraySize, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        //Track correctness check
        CALI_MARK_BEGIN(correctness_check);
        for (int i = 0; i < input_size - 1; ++i) {
            if (finalArray[i + 1] < finalArray[i]) {
                sorted = false;
            }
        }
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