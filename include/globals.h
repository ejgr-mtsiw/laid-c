/*
 ============================================================================
 Name        : globals.h
 Author      : Eduardo Ribeiro
 Description : Global variables definitions
 ============================================================================
 */

#ifndef GLOBALS_H__
#define GLOBALS_H__

#include <stdint.h>

//#define DEBUG 1

//#ifdef DEBUG
#include <time.h>

#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#endif

#define SETUP_TIMING struct timespec tick, tock;
#define TICK clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
#define TOCK(stream) clock_gettime(CLOCK_MONOTONIC_RAW, &tock); fprintf(stream, "[%0.3fs]\n", (tock.tv_nsec - tick.tv_nsec) / 1000000000.0F + (tock.tv_sec  - tick.tv_sec));
//#else
//#define SETUP_TIMING
//#define TICK
//#define TOCK
//#endif

typedef int8_t oknok_t; // Is OK or NOK
#define NOK -1
#define OK 1

#define NOT_BLACKLISTED false
#define BLACKLISTED true

/**
 * The name of the dataset that will store the disjoint matrix
 * with attributes as lines
 */
#define DM_DATASET_COLUMN_DATA "/COLUMN_DATA"

/**
 * The name of the dataset that will store the disjoint matrix
 * with attributes as columns
 */
#define DM_DATASET_LINE_DATA "/LINE_DATA"

/**
 * The name of the dataset that will store the totals for each line
 */
#define DM_DATASET_LINE_TOTALS "/LINE_TOTALS"

/**
 * The name of the dataset that will dtore the attribute totals
 */
#define DM_DATASET_ATTRIBUTE_TOTALS "/ATTRIBUTE_TOTALS"

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
