/*
- . build.sh
- make
- sbatch mpi.grace_job 128 2
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include <iostream>
#include <fstream>
#include <mpi.h>
#include <cmath> 
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define MASTER 0



using namespace std;


int read_file(std::ifstream& lineInput, int* array) {
    int i = 0;
    while (lineInput >> array[i]) {
        i++;
    }

    return i;
}

int elementCount(std::ifstream& lineInput) {
    int count = 0;
    int temp;
    
    while (lineInput >> temp) {
        count++;
    }
    return count;
}

int partition(int arr[], int start, int end) {
    int pivot = arr[start];

    int count = 0;

    for (int i = start + 1; i<= end; i++) {
        if (arr[i] <= pivot) {
            count++;
        }
    }

    int pivotIndex = start + count;
	swap(arr[pivotIndex], arr[start]);

    // Sorting left and right parts of the pivot element
	int i = start, j = end;

	while (i < pivotIndex && j > pivotIndex) {

		while (arr[i] <= pivot) {
			i++;
		}

		while (arr[j] > pivot) {
			j--;
		}

		if (i < pivotIndex && j > pivotIndex) {
			swap(arr[i++], arr[j--]);
		}
	}

	return pivotIndex;

    
}

void quickSort(int arr[], int start, int end) {
    if (start >= end) {
        return;
    }

    int p = partition(arr, start, end);

    quickSort(arr, start, p - 1);
    quickSort(arr, p + 1, end);
}

void merge_elements(int arr[], int counts[], int k) {
	int totalelements = 0;

	for (int i = 0; i < k; i++) {
		totalelements = totalelements + counts[i];
	}

	quickSort(arr, 0, totalelements - 1);
}


void print_array(int* array, int count) {
    std::cout << "Numbers read from the file: ";
    for (int i = 0; i < count; i++) {
        std::cout << array[i] << " ";
    }
    std::cout << std::endl;
}

void multiPivotPartition(int arr[], int n, int pivots[], int numPivots, int** segments) {
    int SegmentStartIndex = 0;

    for (int i = 0; i < numPivots; ++i) {
        int pivot = pivots[i];
        int segmentEndIndex = SegmentStartIndex;

        while (segmentEndIndex < n && arr[segmentEndIndex] <= pivot) {
            ++segmentEndIndex;
        }

        if (segmentEndIndex > SegmentStartIndex) {
            segments[i][0] = SegmentStartIndex;
            segments[i][1] = segmentEndIndex - 1;

        } else {
            segments[i][0] = segments[i][1] = -1;
        }

        SegmentStartIndex = segmentEndIndex;
    }

    if (SegmentStartIndex < n) {
        segments[numPivots][0] = SegmentStartIndex;
        segments[numPivots][1] = n - 1;

    } else {
        segments[numPivots][0] = segments[numPivots][1] = -1;
    }

}




int main(int agrc, char* argv[]) {
    CALI_CXX_MARK_FUNCTION;
    
    //if (agrc != 3) {
      //  std::cout << "Usage: Please enter input filename to perform sorting.\n";
        //return 1;
    //}
    CALI_MARK_BEGIN("data_init_runtime");
    long n = atol(argv[1]);
    int* original_array = (int*)malloc(n * sizeof(int));
    
    int c;
    srand(time(NULL));

    std::string input_type = argx[2];

    switch (input_type[0]) {
    case 'r': { // Random
        for (int c = 0; c < n; c++) {
            original_array[c] = rand() % n;
        }
        break;
    }
    case 's': { // Sorted
        for (int c = 0; c < n; c++) {
            original_array[c] = c;
        }
        break;
    }
    case 'v': { // Reverse sorted
        for (int c = 0; c < n; c++) {
            original_array[c] = n - c - 1;
        }
        break;
    }
    case 'p': { // 1% perturbation
        for (int c = 0; c < n; c++) {
            original_array[c] = c;
        }
        // Introduce 1% perturbation
        int num_perturbations = n / 100; // 1% of the array size
        for (int i = 0; i < num_perturbations; i++) {
            int index = rand() % n;
            original_array[index] = rand() % n; // Replace with random value
        }
        break;
    }
    default: {
        std::cout << "Invalid input type. Use 'sorted', 'random', 'reverse_sorted', or 'perturbed'.\n";
        free(original_array);
        return 1;
    }
}

    
    for (c = 0; c < n; c++) {
        original_array[c] = rand() % n;
        
    }
    CALI_MARK_END("data_init_runtime");
    

    // Read the input filename from the first argument
    //std::string filename = "input_65536_Random.txt";

    // Read the array size from the second argument
    //int n = 65536;  // Array size

    // Open the input file
    //std::ifstream input(filename);
    
    //if (!input) {
    //    std::cerr << "Error: Could not open file " << filename << "\n";
    //    return 1;
    //}

    int numProcess, processId;

    double startTime;

    //Splitting the given dataset into equally divided smaller segments, where each segement is given to a processor
    CALI_MARK_BEGIN("MPI_Init");

    MPI_Init(&agrc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcess);
    MPI_Comm_rank(MPI_COMM_WORLD, &processId); 
    printf("MPI process %d has started...\n", processId);
    CALI_MARK_END("MPI_Init");


    // Initialize Adiak for metadata collection
    adiak::init(NULL);
    adiak::value("algorithm", "Sample Sort");
    adiak::value("programming_model", "mpi");
    adiak::value("group_num", 18);

    // the amount of intergers send to each processor
    int nPerProcess = n / numProcess;

    /*Scattering the arrary into difference processors via receiveBuffer*/
    //Allocating memory for the receive buffer
    int* receiveBuffer = (int*)malloc(nPerProcess * sizeof(int));

    int* sendBuffer = original_array;

    //if (processId == MASTER) {
      //  sendBuffer = (int*)malloc(n * sizeof(int));
        //int arraySize = read_file(input, sendBuffer);
        //cout << "Number of elements read: " << arraySize << endl; // Check read count



    //}

    startTime = MPI_Wtime();
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_scatter");
    MPI_Scatter(sendBuffer, nPerProcess, MPI_INT, receiveBuffer, nPerProcess, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_scatter");
    CALI_MARK_END("comm");



    //Run basic sorting algorithm on each of the segments, where each processor is handling its piece independently.
    CALI_MARK_BEGIN("comp");

    CALI_MARK_BEGIN("local_sort");
    quickSort(receiveBuffer, 0, nPerProcess-1);
    CALI_MARK_END("local_sort");
    CALI_MARK_END("comp");



    //From these sorted segments, each processor selects few samples. then, MPI communication is used to collect samples from all processors
    int sampleSelectThresold = n / (numProcess * numProcess); // elements to skip for selecting pivots
    int* processSamples = (int*)malloc(numProcess * sizeof(int));
    for (int i = 0, j = 0; i < nPerProcess; i = i + sampleSelectThresold) {
        processSamples[j] = receiveBuffer[i];
        j++;
    }

    int* receiveBufferProcessSamples = (int*)malloc(numProcess * numProcess * sizeof(int));
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_gather_samples");
    MPI_Gather(processSamples, numProcess, MPI_INT, receiveBufferProcessSamples, numProcess, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_gather_samples");
    CALI_MARK_END("comm");



    int* pivotsSelected = (int*)malloc((numProcess - 1) * sizeof(int));

    //Sort the selected samples, which will help us establish a global order among the samples.
    //From the sorted samples, we pick few speical elements. which act as a pivot. which are shared with all processors. MPI_Bcast is used to broadcast the pivots to all processors.

    if (processId == MASTER) {
        CALI_MARK_BEGIN("comp");

        CALI_MARK_BEGIN("pivot_sort");

        quickSort(receiveBufferProcessSamples, 0, (numProcess * numProcess) - 1);

        int pivotSelectThresold = numProcess + (int)floor(numProcess / 2.0) - 1;
        for (int i = pivotSelectThresold, j = 0; i < numProcess * numProcess; i = i + numProcess ) {
            pivotsSelected[j] = receiveBufferProcessSamples[i];
            j++;
        }
        CALI_MARK_END("pivot_sort");
        CALI_MARK_END("comp");

    }
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_bcast_pivots");
    MPI_Bcast(pivotsSelected, numProcess - 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_bcast_pivots");
    CALI_MARK_END("comm");




    // Each processor takes its ordered segment and divides it into subsegments based on the choicen pivot.

    int numPivotSection = numProcess - 1;
    int maxSegments = numPivotSection + 1;

    int** segments;
    segments = (int**)malloc(maxSegments * sizeof(int*));
    for (int i = 0; i < maxSegments; i++) {
        segments[i] = (int*)malloc(2 * sizeof(int));
    }

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("pivot_partition");
    multiPivotPartition(receiveBuffer, nPerProcess, pivotsSelected, numPivotSection, segments);
    CALI_MARK_END("pivot_partition");
    CALI_MARK_END("comp");



    // Now, processors share their ordered segments globally with the corresponding processor based on the segment number. using MPI_Alltoall
    int* partitionSizes = (int*)malloc((numProcess - 1) * sizeof(int));

    for (int i = 0; i < maxSegments; ++i) {
        int start = segments[i][0];
        int end  = segments [i][1];

        if (start != -1 && end != -1) {
            partitionSizes[i] = end - start + 1;
        } else {
            partitionSizes[i] = 0;
        }
    }

    int* sendDispIndex = (int*)malloc(numProcess * sizeof(int));
    int* recvDispIndex = (int*)malloc(numProcess * sizeof(int));
    MPI_Barrier(MPI_COMM_WORLD);

    int* newPartitionSizes = (int*)malloc((numProcess - 1) * sizeof(int));
    
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_all_to_all");
    MPI_Alltoall(partitionSizes, 1, MPI_INT, newPartitionSizes, 1, MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END("comm_all_to_all");
    CALI_MARK_END("comm");




    int totalSize = 0;
    for (int i = 0; i < numProcess; i ++) {
        totalSize += newPartitionSizes[i];
    }


    int* newPartitions = (int*)malloc(totalSize * sizeof(int));
	//*newPartitions = (double*)malloc(totalSize * sizeof(double));

	sendDispIndex[0] = 0;
	recvDispIndex[0] = 0; //Calculate the displacement relative to recvbuf, this displacement stores the data received from the process
	for (int i = 1; i < numProcess; i++) {
		sendDispIndex[i] = partitionSizes[i - 1] + sendDispIndex[i - 1];
		recvDispIndex[i] = newPartitionSizes[i - 1] + recvDispIndex[i - 1];
	}
	MPI_Barrier(MPI_COMM_WORLD);

    CALI_MARK_BEGIN("comm");

    CALI_MARK_BEGIN("comm_alltoallv");
	MPI_Alltoallv(receiveBuffer, partitionSizes, sendDispIndex, MPI_INT, newPartitions, newPartitionSizes, recvDispIndex, MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END("comm_alltoallv");
    CALI_MARK_END("comm");


    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("merge_elements");
    merge_elements(newPartitions, newPartitionSizes, numProcess);
    CALI_MARK_END("merge_elements");
    CALI_MARK_END("comp");



    int totalelements = 0;

    for (int i = 0; i < numProcess; i++) {
        totalelements = totalelements + newPartitionSizes[i];
    }

    int* subListSizes = (int*)malloc(numProcess * sizeof(int));
	int* recvDisp2 = (int*)malloc(numProcess * sizeof(int));

	MPI_Gather(&totalelements, 1, MPI_INT, subListSizes, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (processId == 0) {
        recvDisp2[0] = 0;
        for (int i =  1; i < numProcess; i++) {
			recvDisp2[i] = subListSizes[i - 1] + recvDisp2[i - 1];
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
	int* last_array_sorted = (int*)malloc(n * sizeof(int));
	//Send each sorted sublist back to the root process
    CALI_MARK_BEGIN("comm");

    CALI_MARK_BEGIN("final_gather");
	MPI_Gatherv(newPartitions, totalelements, MPI_INT, last_array_sorted, subListSizes, recvDisp2, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("final_gather");
    CALI_MARK_END("comm");


	MPI_Barrier(MPI_COMM_WORLD);

    CALI_MARK_BEGIN("correctness_check");


    if (processId == 0) {

		int isSorted = 1;
        for (int i = 0; i < n - 1; i++) {
            //std::cout << last_array_sorted[i] << std::endl;

            if (last_array_sorted[i] > last_array_sorted[i + 1]) {
                std::cout << last_array_sorted[i] << std::endl;
                isSorted = 0;
                break;
            }
        }

        if (isSorted) {
            std::cout << "The array is sorted." << std::endl;
        } else {
            std::cout << "The array is NOT sorted" << std::endl;
        }
    }

    CALI_MARK_END("correctness_check");

		
        
    double elapsedTime = MPI_Wtime() - startTime;
	if (processId == MASTER) {
		printf("Total Elapsed time during the process : %f\n", elapsedTime);
	}

    // Collect metadata for the experiment
    adiak::value("input_size", n);
    adiak::value("num_procs", numProcess);
    adiak::value("data_type", input_type[0]);
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("scalability", "strong");

    // MPI
    //adiak::finalize();


	MPI_Finalize();
	return 0;


}