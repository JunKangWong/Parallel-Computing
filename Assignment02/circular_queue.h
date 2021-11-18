/* Members:     Name: Chan Kah Hee     Email: kcha0047@student.monash.edu
 *              Name: Wong Jun Kang    Email: jwon0050@student.monash.edu
 * Team: Team-07
 * Lab:  MA_LAB-06 
 * Lecturer:    Dr Vishnu Monn Baskaran
 * Tutor:       Mr Lee Ming Jie     */

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include <stdbool.h>
#include <stdarg.h>
#include "sensor_node.h"
#include "base_station.h"

#define QUEUE_CAPACITY 30 // size of the queue.

/* an enum indicating the element type of the
 * item in queue */
typedef enum {
    SENSOR_NODE,
    SWC_HEIGHT
} element_type_t;

typedef struct {
    element_type_t element_type;
    union {
        float sensor_heights[QUEUE_CAPACITY];
        swc_height_t *swc_heights;  
    } array;
    int rear; 
    int size; // size stores total size, including old records that are no longer in the queue.
} queue_t;


/* Function prototype */
queue_t *init_queue(element_type_t element_type);
bool is_empty(queue_t *q);
bool is_full(queue_t *q);
void enqueue(queue_t *q, ...);
float calc_simple_moving_avg(queue_t *q);
void delete_queue(queue_t *q);

#endif /*CIRCULAR_QUEUE_H*/
