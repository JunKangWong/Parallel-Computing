/* Members:     Name: Chan Kah Hee     Email: kcha0047@student.monash.edu
 *              Name: Wong Jun Kang    Email: jwon0050@student.monash.edu
 * Team: Team-07
 * Lab:  MA_LAB-06 
 * Lecturer:    Dr Vishnu Monn Baskaran
 * Tutor:       Mr Lee Ming Jie     */
 
#ifndef BASE_STATION_H
#define BASE_STATION_H

/* constants */
#define SENTINEL_VALUE 0

/* header files */
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "report.h"
#include "sensor_node.h"

// a structure that stores the data about sea water column height reading
typedef struct {
    time_t timestamp;
    int coords[2];
    float height;
} swc_height_t;

// a structure that contains the information of the created topology
typedef struct {
    int dims[2];
    int size;
} topology_t;

// a structure that contains arguments of the receive report
typedef struct {
    MPI_Comm world_comm;
    FILE *log_file;    
} receive_report_t;


/* function prototypes */
void *satellite_altimeter(void *arg);

void *receive_report(void *arg);

bool has_received_sentinel_value();

int base_station(MPI_Comm world_comm, MPI_Comm sensor_comm, int num_iters, int n, int m);

void generate_rand_coords(topology_t topology, int *buffer);

double get_comm_time(long start_tv_sec, long start_tv_nsec);

swc_height_t init_swc_height(int coords[], float height);

bool is_matching_coords(report_t report, swc_height_t swc_height);

bool is_alert_match(report_t report, swc_height_t swc_height, double max_height_range, double max_time_range);

void write_log(FILE *log_file, int iter, time_t log_time, bool alert_type, 
               report_t report, swc_height_t swc_height, double comm_time,
               const double max_height_range, const double max_time_range);

void write_summary_log(FILE *log_file, int num_true_alerts, int num_false_alerts, double total_comm_time, double time_taken);

int terminates_sensor_nodes(MPI_Comm world_comm);

#endif /* BASE_STATION_H */
