/*
 ============================================================================
 Name        : hdf5_dataset.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage hdf5 datasets
 ============================================================================
 */

#ifndef HDF5_DATASET_H
#define HDF5_DATASET_H

#include "dataset.h"
#include "globals.h"
#include "hdf5.h"
#include <malloc.h>
#include <math.h>
#include <stdint.h>

/**
 * Number of ranks for data
 */
#define DATA_RANK 2

/**
 * Reads only the dataset attributes on file
 */
#define READ_DATASET_ATTRIBUTES 1

/**
 * Reads the dataset data on file
 */
#define READ_DATASET_DATA 2

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

/**
 * Checks if dataset is present in file_id
 */
bool hdf5_dataset_exists(const hid_t file_id, const char *dataset);

/**
 * Checks if dataset is present from filename
 */
bool hdf5_dataset_exists_in_file(const char *filename, const char *datasetname);

/**
 * Opens the file and dataset indicated, returning references
 * in file_id and dataset_id
 */
int hdf5_open_dataset(const char *filename, const char *datasetname,
		hid_t *file_id, hid_t *dataset_id);

/**
 * Fills the dataset structure
 */
int hdf5_read_dataset(const char *filename, const char *datasetname,
		dataset_t *dataset);

/**
 * Fills the dataset attributes only
 */
int hdf5_read_dataset_attributes_only(const char *filename,
		const char *datasetname, dataset_t *dataset);

/**
 * Reads the dataset attributes from the hdf5 file
 */
int hdf5_read_dataset_attributes(hid_t dataset_id, dataset_t *dataset);

/**
 * Reads the value of one attribute from the dataset
 */
int hdf5_read_attribute(hid_t dataset_id, const char *attribute, hid_t datatype,
		void *value);

/**
 * Reads the dataset data from the hdf5 file
 */
int hdf5_read_data(hid_t dataset_id, dataset_t *dataset);

/**
 * Writes an attribute to the dataset
 */
int hdf5_write_attribute(hid_t dataset_id, const char *attribute,
		hid_t datatype, const void *value);

/**
 * Reads chunk dimensions from dataset if chunking was enabled
 */
int hdf5_get_chunk_dimensions(const hid_t dataset_id, hsize_t *chunk_dimensions);

/**
 * Returns the dataset dimensions stored in the hdf5 dataset
 */
void hdf5_get_dataset_dimensions(hid_t dataset_id, hsize_t *dataset_dimensions);

#endif

