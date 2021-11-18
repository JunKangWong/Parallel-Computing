/* Members:     Name: Chan Kah Hee     Email: kcha0047@student.monash.edu
 *              Name: Wong Jun Kang    Email: jwon0050@student.monash.edu
 * Team: Team-07
 * Lab:  MA_LAB-06 
 * Lecturer:    Dr Vishnu Monn Baskaran
 * Tutor:       Mr Lee Ming Jie     */
   
/* header files */
#include <mpi.h>
#include "report.h"

/* function definitions */
MPI_Datatype init_report_type() {
    /* This function creates and initialise a new report_type datatype.
     * The report_type data type is a custom MPI_Struct data type, created to send
     * report_t structure from sensor_node to the base station.
     *
     * args:
     *
     * return: report_type, the report_t structure created from MPI_create_struct. 
     */

    // create sensor_node_type mpi struct datatype.
    MPI_Datatype sensor_node_type;
    int s_lengths[5] = { 1, 2, 1, IPV4_LENGTH, MPI_MAX_PROCESSOR_NAME };
    MPI_Aint s_displacements[5];
    sensor_node_t dummy_sensor;
    MPI_Aint s_base_address;
    MPI_Get_address(&dummy_sensor, &s_base_address);
    MPI_Get_address(&dummy_sensor.rank, &s_displacements[0]);
    MPI_Get_address(&dummy_sensor.coords, &s_displacements[1]);
    MPI_Get_address(&dummy_sensor.height, &s_displacements[2]);
    MPI_Get_address(&dummy_sensor.ip_address[0], &s_displacements[3]);
    MPI_Get_address(&dummy_sensor.processor_name[0], &s_displacements[4]);
    s_displacements[0] = MPI_Aint_diff(s_displacements[0], s_base_address);
    s_displacements[1] = MPI_Aint_diff(s_displacements[1], s_base_address);
    s_displacements[2] = MPI_Aint_diff(s_displacements[2], s_base_address);
    s_displacements[3] = MPI_Aint_diff(s_displacements[3], s_base_address);
    s_displacements[4] = MPI_Aint_diff(s_displacements[4], s_base_address);

    MPI_Datatype s_types[5] = { MPI_INT, MPI_INT, MPI_FLOAT, MPI_CHAR, MPI_CHAR };
    MPI_Type_create_struct(5, s_lengths, s_displacements, s_types, &sensor_node_type);
    MPI_Type_commit(&sensor_node_type);


    // create report type mpi struct datatype
    MPI_Datatype report_type;
    int lengths[6] = { 1, 1, 1, 4, 1, 1 };
    MPI_Aint displacements[6];
    report_t dummy_report;
    MPI_Aint base_address;
    MPI_Get_address(&dummy_report, &base_address);
    MPI_Get_address(&dummy_report.tv_sec, &displacements[0]);
    MPI_Get_address(&dummy_report.tv_nsec, &displacements[1]);
    MPI_Get_address(&dummy_report.reporting_node, &displacements[2]);
    MPI_Get_address(&dummy_report.adjacent_nodes[0], &displacements[3]);
    MPI_Get_address(&dummy_report.adjacent_nodes_size, &displacements[4]);
    MPI_Get_address(&dummy_report.num_matching_adjacent_nodes, &displacements[5]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);
    displacements[2] = MPI_Aint_diff(displacements[2], base_address);
    displacements[3] = MPI_Aint_diff(displacements[3], base_address);
    displacements[4] = MPI_Aint_diff(displacements[4], base_address);
    displacements[5] = MPI_Aint_diff(displacements[5], base_address);

    MPI_Datatype types[6] = { MPI_LONG, MPI_LONG, sensor_node_type, sensor_node_type, MPI_INT, MPI_INT };
    MPI_Type_create_struct(6, lengths, displacements, types, &report_type);
    MPI_Type_commit(&report_type);

    return report_type;
}

MPI_Datatype init_proc_info_type() {
    /* This function creates and initialise a new proc_info_type datatype.
     * The proc_info_type data type is a custom MPI_Struct data type, created to send
     * processor information structure between sensor nodes.
     *
     * args: 
     *
     * return: report_type, the report_t structure created from MPI_create_struct. 
     *
     */
    MPI_Datatype proc_info_type;
    int lengths[2] = { IPV4_LENGTH, MPI_MAX_PROCESSOR_NAME };
    MPI_Aint displacements[2];
    proc_info_t dummy_proc;
    MPI_Aint base_address;

    MPI_Get_address(&dummy_proc, &base_address);
    MPI_Get_address(&dummy_proc.ip_address[0], &displacements[0]);
    MPI_Get_address(&dummy_proc.processor_name[0], &displacements[1]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);

    MPI_Datatype types[2] = { MPI_CHAR, MPI_CHAR };
    MPI_Type_create_struct(2, lengths, displacements, types, &proc_info_type);
    MPI_Type_commit(&proc_info_type);

    return proc_info_type;
}

