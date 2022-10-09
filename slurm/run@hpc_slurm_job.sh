#!/bin/bash

#SBATCH --job-name=run-laid@hpc
#SBATCH --time=0:0:5
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1

# Be sure to request the correct partition to avoid the job to be held in the queue, furthermore
#	on CIRRUS-B (Minho)  choose for example HPC_4_Days
#	on CIRRUS-A (Lisbon) choose for example hpc
#SBATCH --partition=hpc

# Used to guarantee that the environment does not have any other loaded module
module purge

# Load software modules. Please check session software for the details
module load hdf5/1.12.0
##module load gcc-8.3
module load clang/7.0.0

## The operation changes the dataset file used so we need to make a copy
i_dsetfile="../datasets/bench_dataset.hd5.original"
o_dsetfile="../bench_dataset.h5"

#Prepare
exe="../bin/laid"
dsetname="dados"

## Remove the new dataset file? YES/NO
remove_dataset="YES"


# Run
echo "=== Copy dataset ==="
if [ -f "$i_dsetfile" ]; then
    cp $i_dsetfile $o_dsetfile
    
    echo "=== Running ==="
    if [ -f "$exe" ]; then
        chmod u+x $exe
        $exe -d $dsetname -f $o_dsetfile
    fi

    if [ $remove_dataset == "YES" ]; then
        echo "=== Removing dataset ==="
        rm $o_dsetfile
    fi
else
    echo "Input dataset not found!"
fi

echo "Finished with job $SLURM_JOBID"