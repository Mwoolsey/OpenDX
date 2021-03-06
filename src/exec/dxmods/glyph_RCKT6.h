/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#define RCKT6PTS 30
#define RCKT6TRS 22
static Point points[RCKT6PTS] = {
    {0.5000000000F, 0.0000000000F, 0.0000000000F},
    {0.5000000000F, 0.8500000238F, 0.0000000000F},
    {0.2499999851F, 0.0000000000F, 0.4330127239F},
    {0.2499999851F, 0.8500000238F, 0.4330127239F},
    {-0.2500000298F, 0.0000000000F, 0.4330126941F},
    {-0.2500000298F, 0.8500000238F, 0.4330126941F},
    {-0.5000000000F, 0.0000000000F, -0.0000000437F},
    {-0.5000000000F, 0.8500000238F, -0.0000000437F},
    {-0.2499999553F, 0.0000000000F, -0.4330127239F},
    {-0.2499999553F, 0.8500000238F, -0.4330127239F},
    {0.2499999553F, 0.0000000000F, -0.4330127239F},
    {0.2499999553F, 0.8500000238F, -0.4330127239F},
    {0.8333334923F, 0.7500000000F, 0.0000000000F},
    {0.4166667163F, 0.7500000000F, 0.7216879725F},
    {-0.4166667759F, 0.7500000000F, 0.7216879725F},
    {-0.8333334923F, 0.7500000000F, -0.0000000729F},
    {-0.4166666567F, 0.7500000000F, -0.7216880322F},
    {0.4166666865F, 0.7500000000F, -0.7216880322F},
    {0.0000000000F, 1.0000000000F, 0.0000000000F},
    {0.0000000000F, 1.0000000000F, 0.0000000000F},
    {0.0000000000F, 1.0000000000F, 0.0000000000F},
    {0.0000000000F, 1.0000000000F, 0.0000000000F},
    {0.0000000000F, 1.0000000000F, 0.0000000000F},
    {0.0000000000F, 1.0000000000F, 0.0000000000F},
    {0.5000000000F, 0.0000000000F, 0.0000000000F},
    {0.2499999851F, 0.0000000000F, 0.4330127239F},
    {-0.2500000298F, 0.0000000000F, 0.4330126941F},
    {-0.5000000000F, 0.0000000000F, -0.0000000437F},
    {-0.2499999553F, 0.0000000000F, -0.4330127239F},
    {0.2499999553F, 0.0000000000F, -0.4330127239F}};

#include <dxconfig.h>

static Point normals[RCKT6PTS] = {
    {1.0000000000F, 0.0000000000F, 0.0000000000F},
    {1.0000000000F, 0.0000000000F, 0.0000000000F},
    {0.4999999702F, 0.0000000000F, 0.8660254478F},
    {0.4999999702F, 0.0000000000F, 0.8660254478F},
    {-0.5000000596F, 0.0000000000F, 0.8660253882F},
    {-0.5000000596F, 0.0000000000F, 0.8660253882F},
    {-1.0000000000F, 0.0000000000F, -0.0000000874F},
    {-1.0000000000F, 0.0000000000F, -0.0000000874F},
    {-0.4999999106F, 0.0000000000F, -0.8660254478F},
    {-0.4999999106F, 0.0000000000F, -0.8660254478F},
    {0.4999999106F, 0.0000000000F, -0.8660254478F},
    {0.4999999106F, 0.0000000000F, -0.8660254478F},
    {0.4830437303F, 0.8755962253F, 0.0000000000F},
    {0.2415218502F, 0.8755962253F, 0.4183281362F},
    {-0.2415218800F, 0.8755962253F, 0.4183281362F},
    {-0.4830437303F, 0.8755962253F, -0.0000000422F},
    {-0.2415218204F, 0.8755962253F, -0.4183281660F},
    {0.2415218353F, 0.8755962253F, -0.4183281660F},
    {0.4830437303F, 0.8755962253F, 0.0000000000F},
    {0.2415218502F, 0.8755962253F, 0.4183281362F},
    {-0.2415218800F, 0.8755962253F, 0.4183281362F},
    {-0.4830437303F, 0.8755962253F, -0.0000000422F},
    {-0.2415218204F, 0.8755962253F, -0.4183281660F},
    {0.2415218353F, 0.8755962253F, -0.4183281660F},
    {0.0000000000F, -1.0000000000F, 0.0000000000F},
    {0.0000000000F, -1.0000000000F, 0.0000000000F},
    {0.0000000000F, -1.0000000000F, 0.0000000000F},
    {0.0000000000F, -1.0000000000F, 0.0000000000F},
    {0.0000000000F, -1.0000000000F, 0.0000000000F},
    {0.0000000000F, -1.0000000000F, 0.0000000000F}};

static Triangle triangles[] = {{0, 1, 2},
                               {2, 1, 3},
                               {2, 3, 4},
                               {4, 3, 5},
                               {4, 5, 6},
                               {6, 5, 7},
                               {6, 7, 8},
                               {8, 7, 9},
                               {8, 9, 10},
                               {10, 9, 11},
                               {10, 11, 0},
                               {0, 11, 1},
                               {12, 18, 13},
                               {13, 19, 14},
                               {14, 20, 15},
                               {15, 21, 16},
                               {16, 22, 17},
                               {17, 23, 12},
                               {24, 25, 26},
                               {24, 26, 27},
                               {24, 27, 28},
                               {24, 28, 29}};
