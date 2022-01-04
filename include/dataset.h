/*
 ============================================================================
 Name        : dataset.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage datasets
 ============================================================================
 */

#ifndef DATASET_H
#define DATASET_H

#include "hdf5.h"
#include "dataset_lines.h"
#include <malloc.h>
#include <math.h>
#include <string.h>

#define OK 0

#define DATASET_VALID_DIMENSIONS 0
#define DATASET_INVALID_DIMENSIONS 1

#define ERROR_ALLOCATING_DATASET_DATA 1

/**
 * Number of ranks for data
 */
#define DATA_RANK 2

typedef struct dataset {
	hsize_t dimensions[2];

	hsize_t chunk_dimensions[2];

	unsigned int n_classes;

	unsigned long n_observations;

	unsigned long n_attributes;

	unsigned long *data;

	unsigned long *last;
} dataset;

/**
 * Prepares dataset
 */
int setup_dataset(const hid_t dataset_id, dataset *dataset);

/**
 * Reads the value of one attribute from the dataset
 */
herr_t read_attribute(hid_t dataset_id, const char *attribute, hid_t datatype, void *value);

/**
 * Reads chunk dimensions from dataset if chunking was enabled
 */
int get_chunk_dimensions(const hid_t dataset_id, hsize_t *chunk_dimensions);

/**
 * Returns the dataset dimensions stored in the hdf5 dataset
 */
void get_dataset_dimensions(hid_t dataset_id, hsize_t *dataset_dimensions);

/**
 * Adds a new line to the dataset
 */
void* dataset_add_line(dataset *dataset, const unsigned long *data);

/**
 * Reads data from hdf5 dataset and fills the dataset and dataset_lines structures
 */
int build_dataset(const hid_t dataset_id, dataset *dataset, dataset_lines *dataset_lines);

/**
 * Adds jnsq bits to dataset.
 * @returns updated number of attributes
 */
unsigned long setup_jnsq(const dataset_lines *dataset_lines, const unsigned int max_inconsistencies,
		const unsigned long n_attributes);

#endif
