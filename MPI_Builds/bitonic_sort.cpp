// #include "mpi.h"
// #include <stdio.h>
// #include <time.h>
// #include <math.h>
// #include <stdlib.h>
// #include <caliper/cali.h>
// #include <caliper/cali-manager.h>
// #include <adiak.hpp>

// #define MASTER 0

// int process_rank;
// int* array;
// int array_size;

// int ComparisonFunc(const void* a, const void* b) {
//     return (*(int*) a - *(int*) b);
// }

// void CompareLow(int j) {

//     //Send entire array to paired H Process
//     //Exchange with a neighbor whose (d-bit binary) processor number differs only at the jth bit.
//     int send_counter = 0;
//     int* buffer_send = (int*) malloc((array_size + 1) * sizeof(int));
//     MPI_Send(&array[array_size - 1], 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

//     //Receive new min of sorted numbers
//     int min;
//     int recv_counter;
//     int* buffer_recieve = (int*) malloc((array_size + 1) * sizeof(int));
//     MPI_Recv(&min, 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

//     //Buffers all values which are greater than min send from H Process.
//     for (int i = 0; i < array_size; i++) {
//         if (array[i] > min) {
//             buffer_send[send_counter + 1] = array[i];
//             send_counter++;
//         } else {
//             break;
//         }
//     }
//     buffer_send[0] = send_counter;

//     //Send partition to and receive it from the paired H process
//     MPI_Send(buffer_send, send_counter, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);
//     MPI_Recv(buffer_recieve, array_size, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

//     //Take received buffer of values from H Process which are smaller than current max
//     for (int i = 1; i < buffer_recieve[0] + 1; i++) {
//         if (array[array_size - 1] < buffer_recieve[i]) {
//             //Store value from message
//             array[array_size - 1] = buffer_recieve[i];
//         } else {
//             break;
//         }
//     }

//     //Sequential Sort
//     qsort(array, array_size, sizeof(int), ComparisonFunc);

//     //Reset the state of the heap from Malloc
//     free(buffer_send);
//     free(buffer_recieve);

//     return;
// }

// void CompareHigh(int j) {

//     //Receive max from L Process's entire array
//     int max;
//     int recv_counter;
//     int* buffer_recieve = (int*) malloc((array_size + 1) * sizeof(int));
//     MPI_Recv(&max, 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

//     //Send min to L Process of current process's array
//     int send_counter = 0;
//     int* buffer_send = (int*) malloc((array_size + 1) * sizeof(int));
//     MPI_Send(&array[0], 1, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

//     //Buffer a list of values which are smaller than max value
//     for (int i = 0; i < array_size; i++) {
//         if (array[i] < max) {
//             buffer_send[send_counter + 1] = array[i];
//             send_counter++;
//         } else {
//             break;
//         }
//     }

//     //Receive blocks greater than min from paired slave
//     MPI_Recv(buffer_recieve, array_size, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//     recv_counter = buffer_recieve[0];

//     //Send partition to paired slave
//     buffer_send[0] = send_counter;
//     MPI_Send(buffer_send, send_counter, MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

//     //Take received buffer of values from L Process which are greater than current min
//     for (int i = 1; i < recv_counter + 1; i++) {
//         if (buffer_recieve[i] > array[0]) {
//             //Store value from message
//             array[0] = buffer_recieve[i];
//         } else {
//             break;
//         }
//     }

//     //Sequential Sort
//     qsort(array, array_size, sizeof(int), ComparisonFunc);

//     //Reset the state of the heap from Malloc
//     free(buffer_send);
//     free(buffer_recieve);

//     return;
// }

// int main(int argc, char * argv[]) {
//     CALI_CXX_MARK_FUNCTION;

//     //Variables for adiak and other
//     std::string algorithm = "bitonic";
//     std::string programming_model = "mpi";
//     const char* data_type = typeid(int).name();
//     int size_of_data_type = sizeof(int);
//     int input_size = atoi(argv[1]);
//     std::string input_type = "random";
//     std::string scalability = "weak";
//     int group_number = 14;
//     std::string implementation_source = "online";

//     //For timing purposes
//     double timer_start;
//     double timer_end;
//     int num_processes;

//     //Initialization, get number of processes & this PID/rank
//     MPI_Init(&argc, &argv);
//     MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
//     MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

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

// //     CALI_MARK_BEGIN(data_init_runtime);

// //     CALI_MARK_END(data_init_runtime);

// //     CALI_MARK_BEGIN(comm);

// //     CALI_MARK_END(comm);

// //     CALI_MARK_BEGIN(comp);

// //     CALI_MARK_END(comp);

//     //Initialize array for random numbers
//     array_size = input_size / num_processes;
//     array = (int*) malloc(array_size * sizeof(int));

//     //Generate random numbers for sorting (within each process)
//     srand(time(NULL));  //Needed for rand()
//     for (int i = 0; i < array_size; i++) {
//         array[i] = rand() % (input_size);
//     }

//     printf("Displaying initial random array:\n");
//     for (int i = 0; i < input_size; i++) {
//         printf("%d ",array[i]);
//     }
//     printf("\n\n");

//     //Blocks until all processes have finished generating
//     MPI_Barrier(MPI_COMM_WORLD);

//     //Cube dimension
//     int dimensions = (int)(log2(num_processes));

//     //Start timer before starting first sort operation (first iteration)
//     if (process_rank == MASTER) {
//         printf("Number of Processes spawned: %d\n", num_processes);
//         printf("Size of array: %d\n", input_size);
//         timer_start = MPI_Wtime();
//     }

//     //Sequential sort
//     qsort(array, array_size, sizeof(int), ComparisonFunc);

//     //Bitonic sort follows
//     for (int i = 0; i < dimensions; i++) {
//         for (int j = i; j >= 0; j--) {
//             //(window_id is even AND jth bit of process is 0)
//             //OR (window_id is odd AND jth bit of process is 1)
//             if (((process_rank >> (i + 1)) % 2 == 0 && (process_rank >> j) % 2 == 0) || ((process_rank >> (i + 1)) % 2 != 0 && (process_rank >> j) % 2 != 0)) {
//                 CompareLow(j);
//             } else {
//                 CompareHigh(j);
//             }
//         }
//     }

//     //Blocks until all processes have finished sorting
//     MPI_Barrier(MPI_COMM_WORLD);

//     if (process_rank == MASTER) {
//         timer_end = MPI_Wtime();
//         printf("Displaying sorted array:\n");

//         //Print sorting results
//         for (int i = 0; i < array_size; i++) {
//             printf("%d ",array[i]);
//         }

//         printf("\n\n");
//         printf("Time Elapsed (Sec): %f\n", timer_end - timer_start);

//         CALI_MARK_BEGIN(correctness_check);
//         bool correct = true;
//         for (int i = 1; i < array_size; i++) {
//             if (array[i] < array[i - 1]) {
//                 correct = false;
//             }
//         }
//         if (correct) {
//             printf("Array is correctly sorted!\n");
//         } else {
//             printf("Array is not correctly sorted.\n");
//         }
//         CALI_MARK_END(correctness_check);
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
//     adiak::value("num_procs", num_processes); // The number of processors (MPI ranks)
//     adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
//     adiak::value("group_num", group_number); // The number of your group (integer, e.g., 1, 10)
//     adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

//     //Reset the state of the heap from Malloc
//     free(array);

//     //Flush Caliper output before finalizing MPI
//     mgr.stop();
//     mgr.flush();
//     MPI_Finalize();
//     return 0;
// }

#include <omp.h>
#include "mpi.h"
#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/timeb.h>
#include <ctime>

#define SWAP(x,y) t = x; x = y; y = t;

using namespace std;

int up = 1;
int down = 0;

void init(void);
void print(void);
int N, m, numThreads;
int i, s;

void compare(int* arr, int i, int j, int dir)
{
	int t;
	if (dir == (arr[i] > arr[j])) {
		SWAP(arr[i], arr[j]);
	}
}

/*
* Sorts a bitonic sequence in ascending order if dir=1
* otherwise in descending order
*/
void bitonicmerge(int* arr, int start, int length, int dir)
{
	int halfLength, i;
	if (length > 1)
	{
		halfLength = length / 2;
		for (i = start; i < start + halfLength; i++)
			compare(arr, i, i + halfLength, dir);
		m++;
#pragma omp parallel sections
		{
#pragma omp section
			bitonicmerge(arr, start, halfLength, dir);
#pragma omp section
			bitonicmerge(arr, start + halfLength, halfLength, dir);
		}
	}
}

/*
* Generates bitonic sequence by sorting recursively
* two halves of the array in opposite sorting orders
* bitonicmerge will merge the resultant data
*/
void recbitonic(int* arr, int start, int length, int dir)
{
	int halfLength;
	if (length > 1)
	{
		halfLength = length / 2;
		recbitonic(arr, start, halfLength, up);
		recbitonic(arr, start + halfLength, halfLength, down);
		bitonicmerge(arr, start, length, dir);
	}
}

int* changeSizeArray(int* arr, int lenght, int newLenght)
{
	int *array = new int[newLenght];
	for (int i = 0; i < lenght; i++)
	{
		array[i] = arr[i];
	}
	delete[]arr;
	return array;
}
int main(int argc, char **argv){
	int ProcRank, ProcNum;
	int* data;
	//int* curArrSorted;
	data = (int *)malloc(N * sizeof(int));
	int curCountArr = 0;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
	MPI_Status status;

	omp_set_nested(0);
	N = (argc > 1) ? atoi(argv[1]) : 8;
	m = (argc > 2) ? atoi(argv[2]) : 100;
	numThreads = (argc > 3) ? atoi(argv[3]) : omp_get_max_threads();

	printf("Threads count: %d\n", numThreads);
	omp_set_num_threads(numThreads);

	//partSize = N / numThreads;
	//init();
	//print();
	//data[0] = 1;
	//data[1] = 4;
	//data[2] = 7;
	//data[3] = 9;
	//data[4] = 12;
	//data[5] = 6;
	//data[6] = 3;
	//data[7] = 2;

	int* Offsets = new int[ProcNum];
	int* Elements = new int[ProcNum];
	int part;

	if (!ProcRank)
	{

		for (i = 0; i < N; i++) {
			data[i] = rand() % m;
			//cout << data[i] << " ";
		}

		cout << "Data generated" << endl;
	}

	if (ProcNum % 2 == 0) {
		cout << "Odd number" << endl;
		part = N / ProcNum;
		cout << "Part: " << part << endl;

		for (int i = 0; i < ProcNum; i++)
		{
			Offsets[i] = i*part;
			Elements[i] = part;
			//cout << "Offset is: " << Offsets[i] << " For i:" << i << endl;
			//cout << "Elements is: " << Elements[i] << " For i:" << i << endl;
		}
	}
	else {
		cout << "Even number" << endl;
		int free = N / 2;

		if (ProcNum == 1) {
			Elements[0] = N;
			Offsets[0] = 0;
		}
		else
		{
			int sum = 0;
			for (int i = 0; i < ProcNum; i++)
			{
				Elements[i] = free;
				if (i == ProcNum - 1) {
					Elements[i] = Elements[i - 1];
					Offsets[i] = sum;
					cout << "Offset is: " << Offsets[i] << " For i:" << i << endl;
					cout << "Elements is: " << Elements[i] << " For i:" << i << endl;
					break;
				}
				free /= 2;
				Offsets[i] = sum;
				sum += Elements[i];
				cout << "Offset is: "<< Offsets[i] << " For i:"<< i << endl;
				cout << "Elements is: " << Elements[i] << " For i:" << i << endl;
			}
		}
	}
	int currArrSize = Elements[ProcRank];
	//cout << "ProcRank: " << ProcRank << " Size: " << currArrSize << endl;
	double startTime = MPI_Wtime();
	int* curArrSorted = new int[Elements[ProcRank]];
	MPI_Scatterv(data, Elements, Offsets, MPI_INT, curArrSorted, Elements[ProcRank], MPI_INT, 0, MPI_COMM_WORLD);
	
	recbitonic(curArrSorted, 0, currArrSize, ProcRank % 2 == 0 ? up : down);

	//собирание младшими процессами от старших
	int lastProc = -1;
	for (int i = 2; ProcNum / i > 0; i *= 2)
	{
		for (int j = ProcNum - 1 - ((i - 2) / 2); j >= 0; j -= i)
		{
			if ((ProcRank == j) && (ProcRank != 0)) //sender
			{
				MPI_Send(&currArrSize, 1, MPI_INT, (j - i / 2 < 0) ? 0 : j - i / 2, 0, MPI_COMM_WORLD);
				MPI_Send(curArrSorted, currArrSize, MPI_INT, (j - i / 2 < 0) ? 0 : j - i / 2, 0, MPI_COMM_WORLD);
				//cout << ProcRank << " Sended message for: " << (j - i / 2 < 0) ? 0 : j - i / 2;
			}
			if ((ProcRank == j - i / 2) || ((j - i / 2 < 0) && (ProcRank == 0) && (j != 0)))//receiver
			{
				int sizeRecv = 0;
				MPI_Recv(&sizeRecv, 1, MPI_INT, j, 0, MPI_COMM_WORLD, &status);
				curArrSorted = changeSizeArray(curArrSorted, currArrSize, currArrSize + sizeRecv);
				MPI_Recv(curArrSorted + currArrSize, sizeRecv, MPI_INT, j, 0, MPI_COMM_WORLD, &status);
				
				currArrSize += sizeRecv;

				cout << "i + j " << i/2 + j << endl;
				bitonicmerge(curArrSorted, 0, currArrSize, (j + i / 2) % 2 == 0 ? 0 : 1);

				if (ProcRank != 0 && ProcNum / (2 * i) == 0)
					lastProc = ProcRank;
			}
		}
	}
	if (ProcRank == lastProc && ProcRank != 0)
	{
		//cout << "Lat proc: " << ProcRank << endl;
		MPI_Send(&currArrSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(curArrSorted, currArrSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}
	if (ProcRank == 0)
	{
		int sizeRecv = 0;
		if (lastProc != 0 && lastProc != ProcNum)
		{
			MPI_Recv(&sizeRecv, 1, MPI_INT, lastProc, 0, MPI_COMM_WORLD, &status);
			curArrSorted = changeSizeArray(curArrSorted, currArrSize, currArrSize + sizeRecv);
			MPI_Recv(curArrSorted + currArrSize, sizeRecv, MPI_INT, lastProc, 0, MPI_COMM_WORLD, &status);
			int size = currArrSize + sizeRecv;
			bitonicmerge(curArrSorted, 0, size, 1);
		}
		double endTime = MPI_Wtime();
		printf("Time of sort: %f \n", endTime - startTime);
		if (N < 128) {
			cout << "Sorted array: " << endl;
			for (int i = 0; i < N; i++) {
				cout << curArrSorted[i] << ' ';
			}
		}
		
	}

	//startTime = clock();
	//sort();
	//bitonicmerge(0, 8, 1);
	//endTime = clock();
	//printf("Time: %f", (endTime - startTime) / CLOCKS_PER_SEC);
	/*if (N < 128) {
		printf("\n");
		for (i = 0; i < N; i++) {
			printf("%d ", data[i]);
		}
	}
	getchar();*/
	MPI_Finalize();
	return 0;
};