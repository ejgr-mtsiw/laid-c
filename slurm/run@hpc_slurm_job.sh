#!/bin/bash

##CHANGE THIS!

RUN_NAME="run-laid-bench_dataset.hd5.original@hpc"

##SBATCH --time=0:10:0

## The operation changes the dataset file used so we need to make a copy
INPUT_DATASET_FILE="../../datasets/bench_dataset.hd5.original"
OUTPUT_DATASET_FILE="../../datasets/tmp/$(date +%s)_bench_dataset.h5"
DATASET_NAME="dados"

## Remove the new dataset file? YES/NO
REMOVE_DATASET="YES"

## MAYBE CHANGE THIS!

EXE="../bin/laid"

## DON'T CHANGE THIS!

# Used to guarantee that the environment does not have any other loaded module
module purge

# Load software modules. Please check session software for the details
module load gcc11/libs/hdf5/1.14.0

# Disable warning for mismatched library versions
# Cirrus.8 has different hdf5 versions on short and hpc partitions
# even if we load the same module
# ##Headers are 1.14.0, library is 1.10.5
HDF5_DISABLE_VERSION_CHECK=1
export HDF5_DISABLE_VERSION_CHECK

#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1

# Be sure to request the correct partition to avoid the job to be held in the queue, furthermore
#	on CIRRUS-B (Minho)  choose for example HPC_4_Days
#	on CIRRUS-A (Lisbon) choose for example hpc
#SBATCH --partition=hpc

#SBATCH --job-name=$RUN_NAME

# Run
echo "=== Copy dataset ==="
echo "cp $INPUT_DATASET_FILE $OUTPUT_DATASET_FILE"

if [ -f "$INPUT_DATASET_FILE" ]; then
    cp $INPUT_DATASET_FILE $OUTPUT_DATASET_FILE
    
    echo "=== Running ==="
    if [ -f "$EXE" ]; then
        chmod u+x $EXE
        $EXE -d $DATASET_NAME -f $OUTPUT_DATASET_FILE
    else
        echo "$EXE not found!"
    fi

    if [ $REMOVE_DATASET == "YES" ]; then
        echo "=== Removing dataset ==="
        echo "rm $OUTPUT_DATASET_FILE"
        rm $OUTPUT_DATASET_FILE
    fi
else
    echo "Input dataset not found! [$INPUT_DATASET_FILE]"
fi

echo "Finished with job $SLURM_JOBID"
