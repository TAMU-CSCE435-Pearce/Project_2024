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

- Bitonic Sort: 
- Sample Sort:
- Merge Sort: Merge sort is a divide-and-conquer algorithm that sorts an array by splitting the array into halves until each sub-array contains a single element. Each sub-array is then put back together in a sorted order.
- Radix Sort: Radix sorting is a non-comparative sorting algorithm where numbers are placed into buckets based on singular digits going from least significant to most significant.

### 2b. Pseudocode for each parallel algorithm
### need to complete

- For MPI programs, include MPI calls you will use to coordinate between processes


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
