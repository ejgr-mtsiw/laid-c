/*
 ============================================================================
 Name        : dataset_hdf5.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage hdf5 datasets
 ============================================================================
 */

#ifndef DATASET_HDF5_H
#define DATASET_HDF5_H

#include "globals.h"
#include "hdf5.h"
#include <math.h>
#include <malloc.h>

#define DATASET_INVALID_DIMENSIONS 1
#define DATASET_NOT_ENOUGH_CLASSES 2
#define DATASET_NOT_ENOUGH_ATTRIBUTES 3
#define DATASET_NOT_ENOUGH_OBSERVATIONS 4

#define ERROR_ALLOCATING_DATASET_DATA 1

/**
 * Number of ranks for data
 */
#define DATA_RANK 2

/**
 * Reads the dataset attributes and fills the global variables
 */
int read_attributes(const hid_t dataset_id);

/**
 * Reads the value of one attribute from the dataset
 */
herr_t read_attribute(hid_t dataset_id, const char *attribute, hid_t datatype,
		void *value);

/**
 * Reads chunk dimensions from dataset if chunking was enabled
 */
int get_chunk_dimensions(const hid_t dataset_id, hsize_t *chunk_dimensions);

/**
 * Returns the dataset dimensions stored in the hdf5 dataset
 */
void get_dataset_dimensions(hid_t dataset_id, hsize_t *dataset_dimensions);

/**
 * Reads the full dataset data to the array
 */
herr_t read_dataset(const char *filename, const char *datasetname,
		unsigned long **dataset);

#endif
