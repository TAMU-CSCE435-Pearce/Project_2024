# CSCE 435 Group project

## 0. Group number:

## 1. Group members:

1. Amine Sakrout
2. Amol Gupta
3. Quenton Hua
4. Mohsin Khan
5. Oreoluwa Ogunleye-Olawuyi

## 2. Project topic (e.g., parallel sorting algorithms)

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort: This algorithm splits up the dataset into smaller sample sizes and sorts these smaller groups using something like merge or quick sort. This sorting can be parallelized using OpenMPI to speed up this process. Once these groups are sorted they are merged together to yeild a fully sorted list.
- Merge Sort:
- Radix Sort:
- Column Sort: is a parallel sorting algorithm for sorting multi-dimensional data in a matrix. The unsorted input array is split among multiple processors. Each processor sorts its own column of data. The matrix is transposed, so each processor now sorts its row. The matrix is transposed back to its original form, and the processor sort again, giving a fully sorted matrix.

### 2b. Pseudocode for each parallel algorithm

- For MPI programs, include MPI calls you will use to coordinate between processes

- Pseudocode for column sort:
  Input: A matrix M with n x n elements (n is the total number of elements) distributed among p processors
  Output: A fully sorted matrix

  1. Distribute the matrix rows among processes using `MPI_Scatter`.
  2. Repeat these steps until matrix is fully sorted:
     - for each column in the local matrix:
       - Sort the column elements in ascending order.
     - Transpose the local matrix (rows become columns and columns become rows).
     - for each row in the local submatrix:
       - sort the row elements in ascending order.
     - Use `MPI_Alltoall` to exchange rows among processes.
     - After each exchange, each process will have a subset of rows in the correct order.
     - Transpose the local submatrix again to get the original form.
     - for each column in the local submatrix:
       - Sort the column elements in ascending order.
  3. Gather all submatrices from processes into the root process using `MPI_Gather`.
  4. If the current process is the root process:
     - Print the fully sorted matrix.
  5. Finalize the MPI environment using `MPI_Finalize`.

### 2c. Evaluation plan - what and how will you measure and compare

- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
