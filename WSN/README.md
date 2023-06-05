# Assignment 2

## TSUNAMI DETECTION IN A DISTRIBUTED WIRELESS SENSOR NETWORK (WSN)

This assignment is about the simulation of tsunami detection in a distributed wireless sensor network.

## Run program

`make`<br>
`mpirun -np \<number of processes\> -oversubscribe ./WSN \<row of grid\> \<column of grid\> \<sea water column height threshold\> \<number of iterations\>`<br>

### OR (run default)

`make`<br>
`make run`

## Run program in Monarch

`sbatch main.job`

## Terminate during runtime

To terminate during runtime, insert a ***0*** which is the sentinel value into the **sentinel.txt**.

## Clean

`make clean`


