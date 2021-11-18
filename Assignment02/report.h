/* Members:     Name: Chan Kah Hee     Email: kcha0047@student.monash.edu
 *              Name: Wong Jun Kang    Email: jwon0050@student.monash.edu
 * Team: Team-07
 * Lab:  MA_LAB-06 
 * Lecturer:    Dr Vishnu Monn Baskaran
 * Tutor:       Mr Lee Ming Jie     */
 
#ifndef REPORT_H
#define REPORT_H
#define EXIT 3
#define SENSOR_REPORT 4
#define ADJACENT_COUNT 4

/* header files */
#include <mpi.h>
#include "sensor_node.h"

// a structure containing all the information of a reporting node
typedef struct {
    long tv_sec;
    long tv_nsec;
    sensor_node_t reporting_node;
    sensor_node_t adjacent_nodes[ADJACENT_COUNT];
    int adjacent_nodes_size;
    int num_matching_adjacent_nodes;
} report_t;


/* function prototype */
MPI_Datatype init_report_type();
MPI_Datatype init_proc_info_type();

#endif /* REPORT_H */
