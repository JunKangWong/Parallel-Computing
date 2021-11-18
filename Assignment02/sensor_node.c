/* Members:     Name: Chan Kah Hee     Email: kcha0047@student.monash.edu
 *              Name: Wong Jun Kang    Email: jwon0050@student.monash.edu
 * Team: Team-07
 * Lab:  MA_LAB-06 
 * Lecturer:    Dr Vishnu Monn Baskaran
 * Tutor:       Mr Lee Ming Jie     */

/* header files */
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpi.h>
#include <unistd.h>
#include "generate_height.h"
#include "sensor_node.h"
#include "circular_queue.h"
#include "report.h"

/* initialise mutex */
pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;

/* global variables */
/* these variables are public as they are accessed by both 
 * the main process and its thread. */
enum DIRECTIONS {L, R, T, B};
float simple_moving_avg = 0.0;
float recv_heights[ADJACENT_COUNT] = {-1.0, -1.0, -1.0, -1.0};
int adjacent_rank[ADJACENT_COUNT] = {-2.0, -2.0, -2.0, -2.0};
bool recv_exit = false;
char recv_ip_address[ADJACENT_COUNT][IPV4_LENGTH];
char recv_processor_name[ADJACENT_COUNT][MPI_MAX_PROCESSOR_NAME];

int sensor_node_io(MPI_Comm world_comm, MPI_Comm sensor_comm, int n, int m, int thresh) {
    /* This function simulates the tsunameter sensor nodes.
     * This function represents the sensor node and is reponsible to send out
     * request signal to adjacent node when the simple_moving_average the node exceeds
     * a predefined height (e.g. 6000m). Also, 2 or more adjacent nodes have a similar
     * simple moving average, the sensor node will send a report to the base station.
     *

     * args:  world_comm   - MPI_COMM_WORLD, which is the main communicator containing
     *                        both the base station and the sensor node.
     *        sensor_comm  - communicator for sensor nodes cluster.
     *        n            - row count of the cartesian grid.
     *        m            - column count of the cartesian grid.
     *        thresh       - the threshold for the sensor node to decide whether to send a 
     *                       request signal to its adjacent node, for example if the thresh is 6000,
     *                       the node will send a signal to its adjacent node to request for their simple
     *                       moving average value for comparisons.
     *
     * return: 0 
     */
     
    // MPI relaied variables.
    int ndims = 2, world_size, world_rank, reorder, sensor_rank, sensor_size, ierr;
    int dims[ndims];
    int wrap_around[ndims];
    MPI_Comm comm2D;
    MPI_Request req_sma_s[ADJACENT_COUNT];

    // other variables,
    struct timespec report_send_t;
    sensor_node_t my_node;
    sensor_node_t adj_node;
    report_t my_report;
    proc_info_t my_proc_info;
    int i, j, k, l;
    int valid_count = 0;
    int signal = 1;
    int matched_count, count;
    int processor_length;
    const float lower_bound = 5600.0f; // lower bound of the generate rand height
    
    // initialise struct datatype and queue.
    MPI_Datatype report_type = init_report_type();
    MPI_Datatype proc_info_type = init_proc_info_type();
    queue_t* heights = init_queue(SENSOR_NODE);
   
    // initialise mutexes
    pthread_mutex_init(&g_Mutex, NULL);
    
    // get rank and sizes of both comminicators. world cluster and sensor node cluster.
    MPI_Comm_size(world_comm, &world_size);
    MPI_Comm_rank(world_comm, &world_rank);
    MPI_Comm_size(sensor_comm, &sensor_size);
    MPI_Comm_rank(sensor_comm, &sensor_rank);
    dims[0] = n;
    dims[1] = m;

    //create cartesian topology
    MPI_Dims_create(sensor_size, ndims, dims);
    // disabled periodic shift for both rows and columns
    wrap_around[0] = wrap_around[1] = 0;
    reorder = 1;
    ierr = 0;

    // create a 2D cartesian grid with sensor communicator called comm2D.
    // if cartesian grid is not created sucessfully, display error message.
    ierr = MPI_Cart_create(sensor_comm, ndims, dims, wrap_around, reorder, &comm2D);
    if (ierr != 0) printf("ERROR[%d] creating CART\n", ierr);

    // get MPI_Coordinates and rank from comm2D.
    MPI_Cart_coords(comm2D, sensor_rank, ndims, my_node.coords);
    MPI_Cart_rank(comm2D, my_node.coords, &my_node.rank);

    MPI_Cart_shift(comm2D, SHIFT_COL, DISP, &adjacent_rank[L], &adjacent_rank[R]);
    MPI_Cart_shift(comm2D, SHIFT_ROW, DISP, &adjacent_rank[T], &adjacent_rank[B]);

    // calculates number of valid adjacent node
    for (j = 0; j < ADJACENT_COUNT; j++) {
        if (adjacent_rank[j] != MPI_PROC_NULL) {
            valid_count++;
        }
    }

    // create MessageListener thread to listen to incoming messages.
    MPI_Comm* communicators = (MPI_Comm*)malloc(2 * sizeof(MPI_Comm));
    communicators[0] = world_comm;
    communicators[1] = comm2D;
    pthread_t tid;
    pthread_create(&tid, 0, MessageListener, (void*)communicators);

    // obtain ip addresses of adjacent nodes here so only accept once.
    get_ip_address(my_node.ip_address);
    MPI_Get_processor_name(my_node.processor_name, &processor_length);
    
    // load information into a struct buffer to be sent in
    // 1 mpi send request instead of 2 separate communications
    strcpy(my_proc_info.processor_name, my_node.processor_name);
    strcpy(my_proc_info.ip_address, my_node.ip_address);
    
    for (l = 0; l < ADJACENT_COUNT; l++) {
        MPI_Send(&my_proc_info, 1, proc_info_type, adjacent_rank[l], PROCESSOR_INFO_REQUEST, comm2D);
    }

    // temporarily controlled by time, need to change to exit signal mode later.
    while (!recv_exit) {
        // generate random height
        // enqueue new generated sea water height at every iterations.
        enqueue(heights, generate_rand_height(my_node.rank, lower_bound));
        simple_moving_avg = calc_simple_moving_avg(heights);
        my_node.height = simple_moving_avg;

        // if simple_moving_average of current sensor is greater than threshold, send out request signal.
        if (simple_moving_avg > thresh) {
            for (i = 0; i < ADJACENT_COUNT; i++) {
                MPI_Isend(&signal, 1, MPI_INT, adjacent_rank[i], REQUEST_SMA, comm2D, &req_sma_s[i]); // send a signal;
            }
            // add some delay to make sure that the thread has successully receive all income messages.    
            sleep(0.1); // this is optional

            matched_count = 0;
            count = 0;
            
           // protects the value with mutex to ensure that the value will not get modified while being compared.
           // or loaded into var.
            pthread_mutex_lock(&g_Mutex);
            
            // determine the number of "similar" simple moving average sea height level and store
            // readings that are similar into my_report
            for (k = 0; k < ADJACENT_COUNT; k++) {
                
                if (adjacent_rank[k] != MPI_PROC_NULL) {
                    // compares current node's simple moving average with adjacent_nodes and check 
                    if (is_matched(simple_moving_avg, recv_heights[k], ACCEPTANCE_RANGE)) {
                        matched_count++;
                    }
                    adj_node.rank = adjacent_rank[k];                                   // get adjacent rank
                    MPI_Cart_coords(comm2D, adj_node.rank, ndims, adj_node.coords);     // get adjacent node coord.
                    adj_node.height = recv_heights[k];                                  // get adjacent height
                    strcpy(adj_node.ip_address, recv_ip_address[k]);
                    strcpy(adj_node.processor_name, recv_processor_name[k]);
                    my_report.adjacent_nodes[count] = adj_node;                         // store into adjacent_node array of my_report.

                    count++;
                }  
            }
            pthread_mutex_unlock(&g_Mutex);
            
            // send my_report to base station at MPI_COMM_WORLD if there are 2 or more matches.
            if (matched_count >= 2) {
                // prepares report and send to base station
                clock_gettime(CLOCK_REALTIME, &report_send_t);
                my_report.tv_sec = (long)report_send_t.tv_sec;
                my_report.tv_nsec = report_send_t.tv_nsec;
                my_report.reporting_node = my_node;
                my_report.adjacent_nodes_size = valid_count;
                my_report.num_matching_adjacent_nodes = matched_count;

                // send my_report to base station via world communicator.
                MPI_Send(&my_report, 1, report_type, world_size - 1, SENSOR_REPORT, world_comm);
            }
            
        }
        // add some delay
        sleep(SENSOR_NODE_DELAY);
    }
    pthread_join(tid, NULL);
    pthread_mutex_destroy(&g_Mutex);
    delete_queue(heights);
    return 0;
}

int get_ip_address(char* buffer) {
    /* Referenced from
     * https://stackoverflow.com/questions/2283494/get-ip-address-of-an-interface-on-linux 
     *
     * Given a buffer this function will get the ip address of current device and 
     * store the ip address in string into the buffer.
     *
     * args:    buffer - which the ip address is to be stored. 
     *
     * return: 0
     */
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* make IP address attached to "eth0" */
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    strcpy(buffer, inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));

    return 0;
}

int get_index(int mpi_source) {
    /*
     * Given mpi source rank this function will generate an index, the index that
     * that represents the left direction will be denoted by position 0, index that
     * represents, right will be given index 1, top index 2 and bottom index 3.
     *
     * args:    int mpi_source  - the source rank for which the index is to be generated.
     *
     * return: index corressponding to the mpi_source.
     *
    */
    int index;

    if (mpi_source == adjacent_rank[L]) {
        index = 0;
    }
    else if (mpi_source == adjacent_rank[R]) {
        index = 1;
    }
    else if (mpi_source == adjacent_rank[T]) {
        index = 2;
    }
    else if (mpi_source == adjacent_rank[B]) {
        index = 3;
    }
    else {
        printf("ERROR: Invalid MPI_SOURCE for REPLY_SMA");
        return -1;
    }
    return index;
}
 
void* MessageListener(void* pArg) {
    /* This function acts as the thread for the sensor node.
     * This function will be pthread_create by the main process, and will be 
     * constantly listening for the arrival of new message until a exit 
     * message is received.
     *
     * args:    pArg    - void pointer containing information to be passed into the function.
     *
     * return: 0
     */
    MPI_Comm* communicators = (MPI_Comm*)pArg;
    MPI_Comm world_comm = communicators[0];
    MPI_Comm comm2D = communicators[1];
    MPI_Status status;
    MPI_Request request;
    MPI_Datatype proc_info_type = init_proc_info_type();
    proc_info_t my_proc_info;
    int recv_signal = 0, recv_msg = 0, recv_exit_sig;
    int index, world_size;
    MPI_Comm_size(world_comm, &world_size);

    while (true) {
        // apply lock to prevent value being sent is modified while sending.
        pthread_mutex_lock(&g_Mutex);

        // probe to check if there is arrival of exit signal.
        MPI_Iprobe(world_size - 1, EXIT, world_comm, &recv_exit_sig, &status);
        if (recv_exit_sig) {
            // if received exit signal, set recv_exit gloal attibute to true (by accepting true) and breaks from the while loop.
            printf("TERMINATING: Received exit signal.\n");
            MPI_Recv(&recv_exit, 1, MPI_INT, world_size - 1, EXIT, MPI_COMM_WORLD, &status);
            pthread_mutex_unlock(&g_Mutex);
            break;
        }

        // probe to see if there is any other incoming messages.
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm2D, &recv_msg, &status);
        // if a message arrived, perform action based on tag.
        if (recv_msg) {
            switch (status.MPI_TAG) {
            case REQUEST_SMA: {
                MPI_Irecv(&recv_signal, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST_SMA, comm2D, &request);
                MPI_Isend(&simple_moving_avg, 1, MPI_FLOAT, status.MPI_SOURCE, REPLY_SMA, comm2D, &request);
                break;

            }case REPLY_SMA: {
                // based on the rank determine which direction it is from then allocate a index for them
                index = get_index(status.MPI_SOURCE);

                // store / update newly arrived height into the recv_height array for the main thread to access.                     
                MPI_Irecv(&recv_heights[index], 1, MPI_FLOAT, status.MPI_SOURCE, REPLY_SMA, comm2D, &request);
                break;

            }case PROCESSOR_INFO_REQUEST: {
                index = get_index(status.MPI_SOURCE);
                MPI_Recv(&my_proc_info, 1, proc_info_type, status.MPI_SOURCE, PROCESSOR_INFO_REQUEST, comm2D, &status);
                
                // store / update newly arrived ip address and processor name into the recv_height array for the main thread to access.                     
                strcpy(recv_ip_address[index], my_proc_info.ip_address);
                strcpy(recv_processor_name[index], my_proc_info.processor_name);
                break;
            }default: {
                printf("ERROR: Unknown MPI_TAG is encountered.\n"); // to be removed later
                fflush(stdout);
            }
            }
        }
        pthread_mutex_unlock(&g_Mutex);
    }
    return 0;
}

bool is_matched(float my_height, float other_height, float range) {
    /* This function checks if my_height is "similar" to other height.
     * User can define the acceptance range.
     * Acceptance range is a margin that the value will be considered similar.
     * e.g. range 100, indicate that if other height is my_height + 99 or -99, will still be considered
     * similar or matched, but my_height + 100 or -100, will not.
     *
     * args:   my_height    - height (SMA) of current node.
     *         other_height - height (SMA) of the other node to be compared.
     *         range        - acceptance range, i.e. the extents to which two heights will be considered
     *                        similar or matched.  
     *
     * return: true indicate there is a match, false otherwise.
     */
    if (other_height < my_height + range && other_height > my_height - range) {
        return true;
    }
    return false;
}
