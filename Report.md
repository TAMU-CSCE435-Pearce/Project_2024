# CSCE 435 Group project

## 0. Group number: 23

## 1. Group members:
1. Jeffrey Mitchell
2. Ren Mai
3. Brandon Cisneros
4. Christiana Vancura

## 2. Project topic (e.g., parallel sorting algorithms)
This project seeks to both implement and evaluate the similarities and differences between different parallel sorting algorithms with regards to problem size, problem type, and behavior regarding both strong and weak scaling. The parallel sorting algorithms chosen for the scope of this project include the following: bitonic, sample, merge, and radix sort.

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

#### Bitonic Sort:
Bitonic sort assumes an imput size that is a power of 2. This algorithm uses many different processors as a network of comparators in order to sort a sequence. This algorithm must first ensure that data is in a bitonic sequence before sorting. A sequence of numbers is bitonic if it consists of a first substring of strictly non-decreasing numbers, followed by a substring of strictly non-increasing numbers. Once a bitonic sequence is generated, the algorithm can then merge the sequence into a fully sorted list. This can be repeated at scale to sort large sequences. To implement this sorting algorithm, we will be using the MPI architecture.
#### Sample Sort:
Sample sort does not assume any input size, but will split the input into p (the number of processors) buckets. Thus it will be easier to use inputs of size powers of 2. The algorithm splits the initial input into p sections, sorts them using quicksort, and samples s elements from each. These samples are then sorted, and p-1 elements are chosen as splitters. The total input is then split into p buckets, which are sorted using bucket sort by each processor. We will be using MPI architecture to implement sample sort.
- Merge Sort:
#### Radix Sort:
Radix sort assumes that the input elements of the problem statement are k digit numbers. The algorithm sorts the elements through means of buckets; by sorting elements by their digits, Radix Sort is able to linearly sort elements as it sorts from least to most significant digit. For the sake of this sorting algorithm, we will be using the MPI architecture.



### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes
#### Bitonic Sort:
##### MPI calls used to coordinate between processes:
- `MPI_Init(...)`
- `MPI_Comm_rank(...)`
- `MPI_Comm_size(...)`
- `MPI_Send(...)`
- `MPI_Recv(...)`
- `MPI_Comm_split(...)`
- `MPI_Gather(...)`
- `MPI_Scatter(...)`
- `MPI_Finalize(...)`
- `MPI_Barrier(...)`
##### Pseudocode
```txt
if worker process:
    receive subarray

    # Make own subarray bitonic sequence
    for each 2-element pair in subarray:
        if odd count pair:
            sort ascending
        if even count pair:
            sort descending
    for each 4-element pair in subarray:
        if odd count pair:
            sort ascending
        if even count pair:
            sort descending
    etc. until subarray is bitonic

    # Multi-step bitonic merge
    for each compare-swap level needed (1 - log2(# of processes)):
        find partner process
        exchange data with partner (MPI_sendrecv)
        if "inferior" partner:
            keep smaller half of data
        else if "superior" partner:
            keep larger half of data

    Send sorted subarray to master

else if master process:
    send subarrays to worker processes (MPI_Scatter)
    receieve sorted subarrays/gather them (MPI_Gather)
    merge sorted subarrays
    return sorted list
```

#### Sample Sort:

#### Merge Sort:

#### Radix Sort:
##### MPI calls used to coordinate between processes:
- `MPI_Init(...)`
- `MPI_Comm_rank(...)`
- `MPI_Comm_size(...)`
- `MPI_Send(...)`
- `MPI_Recv(...)`
- `MPI_Comm_split(...)`
- `MPI_Gather(...)`
- `MPI_Finalize(...)`
- `MPI_Barrier(...)`
##### Pseudocode
**Note: Implementation can take many forms with regards to digits. For example, we can do base 10, base 2, and etc. For the sake of this pseudocode, we abstract this away by simply calling the extracted variable `digit`.**
```txt
# Note: we can either generate the problem using centralized master, and provide data by sending with offset, or have the processes generate based on rank independently to avoid sending overhead.

if Master:
    generate the specified problem type(sorted, sorted with 1% swap, etc...)
    send offsets to worker processes #MPI_Send(...)
    
if worker process:
    receive offset for processing #MPI_Send(...)
    #create local bucket storage
    histogramBucket[numProcs][numBuckets]

    # compute and build histogram for offset (sort local offset) by determining how many elements should go into each bucket histogramBucket[numProc][bucket]

    for each item  i in the array offset:
        #determine bucket to place a[i]
        bucket = compute_bucket(a[i])
        #increment the bucket count
        histogramBucket[numProc][bucket]++

    MPI_Barrier() # to wait for all worker processors to finish computing

# master processor determines the position for each by doing prefix sum to determine final output location for element
# this is the sequential portion
if master proc:
    base = 0
    for bucket in numBuckets:
        for numProc in numProcs:
            # add base to the histogramBucketLocation and compute partial sum to determine index of each bucket
            histogramBucket[numProc][bucket] += base

            #update base, we are computing prefix
            base = histogramBucket[numProc][bucket]

MPI_Barrier(...) # wait for this indexer algorithm to be determined 
MPI_Scatter(...) # send the indices information to the workers so they correct indices to place their offset elements 
if worker process:
    for each item i in array offset:
        bucket = compute_bucket(a[i])
        #place the item a[i] in the correct location
        # send the item to the proc that takes care of that offset or have a global output array:
        if histogramBucket[numProc][bucket] index is within the worker offset domain:
            outputResult[histogramBucket[numProc][bucket]++] =a[i]
        else:
            MPI_send(...) to the correct proc with the index
    MPI_Recv(...) items and their indices and place into correct offset location
```

### 2c. Evaluation plan - what and how will you measure and compare
#### Input sizes, Input types
With regards to input sizes and input types for the project, there are 
#### Strong scaling
The stong scaling potential of an algorithm can be analyzed by keeping the problem size fixed, while increasing the number of processors/nodes. For each algorithm, this will be measured by recording the time it takes to work through an input of a constant size when utilizing varying amounts of processors. We will increase processor count progressively, testing powers of two for their performance (2, 4, 8, 16 processors, etc.). The actual problem size will be decided based on benchmarking a small processor count in order to ensure that jobs can complete on a reasonable timescale (hours at most). The corresponding decrease (or increase) in execution time will allow us to measure the relative strong scaling of each algorithm.
#### Weak scaling (increase problem size, increase number of processors)

### 2d. How the team will communicate:
- For the sake of this project, the team has decided to go forward with using Slack as main form of communication, with periodic in-person meetings for discussion and implementation of the algorithms at hand.
