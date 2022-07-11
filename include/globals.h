/*
 ============================================================================
 Name        : globals.h
 Author      : Eduardo Ribeiro
 Description : Global variables definitions
 ============================================================================
 */

#ifndef GLOBALS_H__
#define GLOBALS_H__

//#define DEBUG 1

#define NOK -1
#define OK 1

//#ifdef DEBUG
#include <time.h>
#define SETUP_TIMING struct timespec tick, tock;
#define TICK clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
#define TOCK(stream) clock_gettime(CLOCK_MONOTONIC_RAW, &tock); fprintf(stream, "[%0.3fs]\n", (tock.tv_nsec - tick.tv_nsec) / 1000000000.0F + (tock.tv_sec  - tick.tv_sec));
//#else
//#define SETUP_TIMING
//#define TICK
//#define TOCK
//#endif

#define NOT_BLACKLISTED false
#define BLACKLISTED true

/**
 * The name of the dataset that will store the disjoint matrix
 * with attributes as lines
 */
#define DM_DATASET_ATTRIBUTES_LINE "DMX_LINE"

/**
 * The name of the dataset that will store the disjoint matrix
 * with attributes as columns
 */
#define DM_DATASET_ATTRIBUTES_COLUMN "DMX_COLUMN"

/**
 * Attribute for number of classes
 */
#define HDF5_N_CLASSES_ATTRIBUTE "n_classes"

/**
 * Attribute for number of attributes
 */
#define HDF5_N_ATTRIBUTES_ATTRIBUTE "n_attributes"

/**
 * Attribute for number of observations
 */
#define HDF5_N_OBSERVATIONS_ATTRIBUTE "n_observations"

/**
 * Attrinute for the number of lines of the disjoint matrix
 */
#define HDF5_N_MATRIX_LINES_ATTRIBUTE "n_matrix_lines"

#endif
