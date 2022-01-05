/*
 ============================================================================
 Name        : dataset.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage datasets
 ============================================================================
 */

#include "dataset.h"

/**
 * Dataset dimensions
 */
hsize_t dimensions[2] = { 0, 0 };

/**
 * Number of attributes
 */
unsigned long n_attributes = 0;

/**
 * Number of observations
 */
unsigned long n_observations = 0;

/**
 * Number of classes
 */
unsigned int n_classes = 0;

/**
 * Number of bits used to store the class
 */
unsigned int n_bits_for_class = 0;

/**
 * Number of longs needed to store one line of the dataset
 */
unsigned int n_longs = 0;

/**
 * Reads the dataset attributes and fills the global variables
 */
int read_attributes(const hid_t dataset_id) {

	get_dataset_dimensions(dataset_id, dimensions);

	// Number of longs needed to store one line of the dataset
	n_longs = dimensions[1];

	// Get attributes
	read_attribute(dataset_id, "n_classes", H5T_NATIVE_INT, &n_classes);
	printf("The value of the attribute \"n_classes\" is %d \n", n_classes);

	// Number of bits needed to store class info
	n_bits_for_class = (unsigned int) ceil(log2(n_classes));

	// Number of observations (lines) in the dataset
	read_attribute(dataset_id, "n_observations", H5T_NATIVE_ULONG,
			&n_observations);
	printf("The value of the attribute \"n_observations\" is %lu \n",
			n_observations);

	// Number of attributes in the dataset
	read_attribute(dataset_id, "n_attributes", H5T_NATIVE_ULONG, &n_attributes);
	printf("The value of the attribute \"n_attributes\" is %lu \n",
			n_attributes);

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
	int remaining_attributes = n_attributes % LONG_BITS;

	// Class starts here
	int at = LONG_BITS - remaining_attributes - n_bits_for_class;

	return (unsigned int) get_bits(line[n_longs - 1], at, n_bits_for_class);
}

/**
 * Prints a line to stream
 */
void print_line(FILE *stream, const unsigned long *line, const char extra_bits) {

	// Current attribute
	unsigned long columns_to_write = n_attributes;

	if (extra_bits == 1) {
		columns_to_write += n_bits_for_class;
	}

	for (unsigned int i = 0; i < n_longs && columns_to_write > 0; i++) {
		for (int j = LONG_BITS - 1; j >= 0 && columns_to_write > 0; j--) {
			if (j % 8 == 0) {
				fprintf(stream, " ");
			}

			fprintf(stream, "%d", BIT_CHECK(line[i], j));

			columns_to_write--;
		}

		if (i == 0 && n_longs > 2) {
			// Too many to write, let's jump to the end
			i = n_longs - 2;
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
	for (unsigned long i = 0; i < n_observations; i++) {
		print_line(stream, line, extra_bits);
		fprintf(stream, "\n");
		NEXT(line);

		if (n_observations > 20 && i == 9) {
			fprintf(stream, " ... \n");
			//skip
			i = n_observations - 10;
			line = dataset + (n_observations - 10) * n_longs;
		}
	}
}

/**
 * Compares two lines of the dataset
 * Used to sort the dataset
 */
int compare_lines(const void *a, const void *b) {

	long long res = 0;

	for (unsigned int i = 0; i < n_longs; i++) {
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

	for (unsigned int i = 0; i < n_longs - 1; i++) {
		res = *(a + i) - *(b + i);
		if (res != 0) {
			return 0;
		}
	}

	unsigned int remaining_attributes = n_attributes % LONG_BITS;

	if (remaining_attributes == 0) {
		// Nothing more to check
		return 1;
	}

	unsigned long mask = ~0L;
	mask >>= remaining_attributes;
	mask = ~mask;

	if ((a[n_longs - 1] & mask) != (b[n_longs - 1] & mask)) {
		return 0;
	}
	return 1;
}

/**
 * Replaces the class bits with jnsq bits
 * It's ok, because the number of jnsq bits is always <= bits needed for class
 * And we don't need the class anymore because we extracted it to the
 * dataset_line structure
 */
void set_jnsq(unsigned long *line, unsigned long inconsistency) {

	// Check how many attributes remain
	unsigned int remaining_attributes = n_attributes % LONG_BITS;

	// Jnsq starts after this bit
	unsigned int jnsq_start = LONG_BITS - remaining_attributes
			- n_bits_for_class;

	unsigned long last_long = line[n_longs - 1];

	last_long = set_bits(last_long, inconsistency, jnsq_start,
			n_bits_for_class);

	line[n_longs - 1] = last_long;
}

/**
 * Adds the JNSQs attributes to the dataset
 */
unsigned long add_jnsqs(unsigned long *dataset) {

	// Current line
	unsigned long *line = dataset;

	//Previous line
	unsigned long *last = line;

	// Max inconsistency found
	unsigned long max_inconsistency = 0;

	// Inconsistency
	unsigned long inconsistency = 0;

	// First line has jnsq=0
	set_jnsq(line, inconsistency);

	for (unsigned long i = 0; i < n_observations - 1; i++) {

		NEXT(line);

		if (has_same_attributes(line, last)) {
			// Inconsistency!
			// - Because the list is ordered and free of duplicates
			inconsistency++;

			// Update max
			if (inconsistency > max_inconsistency) {
				max_inconsistency = inconsistency;
			}
		} else {
			// Differente attributes - reset JNSQ
			inconsistency = 0;
		}

		// Set the line JNSQ
		set_jnsq(line, inconsistency);

		NEXT(last);
	}

	return max_inconsistency;
}

/**
 * Removes duplicated lines from the dataset.
 * Assumes the dataset is ordered
 */
unsigned long remove_duplicates(unsigned long *dataset) {

	unsigned long *line = dataset;
	unsigned long *last = line;

	unsigned long n_uniques = 1;

	for (unsigned long i = 0; i < n_observations - 1; i++) {
		NEXT(line);
		if (compare_lines(line, last) != 0) {
			NEXT(last);
			n_uniques++;
			if (last != line) {
				memcpy(last, line, sizeof(unsigned long) * n_longs);
			}
		}
	}

	// Update number of unique observations
	n_observations = n_uniques;
	return n_observations;
}

/**
 * Fill the arrays with the number os items per class and also a matrix with
 * references to the lines that belong to eacv class to simplify the
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

	for (unsigned long i = 0; i < n_observations; i++) {
		clas = get_class(line);

		observations_per_class[clas * n_classes + n_items_per_class[clas]] =
				line;

		n_items_per_class[clas]++;

		NEXT(line);
	}
}

/**
 * Calculates and adds the current lines cost to the cost array
 */
void increase_cost_line(const unsigned long *a, const unsigned long *b,
		unsigned long *cost) {

	// Current attribute
	unsigned long c = 0;

	unsigned long xored = 0;

	for (unsigned int n = 0; n < n_longs && c < n_attributes; n++) {
		xored = a[n] ^ b[n];
		for (int bit = LONG_BITS - 1; c < n_attributes && bit >= 0; bit--) {
			if (xored & mask_table[bit]) {
				// Disagreement
				// Add to cost
				cost[c]++;
			}
			c++;
		}
	}
}

/**
 * Calculates the cost of the full (virtual) disjoint matrix
 */
void calculate_initial_cost(unsigned long **observations_per_class,
		const unsigned long *n_items_per_class, unsigned long *cost) {

	unsigned int i, j;
	unsigned long n_i, n_j;

	for (i = 0; i < n_classes - 1; i++) {
		for (j = i + 1; j < n_classes; j++) {

			for (n_i = i * n_classes;
					n_i < i * n_classes + n_items_per_class[i]; n_i++) {

				for (n_j = j * n_classes;
						n_j < j * n_classes + n_items_per_class[j]; n_j++) {

					increase_cost_line(observations_per_class[n_i],
							observations_per_class[n_j], cost);
				}
			}
		}
	}
}

/**
 * Calculates the cost of the attributes that aren't blacklisted (removed)
 * and subtracts the cost from the cost matrix
 */
void decrease_cost_line(const unsigned long *a, const unsigned long *b,
		const unsigned long attribute_to_blacklist,
		const unsigned char *blacklist, unsigned long *cost) {

	unsigned int n = attribute_to_blacklist / LONG_BITS;
	unsigned int r = LONG_BITS - 1 - attribute_to_blacklist % LONG_BITS;

	// Current attribute
	unsigned long c = 0;

	unsigned long xored = a[n] ^ b[n];

	if (xored & mask_table[r]) {
		// These lines contributed to the blacklisted attribute
		// Remove them from the cost array

		c = 0;

		for (unsigned int n = 0; n < n_longs && c < n_attributes; n++) {

			xored = a[n] ^ b[n];

			for (int bit = LONG_BITS - 1; c < n_attributes && bit >= 0; bit--) {

				if ((blacklist[c] == 0) && (xored & mask_table[bit])) {
					// Disagreement
					// Remove from cost

					// TODO: this if shouldn't be necessary
					// but I'm getting negative values here...
					if (cost[c] > 0) {
						cost[c]--;
					}
				}
				c++;
			}
		}
	}
}

/**
 *  We don't want to build the full disjoint matrix
 *  So we check again which line combinations contributed to the
 *  blacklisted attribute and remove their contribution from the cost array
 */
void decrease_cost_blacklisted_attribute(unsigned long **observations_per_class,
		const unsigned long *n_items_per_class,
		const unsigned long attribute_to_blacklist,
		const unsigned char *blacklist, unsigned long *cost) {

	unsigned int i, j;
	unsigned long n_i, n_j;

	for (i = 0; i < n_classes - 1; i++) {
		for (j = i + 1; j < n_classes; j++) {

			for (n_i = i * n_classes;
					n_i < i * n_classes + n_items_per_class[i]; n_i++) {

				for (n_j = j * n_classes;
						n_j < j * n_classes + n_items_per_class[j]; n_j++) {

					decrease_cost_line(observations_per_class[n_i],
							observations_per_class[n_j], attribute_to_blacklist,
							blacklist, cost);
				}
			}
		}
	}
}
