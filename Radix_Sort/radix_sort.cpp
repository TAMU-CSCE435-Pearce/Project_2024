#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "mpi.h"
#include <random>
#include <sstream>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define BASE 2
#define FALSE (1 == 0)
#define TRUE (1 == 1)

int np;

// Create array of given size and values
int *createArray(int size, int initialValue) 
{
    int *arr = (int *)malloc(sizeof(int) * size);
    for(int i = 0; i < size; i++)
    {
        arr[i] = initialValue;
    }
    return arr;
}

// Populate random array
int *getRandomArray(int size)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 100000000.0);
    int *arr = (int *)malloc(sizeof(int) * size);
    for(int i = 0; i < size; i++)
    {
        arr[i] = (int)dis(gen);
    }
    return arr;
}

// Find max number
int findMax(int *arr, int size)
{
    int max = 0;
    for(int i = 0; i < size; i++)
    {
        if(arr[i] > max)
        {
            max = arr[i];
        }
    }
    return max;
}

// Find max numbers digit count
int findMaxDigitCount(int *arr, int size)
{
    int max = findMax(arr, size);
    int digitCount = 0;
    while (max > 0)
    {
        max /= BASE;
        digitCount++;
    }

    return digitCount;
}

// Get maximum digit count across processors
int findMaxDigitCountFromNetwork(int *arr, int size, int rank)
{
    int localMDC = findMaxDigitCount(arr, size);
    int *localMDCs;

    if(rank == 0)
    {
        localMDCs = (int *)malloc(sizeof(int) * np);
    }
    
    MPI_Gather(&localMDC, 1, MPI_INT, localMDCs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    int max = 0;
    if(rank == 0)
    {
        for(int i = 0; i < np; i++)
        {
            if(localMDCs[i] > max)
            {
                max = localMDCs[i];
            }
        }
    }

    MPI_Bcast(&max, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    return max;
}

// Get power value of 2 numbers
int power(int num, int pow)
{
    int result = 1;
    for(int i = 0; i < pow; i++)
    {
        result *= num;
    }
    return result;
}

// Get digit value based on num, the given digit span, and the offset (if any)
int getDigitValue(int number, int digitSpan, int offset)
{
    int divisor = power(BASE, digitSpan);
    for(int i = 0; i < offset; i++)
    {
        number /= divisor;
    }
    int res = number % (divisor);
    return res;
}

// Counting sort on subarray, communicates histogram and subarray data with MPI
// Worker function used to iterate through subarray with digit spans and offsets to sort it
// Communication with other processors allows the total array to sort when combined with histogram data and subarrays.
int *countSort(int *arr, int size, int digitSpan, int offset, int *returnSize)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int iter = power(BASE, digitSpan);
    int *digitValues = (int *)malloc(sizeof(int) * size);
    int *hist = createArray(iter, 0);
    int *preSum = createArray(iter, 0);
    int *preSumGroup = createArray(iter, 0);

    for(int i = 0; i < size; i++)
    {
        digitValues[i] = getDigitValue(arr[i], digitSpan, offset);
        hist[digitValues[i]]++;
    }

    for(int i = 1; i < iter; i++)
    {
        preSum[i] = hist[i-1] + preSum[i-1];
    }

    int *relOffset = createArray(size, 0);
    for(int i = 0; i < size; i++)
    {
        relOffset[i] = preSumGroup[digitValues[i]];
        preSumGroup[digitValues[i]]++;
    }

    int *temp = (int *)malloc(sizeof(int) * size);
    for(int i = 0; i < size; i++)
    {
        temp[preSum[digitValues[i]] + relOffset[i]] = arr[i];
    }

    int **distMap = (int **)malloc(sizeof(int *) * np);
    int *distIndex = (int *)malloc(sizeof(int) * np);
    for(int i = 0; i < np; i++)
    {
        distMap[i] = (int *)malloc(sizeof(int) * hist[i]);
        distIndex[i] = 0;
    }

    for(int i = 0; i < size; i++)
    {
        int dVal = getDigitValue(temp[i], digitSpan, offset);
        distMap[dVal][distIndex[dVal]++] = temp[i];
    }

    int npNp = np * np;
    int *expHist = (int *)malloc(sizeof(int) * npNp);
    for(int i = 0; i < npNp; i++)
    {
        expHist[i] = 0;
    }

    MPI_Allgather(hist, np, MPI_INT, expHist, np, MPI_INT, MPI_COMM_WORLD);

    int *histIncoming = (int *)malloc(sizeof(int) * np);
    for(int i = 0; i < np; i++)
    {
        histIncoming[i] = 0;
    }

    int *displacement = (int *)malloc(sizeof(int) * np);
    int numIncoming = 0;
    for(int i = 0; i < np; i++)
    {
        int idx = (np * i) + rank;
        histIncoming[i] = expHist[idx];
        numIncoming += histIncoming[i];
        displacement[i] = 0;
        if(i > 0)
        {
            displacement[i] = displacement[i-1] + histIncoming[i-1];
        }
    }

    int *nextArr = (int *)malloc(sizeof(int) * numIncoming);
    for(int i = 0; i < np; i++)
    {   
        MPI_Gatherv(distMap[i], hist[i], MPI_INT, nextArr, histIncoming, displacement, MPI_INT, i, MPI_COMM_WORLD);
    }

    // Free memory
    free(digitValues);
    free(preSum);
    free(preSumGroup);
    free(relOffset);
    free(temp);
    free(distIndex);
    free(histIncoming);
    free(displacement);
    free(expHist);

    for(int i = 0; i < np; i++)
    {
        free(distMap[i]);
    }

    free(distMap);

    (*returnSize) = numIncoming;

    return nextArr;
}

// Check if array values are sorted in increasing order
int checkSorted(int *arr, int size)
{
    for(int i = 1; i < size; i++)
    {
        if(arr[i] < arr[i-1])
        {
            return FALSE;
        }
    }
    return TRUE;
}

// Get array size exponent and get true size with BASE
int readArraySize(int argc, char **argv)
{
    for(int i = 0; i < argc; i++)
    {
        if(strcmp(argv[i], "--size") == 0)
        {
            return atoi(argv[i + 1]);
        }
    }
    return 0;
}

// Main program running functions and gathering Caliper data
int main(int argc, char **argv)
{
    CALI_CXX_MARK_FUNCTION;

    // Caliper region names
    const char* data_init_runtime = "data_init_runtime";
    const char* comm = "comm";
    const char* comm_small = "comm_small";
    const char* comm_large = "comm_large";
    const char* comp = "comp";
    const char* comp_small = "comp_small";
    const char* comp_large = "comp_large";
    const char* correctness_check = "correctness_check";
    

    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int arrSize, *arr;
    double startTS;

    // Create caliper ConfigManager object
    cali::ConfigManager mgr;
    mgr.start();

    // Create random array and check if sorted (should not be)
    if(rank == 0)
    {
        startTS = MPI_Wtime();
        CALI_MARK_BEGIN(data_init_runtime);
        arrSize = readArraySize(argc, argv);
        arr = getRandomArray(arrSize);
        CALI_MARK_END(data_init_runtime);
        printf("[ARRAY SIZE]: %d\n", arrSize);
        printf("[SORTED]: %s\n", checkSorted(arr, arrSize) ? "TRUE" : "FALSE");
    }

    int digitSpan = (int)log2(np);
    
    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    MPI_Bcast(&arrSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_large);
    CALI_MARK_END(comm);

    int subSize = arrSize / np;
    int *subArr = (int *)malloc(sizeof(int) * subSize);

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Scatter(arr, subSize, MPI_INT, subArr, subSize, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_small);
    int maxDigitCount = findMaxDigitCountFromNetwork(subArr, subSize, rank);
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);
    int noExcessScanCount = maxDigitCount / digitSpan;

    for(int i = 0; i < noExcessScanCount + 1; i++)
    {   
        CALI_MARK_BEGIN(comp);
        CALI_MARK_BEGIN(comp_large);
        int *next = countSort(subArr, subSize, digitSpan, i, &subSize);
        CALI_MARK_END(comp_large);
        CALI_MARK_END(comp);
        free(subArr);
        subArr = next;
    }

    int *expectedIncoming, *displacements, *sortedArr;

    if(rank == 0)
    {
        expectedIncoming = (int *)malloc(sizeof(int) * np);
    }
    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Gather(&subSize, 1, MPI_INT, expectedIncoming, 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    if(rank == 0)
    {
        displacements = (int *)malloc(sizeof(int) * np);
        displacements[0] = 0;
        sortedArr = (int *)malloc(sizeof(int) * arrSize);
        for(int i = 1; i < np; i++)
        {
            displacements[i] = displacements[i-1] + expectedIncoming[i-1];
        }
    }

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Gatherv(subArr, subSize, MPI_INT, sortedArr, expectedIncoming, displacements, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    // Check if final array is sorted
    if(rank == 0)
    {
        CALI_MARK_BEGIN(correctness_check);
        printf("[SORTED]: %s\n", checkSorted(sortedArr, arrSize) ? "TRUE" : "FALSE");
        CALI_MARK_END(correctness_check);

        free(sortedArr);
        printf("Elapsed time = %f seconds\n", MPI_Wtime()-startTS);
    }

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "radix"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi"); // e.g. "mpi"
    adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", arrSize); // The number of elements in input dataset (1000)
    adiak::value("input_type", "Random"); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", np); // The number of processors (MPI ranks)
    adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", 18); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "online"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").
    
    // Flush Caliper output before finalizing MPI
    mgr.stop();
    mgr.flush();

    free(subArr);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}