#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define SIZE 10

static int intComp(const void *i, const void *j)
{
  if ((*(int *)i) > (*(int *)j))
    return (1);
  if ((*(int *)i) < (*(int *)j))
    return (-1);
  return (0);
}

int main (int argc, char **argv)
{
  // Variables
  int 	     numProcessors, worldRank = 0;
  int 	     i,j,k, numElements, numElements_Bloc, numSortElements, count;
  int 	     *originalArray, *originalArrayData;
  int 	     *splitter, *totalSplitters;
  int 	     *buckets, *bucketBuffer, *localBucket;
  int 	     *sortedArrayBuffer, *sortedArray;
  CALI_CXX_MARK_FUNCTION;
  // Initialization
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcessors);
  MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
  if(argv[2] == nullptr) {
      if(worldRank == 0) 
          printf(" Error: array_type argument is missing\n");
      MPI_Finalize();
      exit(1);
  }
  std::string array_type = argv[2];
  // Data Initialization Start
  CALI_MARK_BEGIN("data_init_runtime");
  if (worldRank == 0){
    numElements = atoi(argv[1]);
    originalArray = (int *) malloc (numElements*sizeof(int));
	 if(originalArray == NULL) {
		printf("Error : Can not allocate memory \n");
    }
  }
  /* Initialize array depending on array_type  */ 
  printf ( "Input Array for Sorting \n\n ");
  if(array_type == "Random") {
		srand(time(NULL));
		if(worldRank == 0) {
			printf("Random\n");
		}
		for(int i = 0; i < numElements; i++) {
			originalArray[i] = rand() % numElements;
		}
	}
	else if(array_type == "Sorted") {
		if(worldRank == 0) {
			printf("Sorted\n");
		}
		for(int i = 0; i < numElements; i++) {
            originalArray[i] = i;
        }
	}
	else if(array_type == "ReverseSorted") {
		if(worldRank == 0) {
			printf("ReverseSorted\n");
		}
		for(int i = 0; i < numElements; ++i) {
            originalArray[i] = numElements - i;
        }
	}
	else if(array_type == "1_perc_perturbed") {
		if(worldRank == 0) {
			printf("1_perc_perturbed\n");
		}
		int start = (int)(0.01 * numElements + 0.999);
		srand(time(NULL));  // Seed for random number generation

		// Initialize the array in ascending order
		for (int i = 0; i < numElements; ++i) {
			originalArray[i] = i;
		}

		// Randomly perturb 'start' elements
		for (int i = 0; i < start; ++i) {
			int random_index = rand() % numElements;  // Select a random index in the array
			originalArray[random_index] = rand() % numElements;  // Replace element with a random value
		}
	} else {
    printf("Invalid Input \n");
  }
  printf ( "\n\n ");

  CALI_MARK_END("data_init_runtime");
  adiak::init(NULL);
	adiak::launchdate();    // launch date of the job
	adiak::libraries();     // Libraries used
	adiak::cmdline();       // Command line used to launch the job
	adiak::clustername();   // Name of the cluster
	adiak::value("algorithm", "sample"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
	adiak::value("programming_model", "mpi"); // e.g. "mpi"
	adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
	adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
	adiak::value("input_size", numElements); // The number of elements in input dataset (1000)
	adiak::value("input_type", array_type.c_str()); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
	adiak::value("num_procs", numProcessors); // The number of processors (MPI ranks)
	adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
	adiak::value("group_num", 14); // The number of your group (integer, e.g., 1, 10)
	adiak::value("implementation_source", "online"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

  // Send the data
  CALI_MARK_BEGIN("comm");
  MPI_Bcast (&numElements, 1, MPI_INT, 0, MPI_COMM_WORLD);
  if((numElements % numProcessors) != 0) {
	    if(worldRank == 0)
		    printf("Number of Elements are not divisible by numProcessors \n");
      MPI_Finalize();
	    exit(0);
  }

  numElements_Bloc = numElements / numProcessors;
  originalArrayData = (int *) malloc (numElements_Bloc * sizeof (int));

  CALI_MARK_BEGIN("comm_small");
  MPI_Scatter(originalArray, numElements_Bloc, MPI_INT, originalArrayData, numElements_Bloc, MPI_INT, 0, MPI_COMM_WORLD);
  CALI_MARK_END("comm_small");

  CALI_MARK_END("comm");
  // Local sort using built in qsort function
  CALI_MARK_BEGIN("comp");
  qsort((char *) originalArrayData, numElements_Bloc, sizeof(int), intComp);
  CALI_MARK_END("comp");
  // Choose local splitters
  splitter = (int *) malloc (sizeof (int) * (numProcessors-1));
  for (i = 0; i < (numProcessors - 1); i++) {
    splitter[i] = originalArrayData[numElements / (numProcessors * numProcessors) * (i + 1)];
  } 

  // Gather local splitters to root process
  totalSplitters = (int *) malloc (sizeof(int) * numProcessors * (numProcessors - 1));
  CALI_MARK_BEGIN("comm");
  MPI_Gather (splitter, numProcessors - 1, MPI_INT, totalSplitters, numProcessors - 1, MPI_INT, 0, MPI_COMM_WORLD);
  CALI_MARK_END("comm");
  // Choose global splitters
  if (worldRank == 0) {
    qsort((char *) totalSplitters, numProcessors * (numProcessors - 1), sizeof(int), intComp);
    for (i = 0; i < numProcessors - 1; i++)
      splitter[i] = totalSplitters[(numProcessors - 1) * (i + 1)];
  }
  
  // Broadcast global splitters
  MPI_Bcast (splitter, numProcessors - 1, MPI_INT, 0, MPI_COMM_WORLD);

  // Create numProcessors amount of buckets locally
  buckets = (int *) malloc (sizeof (int) * (numElements + numProcessors));
  
  j = 0;
  k = 1;

  for (i = 0; i<numElements_Bloc; i++) {
    if(j < (numProcessors-1)) {
      if (originalArrayData[i] < splitter[j]) 
			  buckets[((numElements_Bloc + 1) * j) + k++] = originalArrayData[i]; 
      else {
	      buckets[(numElements_Bloc + 1) * j] = k - 1;
        k = 1;
        j++;
        i--;
      }
    }
    else 
      buckets[((numElements_Bloc + 1) * j) + k++] = originalArrayData[i];
  }
  buckets[(numElements_Bloc + 1) * j] = k - 1;
      
  // Send each bucket to their respective processor

  bucketBuffer = (int *) malloc (sizeof(int) * (numElements + numProcessors));

  CALI_MARK_BEGIN("comm");
  MPI_Alltoall (buckets, numElements_Bloc + 1, MPI_INT, bucketBuffer, numElements_Bloc + 1, MPI_INT, MPI_COMM_WORLD);
  CALI_MARK_END("comm");
  // Rearrange the bucket buffer
  localBucket = (int *) malloc (sizeof(int) * 2 * numElements / numProcessors);
  count = 1;
  for (j=0; j<numProcessors; j++) {
    k = 1;
    for (i = 0; i < bucketBuffer[(numElements / numProcessors + 1) * j]; i++) 
      localBucket[count++] = bucketBuffer[(numElements/numProcessors + 1) * j + k++];
  }
  localBucket[0] = count - 1;
    
  // Sort the local buckets
  numSortElements = localBucket[0];
  CALI_MARK_BEGIN("comp");
  qsort ((char *) &localBucket[1], numSortElements, sizeof(int), intComp); 
  CALI_MARK_END("comp");
  // Gathering sorted sub blocks to root
  if(worldRank == 0) {
  		sortedArrayBuffer = (int *) malloc (sizeof(int) * 2 * numElements);
  		sortedArray = (int *) malloc (sizeof(int) * numElements);
  }
  CALI_MARK_BEGIN("comm");
  MPI_Gather (localBucket, 2 * numElements_Bloc, MPI_INT, sortedArrayBuffer, 2 * numElements_Bloc, MPI_INT, 0, MPI_COMM_WORLD);
  CALI_MARK_END("comm");
  // Rearranging the final array buffer
	if (worldRank == 0) {
		count = 0;
		for(j = 0; j < numProcessors; j++){
        k = 1;
        for(i = 0; i < sortedArrayBuffer[(2 * numElements/numProcessors) * j]; i++) 
        sortedArray[count++] = sortedArrayBuffer[(2 * numElements/numProcessors) * j + k++];
    }

    // Print the final sorted array
    printf ( "Number of Elements to be sorted : %d \n", numElements);
    printf( "Sorted output sequence is\n\n");
    for (i=0; i<numElements; i++){
      printf( "%d ", sortedArray[i]);
	}
	printf ( " \n " );

  // Check if the array is sorted
  CALI_MARK_BEGIN("correctness_check");
  bool sortedCheck = true;
  for(int x = 0; x < numElements - 1; ++x) {
    if(sortedArray[x] > sortedArray[x + 1]) {
      sortedCheck = false;
      break;
    }
  }
  if(sortedCheck) {
    printf("This array is sorted.");
  }
  else {
    printf("This array is not sorted.");
  }
  CALI_MARK_END("correctness_check");

  // Free all the pointers
  free(originalArray);
  free(sortedArrayBuffer);
  free(sortedArray);
  }

  free(originalArrayData);
  free(splitter);
  free(totalSplitters);
  free(buckets);
  free(bucketBuffer);
  free(localBucket);
  
  MPI_Finalize();
}