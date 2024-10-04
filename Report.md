# CSCE 435 Group project

## 0. Group number: 24

## 1. Group members:
1. Jos Sebastian 
2. Jackson Stone
3. Chase Fletcher
4. Ryan Swetonic

## 2. Project topic (e.g., parallel sorting algorithms)
The topic of this project is the implementation, evaluation, and analysis of various parallel sorting algorithms and how they will behave in various situations with differing problems sizes, number of available processors, and levels of sorting completion.

## 3. Communication
The main method of communication for this group project will be through Slack.

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort:
    Sample sort sorts datasets by following steps. First, random samples are selected and sorted to create defined buckets. Next, elements from the original dataset are assigned to these buckets based on their values, and each bucket is then sorted independently. Finally, the sorted buckets are merged to produce the fully sorted dataset.

- Merge Sort:
    First, each processor sequentially sorts its data. Then, arrays are merged 2 at a time, keeping them sorted. This continues until all data is merged into one array.

- Radix Sort:

### 2b. Pseudocode for each parallel algorithm

#### Sample Sort
```
p = Number of Processes 
array = Array for Problem Type 
n = Problem Size

if master_process:

    b = n / p 
    
    for index in range(p):
        start = index * b; 
        end = (index + 1) * b 
        
        Send Array Segment array[start:end] to Worker Processes

    Recieve p * (p - 1) Splitter Candidates from all Worker Processes

    Sort Splitter Candidate
    Choose p - 1 "Good" Splitters

    Send Splitters to all Worker Processes
    Recieve Sorted Array

else if worker_process: 

    Recieve Array Segment from Master Process

    Sort Array Segment
    Choose p - 1 Splitter Candidates from the Array Segment

    Send p - 1 Splitter Candidates to Master Processes
    Recieve Splitters from Master Process

    Create Buckets based on Splitters
    Partition Array Segment into Buckets based on Splitters
    Sort Buckets

    Tournament Merge with Adacent Processes
    Send Sorted Array to Master Process
```

#### Merge Sort
'''
Main:
    Perform sequential sort on each processor

    merging_arrays = 2
    p = processor rank

    while merging_arrays <= num_procs:
        if p % merging_arrays == 0:
            receive from (p + (merging_arrays/2))
            mergeArrays
        if p % merging_arrays == merging_arrays/2:
            send to (p - (merging_arrays/2))
        merging_arrays *= 2

mergeArrays:
	while there are elements unadded in both input arrays:
		append smallest element from input arrays to output array
		advance past element added
	Append remaining elements from one array
	Return output array
'''

### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types

    Input Sizes: 2^10 to 2^20
    Input Types: Sorted, Sorted with 1% Perturbed, Random, Reverse Sorted

- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
