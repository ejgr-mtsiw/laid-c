/*
 ============================================================================
 Name        : dataset.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage datasets
 ============================================================================
 */

#include "dataset.h"

/**
 * Prepares dataset
 */
void setup_dataset(const hid_t dataset_id, dataset *dataset) {
	dataset->data = NULL;
	dataset->last = NULL;

	get_dataset_dimensions(dataset_id, dataset->dimensions);

	// Get attributes
	read_attribute(dataset_id, "n_classes", H5T_NATIVE_INT, &dataset->n_classes);
	printf("The value of the attribute \"n_classes\" is %d \n", dataset->n_classes);

	read_attribute(dataset_id, "n_observations", H5T_NATIVE_ULONG, &dataset->n_observations);
	printf("The value of the attribute \"n_observations\" is %lu \n", dataset->n_observations);

	read_attribute(dataset_id, "n_attributes", H5T_NATIVE_ULONG, &dataset->n_attributes);
	printf("The value of the attribute \"n_attributes\" is %lu \n", dataset->n_attributes);

	// Allocate main buffer
	dataset->data = (unsigned long*) malloc(sizeof(unsigned long) * dataset->dimensions[0] * dataset->dimensions[1]);
}

/**
 * Reads the value of one attribute from the dataset
 */
herr_t read_attribute(hid_t dataset_id, const char *attribute, hid_t datatype, void *value) {

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
		printf("chunk dimensions %lu x %lu\n", (unsigned long) (chunk_dimensions[0]),
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
	fprintf(stdout, "dataset dimensions %lu x %lu\n", (unsigned long) (dataset_dimensions[0]),
			(unsigned long) (dataset_dimensions[1]));

	H5Sclose(dataset_space_id);
}

/**
 * Adds a new line to the dataset
 */
void* dataset_add_line(dataset *dataset, const unsigned long *data) {

	size_t data_size = sizeof(unsigned long) * dataset->dimensions[1];

	if (dataset->last == NULL) {
		dataset->last = dataset->data;
	} else {
		dataset->last += dataset->dimensions[1];
	}

	return memcpy(dataset->last, data, data_size);
}

/**
 * Reads data from hdf5 dataset and fills the dataset and dataset_lines structures
 */
int build_dataset(const hid_t dataset_id, dataset *dataset, dataset_lines *dataset_lines) {
	/**
	 * Dataset dataspace identifier
	 */
	hid_t dataset_space_id = 0;

	/**
	 * In memory dataspace identifier
	 */
	hid_t memory_space_id = 0;

	/**
	 * Store the result of operations
	 */
	//herr_t status = 0;
	/**
	 * Buffer to store one line/chunk of data
	 */
	unsigned long *buffer = NULL;

	/**
	 * Number of bits for classes
	 */
	unsigned int n_bits_for_classes = 0;

	/**
	 * Number of duplicated observations
	 */
	unsigned long n_duplicates = 0;

	/**
	 * Number of inconsistent rows in the dataset
	 */
	unsigned long n_inconsistencies = 0;

	/**
	 * Maximum number of inconsistencies
	 */
	unsigned long max_inconsistencies = 0;

	/**
	 * Buffer for a new line
	 */
	dataset_line new_line;

	if (H5D_CHUNKED != get_chunk_dimensions(dataset_id, dataset->chunk_dimensions)) {
		// No chunking defined, but we can still read line by line
		dataset->chunk_dimensions[0] = 1;
		dataset->chunk_dimensions[1] = dataset->dimensions[1];
	}

	// Define the dataset dataspace
	dataset_space_id = H5Dget_space(dataset_id);

	/*
	 * Define the memory space to read a chunk.
	 */
	memory_space_id = H5Screate_simple(DATA_RANK, dataset->chunk_dimensions, NULL);

	// Allocate chunk buffer
	buffer = (unsigned long*) malloc(sizeof(unsigned long) * dataset->chunk_dimensions[1]);

	// Define hyperslab properties
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { dataset->chunk_dimensions[0], dataset->chunk_dimensions[1] };

	// Number of bits for classes
	n_bits_for_classes = (unsigned int) log2(dataset->n_classes) + 1;

	for (hsize_t line = 0; line < dataset->n_observations; line++) {

		offset[0] = line;

		// Define chunk in the file (hyperslab) to read.
		H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset, NULL, count, NULL);

		// Read chunk back
		H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id, dataset_space_id, H5P_DEFAULT, buffer);

		new_line.inconsistency = 0;
		new_line.class_id = get_class(buffer, dataset->n_attributes, n_bits_for_classes);
		new_line.data = buffer;

		// Check for duplicates and inconsistencies
		int line_status = check_duplicate_inconsistent(dataset_lines, &new_line, dataset->n_attributes);

		if (line_status == DATASET_LINE_DISTINCT || line_status == DATASET_LINE_INCONSISTENT) {
			// It's either a new entry or an inconsistent one
			// and must be added to the list

			unsigned long *data_pos = (unsigned long*) dataset_add_line(dataset, buffer);
			if (data_pos == NULL) {
				//Error!
			}
			new_line.data = data_pos;
			add_line(dataset_lines, &new_line);

			if (line_status == DATASET_LINE_INCONSISTENT) {
				// Update global counter
				n_inconsistencies++;

				// Update max
				if (new_line.inconsistency > max_inconsistencies) {
					max_inconsistencies = new_line.inconsistency;
				}
			}
		}
	}

	// Update jnsq bits. n_attributes now includes jnsq bits too!
	dataset->n_attributes = setup_jnsq(dataset_lines, max_inconsistencies, dataset->n_attributes);

	fprintf(stdout, "Found %lu observations, %lu duplicates.\n", dataset->n_observations, n_duplicates);
	fprintf(stdout, "Found %lu inconsistencies, max inconsistencies: %lu.\n", n_inconsistencies, max_inconsistencies);

	for (unsigned long i = 0; i < dataset_lines->n_lines; i++) {
		print_line(&dataset_lines->lines[i], dataset->n_attributes);

		if (i == 9 && dataset_lines->n_lines > 20) {
			// Too many lines, jump to end
			printf(" ... \n");
			i = dataset_lines->n_lines - 11;
		}
	}

	/*
	 * Close/release resources.
	 */
	free(buffer);
	H5Sclose(memory_space_id);
	H5Sclose(dataset_space_id);

	return 0;
}

/**
 * Adds jnsq bits to dataset.
 * @returns updated number of attributes
 */
unsigned long setup_jnsq(const dataset_lines *dataset_lines, const unsigned int max_inconsistencies,
		const unsigned long n_attributes) {

	unsigned int n_bits_for_jnsqs = 0;

	if (max_inconsistencies > 0) {
		// Calculate update number of bits needed to store jnsq
		n_bits_for_jnsqs = log2(max_inconsistencies) + 1;

		// Update all lines
		for (dataset_line *line = dataset_lines->first; line <= dataset_lines->last; line++) {
			set_jnsq(line, n_attributes, n_bits_for_jnsqs);
		}

		// Update n_attributes
		return n_attributes + n_bits_for_jnsqs;
	}

	// No inconsistencies, means no extra attributes
	return n_attributes;
}
