# CSCE 435 Group project

## 0. Group number: 18

## 1. Group members:

1. Anna Hartman
2. Tate Moskal
3. Nicole Hernandez
4. Vasudev Agarwal

Our team will communicate primarily through Text message for real-time discussions and updates. We will also use GitHub to track code contributions and Trello for project management, ensuring transparency and accountability for task assignments.

## 2. Parallel Sorting Algorithms 

### 2a. Brief project description
In this project, we will be comparing the efficiencies of different sorting algorithms running in parallel. The algorithms will be implemented and tested on parallel architectures, such as multi-core processors and possibly distributed systems. Each team member will focus on one of the following algorithms:

- Bitonic Sort: Anna Hartman
- Sample Sort: Vasudev Agarwal
- Merge Sort: Nicole Hernandez
- Radix Sort: Tate Moskal

The comparison will assess execution time, we will evaluate the efficiency on different input sizes (small, medium, large) to measure scalability, and analyzing how these algorithms leverage parallelism to improve sorting performance.

### 2b. Pseudocode for each parallel algorithm

#### 2b.1 Bitonic Sort
In Bitonic sort, a bitonic sequence is built (a sequence that first increases and then decreases) and sorted by merging. Within multiple threads, chunks of the given sequence are sorted into bitonic order. The merging process is carried out in parallel, merging each piece into a large bitonic sequence. Threads should be synchronized. The entire algorithm is carried out recursively in order to build the final bitonic sequence. 

1. Divide the given array to be sorted into parallel chunks with corresponding threads
2. For each thread ands its piece (in parallel): sort the piece into bitonic order by recursively splitting the piece into two halves and sorting the first half into ascending order and the second into descending order, and then merging the two haves into a bitonic sequence.
3. Synchronize threads by ensuring each thread has completed its sorting before continuing on.
4. Merge all of the pieces bitonically (in parallel), for each chunk and thread: compare and swap such that if the current chunk is in the lower half, merge it in ascending order, and if it is in the upper half, merge it in descending order. Recursively merge the two halves of the sequence to ensure they are fully sorted in either ascending or descending order. This should be carried out log(array_size) times, as each chunk doubles in size after being recursively merged. 
5. Output the result.

Actual Code Algorithm Description:
1. Initializes the MPI environment with MPI_Init, retrieving the rank of each process and the total number of processes using MPI_Comm_rank and MPI_Comm_size. The root process (rank 0) parses command-line arguments to determine the size of the array (as a power of 2) and the number of processes to be used.
2. Root process (rank 0) generates a random array of integers of size 2^exponent. This size is calculated from the exponent passed as an argument. The root process seeds the random number generator with the current time to ensure different data for each run.
3. Root process then divides the array into equal-sized chunks and distributes these chunks to all processes, including itself, using MPI_Scatter. Each process receives a local sub-array (of size total array size / number of processes) for sorting, ensuring that the sorting workload is spread evenly across all processes.
4. Once each process receives its portion of the array, it applies the Bitonic Sort algorithm locally.
   the Bitonic Sort works as follows:
a. Recursively divides the sub-array into smaller parts.
b. Sorts the smaller parts in ascending and descending order alternately.
c. Merges these parts using the Bitonic Merge step, which compares and swaps elements in such a way that the sub-array becomes sorted.
5. After sorting their local sub-arrays, each process sends its sorted sub-array back to the root process using MPI_Gather. The root process collects all the sorted sub-arrays into the original array.
6. Once all sub-arrays are gathered, the root process performs a final Bitonic Sort on the entire array to merge the sorted sub-arrays into a fully sorted array. This final step is necessary because the sub-arrays are only partially sorted, and a global sort ensures that the entire array is in the correct order.
7. The root process then finally checks if the final array is correctly sorted by comparing each element with the next. If any element is out of order, it reports an error; otherwise, it confirms that the array is sorted.
8. Finally, the program shuts down the MPI environment with MPI_Finalize, cleaning up all resources used by MPI.

#### 2b.2 Sample Sort

In sample sort a large dataset is divided into smaller paritions,
after which each partition is sorted independently, and then these
sorted paritions are merged to obtain the final sorted result.

1. Splitting the given dataset into equally divided smaller
   segments, where each segement is given to a processor
2. Run basic sorting algorithm on each of the segments,
   where each processor is handling its piece independently.
3. From these sorted segments, each processor selects
   few samples. then, MPI communication is used to collect samples from all processors
4. Sort the selected samples, which will help us establish a
   global order among the samples.
5. From the sorted samples, we pick few speical elements. which act
   as a pivot. which are shared with all processors. MPI_Bcast is used to broadcast the pivots to all processors.
6. Each processor takes its ordered segment and divides it into
   subsegments based on the choicen pivot.
7. Now, processors share their ordered segments globally with the
   corresponding processor based on the segment number. using MPI_Alltoall
8. Finally, each processor merges and sorts the recieved elements.

#### 2b.3 Merge Sort
Parallel Merge Sort uses the divide and conquer technique, recursively dividing the dataset into smaller parts, sorting them, and merging the results where in parallel ver. it is distributed across multiple processors. Parallel Merge Sort will be implemented using MPI. Each processor will perform its own sorting idenpendently, merging sorted data using MPI communication.

1. Start MPI for Communication between processors.
2. Identify if the process is the master (rank 0) or worker (rank > 0)
3. The master splits the dataset into smaller parts
4. The master sends each part to a worker process
5. Each worker process sorts its part of the dataset independently
6. Workers send their sorted parts back to the master
7. The master merges all sorted parts into one sorted array
8. Close MPI after sorting is complete

#### 2b.4 Radix Sort
Radix Sort is an algorithm that sorts by processing through individual digits, sorting along the way. The process can be sped up by allowing each processor to handle a portion of the total array. By sorting a subarray and keeping note of the order of subarray chunks being sorted in each processor, they can be placed accordingly back into the main array. While this example sorts via binary, the process can account for numbers of any base as long as the # of arrays corresponds correctly.

   Pseudocode:
   1. Initialize MPI for multiprocessor communication
   2. Convert array digits into binary (helps with initial implementation).
   3. Find the maximum element and its # of digits.
   5. Begin iterating through digits starting at the least significant digit up to the maximum digit significance.
   7. Split the array into subarrays depending on the # of processors used and send to workers,
      keeping track of the order in which each subarray gets sent where.
   9. Each worker will sort its subarray into 2 arrays, the first with digits that are 0, the second with digits that are 1.
   10. Worker returns arrays and master combines the 0 array in order of worker process, then repeats for the 1 array.
   11. Repeat with the next digit until all digits places have been parsed.
   12. End MPI once complete.

   Coded Algorithm (Differs from pseudocode as this version more optimally parallelizes Radix Sort):
   1. Initializes MPI environment with MPI_Init(), MPI_Comm_size(), and MPI_Comm_rank().
   2. Master process at rank 0 creates array based on command line arguments.
   3. Total array size is broadcasted to all processes with MPI_Bcast().
   4. Total array is split into subarrays based on # of processers and distributed amongst the processors with MPI_Scatter().
   5. Each process finds the maximum value of its subarray in base 2 as well as the amount of digits it has.
   6. Each process sorts its subarray by digit group in countSort(). A histogram counts the occurances of each digit group in the subarray. Using the histogram and summations, it sorts the local subarray and redistributes the info to the rest of the processes.
   7. Arrays are gathered together, exchanging information and parts of subarrays to globally sort the total array.
   8. Using MPI_Gatherv() All subarrays are collected in the master.
   9. The total array is checked if it is sorted and prints the result.
   10. Memory is deallocated and MPI_Finalize() ends the MPI environment.
   
### 2c. Evaluation plan - what and how will you measure and compare

- Input sizes, Input types (Alter input array to be: random, sorted, reverse sorted,...)
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
- Parallelization strategies (master/worker vs SPMD, calculating speedup and runtime differences)
- Communication strategies (collectives vs point-to-point, measure runtime differences between code for each communication strategy)


### 3a. Caliper instrumentation
Please use the caliper build `/scratch/group/csce435-f24/Caliper/caliper/share/cmake/caliper` 
(same as lab2 build.sh) to collect caliper files for each experiment you run.

Your Caliper annotations should result in the following calltree
(use `Thicket.tree()` to see the calltree):
```
main
|_ data_init_X      # X = runtime OR io
|_ comm
|    |_ comm_small
|    |_ comm_large
|_ comp
|    |_ comp_small
|    |_ comp_large
|_ correctness_check
```

Required region annotations:
- `main` - top-level main function.
    - `data_init_X` - the function where input data is generated or read in from file. Use *data_init_runtime* if you are generating the data during the program, and *data_init_io* if you are reading the data from a file.
    - `correctness_check` - function for checking the correctness of the algorithm output (e.g., checking if the resulting data is sorted).
    - `comm` - All communication-related functions in your algorithm should be nested under the `comm` region.
      - Inside the `comm` region, you should create regions to indicate how much data you are communicating (i.e., `comm_small` if you are sending or broadcasting a few values, `comm_large` if you are sending all of your local values).
      - Notice that auxillary functions like MPI_init are not under here.
    - `comp` - All computation functions within your algorithm should be nested under the `comp` region.
      - Inside the `comp` region, you should create regions to indicate how much data you are computing on (i.e., `comp_small` if you are sorting a few values like the splitters, `comp_large` if you are sorting values in the array).
      - Notice that auxillary functions like data_init are not under here.
    - `MPI_X` - You will also see MPI regions in the calltree if using the appropriate MPI profiling configuration (see **Builds/**). Examples shown below.

All functions will be called from `main` and most will be grouped under either `comm` or `comp` regions, representing communication and computation, respectively. You should be timing as many significant functions in your code as possible. **Do not** time print statements or other insignificant operations that may skew the performance measurements.

### **Nesting Code Regions Example** - all computation code regions should be nested in the "comp" parent code region as following:
```
CALI_MARK_BEGIN("comp");
CALI_MARK_BEGIN("comp_small");
sort_pivots(pivot_arr);
CALI_MARK_END("comp_small");
CALI_MARK_END("comp");

# Other non-computation code
...

CALI_MARK_BEGIN("comp");
CALI_MARK_BEGIN("comp_large");
sort_values(arr);
CALI_MARK_END("comp_large");
CALI_MARK_END("comp");
```

### **Algo Calltree**:

#### Merge Sort Calltree
```
0.504 main
├─ 0.000 MPI_Init
├─ 0.003 MPI_Bcast
├─ 0.000 comm
│  ├─ 0.000 MPI_Scatter
│  └─ 0.000 comm_large
│     └─ 0.000 MPI_Gather
├─ 0.000 comp
│  └─ 0.000 comp_large
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Initialized
├─ 0.000 MPI_Finalized
└─ 0.000 MPI_Comm_dup
```
#### Sample Sort Calltree
```
0.271 main
├─ 0.000 data_init_runtime
├─ 0.267 MPI_Init
│  └─ 0.000 MPI_Init
├─ 0.003 comm
│  ├─ 0.001 comm_scatter
│  │  └─ 0.000 MPI_Scatter
│  ├─ 0.000 comm_gather_samples
│  │  └─ 0.000 MPI_Gather
│  ├─ 0.001 comm_bcast_pivots
│  │  └─ 0.001 MPI_Bcast
│  ├─ 0.001 comm_all_to_all
│  │  └─ 0.001 MPI_Alltoall
│  ├─ 0.000 comm_alltoallv
│  │  └─ 0.000 MPI_Alltoallv
│  └─ 0.000 final_gather
│     └─ 0.000 MPI_Gatherv
├─ 0.000 comp
│  ├─ 0.000 local_sort
│  ├─ 0.000 pivot_sort
│  ├─ 0.000 pivot_partition
│  └─ 0.000 merge_elements
├─ 0.000 MPI_Barrier
├─ 0.000 MPI_Gather
├─ 0.000 correctness_check
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Initialized
├─ 0.000 MPI_Finalized
└─ 0.001 MPI_Comm_dup

```
#### Radix Sort Calltree
```
0.396 main
├─ 0.000 MPI_Init
├─ 0.000 data_init_runtime
├─ 0.004 comm
│  ├─ 0.003 comm_large
│  │  └─ 0.003 MPI_Bcast
│  └─ 0.001 comm_small
│     ├─ 0.000 MPI_Scatter
│     ├─ 0.000 MPI_Gather
│     └─ 0.000 MPI_Gatherv
├─ 0.003 comp
│  ├─ 0.000 comp_small
│  │  ├─ 0.000 MPI_Gather
│  │  └─ 0.000 MPI_Bcast
│  └─ 0.003 comp_large
│     ├─ 0.001 MPI_Allgather
│     └─ 0.001 MPI_Gatherv
├─ 0.000 MPI_Barrier
├─ 0.000 correctness_check
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Initialized
├─ 0.000 MPI_Finalized
└─ 0.002 MPI_Comm_dup
```

#### Bitonic Sort Calltree

```
4.972 main
├─ 0.000 MPI_Init
└─ 2.803 comp
   └─ 2.803 comp_large
├─ 2.169 comm
│  └─ 2.169 comm_large
│     ├─ 2.163 MPI_Scatter 
│     └─ 0.007 MPI_Gather
├─ 0.000 correctness_check
├─ 0.000 MPI_Init
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Finalized
├─ 0.000 MPI_Initialized
└─ 24.052 MPI_Comm_dup
```





### 3b. Collect Metadata

Have the following code in your programs to collect metadata:
```
adiak::init(NULL);
adiak::launchdate();    // launch date of the job
adiak::libraries();     // Libraries used
adiak::cmdline();       // Command line used to launch the job
adiak::clustername();   // Name of the cluster
adiak::value("algorithm", algorithm); // The name of the algorithm you are using (e.g., "merge", "bitonic")
adiak::value("programming_model", programming_model); // e.g. "mpi"
adiak::value("data_type", data_type); // The datatype of input elements (e.g., double, int, float)
adiak::value("size_of_data_type", size_of_data_type); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
adiak::value("input_size", input_size); // The number of elements in input dataset (1000)
adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
adiak::value("group_num", group_number); // The number of your group (integer, e.g., 1, 10)
adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").
```

They will show up in the `Thicket.metadata` if the caliper file is read into Thicket.

### **See the `Builds/` directory to find the correct Caliper configurations to get the performance metrics.** They will show up in the `Thicket.dataframe` when the Caliper file is read into Thicket.

#### Merge Sort Metadata
```
           cali.caliper.version  mpi.world.size  \
profile                                           
3133824840               2.11.0               2   

                                                 spot.metrics  \
profile                                                         
3133824840  min#inclusive#sum#time.duration,max#inclusive#...   

           spot.timeseries.metrics  spot.format.version  \
profile                                                   
3133824840                                            2   

                                                 spot.options  spot.channels  \
profile                                                                        
3133824840  time.variance,profile.mpi,node.order,region.co...  regionprofile   

           cali.channel spot:node.order   spot:output spot:profile.mpi  \
profile                                                                  
3133824840         spot            true  p2-a128.cali             true   

           spot:region.count spot:time.exclusive spot:time.variance algorithm  \
profile                                                                         
3133824840              true                true               true     merge   

           programming_model  group_num  input_size  num_procs data_type  \
profile                                                                    
3133824840               mpi         18         128          2    random   

            size_of_data_type scalability  
profile                                    
3133824840                  4      strong
```

#### Sample Sort Metadata
```
   cali.caliper.version  mpi.world.size  \
profile                                           
3133824840               2.11.0               2   

                                                 spot.metrics  \
profile                                                         
3133824840  min#inclusive#sum#time.duration,max#inclusive#...   

           spot.timeseries.metrics  spot.format.version  \
profile                                                   
3133824840                                            2   

                                                 spot.options  spot.channels  \
profile                                                                        
3133824840  time.variance,profile.mpi,node.order,region.co...  regionprofile   

           cali.channel spot:node.order   spot:output spot:profile.mpi  \
profile                                                                  
3133824840         spot            true  p2-a128.cali             true   

           spot:region.count spot:time.exclusive spot:time.variance  \
profile                                                               
3133824840              true                true               true   

              algorithm programming_model  group_num  input_size  num_procs  \
profile                                                                       
3133824840  Sample Sort               mpi         18         128          2   

           data_type  size_of_data_type scalability  
profile                                              
3133824840    random                  4      strong  

```

#### Radix Sort Metadata
```
           cali.caliper.version  mpi.world.size  \
profile                                           
3133824840               2.11.0               2   

                                                 spot.metrics  \
profile                                                         
3133824840  min#inclusive#sum#time.duration,max#inclusive#...   

           spot.timeseries.metrics  spot.format.version  \
profile                                                   
3133824840                                            2   

                                                 spot.options  spot.channels  \
profile                                                                        
3133824840  time.variance,profile.mpi,node.order,region.co...  regionprofile   

           cali.channel spot:node.order   spot:output spot:profile.mpi  \
profile                                                                  
3133824840         spot            true  p2-a128.cali             true   

           spot:region.count spot:time.exclusive spot:time.variance  \
profile                                                               
3133824840              true                true               true   

            launchdate                                          libraries  \
profile                                                                     
3133824840  1729137630  [/scratch/group/csce435-f24/Caliper/caliper/li...   

                                cmdline cluster algorithm programming_model  \
profile                                                                       
3133824840  [./radix_sort, --size, 128]       c     radix               mpi   

           data_type  size_of_data_type  input_size input_type  num_procs  \
profile                                                                     
3133824840       int                  4         128     Random          2   

           scalability  group_num implementation_source  
profile                                                  
3133824840      strong         18                online  
```

#### Bitonic Sort Metadata
```
cali.caliper.version  mpi.world.size  \
profile                                           
1981606483               2.11.0              16   

                                                 spot.metrics  \
profile                                                         
1981606483  min#inclusive#sum#time.duration,max#inclusive#...   

           spot.timeseries.metrics  spot.format.version  \
profile                                                   
1981606483                                            2   

                                                 spot.options  spot.channels  \
profile                                                                        
1981606483  time.variance,profile.mpi,node.order,region.co...  regionprofile   

           cali.channel spot:node.order   spot:output spot:profile.mpi  \
profile                                                                  
1981606483         spot            true  p16-a16.cali             true   

           spot:region.count spot:time.exclusive spot:time.variance  \
profile                                                               
1981606483              true                true               true   

            launchdate                                          libraries  \
...

           scalability  group_num implementation_source  
profile
1981606483      weak          1           online  
```                                        

## 4. Performance evaluation

## Bitonic Sort
### Strong Scaling
#### Main
![main_a28](https://github.com/user-attachments/assets/5b2cfc6a-05ee-4f76-8daa-994ba0b03b8c)
![main_a26](https://github.com/user-attachments/assets/80bc0b59-2139-4b13-94f1-15270b5f0ddc)
![main_a24](https://github.com/user-attachments/assets/e9f877ab-df88-4e45-870a-a5c9bde77697)
![main_a22](https://github.com/user-attachments/assets/d212112f-75b2-4cf6-a326-1416b30f85c1)
![main_a20](https://github.com/user-attachments/assets/9d8f0e29-f989-4d39-982f-2b7e9f7b64d8)
![main_a18](https://github.com/user-attachments/assets/8e23e719-a4d6-412c-89b5-a8988ea6197c)
![main_a16](https://github.com/user-attachments/assets/05c9331f-2862-4ce0-a745-2fe9726e235c)

#### Comm
![comm_a28](https://github.com/user-attachments/assets/b15df0f9-4982-4ad4-b3bc-b81f61ca397b)
![comm_a26](https://github.com/user-attachments/assets/7e16bad9-664e-4a7d-8693-2896aae34dae)
![comm_a24](https://github.com/user-attachments/assets/aa8dfe93-d7b6-4e99-adae-1e2dc81abeb3)
![comm_a22](https://github.com/user-attachments/assets/0832eaf9-d471-4033-a51f-30fc270f8172)
![comm_a20](https://github.com/user-attachments/assets/f5578ad4-5b34-4591-abca-4c111bf27ac5)
![comm_a18](https://github.com/user-attachments/assets/61f37c15-2855-41b9-974a-0f80d217615b)
![comm_a16](https://github.com/user-attachments/assets/4ba9d325-f8eb-4f2c-ab6d-88b656c8081d)

#### Comp Large
![comp_a28](https://github.com/user-attachments/assets/59b71c69-2023-404c-a556-e245dba677fa)
![comp_a26](https://github.com/user-attachments/assets/452b15b3-5061-4eee-be03-e0e48ab3f749)
![comp_a24](https://github.com/user-attachments/assets/b71230ac-3feb-41c0-8aec-0522996e53fa)
![comp_a22](https://github.com/user-attachments/assets/09fc0357-914c-49a3-90b0-c989a225e8fa)
![comp_a20](https://github.com/user-attachments/assets/1ff06fcf-b1c0-4c27-ae0b-bbe2b9eba29a)
![comp_a18](https://github.com/user-attachments/assets/d44ee59c-d259-41dc-91b3-38d197d1e27f)
![comp_a16](https://github.com/user-attachments/assets/40212fb9-e48f-44e0-8509-0248cc89b106)

Analysis:
The strong scaling plots show how the execution time decreases as the number of processors increases while keeping the problem size constant for sizes 2^16 through 2^28. Ideally, the execution time should decrease proportionally with the increase in the number of processors. With these plots, the strong scaling curve shows diminishing returns as more processors are added. This is due to overhead from inter-processor communication, synchronization, and non-parallelizable portions of the algorithm (as per Amdahl's law). It is worth noting that the time reduces much more linearly with large problem sizes, meaning that parallelization is more effective and has more returns with a larger problem size. For larger problem sizes, the actual computational work increases significantly, and the overhead becomes a smaller fraction of the total work. The efficiency of parallelization improves with larger problem sizes, because the computational gains outweigh the overhead.
Another observation is that the communication time is overall much higher for random inputs than for the other types of inputs (as seen in the Comm plots). This could be because random inputs generally result in more frequent and scattered data transfers between processors, which increases the need for synchronization. Each time processors exchange data, they must often wait for each other to complete their communication before proceeding to the next step. This synchronization could introduce delays, especially when communication patterns are irregular and unpredictable, as with random data.
For the comp_large graphs, it is clear that the times decreased linearly consistently with the increase in the number of processes. The underlying bitonic algorithm used for comp_large scales well with the number of processes. The computational workload increases enough to offset the overhead of managing multiple processes, as with comp_large, where the algorithm is designed to take full advantage of the parallel architecture, minimizing any bottlenecks that might otherwise prevent linear scaling.



### Strong Speed-Up
![sorted](https://github.com/user-attachments/assets/f48b7fbe-05ad-41fc-9d54-b2ea4d1f0374)
![reverse](https://github.com/user-attachments/assets/0d5c0a6e-b30c-47d7-9956-45525b41161f)
![random](https://github.com/user-attachments/assets/a9c39c8b-d020-4d14-b9bb-d87d1aaeb2ce)
![perturbed](https://github.com/user-attachments/assets/71c76849-9baf-4487-822f-3dd3b11910ea)

Analysis:
It is clear from the plots that the speedup decreases for all of the types of inputs and problem sizes, though it is the highest for the larger problem sizes. The consistent decrease in speedup is likely due to growing communication overhead, as more processes require more data exchanges, and synchronization delays increase. Amdahl's Law plays a role by limiting the speedup as non-parallelizable parts of the algorithm, like merging and comparing, become bottlenecks. Load imbalances between processes, increased cache contention, and memory bandwidth issues could also further reduce efficiency. Data needs to be exchanged between processes frequently to ensure the correct ordering of elements. This communication overhead can start to dominate the benefits of parallelization, especially as the number of processes grows. The more processors involved, the more data exchanges are needed, which results in diminishing returns in speedup.


### Weak Scaling
![main_sorted](https://github.com/user-attachments/assets/a1458da2-18d2-4e0e-9dbf-bbaa39137c72)
![main_reverse](https://github.com/user-attachments/assets/e6ee400e-fcd9-4f2e-bf83-6ccbee8d3ab9)
![main_random](https://github.com/user-attachments/assets/9ebce101-af6f-4393-a88a-196e1ef7f5d7)
![main_perturbed](https://github.com/user-attachments/assets/1cbed48f-2766-4a86-8b05-b2d6ed66c501)

Analysis:
It is observed that the average time per rank increases for the smaller problem sizes and decreases for the larger problem sizes. For smaller problem sizes, the communication overhead becomes more significant compared to the actual computation each process has to perform. Bitonic sort involves frequent communication between processes, so smaller inputs don't have enough computational work to offset the communication time. As a result, the average time per rank increases because the overhead dominates the computation. But for larger problem sizes, the amount of computation per process increases significantly, which helps amortize the communication overhead. The processes spend more time on computation relative to communication, leading to a decrease in the average time per rank. In addition, with small inputs, adding more processes doesn’t fully utilize the available resources, meaning that some processes may remain idle or underused. The increased number of processes introduces unnecessary communication and synchronization overhead for small workloads, resulting in increased time per rank. For larger inputs, however, the workload scales with the number of processes, making each process more efficiently utilized, which lowers the average time per rank.

### Overall Analysis
Based on the resulting plots, the performance of the bitonic sort algorithm shows a balance between parallel efficiency and the inherent challenges of communication and synchronization. The algorithm benefits from parallelization, particularly with larger problem sizes, where computation dominates and overheads like communication are amortized, leading to near-linear reductions in execution time as the number of processes increases. This is shown in the consistent linear decrease in comp_large times, indicating that the algorithm scales well for large datasets.
However, the algorithm has diminishing returns as more processes are added, especially with smaller problem sizes. This is because communication overhead and synchronization begin to dominate, as shown by the decreasing speedup with additional processes. Amdahl’s Law limits the potential speedup due to the non-parallelizable parts of the algorithm, such as merging and comparing, which require extensive communication between processes. Additionally, random inputs make communication overhead worse due to unpredictable data movement, while the other input types lead to more efficient execution due to more predictable data patterns.
In weak scaling, the average time per rank initially increases for smaller problem sizes due to communication overhead, but decreases with larger problems as processes are more efficiently utilized. This indicates that while the bitonic sort algorithm can handle large-scale parallelism well, its performance is limited by communication costs and load imbalances at smaller scales.
Overall, the parallelized bitonic sort algorithm performs well for large problem sizes with high computation-to-communication ratios but struggles with communication overhead and diminishing returns as the number of processes increases, especially for smaller inputs. 


## Merge Sort

### Weak Scaling
![weak_scaling_main_data_type_p](https://github.com/user-attachments/assets/5f8d344e-db31-491e-9fc2-ef488d11b4aa)

This plot demonstrates the weak scaling behavior of the communication phase for data type "p" as the number of processes increases. The average time per chunk generally increases with the number of processes, which is expected in weak scaling scenarios where the workload per process remains constant as the system size grows.


### Speed up
![speedup_comm_data_type_p](https://github.com/user-attachments/assets/7e0a9a67-b8a2-4009-9d32-ad28fef8ccec)

The speedup plot for the communication phase shows an initial increase in performance as the number of processes scales up to 8, indicating effective parallelization. However, beyond 16 processes, the speedup fluctuates and drops off, reflecting diminishing returns due to communication overhead.

### Strong Scaling
![strong_scaling_main_input_size_65536](https://github.com/user-attachments/assets/9f6af68b-a735-45a6-b824-d19f8814ba95)

This plot shows the main task's strong scaling behavior for an input size of \(2^{16}\) (65536), focusing on the average time per chunk as the number of processes increases. As the number of processes increases, the average time per chunk rises consistently, indicating that the overhead of managing more processes adds to the time complexity. This suggests that the main process becomes a bottleneck as more processes are introduced, likely due to the increased coordination and synchronization required.



## Sample Sort
### Strong Scaling
![comp_$2^{16}$_strong_scaling](https://github.com/user-attachments/assets/c843ddde-6da6-4d2c-8c18-edb2f1638cd3)

### Strong Speed-Up
![main_s_strong_speeduP](https://github.com/user-attachments/assets/bd9f9dc3-a82c-4e1e-9ea6-80297ddb0076)


### Weak Scaling
![comm_s_weak_scaling](https://github.com/user-attachments/assets/00f6d1c2-0142-4c75-b4d1-31337fead692)



Sample sort overall performance: As we increase the no. of processors and length of our array, we see an increase in communication time due to the overhead to sync between all the processors, but we also observe an decrease in computation time as we increased the no. of processors. As we sample sort we could efficiently dvided our array into more no. of buckets which made the sorting easier.





## Radix Sort
### Strong Scaling

![commStrong28](https://github.com/user-attachments/assets/b23715aa-37af-419e-8595-8f381e238e5f)

![compStrong28](https://github.com/user-attachments/assets/03ecc45c-686a-4eca-aebc-315e5a754198)

![mainStrong28](https://github.com/user-attachments/assets/72e2eb3c-5550-4d67-8f17-2e9a98319dcc)

### Strong Scaling Speed-Up

![SSScomm](https://github.com/user-attachments/assets/ae4abff9-5309-411e-b146-511c2d87991c)

![SSScomp](https://github.com/user-attachments/assets/c336c1ea-e1e5-4061-85d5-b84c3210cda1)

![SSSmain](https://github.com/user-attachments/assets/db9b5cef-03b6-4e52-b688-506d7dac2f34)

### Weak Scaling 

![weakcomm](https://github.com/user-attachments/assets/1ab9c34e-894c-4684-b1f9-0a9f1374f78d)

![weakcomp](https://github.com/user-attachments/assets/6723023e-482e-43c6-8aec-f43a8206715e)

![weakmain](https://github.com/user-attachments/assets/b188979d-774c-4525-8533-73d2ba404b3c)

### Analysis
Radix Sort Overall Performance: The plots showed behaviors mostly in line with behaviors observed in previous labs when working with different input sizes and processor counts. This time however, there were different input types, and they mostly had impacts on communication. As input sizes increased, communication times often took longer. As processor counts increased, computation times often took less time. There were some kinks that could be worked out to improve the Caliper data, but this is mostly representative of the true data.

Notes: Due to the networking issues that many other students were facing, I was unable to successfully produce runs for 1024 processors. I was able to produce a few, which can be seen in a few of the plots above, but most other runs would hang and fail. To save credits from being unnecessarily used, I opted to just not run these and to possibly try over the weekend when there are not as many users. There are some improvements that could be made to the code, however due to issues with the Grace scheduler, getting new data has been incredibly difficult. Issues with Jupyter and with local modules also made getting plots difficult.





Include detailed analysis of computation performance, communication performance. 
Include figures and explanation of your analysis.

### 4a. Vary the following parameters
For input_size's:
- 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28

For input_type's:
- Sorted, Random, Reverse sorted, 1%perturbed

MPI: num_procs:
- 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024

This should result in 4x7x10=280 Caliper files for your MPI experiments.

### 4b. Hints for performance analysis

To automate running a set of experiments, parameterize your program.

- input_type: "Sorted" could generate a sorted input to pass into your algorithms
- algorithm: You can have a switch statement that calls the different algorithms and sets the Adiak variables accordingly
- num_procs: How many MPI ranks you are using

When your program works with these parameters, you can write a shell script 
that will run a for loop over the parameters above (e.g., on 64 processors, 
perform runs that invoke algorithm2 for Sorted, ReverseSorted, and Random data).  

### 4c. You should measure the following performance metrics
- `Time`
    - Min time/rank
    - Max time/rank
    - Avg time/rank
    - Total time
    - Variance time/rank


## 5. Presentation
Plots for the presentation should be as follows:
- For each implementation:
    - For each of comp_large, comm, and main:
        - Strong scaling plots for each input_size with lines for input_type (7 plots - 4 lines each)
        - Strong scaling speedup plot for each input_type (4 plots)
        - Weak scaling plots for each input_type (4 plots)

Analyze these plots and choose a subset to present and explain in your presentation.

## 6. Final Report
Submit a zip named `TeamX.zip` where `X` is your team number. The zip should contain the following files:
- Algorithms: Directory of source code of your algorithms.
- Data: All `.cali` files used to generate the plots seperated by algorithm/implementation.
- Jupyter notebook: The Jupyter notebook(s) used to generate the plots for the report.
- Report.md
