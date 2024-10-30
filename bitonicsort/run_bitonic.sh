#!/bin/bash

# Define input types, sizes, and process counts
input_types=("sorted" "random" "reverse_sorted" "perturbed")
sizes=(16 18 20 22 24 26 28)
processes=(2 4 8 16 32 64 128 256 512 1024)

# Loop over input types, sizes, and processes
for input_type in "${input_types[@]}"; do
    for size in "${sizes[@]}"; do
        for num_procs in "${processes[@]}"; do
            # Run the program with the current parameters
            sbatch mpi.grace_job $size $num_procs $input_type
        done
    done
done
