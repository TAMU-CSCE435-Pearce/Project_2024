# CSCE 435 Group project

## 0. Group number: 
14
## 1. Group members:
1. Gavin Bardwell
2. Tyson Long
3. Andrew Pribadi
4. Darwin White

## 2. Project topic
We will implement and analyze the effectiveness of four parallel sorting algorithms. In our project we will implement bitonic, sample, merge, and radix sorts. We will compare overall sorting times between processes, efficiency with limited number of cores, and compare and contrast various edge cases to find maximum and minimum times for all sorting algorithms. 
For primary communication we will utilize a text message group chat.    
### 2a. Brief project description (what algorithms will you be comparing and on what architectures)
### need to complete

- Bitonic Sort: Bitonic sort is a recursive sorting algorithm which sorts bitonic sequences by comparing and swapping sections of the of the array in a predefined order. A bitonic sequence is a sequence that is strictly increasing, then decreasing. At each stage of the sort a portion
of the sequence is swapped and then remerged into a larger portion of the sequence. Only arrays of size 2^n can be sorted.
- Sample Sort: Sample sort is a divide-and-conquer algorithm similar to how quicksort partitions its input into two parts at each step revolved around a singluar pivot value, but what makes sample sort unique is that it chooses a large amount of pivot values and partitions the rest of the elements on these pivots and sorts all these partitions.
- Merge Sort: Merge sort is a divide-and-conquer algorithm that sorts an array by splitting the array into halves until each sub-array contains a single element. Each sub-array is then put back together in a sorted order.
- Radix Sort: Radix sorting is a non-comparative sorting algorithm where numbers are placed into buckets based on singular digits going from least significant to most significant.

### 2b. Pseudocode for each parallel algorithm
### need to complete

- For MPI programs, include MPI calls you will use to coordinate between processes

- Bitonic Sort Pseudocode
- Inputs is your global array

//Bitonic Merge
bitonicMerge(data, low, count, direction) {
    if (count > 1)
        int k = count / 2;
        for (int i = low; i < low + k; ++i)
            if ((data[i] > data[i + k]) == direction)
                std::swap(data[i], data[i + k]);
        bitonicMerge(data, low, k, direction);
        bitonicMerge(data, low + k, k, direction);
}

//Bitonic Sort (sequential on local data)
bitonicSort(data, low, count, direction) {
    if (count > 1)
        int k = count / 2;
        bitonicSort(data, low, k, true);  // Sort in ascending order
        bitonicSort(data, low + k, k, false);  // Sort in descending order
        bitonicMerge(data, low, count, direction);
}

main() {
    //Initialize array and MPI
    MPI_Init();

    //Getting the total processes and rank
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //Initializing the array to be sorted
    if (rank == 0)
        arr = random array;
    subarray(2^n / size);
    int localSize = 2^n / size;

    //Scatter the data from root to all processes
    MPI_Scatter(arr, localSize, MPI_INT, arr, subarray, MPI_INT, 0, MPI_COMM_WORLD);

    //Sort the local data using bitonic sort
    bitonicSort(subarray, 0, localSize, true);

    //Perform parallel bitonic sorting
    for (int phase = 1; phase <= size; ++phase)
        int partner = rank ^ (1 << (phase - 1));  //Find partner using bitwise XOR

        //Exchange data with partner process
        dataReceived(localSize);
        MPI_Sendrecv(subarray, localSize, MPI_INT, partner, 0,
                     dataReceived, localSize, MPI_INT, partner, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //Merge received data with local data
        if (rank < partner)
            // Ascending order
            subarray.insert(subarray.end(), dataReceived.begin(), dataReceived.end());
            bitonicMerge(subarray, 0, subarray.size(), true);
            subarray.resize(localSize);  // Keep only first half
        else
            //Descending order
            subarray.insert(subarray.end(), dataReceived.begin(), dataReceived.end());
            bitonicMerge(subarray, 0, subarray.size(), false);
            subarray.resize(localSize);  // Keep only second half

    //Gather the sorted subarrays at root process
    MPI_Gather(subarray, localSize, MPI_INT, arr, localSize, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}

- Sample Sort Pseudocode 
- Inputs is your global array

main () {
    // Initialization 
    arr = input array MPI_Init(); 

    int num_proc, rank; 
    MPI_COMM_RANK(MPI_COMM_WORLD, &rank) 
    MPI_COMM_SIZE(MPI_COMM_WORLD, &num_proc) size = arr / num_proc 

    // Distribute data if (rank == 0) { 
        localArr = arr with 'size' amount of elements MPI_Scatter(arr, size, localArr) 
    } 

    // Local Sort on each Processor 
    sampleSort(localArr[rank])

    // Sampling 
    sample_size = num_proc - 1 samples = select_samples(localArr[rank], sample_size)

    // Gather samples on root 
    MPI_Gather(localArr, size, sortedArr) 
    if (rank == 0) { 
        sorted_samples = SampleSort(sortedArr) 
        pivots = Choose_Pivots(sorted_samples, num_proc - 1) 
    }

    // Broadcast MPI_Bcast(&pivots, size, MPI_INT, root, MPI_COMM_WORLD);

    // Redistribute data according to pivots 
    send_counts, recv_counts, send_displacements, recv_displacements = arr size num_proc 
    for(int i = 0; i < size; i++) { 
        for(int j = 0; j < num_proc; j++) { 
            if(localArr[rank][i] <= pivots[j]) 
                send_data_to_processor(j) 
        } 
    }

    // Perform All-to-All communication 
    MPI_Alltoall(send_counts, send_displacements, recv_counts, recv_displacements)

    // Local sort again after redistribution 
    sampleSort(localArr[rank])

    // Gather sorted subarrays 
    MPI_Gather(localArr[rank], size, gatherSortedArr)

    // Final merge at root processor 
    if (rank == 0) 
        sampleSort(gatherSortedArr)

    // Finalize MPI 
    MPI_Finalize();
}

- Merge Sort Pseudocode
- Inputs is your global array

main() {
    // Initialize an unsorted array (arr)
    arr = unsorted array;

    // Variables to track rank and number of processes
    int world_rank;
    int world_size;

    // Initialize MPI
    MPI_INIT();

    // Get current process rank
    MPI_COMM_RANK(MPI_COMM_WORLD, &world_rank);

    // Get total number of processes
    MPI_COMM_SIZE(MPI_COMM_WORLD, &world_size);

    // Divide array into equal-sized chunks
    int size = size_of_array / world_size;

    // Allocate space for each process's sub-array (subArr)
    subArr = allocate array of size 'size'

    // Scatter the array across processes
    MPI_Scatter(arr, size, subArr);

    // Each process performs mergeSort on its sub-array
    mergeSort(subArr);

    // Gather the sorted sub-arrays at root process (world_rank == 0)
    MPI_Gather(subArr, size, sorted_arr);

    // Root process merges the sorted sub-arrays into a final sorted array
    if(world_rank == 0) {
        mergeSort(sorted_arr);
    }

    // Finalize MPI
    MPI_Finalize();
}

mergeSort(arr) {
    // Base case: If array has only one element, it is already sorted
    if left < right {
        // Find the midpoint of the array
        mid = (left + right) / 2;

        // Recursively sort the left half of the array
        mergeSort(left half of arr);

        // Recursively sort the right half of the array
        mergeSort(right half of arr);

        // Merge the two sorted halves
        merge(arr, left, mid, right);
    }
}

merge(arr, left, mid, right) {
    // Allocate a temporary array to store the merged result
    tempArr = temporary array of size (right - left + 1)

    // Initialize pointers for the left and right halves
    left_pointer = left;
    right_pointer = mid + 1;
    temp_pointer = left;

    // While both halves have elements
    while left_pointer <= mid AND right_pointer <= right {
        if arr[left_pointer] <= arr[right_pointer] {
            // Add the smaller element from the left half to tempArr
            tempArr[temp_pointer] = arr[left_pointer];
            left_pointer++;
        } else {
            // Add the smaller element from the right half to tempArr
            tempArr[temp_pointer] = arr[right_pointer];
            right_pointer++;
        }
        temp_pointer++;
    }

    // Copy any remaining elements from the left half
    while left_pointer <= mid {
        tempArr[temp_pointer] = arr[left_pointer];
        left_pointer++;
        temp_pointer++;
    }

    // Copy any remaining elements from the right half
    while right_pointer <= right {
        tempArr[temp_pointer] = arr[right_pointer];
        right_pointer++;
        temp_pointer++;
    }

    // Copy the merged elements from tempArr back to arr
    for i = left to right {
        arr[i] = tempArr[i];
    }
}

- Radix Sort Pseudocode
- Inputs is your global array


MPI_Init()

MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);

if(rank == 0) {
    maxVal = find_max(global_array)
    maxDigits = calculate_num_digits(maxVal)
}
MPI_Bcast(&maxDigits, 1, MPI_INT, root, MPI_COMM_WORLD);


MPI_SCATTER(global_array, localArray)//split the array amongst the different processes
for(digit =0; digit < maxDigits; digit++) {
    //put each array subsection in buckets based on current digit
    local_count = count_digits(local_array, digit)

    //figure out the offet for each value
    for(i = 1; i < len(count_arr); i++)
        count_arr[i] =+ count_arr[i-1]
    
    MPI_GATHER()
    MPI_BCAST
    MPI_SCATTER()
}



### 2c. Evaluation plan - what and how will you measure and compare
We will be working through arrays of sizes 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28. We will test the sorting speed of presorted, randomly sorted, reverse sorted, and 1% perturbed. 

We will also test the all of these with increasing processors in range 2, 4, 8 ,16, 32, 64, 128, 256, 512, and 1024. At the end we will have run 280 sorts one for each array size with each array type and each processor count. This will allow us to analyze and understand the advantages and disadvantages of all sorting algorithms tested.
