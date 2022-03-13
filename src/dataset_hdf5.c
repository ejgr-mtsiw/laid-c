/*
 ============================================================================
 Name        : dataset_hdf5.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage HDF5 datasets
 ============================================================================
 */

#include "dataset_hdf5.h"

/**
 * Reads the dataset attributes and fills the global variables
 */
int read_attributes(const hid_t dataset_id) {

	get_dataset_dimensions(dataset_id, g_dimensions);

	// Number of longs needed to store one line of the dataset
	g_n_longs = g_dimensions[1];

	// Get attributes
	read_attribute(dataset_id, "n_classes", H5T_NATIVE_INT, &g_n_classes);

	if (g_n_classes < 2) {
		fprintf(stderr, "Dataset must have at least 2 classes\n");
		return DATASET_NOT_ENOUGH_CLASSES;
	}

	// Number of bits needed to store class info
	g_n_bits_for_class = (unsigned int) ceil(log2(g_n_classes));

	// Number of observations (lines) in the dataset
	read_attribute(dataset_id, "n_observations", H5T_NATIVE_ULONG,
			&g_n_observations);

	if (g_n_observations < 2) {
		fprintf(stderr, "Dataset must have at least 2 observations\n");
		return DATASET_NOT_ENOUGH_OBSERVATIONS;
	}

	// Number of attributes in the dataset
	read_attribute(dataset_id, "n_attributes", H5T_NATIVE_ULONG,
			&g_n_attributes);

	if (g_n_attributes < 1) {
		fprintf(stderr, "Dataset must have at least 1 attribute\n");
		return DATASET_NOT_ENOUGH_ATTRIBUTES;
	}

	return OK;
}

/**
 * Reads the value of one attribute from the dataset
 */
herr_t read_attribute(hid_t dataset_id, const char *attribute, hid_t datatype,
		void *value) {

	// Open the attribute
	hid_t attr = H5Aopen(dataset_id, attribute, H5P_DEFAULT);

	// read the attribute value
	herr_t status = H5Aread(attr, datatype, value);
	if (status < 0) {
		fprintf(stderr, "Error reading attribute %s", attribute);
		return status;
	}

	// close the attribute
	status = H5Aclose(attr);
	if (status < 0) {
		fprintf(stderr, "Error closing the attribute %s", attribute);
	}
	return status;
}

/**
 * Reads chunk dimensions from dataset if chunking was enabled
 */
int get_chunk_dimensions(const hid_t dataset_id, hsize_t *chunk_dimensions) {

	int chunked = 0;

	// Get creation properties list.
	hid_t property_list_id = H5Dget_create_plist(dataset_id);

	if (H5D_CHUNKED == H5Pget_layout(property_list_id)) {
		// Get chunking information: rank and dimensions
		H5Pget_chunk(property_list_id, DATA_RANK, chunk_dimensions);
		printf("chunk dimensions %lu x %lu\n",
				(unsigned long) (chunk_dimensions[0]),
				(unsigned long) (chunk_dimensions[1]));
		chunked = H5D_CHUNKED;
	}

	H5Pclose(property_list_id);
	// No chunking defined
	return chunked;
}

/**
 * Returns the dataset dimensions stored in the hdf5 dataset
 */
void get_dataset_dimensions(hid_t dataset_id, hsize_t *dataset_dimensions) {

	// Get filespace handle first.
	hid_t dataset_space_id = H5Dget_space(dataset_id);

	// Get dataset dimensions.
	H5Sget_simple_extent_dims(dataset_space_id, dataset_dimensions, NULL);

	// Close dataspace
	H5Sclose(dataset_space_id);
}

/**
 * Reads the full dataset data to the array
 */
herr_t read_dataset(const char *filename, const char *datasetname,
		uint_fast64_t **dataset) {

	herr_t status = OK;

	//Open the data file
	hid_t file_id = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", filename);
		return NOK;
	}

	hid_t dataset_id = H5Dopen2(file_id, datasetname, H5P_DEFAULT);
	if (dataset_id < 1) {
		// Error creating file
		fprintf(stderr, "Dataset %s not found!\n", datasetname);

		// Free resources
		H5Fclose(file_id);
		return NOK;
	}

	/**
	 * Read dataset attributes
	 */
	if (read_attributes(dataset_id) != OK) {
		fprintf(stderr, "Error readings attributes from dataset\n");

		// Free resources
		H5Dclose(dataset_id);
		H5Fclose(file_id);
		return NOK;
	}

	// Allocate main buffer
	// https://vorpus.org/blog/why-does-calloc-exist/
	/**
	 * The dataset data
	 */
	*dataset = (uint_fast64_t*) calloc(g_dimensions[0] * g_dimensions[1],
			sizeof(uint_fast64_t));
	if (dataset == NULL) {
		fprintf(stderr, "Error allocating dataset\n");

		// Free resources
		H5Dclose(dataset_id);
		H5Fclose(file_id);
		return NOK;
	}

	// Fill dataset from hdf5 file
	status = H5Dread(dataset_id, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL,
	H5P_DEFAULT, *dataset);
	if (status < 0) {
		fprintf(stderr, "Error reading the dataset data\n");

		// Free resources
		H5Dclose(dataset_id);
		H5Fclose(file_id);
		return NOK;
	}

	return OK;
}
