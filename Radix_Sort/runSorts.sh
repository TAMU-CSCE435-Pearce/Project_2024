processors=(2 4 8 16 32 64 128 256 512 1024)
array_sizes=()
for i in {16..28..2}; do
  array_sizes+=($((2**i)))
done
array_type=(0,1,2,3) #unsorted, sorted, reverse sorted, 1% pertubed
# Loop over each processor count
for p in "${processors[@]}"
do
  tasks=1
  if[p > 48] then
    tasks=48
  else
    tasks=p
  fi
  nodeNum=$(( (p + 24) / 48 ))
  # Loop over each matrix size
  for n in "${array_sizes[@]}"
  do
    # Create a unique job name
    jobname="Sort_n${n}_p${p}"
    outputFile="outputs/${jobname}"
    # Submit the job with the unique job name and arguments
    sbatch --job-name="$jobname"  --nodes="$nodeNum" \
    --time=6:00:00 --output="$outputFile" \
    --ntasks-per-node=tasks \
    --mail-user=gmbwell@tamu.edu  \
     mpi.grace_job "$n" "$p" 0
  done
done
