#!/bin/sh

for f in dataA.*; do
  for procs in 26 5 2; do
    echo "mpirun -np $procs dist/app $f `echo $f | sed -e 's/A/B/'` 2>/dev/null"
    mpirun -np $procs dist/app $f `echo $f | sed -e 's/A/B/'` 2>/dev/null
    for hosts in hosts-sc-*; do
      echo "mpirun -np $procs --hostfile $hosts dist/app $f `echo $f | sed -e 's/A/B/'` 2>/dev/null"
      mpirun -np $procs --hostfile $hosts dist/app $f `echo $f | sed -e 's/A/B/'` 2>/dev/null
    done
  done
done
