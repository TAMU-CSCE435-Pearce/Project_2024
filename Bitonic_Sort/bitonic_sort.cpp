#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define MASTER 0

//7 plots -> each of input size (title)
// Each line is a graph

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
        int partnerIndex = count / 2;
        for (int i = low; i < low + partnerIndex; ++i) {
            compAndSwap(a, i, i + partnerIndex, direction);
        }
        bitonicMerge(a, low, partnerIndex, direction);
        bitonicMerge(a, low + partnerIndex, partnerIndex, direction);
    }
}

void bitonicSort(int a[],int low, int count, int direction)
{
    if (count > 1)
    {
        int partnerIndex = count / 2;
 
        //Sort in ascending order since dir here is 1
        bitonicSort(a, low, partnerIndex, 1);
 
        //Sort in descending order since dir here is 0
        bitonicSort(a, low + partnerIndex, partnerIndex, 0);
 
        //Will merge whole sequence in ascending order since dir=1.
        bitonicMerge(a, low, count, direction);
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
    std::string input_type = argv[2];
    std::string scalability = "strong";
    int group_number = 14;
    std::string implementation_source = "ai";

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

    //Track data initialization
    CALI_MARK_BEGIN(data_init_runtime);

    //Random values
    if (input_type == "Random") {
        for (int i = 0; i < localArraySize; ++i) {
            localArray[i] = rand() % 10000;
        }
    }

    //Sorted values
    if (input_type == "Sorted") {
        for (int i = 0; i < localArraySize; ++i) {
            localArray[i] = i + rank * localArraySize;
        }
    }

    //Reverse sorted values
    if (input_type == "ReverseSorted") {
        for (int i = 0; i < localArraySize; ++i) {
            localArray[i] = localArraySize - i + (num_processes - rank - 1) * localArraySize;
        }
    }

    //1% perturbed values
    if (input_type == "1_perc_perturbed") {
        for (int i = 0; i < localArraySize; ++i) {
            localArray[i] = i + rank * localArraySize;
        }
        for (int i = 0; i < localArraySize * 0.01; i++){
            int index = rand() % localArraySize;
            localArray[index] = rand() % 10000;
        }
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

    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_large);
    int copyIndex;
    int partnerIndex;
    for (int i = 0; i < (int) std::log2(num_processes); ++i) {
        for (int j = i; j >= 0; --j) {
            int bit1 = (rank >> (i + 1)) & 1;
            int bit2 = (rank >> j) & 1;
            int partnerArray[localArraySize];
            int copyArray[localArraySize];

            CALI_MARK_BEGIN(comm);
            CALI_MARK_BEGIN(comm_large);
            MPI_Send(&localArray, localArraySize, MPI_INT, rank ^ (1 << j), 0, MPI_COMM_WORLD);
            MPI_Recv(&partnerArray, localArraySize, MPI_INT, rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            CALI_MARK_END(comm_large);
            CALI_MARK_END(comm);

            for (copyIndex = 0; copyIndex < localArraySize; copyIndex++) {
                copyArray[copyIndex] = localArray[copyIndex];
            }

            //Merging the smaller elements to the lower process
            if (bit1 == bit2) {
                copyIndex = 0;
                partnerIndex = 0;
                while (copyIndex + partnerIndex < localArraySize) {
                    if (copyArray[copyIndex] > partnerArray[partnerIndex]) {
                        localArray[copyIndex + partnerIndex] = partnerArray[partnerIndex];
                        partnerIndex++;
                    }
                    else {
                        localArray[copyIndex + partnerIndex] = copyArray[copyIndex];
                        copyIndex++;
                    }
                }
            }

            //Merging the larger elements to the higher process
            else {
                copyIndex = localArraySize;
                partnerIndex = localArraySize;
                while (copyIndex + partnerIndex > localArraySize) {
                    if (copyArray[copyIndex - 1] < partnerArray[partnerIndex - 1]) {
                        localArray[copyIndex + partnerIndex - localArraySize - 1] = partnerArray[partnerIndex - 1];
                        partnerIndex--;
                    }
                    else {
                        localArray[copyIndex + partnerIndex - localArraySize - 1] = copyArray[copyIndex - 1];
                        copyIndex--;
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
    CALI_MARK_END(comp_large);
    CALI_MARK_END(comp);

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
    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Gather(localArray, localArraySize, MPI_INT, finalArray, localArraySize, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

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