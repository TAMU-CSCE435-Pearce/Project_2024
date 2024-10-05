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

id = Process Identification

master_process = 0
worker_processes = [1, 2, 3, ..., p - 1]

if id is master_process:

    b = n / p 
    
    for index, process in worker_processes:
        start = index * b; 
        end = (index + 1) * b 
        Send(array[start:end], to=process)

    candidates = []
    for process in worker_processes:
        candidate = Receive(from=process)
        candidates.append(candidate)
    candidates = Sort(candidates)

    splitters = ChooseSplitters(candidates, count=p-1)
    for process in worker_processes:
        Send(splitters, to=worker_processes)
    
    sample_sort = Receive(from=p-1) 

else if id is in worker_process: 

    segment = Receive(from=master_process)
    segment = Sort(segment)
    
    candidates = ChooseCandidates(segment, count=p-1)
    Send(candidates, to=master_process)

    splitters = Receive(from=master_process)

    buckets = [[] * (size(splitters) + 1)]
    for element in segment:
        found = false
        for index, splitter in splitters:
            if element < splitters:
                buckets[index].append(element)
                found = true
                break
        if not found:
            buckets[-1].append(element)

    for index, bucket in buckets:
        buckets[index] = Sort(bucket)

    merged_buckets = []
    if id % 2 == 0:
        Send(buckets, to=process_id + 1) 
        received_buckets = Receive(from=process_id - 1)
        merged_buckets = Merge(buckets, received_buckets)
        Send(merged_buckets, to=process_id - 1)
    else if id % 2 == 1: 
        received_buckets = Receive(from=process_id - 1)
        merged_buckets = Merge(buckets, received_buckets)
        Send(merged_buckets, to=process_id + 1)

    if id == p - 1:
        Send(merged_buckets, to=master_process)

```

#### Merge Sort
```
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
```

### 2c. Evaluation plan - what and how will you measure and compare
- Input Sizes, Input Types, Processes

    Input Sizes: 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28
    Input Types: Sorted, Sorted with 1% Perturbed, Random, Reverse Sorted
    Processes: 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024

- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
