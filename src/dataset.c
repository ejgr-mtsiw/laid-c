/*
 ============================================================================
 Name        : dataset.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage datasets
 ============================================================================
 */

#include "dataset.h"

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
 * Returns the class of this data line
 */
unsigned int get_class(const unsigned long *line) {

	// Check how many attributes remain on last long
	int remaining_attributes = g_n_attributes % LONG_BITS;

	// Class starts here
	int at = LONG_BITS - remaining_attributes - g_n_bits_for_class;

	return (unsigned int) get_bits(line[g_n_longs - 1], at, g_n_bits_for_class);
}

/**
 * Prints a line to stream
 */
void print_line(FILE *stream, const unsigned long *line, const char extra_bits) {

	// Current attribute
	unsigned long columns_to_write = g_n_attributes;

	if (extra_bits == 1) {
		columns_to_write += g_n_bits_for_class;
	}

	for (unsigned int i = 0; i < g_n_longs && columns_to_write > 0; i++) {
		for (int j = LONG_BITS - 1; j >= 0 && columns_to_write > 0; j--) {
			if (j % 8 == 0) {
				fprintf(stream, " ");
			}

			fprintf(stream, "%d", BIT_CHECK(line[i], j));

			columns_to_write--;
		}

		if (i == 0 && g_n_longs > 2) {
			// Too many to write, let's jump to the end
			i = g_n_longs - 2;
			fprintf(stream, " ... ");
		}
	}

	//fprintf(stream, "\n");
}

/**
 * Prints the whole dataset
 */
void print_dataset(FILE *stream, const char *title, unsigned long *dataset,
		const char extra_bits) {

	fprintf(stream, "%s\n", title);

	unsigned long *line = dataset;
	for (unsigned long i = 0; i < g_n_observations; i++) {
		print_line(stream, line, extra_bits);
		fprintf(stream, "\n");
		NEXT(line);

		if (g_n_observations > 20 && i == 9) {
			fprintf(stream, " ... \n");
			//skip
			i = g_n_observations - 10;
			line = dataset + (g_n_observations - 10) * g_n_longs;
		}
	}
}

/**
 * Compares two lines of the dataset
 * Used to sort the dataset
 */
int compare_lines(const void *a, const void *b) {

	long long res = 0;

	for (unsigned int i = 0; i < g_n_longs; i++) {
		res = *((unsigned long*) a + i) - *((unsigned long*) b + i);
		if (res > 0) {
			return 1;
		}
		if (res < 0) {
			return -1;
		}
	}
	return 0;
}

/**
 * Checks if the lines have the same attributes
 */
int has_same_attributes(const unsigned long *a, const unsigned long *b) {

	long long res = 0;

	for (unsigned int i = 0; i < g_n_longs - 1; i++) {
		res = *(a + i) - *(b + i);
		if (res != 0) {
			return 0;
		}
	}

	unsigned int remaining_attributes = g_n_attributes % LONG_BITS;

	if (remaining_attributes == 0) {
		// Nothing more to check
		return 1;
	}

	unsigned long mask = ~0L;
	mask >>= remaining_attributes;
	mask = ~mask;

	if ((a[g_n_longs - 1] & mask) != (b[g_n_longs - 1] & mask)) {
		return 0;
	}
	return 1;
}

/**
 * Removes duplicated lines from the dataset.
 * Assumes the dataset is ordered
 */
unsigned long remove_duplicates(unsigned long *dataset) {

	unsigned long *line = dataset;
	unsigned long *last = line;

	unsigned long n_uniques = 1;

	for (unsigned long i = 0; i < g_n_observations - 1; i++) {
		NEXT(line);
		if (compare_lines(line, last) != 0) {
			NEXT(last);
			n_uniques++;
			if (last != line) {
				memcpy(last, line, sizeof(unsigned long) * g_n_longs);
			}
		}
	}

	// Update number of unique observations
	g_n_observations = n_uniques;
	return g_n_observations;
}

/**
 * Fill the arrays with the number os items per class and also a matrix with
 * references to the lines that belong to each class to simplify the
 * calculation of the disjoint matrix
 *
 * Inputs are expected to be zeroed arrays
 */
void fill_class_arrays(unsigned long *dataset, unsigned long *n_items_per_class,
		unsigned long **observations_per_class) {

	// Current line
	unsigned long *line = dataset;

	// This line class
	unsigned int clas = 0;

	for (unsigned long i = 0; i < g_n_observations; i++) {
		clas = get_class(line);

		observations_per_class[clas * g_n_observations + n_items_per_class[clas]] =
				line;

		n_items_per_class[clas]++;

		NEXT(line);
	}
}
