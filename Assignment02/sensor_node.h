/* Members:     Name: Chan Kah Hee     Email: kcha0047@student.monash.edu
 *              Name: Wong Jun Kang    Email: jwon0050@student.monash.edu
 * Team: Team-07
 * Lab:  MA_LAB-06 
 * Lecturer:    Dr Vishnu Monn Baskaran
 * Tutor:       Mr Lee Ming Jie     */
 
#ifndef SENSOR_NODE_H
#define SENSOR_NODE_H

/* header files */
#include <time.h>
#include <mpi.h>
#include <stdbool.h>

/* MPI shift */
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1

/* global constants */
#define ACCEPTANCE_RANGE 100.0f
#define SENSOR_NODE_DELAY 1
#define IPV4_LENGTH 16

/* MPI_TAGS */
#define REQUEST_SMA 1   // request simple moving average tag
#define REPLY_SMA 2     // reply simple moving average
#define PROCESSOR_INFO_REQUEST 5

// a structure representing the sensor node
typedef struct {
    int rank;
    int coords[2];
    float height;
    char ip_address[IPV4_LENGTH];
    char processor_name[MPI_MAX_PROCESSOR_NAME];
} sensor_node_t;

typedef struct {
    char ip_address[IPV4_LENGTH];
    char processor_name[MPI_MAX_PROCESSOR_NAME];
} proc_info_t;

/* function prototype */
int sensor_node_io(MPI_Comm world_comm, MPI_Comm sensor_comm, int n, int m, int thresh);
int terminates_sensor_nodes(MPI_Comm world_comm);
void* MessageListener(void* pArg);
bool is_matched(float my_height, float other_height, float range);
int get_ip_address(char* buffer);
int get_index(int mpi_source);
#endif /* SENSOR_NODE_H */
