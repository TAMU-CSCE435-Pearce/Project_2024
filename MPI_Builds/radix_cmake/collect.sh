#!/bin/bash

# Define arrays for array sizes, process sizes, and algorithms
array_sizes=(16 18 20 22 24 26 28)
process_sizes=(2 4 8 16 32 64 128 256 512 1024)
algorithms=("Sorted" "ReverseSorted" "Random" "1_perc_perturbed")

# Iterate over each combination of array size, process size, and algorithm
for array_size in "${array_sizes[@]}"; do
  for process_size in "${process_sizes[@]}"; do
    for algorithm in "${algorithms[@]}"; do
      # Construct and execute the sbatch command
      process_size_to_value=$((2**array_size))
      echo sbatch mpi.grace_job $process_size_to_value $process_size \"$algorithm\"
    done
  done
done