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
    Each process is given an equal (or as close to equal as possible) amount of data which is sorted locally. The processes are then paired up in a network log_2(n) times and merged into bitonic sequences, until on the final merge they are merged into a sorted sequence.

- Sample Sort:
    Sample sort sorts datasets by following steps. First, random samples are selected and sorted to create defined buckets. Next, elements from the original dataset are assigned to these buckets based on their values, and each bucket is then sorted independently. Finally, the sorted buckets are merged to produce the fully sorted dataset.

- Merge Sort:
    First, each processor sequentially sorts its data. Then, arrays are merged 2 at a time, keeping them sorted. This continues until all data is merged into one array.

- Radix Sort:
    Radix sort sorts arrays of numbers by comparing them digit-by-digit. It begins
    with the least significant digit, and groups numbers into buckets based on 
    their current digit. This process repeats for all digits, until the number 
    with the most digits is fully sorted across all processes.

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

#### Bitonic Sort
```
main() {
  MASTER, UP = 0
  DOWN = 1

  MPI_Status status;

  n = number of elements to be sorted

  MPI_Init()

  MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    
  if numtasks is not a power of 2:
    return 1

  n_each = n / numtasks

  local_data = empty array of size n_each

  if taskid == 0:
    A = array of elements to be sorted

  MPI_Scatter(a, n_each, MPI_INT, local_data, n_each, MPI_INT, 0, MPI_COMM_WORLD) // scatter an equal amount of the data among all processes

  // Sort this processes data in UP direction
  bitonic_up(local_data, n_each)

  // Parallel sort
  log_numtasks = log_2(numtasks)
  for (s = 0; s < log_numtasks; s++) {
    for (t = s; t >= 0; t--) {
      // Determine partner
      partner_task = taskid ^ (1 << t)

      // Determine direction based on bit at s + 1 in taskid
      direction = UP if ((taskid >> (s + 1)) mod 2 == 0) else DOWN

      // Data exchange with partner
      partner_data = empty array of size n_each
      if (taskid > partner_task) {
        MPI_Send(local_data, n_each, MPI_INT, partner_task, 0, MPI_COMM_WORLD, status)
        MPI_Recv(partner_data, n_each, MPI_INT, partner_task, 0 , MPI_COMM_WORLD, status)
      } else {
        MPI_Recv(partner_data, n_each, MPI_INT, partner_task, 0 , MPI_COMM_WORLD, status)
        MPI_Send(local_data, n_each, MPI_INT, partner_task, 0, MPI_COMM_WORLD, status)
      }

      // Combine data with partner
      merged = merge(local_data, partner_data, direction)

      // Split data with partner
      if direction == UP and taskid < partner or direction == DOWN and rank > partner {
        copy(merged, merged + n_each - 1, local_data)
      } else {
        copy(merged + n_each, merged + n_each * 2 - 1, local_data)
      }
    }
  }

  // Collect data
  MPI_Gather(local_data, n_each, MPI_INT, A, n_each, MPI_INT, 0, MPI_COMM_WORLD)

  // Output
  if taskid == 0 {
    output A
  }

  MPI_Finalize()
}
```

#### Radix Sort
```
p = number of processes
array = inputted array to be sorted
n = problem size
max_digit = maximum number of digits in largest number

id = Process ID
master_process = 0
worker_processes = [1, 2, 3, ..., p - 1]

if id == master_process:
    chunk_size = n / p
    for index, process in worker_processes:
        start = index * chunk_size
        end = (index + 1) * chunk_size
        Send(array[start:end], to=process)

else if id is in worker_process:
    local_array = Receive(from=master_process)
    
    for exp in range(1, max_digit):

        buckets = ten empty arrays inside a big array
        for number in local_array:
            digit = (number // exp) % 10
            buckets[digit].append(number)
        
        for i in range(10):
            Send(buckets[i], to=all_other_processes)
            Receive(buckets[i], from=all_other_processes)
        
        local_array = concatenate(buckets)
    
    Send(local_array, to=master_process)

if id == master_process:
    for process in worker_processes:
        sorted_chunk = Receive(from=process)
        Append sorted_chunk to the final sorted array

```


### 2c. Evaluation plan - what and how will you measure and compare
- Input Sizes, Input Types, Processes

    - Input Sizes: 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28

    - Input Types: Sorted, Sorted with 1% Perturbed, Random, Reverse Sorted

    - Processes: 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024

- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
