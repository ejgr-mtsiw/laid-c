/*
 ============================================================================
 Name        : hdf5_dataset.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage HDF5 datasets
 ============================================================================
 */

#include "hdf5_dataset.h"

bool hdf5_dataset_exists(const hid_t file_id, const char *name) {

	return (H5Lexists(file_id, name, H5P_DEFAULT) > 0);
}

/**
 * Reads the dataset attributes from the hdf5 file
 */
herr_t hdf5_read_dataset_attributes(const char *filename,
		const char *datasetname, dataset_t *dataset) {

	herr_t ret = DATASET_OK;

	//Open the data file
	hid_t file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", filename);
		return NOK;
	}

	// Open input dataset
	hid_t dataset_id = H5Dopen2(file_id, datasetname, H5P_DEFAULT);
	if (dataset_id < 1) {
		// Error creating file
		fprintf(stderr, "Dataset %s not found!\n", datasetname);
		ret = NOK;
		goto out_close_file;
	}

	unsigned int n_classes = 0;
	hdf5_read_attribute(dataset_id, HDF5_N_CLASSES_ATTRIBUTE, H5T_NATIVE_INT,
			&n_classes);

	if (n_classes < 2) {
		fprintf(stderr, "Dataset must have at least 2 classes\n");
		ret = DATASET_NOT_ENOUGH_CLASSES;
		goto out_close_dataset;
	}

	unsigned int n_observations = 0;
	// Number of observations (lines) in the dataset
	hdf5_read_attribute(dataset_id, HDF5_N_OBSERVATIONS_ATTRIBUTE,
	H5T_NATIVE_INT, &n_observations);

	if (n_observations < 2) {
		fprintf(stderr, "Dataset must have at least 2 observations\n");
		ret = DATASET_NOT_ENOUGH_OBSERVATIONS;
		goto out_close_dataset;
	}

	unsigned int n_attributes = 0;
	// Number of attributes in the dataset
	hdf5_read_attribute(dataset_id, HDF5_N_ATTRIBUTES_ATTRIBUTE, H5T_NATIVE_INT,
			&n_attributes);

	if (n_attributes < 1) {
		fprintf(stderr, "Dataset must have at least 1 attribute\n");
		ret = DATASET_NOT_ENOUGH_ATTRIBUTES;
		goto out_close_dataset;
	}

	// Store data
	dataset->data = NULL;
	dataset->n_observations_per_class = NULL;
	dataset->observations_per_class = NULL;
	dataset->n_attributes = n_attributes;
	dataset->n_bits_for_class = (unsigned int) ceil(log2(n_classes));
	dataset->n_bits_for_jnsqs = 0;
	dataset->n_classes = n_classes;
	dataset->n_observations = n_observations;

	unsigned long total_bits = dataset->n_attributes
			+ dataset->n_bits_for_class;
	unsigned int n_longs = total_bits / BLOCK_BITS
			+ (total_bits % BLOCK_BITS != 0);

	dataset->n_longs = n_longs;

	out_close_dataset: H5Dclose(dataset_id);

	out_close_file: H5Fclose(file_id);

	return ret;
}

/**
 * Reads the value of one attribute from the dataset
 */
herr_t hdf5_read_attribute(hid_t dataset_id, const char *attribute,
		hid_t datatype, void *value) {

	herr_t status = H5Aexists(dataset_id, attribute);
	if (status < 0) {
		// Error reading attribute
		fprintf(stderr, "Error reading attribute %s", attribute);
		return status;
	}

	if (status == 0) {
		// Attribute does not exist
		*(unsigned int*) value = 0;
		return status;
	}

	// Open the attribute
	hid_t attr = H5Aopen(dataset_id, attribute, H5P_DEFAULT);
	if (attr < 0) {
		fprintf(stderr, "Error closing the attribute %s", attribute);
		return attr;
	}

	// read the attribute value
	status = H5Aread(attr, datatype, value);
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
