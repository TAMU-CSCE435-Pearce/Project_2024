#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

void merge(int *, int *, int, int, int);
void mergeSort(int *, int *, int, int);

int main(int argc, char** argv) {
	CALI_CXX_MARK_FUNCTION;
	CALI_MARK_BEGIN("data_init_runtime");
	// Create and populate the array
	int n = atoi(argv[1]);
	int *original_array = (int*)malloc(n * sizeof(int));
	
	int c;
	srand(time(NULL));
	printf("This is the unsorted array: ");
	for(c = 0; c < n; c++) {
		
		original_array[c] = rand() % n;
		printf("%d ", original_array[c]);
		
	}

	printf("\n");
	printf("\n");
	
	// Initialize MPI
	int world_rank;
	int world_size;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	adiak::init(NULL);
	adiak::launchdate();    // launch date of the job
	adiak::libraries();     // Libraries used
	adiak::cmdline();       // Command line used to launch the job
	adiak::clustername();   // Name of the cluster
	adiak::value("algorithm", "merge"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
	adiak::value("programming_model", "mpi"); // e.g. "mpi"
	adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
	adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
	adiak::value("input_size", n); // The number of elements in input dataset (1000)
	adiak::value("input_type", "Random"); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
	adiak::value("num_procs", world_size); // The number of processors (MPI ranks)
	adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
	adiak::value("group_num", 10); // The number of your group (integer, e.g., 1, 10)
	adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").
		
	// Divide the array in equal-sized chunks
	int size = n/world_size;
	
	// Send each subarray to each process
	int *sub_array = (int*)malloc(size * sizeof(int));
	MPI_Scatter(original_array, size, MPI_INT, sub_array, size, MPI_INT, 0, MPI_COMM_WORLD);
	
	// Perform the mergesort on each process
	int *tmp_array = (int*)malloc(size * sizeof(int));
	mergeSort(sub_array, tmp_array, 0, (size - 1));
	
	// Gather the sorted subarrays into one
	int *sorted = NULL;
	if(world_rank == 0) {
		
		sorted = (int*)malloc(n * sizeof(int));
		
	}
	
	MPI_Gather(sub_array, size, MPI_INT, sorted, size, MPI_INT, 0, MPI_COMM_WORLD);
	
	// Make the final mergeSort call
	if(world_rank == 0) {
		
		int *other_array = (int*)malloc(n * sizeof(int));
		mergeSort(sorted, other_array, 0, (n - 1));
		
		// Display the sorted array
		printf("This is the sorted array: ");
		for(c = 0; c < n; c++) {
			
			printf("%d ", sorted[c]);
			
			}
			
		printf("\n");
		printf("\n");

		// Check if the array is sorted
		bool sortedCheck = true;
		for(int x = 0; x < n - 1; ++x) {
			if(sorted[x] > sorted[x + 1]) {
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
			
		// Clean up root
		free(sorted);
		free(other_array);
			
	}
	
	// Clean up rest
	free(original_array);
	free(sub_array);
	free(tmp_array);
	
	// Finalize MPI
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	CALI_MARK_END("data_init_runtime");
}

// Merge Function
void merge(int *a, int *b, int l, int m, int r) {
	int h, i, j, k;
	h = l;
	i = l;
	j = m + 1;
	
	while((h <= m) && (j <= r)) {
		if(a[h] <= a[j]) {
			b[i] = a[h];
			h++;		
		}
		else {
			b[i] = a[j];
			j++;
		}	
		i++;
	}
		
	if(m < h) {
		for(k = j; k <= r; k++) {
			b[i] = a[k];
			i++;
		}	
	}
	else {
		for(k = h; k <= m; k++) {
			b[i] = a[k];
			i++;
		}	
	}
	for(k = l; k <= r; k++) {
		a[k] = b[k];
	}	
}

// Recursive Merge Function
void mergeSort(int *a, int *b, int l, int r) {
	int m;
	
	if(l < r) {
		m = (l + r)/2;
		
		mergeSort(a, b, l, m);
		mergeSort(a, b, (m + 1), r);
		merge(a, b, l, m, r);
	}	
}