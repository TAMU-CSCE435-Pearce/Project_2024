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
of the sequence is swapped and then remerged into a larger portion of the sequence.
- Sample Sort:
- Merge Sort: Merge sort is a divide-and-conquer algorithm that sorts an array by splitting the array into halves until each sub-array contains a single element. Each sub-array is then put back together in a sorted order.
- Radix Sort: Radix sorting is a non-comparative sorting algorithm where numbers are placed into buckets based on singular digits going from least significant to most significant.

### 2b. Pseudocode for each parallel algorithm
### need to complete

- For MPI programs, include MPI calls you will use to coordinate between processes


- Radix Sort Pseudocode

maxNum = find max value
maxDigits = find digit count in maxNum
arraySize = find array size

MPI_INIT()
MPI_SCATTER(array)//give subparts of each array to each process
for(digit = 0; digit < maxDigits; digit++) {
    put each array subsection in buckets based on current digit
    MPI_GATHER()
}

### 2c. Evaluation plan - what and how will you measure and compare
### need to complete
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
