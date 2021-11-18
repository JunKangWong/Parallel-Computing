/* Members:     Name: Chan Kah Hee     Email: kcha0047@student.monash.edu
 *              Name: Wong Jun Kang    Email: jwon0050@student.monash.edu
 * Team: Team-07
 * Lab:  MA_LAB-06 
 * Lecturer:    Dr Vishnu Monn Baskaran
 * Tutor:       Mr Lee Ming Jie     */
 
/* header files */
#include <mpi.h>
#include <stdlib.h>
#include "base_station.h"
#include "sensor_node.h"

int main(int argc, char **argv) {
    /* Initialises a MPI environment, split the MPI processes into a node
     * representing the base station and remaining nodes representing the
     * tsunameter sensor nodes. Get the command line arguments and run the 
     * corresponding functions for the nodes *
     *
     * args:    argc    -   argument count
     *          argv    -   arguments   
     * 
     * return:  0   */
    int rank, size, provided;
    int n, m, threshold;
    int num_iters;
    MPI_Comm sensor_comm;   // declare a new sensor node communicator 

    // initialise MPI environment
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Splits the last process into a seperate process as the base station.
    MPI_Comm_split(MPI_COMM_WORLD, rank == size - 1, 0, &sensor_comm);

    // get command line arguments.
    n = atoi(argv[1]);
    m = atoi(argv[2]);
    threshold = atoi(argv[3]);
    num_iters = atoi(argv[4]);
    
    if (rank == size - 1) {
        base_station(MPI_COMM_WORLD, sensor_comm, num_iters, n, m);
    }
    else if (rank < n * m) {
        sensor_node_io(MPI_COMM_WORLD, sensor_comm, n, m, threshold);
    }
    MPI_Finalize();
    return 0;
}
