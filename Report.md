# CSCE 435 Group project

## 0. Group number:

## 1. Group members:
1. First
2. Second
3. Third
4. Fourth

## 2. Project topic (e.g., parallel sorting algorithms)

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort:
- Merge Sort:
- Radix Sort:

### 2b. Pseudocode for each parallel algorithm

Bitonic sort:
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

- Sample Sort:
- Merge Sort:
- Radix Sort:

### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
