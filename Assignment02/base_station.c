/* Members:     Name: Chan Kah Hee     Email: kcha0047@student.monash.edu
 *              Name: Wong Jun Kang    Email: jwon0050@student.monash.edu
 * Team: Team-07
 * Lab:  MA_LAB-06 
 * Lecturer:    Dr Vishnu Monn Baskaran
 * Tutor:       Mr Lee Ming Jie     */

/* header files */
#include <mpi.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include "circular_queue.h"
#include "generate_height.h"
#include "base_station.h"
#include "report.h"

/* global variables */
queue_t *g_queue; // queue of sea water column height readings
bool g_termination_alert = false;     // a boolean indicating termination alert

/* these global variables do not need mutex as they are coded 
 * in a way such that race condition will not occur */
double total_comm_time = 0.0;   // total communication time
int num_true_alerts = 0;     // total number of true alerts
int num_false_alerts = 0;    // total number of false alerts
int iteration;  // current iteration

/* initialise mutexes */
pthread_mutex_t g_queue_mutex = PTHREAD_MUTEX_INITIALIZER;    // mutex for sea water column height readings
pthread_mutex_t g_termination_alert_mutex = PTHREAD_MUTEX_INITIALIZER;  // mutex for termination alert


/* function definitions */
int base_station(MPI_Comm world_comm, MPI_Comm sensor_comm, int num_iters, int n, int m) {
    /* Simulates the base station. It creates two threads which
     * one of them simulates the satellite altimeter and the other one receives
     * report from a reporting tsunameter sensor node, compares data populated 
     * by satellite altimeter and reporting sensor node, and write log file. 
     * 
     * args:    world_comm  - a world communicator
     *          sensor_comm - a communicator consists of all tsunameter sensor nodes
     *          num_iters   - number of iterations
     *          n           - row of grid
     *          m           - column of grid
     *          
     * return:  0   */ 
    g_queue = init_queue(SWC_HEIGHT);     // initialise the global queue
    int sensor_size;        // sensor node size
    
    double program_start;       // start time of base station program
    double program_time;        // total time of base station program
    const double cycle = 10.0;   // time interval of each iteration

    topology_t satellite_altimeter_args;    // topology to be sent to satellite altimeter
    receive_report_t receive_report_args;    // struct to store arguments to be sent to receive_report func
    
    FILE *log_file = fopen("log.txt", "w"); // open a file to write log to;
    
    
    program_start = MPI_Wtime();    // start of program time
    
    MPI_Comm_size(sensor_comm, &sensor_size);
    
    // fill up receive_report_t
    receive_report_args.world_comm = world_comm;
    receive_report_args.log_file = log_file;
    
    // fill up topology_t
    satellite_altimeter_args.dims[0] = n;
    satellite_altimeter_args.dims[1] = m;
    satellite_altimeter_args.size = sensor_size;
    
    // declare thread ids
    pthread_t altimeter_id;
    pthread_t report_id; 
    
    // initialise mutexes
    pthread_mutex_init(&g_queue_mutex, NULL);
    pthread_mutex_init(&g_termination_alert_mutex, NULL);
    
    // spawn a thread to run satellite altimeter
	if (pthread_create(&altimeter_id, 0, satellite_altimeter, &satellite_altimeter_args) != 0) {
	    perror("Satellite altimeter thread creation has error!");
	    exit(1);
	}
    
    // spawn a thread to receive report and write log
	if (pthread_create(&report_id, 0, receive_report, &receive_report_args) != 0) {
	    perror("Receive report thread creation has error!");
	    exit(1);
	}
	  
	// run for specified iterations, if sentinel value is received then break
	for (iteration = 0; iteration < num_iters; iteration++) {
        sleep(cycle);
        if (has_received_sentinel_value()) {
            break;
        }
	}
	
	// send termination alert to satellite altimeter
	pthread_mutex_lock(&g_termination_alert_mutex);
	g_termination_alert = true;
	pthread_mutex_unlock(&g_termination_alert_mutex);
	
	// send termination alert to sensor nodes
	terminates_sensor_nodes(world_comm);
	
	// join the threads
	if (pthread_join(altimeter_id, NULL) != 0) {
	    perror("Satellite altimeter thread join has error!");
	    exit(2);
	}
	
	if (pthread_join(report_id, NULL) != 0) {
	    perror("Receive report thread join has error!");
	    exit(2);
	}
	
	// clean up
	// destroy the swc heights mutex
	if (pthread_mutex_destroy(&g_queue_mutex) != 0) {
	    perror("g_swc_heights_mutex has error when destroying it!");
	    exit(3);
	}
	
	// destroy the termination alert mutex
	if (pthread_mutex_destroy(&g_termination_alert_mutex) != 0) {
	    perror("g_termination_alert_mutex has error when destroying it!");
	    exit(3);
	}
    
    // write summary log
    program_time = MPI_Wtime() - program_start; // calculate time taken for base station program
    write_summary_log(log_file, num_true_alerts, num_false_alerts, total_comm_time, program_time);
    
    // clean up
	delete_queue(g_queue);    // free the memory of global queue
    fclose(log_file);   // close log file
    return 0;
}

bool has_received_sentinel_value() {
    /* Reads the sentinel file and returns true if its the sentinel
     * value else false.
     *
     * args: 
     *
     * return: true if read value is equal to predefined 
               sentinel value else false  */
    FILE *sentinel_file = fopen("sentinel.txt", "r"); // sentinel file to be used to terminate during runtime
    int sentinel_value = -1;
    fscanf(sentinel_file, "%d", &sentinel_value);
    fclose(sentinel_file);
    return sentinel_value == SENTINEL_VALUE;
}

double get_comm_time(long start_tv_sec, long start_tv_nsec) {
    /* Gets the communication time by calculating the difference of received
     * argument time with the current time.
     * 
     * 
     * args:    start_tv_sec    -   start time in seconds 
     *          start_tv_nsec   -   start time in nanoseconds
     *
     * return:  comm_time   - communication time    */ 
    double comm_time;
    struct timespec end;
    
    clock_gettime(CLOCK_REALTIME, &end);
    
    comm_time = (end.tv_sec - ((time_t) start_tv_sec)) * 1e9;
    comm_time = (comm_time + (end.tv_nsec - start_tv_nsec)) * 1e-9;
    
    return comm_time;
}

void generate_rand_coords(topology_t topology, int *buffer) {
    /* Generates random coordinates based on the given topology
     * and assign it to the buffer.
     * 
     * 
     * args:    topology   -   topology of the tsunameter sensor nodes
     *          buffer     -   buffer to store the generated coordinates
     *
     * return:          */ 
    srand(time(NULL));
    
    int x = rand() % topology.dims[0];
    int y = rand() % topology.dims[1];
    
    buffer[0] = x;
    buffer[1] = y > topology.size ? topology.size : y;  // guarantees that generated y coordinate wont overflow
}

swc_height_t init_swc_height(int coords[], float height) {
    /* Initialises a sea water column height reading and returns to it.
     * 
     * args:    coords  -   coordinates of the reading
     *          height  -   height of the reading
     *
     * return:  swc_height  - initialised sea water column height reading   */
    swc_height_t swc_height;
    swc_height.timestamp = time(NULL);  // assign timestamp to current time
    
    // assign the coordinates 
    for (int i = 0; i < 2; i++) {
        swc_height.coords[i] = coords[i];
    }
    
    swc_height.height = height; //  assign height
    return swc_height;
}

bool is_matching_coords(report_t report, swc_height_t swc_height) {
    /* Returns true if the coordinates of reporting node and sea water column
     * height reading are the same else false
     * 
     * args:    report      -   report sent by reporting node
     *          swc_height  -   sea water column height reading
     *
     * return:  true if the coordinates of reporting node and sea water column
     *          height reading are the same else false  */
    for (int i = 0; i < 2; i++) {
        if (report.reporting_node.coords[i] != swc_height.coords[i]) {
            return false;
        }
    }
    return true;
}

bool is_alert_match(report_t report, swc_height_t swc_height, double max_height_range, double max_time_range) {
    /* Returns true if the height and timestamp of the reporting node and 
     * sea water column height reading are the same else false.
     * 
     * args:    report              -   report sent by reporting node
     *          swc_height          -   sea water column height reading
     *          max_height_range    -   max tolerance height range
     *          max_time_range      -   max tolerance time range
     *          
     * return:  true if the height and timestamp of the reporting node and 
     *          sea water column height reading are the same else false     */
    double height_diff = fabs(report.reporting_node.height - swc_height.height);
    double time_diff = fabs(difftime(((time_t) report.tv_sec), swc_height.timestamp));
    
    /* return false if the absolute difference of reporting node height and 
     * sea water column height reading is greater than max tolerance range */
    if (height_diff > max_height_range) {
        return false;
    }
    
    /* return false if the time between reporting node and sea water column
     * height reading is greater than max time range */
    if (time_diff > max_time_range) {
        return false;
    }
    
    return true;
}

void write_log(FILE *log_file, int iter, time_t log_time, bool alert_type, 
               report_t report, swc_height_t swc_height, double comm_time,
               const double max_height_range, const double max_time_range) {
    /* Write a log to the provided argument log_file based on the given arguments.
     * 
     * args:    log_fie             -   file descriptor of log file
     *          iter                -   current iteration number
     *          log_time            -   logging time
     *          alert_type          -   alert type
     *          report              -   report sent by sensor node
     *          swc_height          -   sea water column height reading
     *          comm_time           -   communication time
     *          max_height_range    -   max tolerance height range
     *          max_time_range      -   max tolerance time range
     *
     * return:    */
    const int length_line = 70;
    const char *legend_format = "%-19s%-14s%-15s%s\n";
    const char *values_format = "%-19d%-14s%-15.3f%s (%s)\n";
    const char *values_break_format = "%-19d%-14s%-15.3f%s (%s)\n\n";
    char buffer[256];
    
    // print a horizontal line
    for (int i = 0; i < length_line; i++) {
        fprintf(log_file, "-");
    }
    
    time_t report_time = (time_t) report.tv_sec;
    // print the metadata
    fprintf(log_file, "\nIteration: %d\nLogged time: %43sAlert reported time: %35sAlert type: %s\n\n", iter, ctime(&log_time), ctime(&report_time), alert_type ? "True" : "False");
    
    // print the legend text
    fprintf(log_file, legend_format, "Reporting Node", "Coord", "Height(m)", "IPv4");
    
    // print the values of reporting node
    sprintf(buffer, "(%d,%d)", report.reporting_node.coords[0], report.reporting_node.coords[1]);
    fprintf(log_file, values_break_format,  
            report.reporting_node.rank, buffer, 
            report.reporting_node.height, report.reporting_node.ip_address,
            report.reporting_node.processor_name);
    
    // print the legend text
    fprintf(log_file, legend_format, "Adjacent Nodes", "Coord", "Height(m)", "IPv4");
    
    // print the values of adjacent nodes
    for (int i = 0; i < report.adjacent_nodes_size; i++) {
        sprintf(buffer, "(%d,%d)", report.adjacent_nodes[i].coords[0], report.adjacent_nodes[i].coords[1]);
        fprintf(log_file, values_format, 
                report.adjacent_nodes[i].rank, buffer,
                report.adjacent_nodes[i].height, report.adjacent_nodes[i].ip_address,
                report.adjacent_nodes[i].processor_name);
    }
    
    // print satellite altimeter data time
    fprintf(log_file, "\nSatellite altimeter reporting time: %s", swc_height.height > 0.0f ? ctime(&(swc_height.timestamp)) : "None\n");
    
    // print satellite altimeter data height
    fprintf(log_file, "Satellite altimeter reporting height (m): %.3f\n", swc_height.height);
    
    // print satellite altimeter data coordinates
    fprintf(log_file, "Satellite altimeter reporting Coord: (%d,%d)\n", swc_height.coords[0], swc_height.coords[1]);

    // print metadata
    fprintf(log_file, "\nCommunication Time (seconds): %f\n", comm_time);
    
    fprintf(log_file, "Total messages send between reporting node and base station: %d\n", 1);
    
    fprintf(log_file, "Number of adjacent matches to reporting node: %d\n", report.num_matching_adjacent_nodes);
    
    fprintf(log_file, "Max. tolerance height range between nodes readings (m): %.3f\n", ACCEPTANCE_RANGE);
    
    fprintf(log_file, "Max. tolerance height range between satellite altimeter and reporting node readings (m): %.3f\n", max_height_range);
    
    fprintf(log_file, "Max. tolerance time range between satellite altimeter and reporting node readings (seconds): %.3f\n", max_time_range);
    
    // print a horizontal line
    for (int i = 0; i < length_line; i++) {
        fprintf(log_file, "-");
    }
    
    // print a new line
    fprintf(log_file, "\n");
}

void write_summary_log(FILE *log_file, int num_true_alerts, int num_false_alerts, double total_comm_time, double time_taken) {
    /* Write a summary log to the provided argument log_file based on the 
     * given arguments.
     * 
     * args:    log_fie             -   file descriptor of log file
     *          num_true_alerts     -   total number of true alerts
     *          num_false_alerts    -   total number of false alerts
     *          total_comm_time     -   total communication time
     *          time_taken          -   total time taken for the base station
     *
     * return:    */
    const int length_line = 70;
    
    // print a horizontal line
    for (int i = 0; i < length_line; i++) {
        fprintf(log_file, "-");
    }
    
    fprintf(log_file, "\nSummary\n\nNumber of True alerts: %d\nNumber of False alerts: %d\n",
                num_true_alerts, num_false_alerts);
    
    fprintf(log_file, "Average communication time (seconds): %lf\nTime taken for the base program (seconds): %.3lf\n",
                total_comm_time / (double) (num_true_alerts + num_false_alerts), time_taken);
                
    // print a horizontal line
    for (int i = 0; i < length_line; i++) {
        fprintf(log_file, "-");
    }
}

int terminates_sensor_nodes(MPI_Comm world_comm) {
    /* Sends a exit signal to all tsunameter sensor nodes and terminates them.    
     * 
     * args:    world_comm  -   MPI_COMM_WORLD communicator containing all MPI components.
     *
     * return:  0, indicating successful process termination.   */
    int world_size;
    MPI_Comm_size(world_comm, &world_size);
    bool exit_signal = true; // signal to be sent to top sensor node.
    printf("Broadcasting exit signal to sensor nodes\n");
    for (int i = 0; i < world_size - 1; i++) {
        // send boolean "true" to notify sensor nodes to terminate.
        MPI_Send(&exit_signal, 1, MPI_C_BOOL, i, EXIT, world_comm);
    }
    return 0;
}

void *satellite_altimeter(void *arg) {
    /* Simulates the satellite altimeter which populates the global shared queue
     * with randomly generated sea water column height readings.
     * 
     * args:    arg     -   topology of the tsunameter sensor nodes
     *
     * return:    */
    topology_t topology = *(topology_t *)arg;
    
    int lower_bound = 6000;
    float height = 0.0f;
    int coords[2];

    const double cycle = 1.0;


    while (1) {
        // run for a certain cycle
        generate_rand_coords(topology, coords);
        height = generate_rand_height(0, lower_bound);
        
        // enqueue the swc_height data into the shared global array
        pthread_mutex_lock(&g_queue_mutex);
        enqueue(g_queue, init_swc_height(coords, height));
        pthread_mutex_unlock(&g_queue_mutex);
        
        // check if termination alert has been received, and break if if its true
        pthread_mutex_lock(&g_termination_alert_mutex);
        if (g_termination_alert) {
            pthread_mutex_unlock(&g_termination_alert_mutex);
            break;
        }
        pthread_mutex_unlock(&g_termination_alert_mutex);
        
        sleep(cycle);   // sleep for a cycle
    }
    return 0;
}

void *receive_report(void *arg) {
    /* Listens to incoming report and compares the reporting node with the
     * sea water column height readings populated by the satellite altimeter and
     * calculates the necessary information to write the log file such as 
     * communication time, alert type, number of alerts for each type etc.
     * 
     * args:    args    -   a strcuture containing the log file file descriptor
                            and the world communicator
     *
     * return:    */
    receive_report_t args = *(receive_report_t *)arg;
    MPI_Comm world_comm = args.world_comm;
    FILE *log_file = args.log_file;
    
    const float max_height_range = 100.0f;  // max tolerance height range between satellite altimeter and reporting node
    const float max_time_range = 60.0f;    // max tolerance time range between satellite altimer and reporting node
    bool alert_match = false;   // flag for alert match
    bool has_matching_coords_data = false;   // flag for holding matching coord data
    double comm_time = 0.0;     // commnunication time
    int i;      // loop variable
    
    
    int default_coords[2] = {-1, -1};   // default coordinates to initialise swc height
    float default_height = 0.0f;        // default height to initialise swc height
    swc_height_t swc_height;    // buffer for data from global queue

    report_t report;    // buffer for receiving report
    MPI_Status status;
    MPI_Datatype report_type = init_report_type();  // initialise the required MPI_Datatype
    int has_report_arrived = 0;     // flag for arrived report
    
    
    while (1) {
        // Listens to incoming report
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, world_comm, &has_report_arrived, &status);
        if (has_report_arrived) {
            switch (status.MPI_TAG) {
                case SENSOR_REPORT:
                {
                    MPI_Recv(&report, 1, report_type, status.MPI_SOURCE, SENSOR_REPORT, world_comm, &status);
                    
                    // get communication time
                    comm_time = get_comm_time(report.tv_sec, report.tv_nsec);
                    break;
                }
            default:
                {
                    printf("base default");
                }
            }
            
            // get total communication time
            total_comm_time += comm_time;
            
            // compare the report with the data from g_queue
            pthread_mutex_lock(&g_queue_mutex);
            
            // reset all flags and data to default
            swc_height = init_swc_height(default_coords, default_height);   // initialise the swc_height
            alert_match = false;
            has_matching_coords_data = false;
            
            // if global shared queue has data
            if (!is_empty(g_queue)) {
                i = g_queue->rear;  // initialise the loop variable
                
                // loop reversely the global shared queue to access the latest data first
                do {
                    // if found a matching coords
                    if (is_matching_coords(report, g_queue->array.swc_heights[i])) {
                        // if found matching sea water column height reading
                        if ((alert_match = is_alert_match(report, g_queue->array.swc_heights[i], max_height_range, max_time_range))) {
                            swc_height = g_queue->array.swc_heights[i];
                            has_matching_coords_data = true;
                            break;
                        }
                        /* else if there is no holding matching coords data yet then assign
                         * the latest matching coords data to the swc_height */ 
                        else if (!has_matching_coords_data) {
                            swc_height = g_queue->array.swc_heights[i];
                            has_matching_coords_data = true;
                        }
                    }
                    
                    i = (i + QUEUE_CAPACITY - 1) % QUEUE_CAPACITY;  // get next index
                    
                } while ((g_queue->size < QUEUE_CAPACITY && i != QUEUE_CAPACITY-1) || (g_queue->size >= QUEUE_CAPACITY && i != g_queue->rear));
            }
            
            pthread_mutex_unlock(&g_queue_mutex);
            
            // track the number of true and false alerts
            if (alert_match) {
                num_true_alerts++;
            }
            else {
                num_false_alerts++;
            }
                
            // log report
            write_log(log_file, iteration, time(NULL), alert_match, report, swc_height, comm_time, max_height_range, max_time_range);
            has_matching_coords_data = false;    // reset the flag
        }
        
        // check if termination alert has been received, and break if if its true
        pthread_mutex_lock(&g_termination_alert_mutex);
        if (g_termination_alert) {
            pthread_mutex_unlock(&g_termination_alert_mutex);
            break;
        }
        pthread_mutex_unlock(&g_termination_alert_mutex);
    }
    return 0;
}
