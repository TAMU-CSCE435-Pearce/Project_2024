#!/bin/bash
##ENVIRONMENT SETTINGS; CHANGE WITH CAUTION
#SBATCH --export=NONE            #Do not propagate environment
#SBATCH --get-user-env=L         #Replicate login environment
#
##NECESSARY JOB SPECIFICATIONS
#SBATCH --job-name=JobName       #Set the job name to "JobName"
#SBATCH --time=03:30:00           #Set the wall clock limit
#SBATCH --nodes=2                #Request nodes
#SBATCH --ntasks-per-node=32    # Request tasks/cores per node
#SBATCH --mem=128G                 #Request GB per node 
#SBATCH --output=output.%j       #Send stdout/err to "output.[jobID]" 
#
##OPTIONAL JOB SPECIFICATIONS
##SBATCH --mail-type=ALL              #Send email on all job events
##SBATCH --mail-user=email_address    #Send all emails to email_address 
#
##First Executable Line

module load intel/2020b
module load CMake/3.12.1
module load GCCcore/8.3.0
module load PAPI/6.0.0

# Accept input_size as a parameter when the script is called
input_size=$1
num_procs=(2 4 8 16 32 64 128 256 512 1024)  # MPI process counts
input_types=('r' 's' 'v' 'p')  # Input types: random, sorted, reverse sorted, 1% perturbed
output_dir="cali_outputs"      # Directory for .cali files

mkdir -p "$output_dir"

for processes in "${num_procs[@]}"; do
    for input_type in "${input_types[@]}"; do
        echo "Running with input_size=$input_size, processes=$processes, input_type=$input_type"
        CALI_CONFIG="spot(output=${output_dir}/p${processes}-a${input_size}-t${input_type}.cali, time.variance,profile.mpi)" \
        mpirun -np $processes ./merge_sort $input_size $input_type
    done
done
