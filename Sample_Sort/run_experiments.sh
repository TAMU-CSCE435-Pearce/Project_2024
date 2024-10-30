#!/bin/bash
## ENVIRONMENT SETTINGS; CHANGE WITH CAUTION
#SBATCH --export=NONE            # Do not propagate environment
#SBATCH --get-user-env=L         # Replicate login environment
#
## NECESSARY JOB SPECIFICATIONS
#SBATCH --job-name=JobName       # Set the job name to "JobName"
#SBATCH --time=00:30:00          # Set the wall clock limit
#SBATCH --nodes=2                # Request nodes
#SBATCH --ntasks-per-node=32     # Request tasks/cores per node
#SBATCH --mem=32G                # Request GB per node 
#SBATCH --output=output.%j       # Send stdout/err to "output.[jobID]" 
#
## OPTIONAL JOB SPECIFICATIONS
##SBATCH --mail-type=ALL         # Send email on all job events
##SBATCH --mail-user=email_address # Send all emails to email_address 
#
## First Executable Line

# Define parameters
input_sizes=(65536 262144 1048576 4194304 16777216 67108864 268435456) # 2^16 to 2^28
num_procs=(2 4 8 16 32 64 128 256 512 1024)
input_types=('r' 's' 'v' 'p')  # Input types: 'r' for random, 's' for sorted, 'v' for reverse sorted, 'p' for 1% perturbed
output_dir="cali_outputs"      # Directory for .cali files

# Create output directory if it doesn't exist
mkdir -p "$output_dir"

# Load necessary modules
module load intel/2020b       # Load Intel software stack
module load CMake/3.12.1
module load GCCcore/8.3.0
module load PAPI/6.0.0

# Loop through each input size, number of processes, and input type
for array_size in "${input_sizes[@]}"; do
    for processes in "${num_procs[@]}"; do
        for input_type in "${input_types[@]}"; do
            CALI_CONFIG="spot(output=${output_dir}/p${processes}-a${array_size}-t${input_type}.cali, time.variance,profile.mpi)" \
            mpirun -np $processes ./sample_sort $array_size $input_type
        done
    done
done

## HOW-TO-RUN
# 1. Run ". build.sh"
# 2. Run "make"
# 3. Run "sbatch run_experiments.sh"
