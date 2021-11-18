/* Members:     Name: Chan Kah Hee     Email: kcha0047@student.monash.edu
 *              Name: Wong Jun Kang    Email: jwon0050@student.monash.edu
 * Team: Team-07
 * Lab:  MA_LAB-06 
 * Lecturer:    Dr Vishnu Monn Baskaran
 * Tutor:       Mr Lee Ming Jie     */
 
/* header files */
#include <stdlib.h>
#include <time.h>
#include "generate_height.h"

/* function definitions */
float generate_rand_height(int rank, float lower_bound){
    /* 
     * Given the rank of the process, this function use the rank together 
     * with the time as seed for random() and generate a random value
     * betwen lower_bound and UPPER_BOUND.
     *
     * args:    rank        - the rank of current process.
     *          lower_bound - the lowerbound such that the value is generated.
     * 
     * return:  randomly generated float value between UPPER_BOUND and lower_bound.
     */
    srand(time(NULL) + rank);
    float random = ((float) rand()) / ((float) RAND_MAX);
    return random * (UPPER_BOUND - lower_bound) + lower_bound;
}
