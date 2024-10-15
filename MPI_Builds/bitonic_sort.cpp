// #include "mpi.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <limits.h>
// #include <caliper/cali.h>
// #include <caliper/cali-manager.h>
// #include <adiak.hpp>
// #include <vector>
// #include <algorithm>
// #include <string>

// //Bitonic Merge
// void bitonicMerge(std::vector<int>& localData, int low, int count, bool direction) {
//     if (count > 1) {
//         int k = count / 2;
//         for (int i = low; i < low + k; ++i) {
//             if ((localData[i] > localData[i + k]) == direction) {
//                 std::swap(localData[i], localData[i + k]);
//             }
//         }
//         bitonicMerge(localData, low, k, direction);
//         bitonicMerge(localData, low + k, k, direction);
//     }
// }

// //Bitonic Sort (sequential on local data)
// void bitonicSort(std::vector<int>& localData, int low, int count, bool direction) {
//     if (count > 1) {
//         int k = count / 2;
//         bitonicSort(localData, low, k, true); //Sort in ascending order
//         bitonicSort(localData, low + k, k, false); //Sort in descending order
//         bitonicMerge(localData, low, count, direction);
//     }
// }

// //Function to check the correctness of the sorted array
// bool check_correctness(const std::vector<int>& arr) {
//     return std::is_sorted(arr.begin(), arr.end());
// }

// int main (int argc, char** argv) {
//     CALI_CXX_MARK_FUNCTION;

//     //Variables for adiak and other
//     std::string algorithm = "bitonic";
//     std::string programming_model = "mpi";
//     const char* data_type = typeid(int).name();
//     int size_of_data_type = sizeof(int);
//     int input_size = atoi(argv[1]);
//     std::string input_type = "random";
//     int num_procs;
//     std::string scalability = "strong";
//     int group_number = 14;
//     std::string implementation_source = "ai";
//     int rank;

//     //MPI initialization
//     MPI_Init(&argc, &argv);
//     MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
//     MPI_Comm_rank(MPI_COMM_WORLD, &rank);

//     //Create caliper ConfigManager object
//     cali::ConfigManager mgr;
//     mgr.start();

//     //Caliper region names
//     const char* data_init_runtime = "data_init_runtime";
//     const char* comm = "comm";
//     const char* comm_send_rcv = "comm_send_rcv";
//     const char* comm_gather = "comm_gather";
//     const char* comp = "comp";
//     const char* comp_small = "comp_small";
//     const char* comp_large = "comp_large";
//     const char* correctness_check = "correctness_check";

//     int local_n = input_size / num_procs; //Number of elements per process
//     std::vector<int> local_data(local_n);

//     CALI_MARK_BEGIN(data_init_runtime);
//     //Initialize local data (each process gets a portion of the array)
//     srand(rank + 1); //Seed with rank for different random numbers per process
//     for (int i = 0; i < local_n; ++i) {
//         local_data[i] = rand() % 100;
//     }
//     CALI_MARK_END(data_init_runtime);

//     CALI_MARK_BEGIN(comm);

//     CALI_MARK_END(comm);

//     CALI_MARK_BEGIN(comp);

//     CALI_MARK_END(comp);

//     std::cout << "Initial data for process " << rank << ": ";
//     for (int i = 0; i < local_n; ++i) {
//         std::cout << local_data[i] << " ";
//     }
//     std::cout << std::endl;

//     //Sort local data using sequential bitonic sort
//     bitonicSort(local_data, 0, local_n, true);

//     //Perform the parallel part of bitonic sort using MPI
//     for (int phase = 1; phase <= num_procs; ++phase) {
//         int partner = rank ^ (1 << (phase - 1)); //Find the process to communicate with

//         //Exchange data with the partner process
//         std::vector<int> recv_data(local_n);
//         MPI_Sendrecv(local_data.data(), local_n, MPI_INT, partner, 0,
//                      recv_data.data(), local_n, MPI_INT, partner, 0,
//                      MPI_COMM_WORLD, MPI_STATUS_IGNORE);

//         //Merge the received data with the local data
//         if (rank < partner) {
//             //Ascending order
//             local_data.insert(local_data.end(), recv_data.begin(), recv_data.end());
//             bitonicMerge(local_data, 0, local_data.size(), true);
//             local_data.resize(local_n); //Keep only the first half
//         } else {
//             //Descending order
//             local_data.insert(local_data.end(), recv_data.begin(), recv_data.end());
//             bitonicMerge(local_data, 0, local_data.size(), false);
//             local_data.resize(local_n); //Keep only the second half
//         }
//     }

//     //Gather the sorted subarrays back to the root process
//     std::vector<int> sorted_data;
//     if (rank == 0) {
//         sorted_data.resize(input_size);
//     }
//     MPI_Gather(local_data.data(), local_n, MPI_INT, sorted_data.data(), local_n, MPI_INT, 0, MPI_COMM_WORLD);

//     if (rank == 0) {
//         CALI_MARK_BEGIN("correctness_check");
//         bool sorted = check_correctness(sorted_data);
//         CALI_MARK_END("correctness_check");

//         if (sorted) {
//             std::cout << "Array is sorted correctly." << std::endl;
//         } else {
//             std::cout << "Array is not sorted correctly!" << std::endl;
//         }
//     }

//     adiak::init(NULL);
//     adiak::launchdate();    // launch date of the job
//     adiak::libraries();     // Libraries used
//     adiak::cmdline();       // Command line used to launch the job
//     adiak::clustername();   // Name of the cluster
//     adiak::value("algorithm", algorithm); // The name of the algorithm you are using (e.g., "merge", "bitonic")
//     adiak::value("programming_model", programming_model); // e.g. "mpi"
//     adiak::value("data_type", data_type); // The datatype of input elements (e.g., double, int, float)
//     adiak::value("size_of_data_type", size_of_data_type); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
//     adiak::value("input_size", input_size); // The number of elements in input dataset (1000)
//     adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
//     adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
//     adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
//     adiak::value("group_num", group_number); // The number of your group (integer, e.g., 1, 10)
//     adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

//     //Flush Caliper output before finalizing MPI
//     mgr.stop();
//     mgr.flush();
//     MPI_Finalize();
// }

#include "mpi.h"
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define MASTER 0

int process_rank;
int* array;
int array_size;

int ComparisonFunc(const void* a, const void* b) {
    return (*(int*) a - *(int*) b);
}

void CompareLow(int j) {

    //Send entire array to paired H Process
    //Exchange with a neighbor whose (d-bit binary) processor number differs only at the jth bit.
    int send_counter = 0;
    int* buffer_send = (int*) malloc((array_size + 1) * sizeof(int));
    MPI_Send(&array[array_size - 1], 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

    //Receive new min of sorted numbers
    int min;
    int recv_counter;
    int* buffer_recieve = (int*) malloc((array_size + 1) * sizeof(int));
    MPI_Recv(&min, 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //Buffers all values which are greater than min send from H Process.
    for (int i = 0; i < array_size; i++) {
        if (array[i] > min) {
            buffer_send[send_counter + 1] = array[i];
            send_counter++;
        } else {
            break;
        }
    }
    buffer_send[0] = send_counter;

    //Send partition to and receive it from the paired H process
    MPI_Send(buffer_send, send_counter, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);
    MPI_Recv(buffer_recieve, array_size, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //Take received buffer of values from H Process which are smaller than current max
    for (int i = 1; i < buffer_recieve[0] + 1; i++) {
        if (array[array_size - 1] < buffer_recieve[i]) {
            //Store value from message
            array[array_size - 1] = buffer_recieve[i];
        } else {
            break;
        }
    }

    //Sequential Sort
    qsort(array, array_size, sizeof(int), ComparisonFunc);

    //Reset the state of the heap from Malloc
    free(buffer_send);
    free(buffer_recieve);

    return;
}

void CompareHigh(int j) {

    //Receive max from L Process's entire array
    int max;
    int recv_counter;
    int* buffer_recieve = (int*) malloc((array_size + 1) * sizeof(int));
    MPI_Recv(&max, 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //Send min to L Process of current process's array
    int send_counter = 0;
    int* buffer_send = (int*) malloc((array_size + 1) * sizeof(int));
    MPI_Send(&array[0], 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

    //Buffer a list of values which are smaller than max value
    for (int i = 0; i < array_size; i++) {
        if (array[i] < max) {
            buffer_send[send_counter + 1] = array[i];
            send_counter++;
        } else {
            break;
        }
    }

    //Receive blocks greater than min from paired slave
    MPI_Recv(buffer_recieve, array_size, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    recv_counter = buffer_recieve[0];

    //Send partition to paired slave
    buffer_send[0] = send_counter;
    MPI_Send(buffer_send, send_counter, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

    //Take received buffer of values from L Process which are greater than current min
    for (int i = 1; i < recv_counter + 1; i++) {
        if (buffer_recieve[i] > array[0]) {
            //Store value from message
            array[0] = buffer_recieve[i];
        } else {
            break;
        }
    }

    //Sequential Sort
    qsort(array, array_size, sizeof(int), ComparisonFunc);

    //Reset the state of the heap from Malloc
    free(buffer_send);
    free(buffer_recieve);

    return;
}

int main(int argc, char * argv[]) {
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

    //For timing purposes
    double timer_start;
    double timer_end;
    int num_processes;

    //Initialization, get number of processes & this PID/rank
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    //Create caliper ConfigManager object
    cali::ConfigManager mgr;
    mgr.start();

    //Caliper region names
    const char* data_init_runtime = "data_init_runtime";
    const char* comm = "comm";
    const char* comm_send_rcv = "comm_send_rcv";
    const char* comm_gather = "comm_gather";
    const char* comp = "comp";
    const char* comp_small = "comp_small";
    const char* comp_large = "comp_large";
    const char* correctness_check = "correctness_check";

    //Initialize array for random numbers
    array_size = input_size / num_processes;
    array = (int*) malloc(array_size * sizeof(int));

    //Generate random numbers for sorting (within each process)
    srand(time(NULL));  //Needed for rand()
    for (int i = 0; i < array_size; i++) {
        array[i] = rand() % (input_size);
    }

    printf("Displaying initial random array:\n");
    for (int i = 0; i < input_size; i++) {
        printf("%d ",array[i]);
    }
    printf("\n\n");

    //Blocks until all processes have finished generating
    MPI_Barrier(MPI_COMM_WORLD);

    //Cube dimension
    int dimensions = (int)(log2(num_processes));

    //Start timer before starting first sort operation (first iteration)
    if (process_rank == MASTER) {
        printf("Number of Processes spawned: %d\n", num_processes);
        printf("Size of array: %d\n", input_size);
        timer_start = MPI_Wtime();
    }

    //Sequential sort
    qsort(array, array_size, sizeof(int), ComparisonFunc);

    //Bitonic sort follows
    for (int i = 0; i < dimensions; i++) {
        for (int j = i; j >= 0; j--) {
            //(window_id is even AND jth bit of process is 0)
            //OR (window_id is odd AND jth bit of process is 1)
            if (((process_rank >> (i + 1)) % 2 == 0 && (process_rank >> j) % 2 == 0) || ((process_rank >> (i + 1)) % 2 != 0 && (process_rank >> j) % 2 != 0)) {
                CompareLow(j);
            } else {
                CompareHigh(j);
            }
        }
    }

    //Blocks until all processes have finished sorting
    MPI_Barrier(MPI_COMM_WORLD);

    if (process_rank == MASTER) {
        timer_end = MPI_Wtime();
        printf("Displaying sorted array:\n");

        //Print sorting results
        for (int i = 0; i < array_size; i++) {
            printf("%d ",array[i]);
        }

        printf("\n\n");
        printf("Time Elapsed (Sec): %f\n", timer_end - timer_start);

        CALI_MARK_BEGIN(correctness_check);
        bool correct = true;
        for (int i = 1; i < array_size; i++) {
            if (array[i] < array[i - 1]) {
                correct = false;
            }
        }
        if (correct) {
            printf("Array is correctly sorted!\n");
        } else {
            printf("Array is not correctly sorted.\n");
        }
        CALI_MARK_END(correctness_check);
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

    //Reset the state of the heap from Malloc
    free(array);

    //Flush Caliper output before finalizing MPI
    mgr.stop();
    mgr.flush();
    MPI_Finalize();
    return 0;
}