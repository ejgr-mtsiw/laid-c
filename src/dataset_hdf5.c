/*
 ============================================================================
 Name        : dataset_hdf5.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage HDF5 datasets
 ============================================================================
 */

#include "dataset_hdf5.h"

/**
 * Reads the dataset attributes from the hdf5 file
 */
int read_attributes(const hid_t dataset_id, dataset_t *dataset) {

	unsigned int n_classes = 0;
	read_attribute(dataset_id, HDF5_N_CLASSES_ATTRIBUTE, H5T_NATIVE_INT,
			&n_classes);

	if (n_classes < 2) {
		fprintf(stderr, "Dataset must have at least 2 classes\n");
		return DATASET_NOT_ENOUGH_CLASSES;
	}

	unsigned int n_observations = 0;
	// Number of observations (lines) in the dataset
	read_attribute(dataset_id, HDF5_N_OBSERVATIONS_ATTRIBUTE, H5T_NATIVE_INT,
			&n_observations);

	if (n_observations < 2) {
		fprintf(stderr, "Dataset must have at least 2 observations\n");
		return DATASET_NOT_ENOUGH_OBSERVATIONS;
	}

	unsigned int n_attributes = 0;
	// Number of attributes in the dataset
	read_attribute(dataset_id, HDF5_N_ATTRIBUTES_ATTRIBUTE, H5T_NATIVE_INT,
			&n_attributes);

	if (n_attributes < 1) {
		fprintf(stderr, "Dataset must have at least 1 attribute\n");
		return DATASET_NOT_ENOUGH_ATTRIBUTES;
	}

	// Store data
	dataset->n_attributes = n_attributes;
	dataset->n_bits_for_class = (unsigned int) ceil(log2(n_classes));
	dataset->n_bits_for_jnsqs = dataset->n_bits_for_class;
	dataset->n_classes = n_classes;
	dataset->n_observations_per_class = NULL;
	dataset->n_observations = n_observations;
	dataset->data = NULL;
	dataset->observations_per_class = NULL;

	//! TODO: Remove duplicate code!
	unsigned long total_bits = dataset->n_attributes
			+ dataset->n_bits_for_class;
	unsigned int n_longs = total_bits / BLOCK_BITS
			+ (total_bits % BLOCK_BITS != 0);

	dataset->n_longs = n_longs;

	return DATASET_OK;
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

///**
// * Writes an attribute to the dataset
// */
//herr_t write_attribute(hid_t dataset_id, const char *attribute, hid_t datatype,
//		const void *value) {
//
//	hid_t attr_dataspace = H5Screate(H5S_SCALAR);
//	hid_t attr = H5Acreate2(dataset_id, attribute, datatype, attr_dataspace,
//	H5P_DEFAULT, H5P_DEFAULT);
//
//	// Write the attribute to the dataset
//	herr_t status = H5Awrite(attr, datatype, value);
//	if (status < 0) {
//		fprintf(stderr, "Error writing attribute %s.\n", attribute);
//		return status;
//	}
//
//	// Close the attribute.
//	status = H5Aclose(attr);
//	if (status < 0) {
//		fprintf(stderr, "Error closing attribute %s.\n", attribute);
//		return status;
//	}
//
//	// Close the dataspace.
//	status = H5Sclose(attr_dataspace);
//	if (status < 0) {
//		fprintf(stderr, "Error closing attribute %s datatspace.\n", attribute);
//	}
//
//	return status;
//}

/**
 * Reads chunk dimensions from dataset if chunking was enabled
 */
int get_chunk_dimensions(const hid_t dataset_id, hsize_t *chunk_dimensions) {

	// No chunking defined
	int chunked = 0;

	// Get creation properties list.
	hid_t property_list_id = H5Dget_create_plist(dataset_id);

	if (H5D_CHUNKED == H5Pget_layout(property_list_id)) {
		// Get chunking information: rank and dimensions
		H5Pget_chunk(property_list_id, DATA_RANK, chunk_dimensions);

		chunked = H5D_CHUNKED;
	}

	H5Pclose(property_list_id);

	return chunked;
}

///**
// * Returns the dataset dimensions stored in the hdf5 dataset
// */
//void get_dataset_dimensions(hid_t dataset_id, hsize_t *dataset_dimensions) {
//
//	// Get filespace handle first.
//	hid_t dataset_space_id = H5Dget_space(dataset_id);
//
//	// Get dataset dimensions.
//	H5Sget_simple_extent_dims(dataset_space_id, dataset_dimensions, NULL);
//
//	// Close dataspace
//	H5Sclose(dataset_space_id);
//}
//
