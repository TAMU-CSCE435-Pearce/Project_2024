# CSCE 435 Group project

## 0. Group number: 23

## 1. Group members:
1. Jeffrey Mitchell
2. Ren Mai
3. Third
4. Fourth

## 2. Project topic (e.g., parallel sorting algorithms)
This project seeks to both implement and evaluate the similarities and differences between different parallel sorting algorithms with regards to problem size, problem type, and behavior regarding both strong and weak scaling. The parallel sorting algorithms chosen for the scope of this project include the following: bitonic, sample, merge, and radix sort.

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort:
- Merge Sort:
#### Radix Sort:
Radix sort assumes that the input elements of the problem statement are k digit numbers. The algorithm sorts the elements through means of buckets; by sorting elements by their digits, Radix Sort is able to linearly sort elements as it sorts from least to most significant digit. For the sake of this sorting algorithm, we will be using the MPI architecture.



### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes
#### Bitonic Sort:

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

generate the specified problem type(sorted, sorted with 1% swap, etc...)

if worker process:
    receive information from master process
    
    create "shadow" array/histogram to temporarilly store output

    # compute and sort offset step, in short, perform k passes on the array until sorted
    from least significant digit to most significant digit n:
        iterate through offset and place array items into the shadow array (buckets) based on respective value of digit (i.e. sort based on significant digit chosen)
        continue to sort based on increasing digit place such that the newly placed digit in the bucket does not violate the previously established order.

    send offset results to master process

else if master process:
    recieve/gather the sorted offsets from worker processes
    
    combine intermediate results from worker processes by performing final radix sweep on the combined results. (shown in worker process)
```

### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)

### 2d. How the team will communicate:
- For the sake of this project, the team has decided to go forward with using Slack as main form of communication, with periodic in-person meetings for discussion and implementation of the algorithms at hand.