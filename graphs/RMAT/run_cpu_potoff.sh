#!/bin/bash

#SBATCH --job-name rmat 

#SBATCH -N 1

#SBATCH -n 8 

#SBATCH --exclusive
#SBATCH --mem=24G
#SBATCH --nodelist=ressrv4ai8111
#SBATCH --mail-type=ALL
#SBATCH --mail-user=go2432@wayne.edu

#SBATCH -o output_%j.out

#SBATCH -e errors_%j.err

#SBATCH -t 7-0:0:0
#eval "$(conda shell.bash hook)"
echo $HOSTNAME
#bash ./test.sh
bash ./test.sh
#bash ../graphs/dimacs/run_cpu.txt
