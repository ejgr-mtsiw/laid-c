/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Basic implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

#include "clargs.h"
#include "bit_utils.h"
#include "dataset.h"
#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"

/**
 *
 */
int main(int argc, char **argv) {

	/**
	 * Command line arguments set by the user
	 */
	clargs args;

	/**
	 * File identifier
	 */
	hid_t file_id = 0;

	/**
	 * Dataset identifier
	 */
	hid_t dataset_id = 0;

	/**
	 * The dataset data
	 */
	unsigned long *dataset = NULL;

	/**
	 * Array that stores the number of observations for each class
	 */
	unsigned long *n_items_per_class = NULL;
	/**
	 * Matrix that stores the list of observations per class
	 */
	unsigned long **observations_per_class = NULL;

	// Parse command line arguments
	if (read_args(argc, argv, &args) == READ_CL_ARGS_NOK) {
		return EXIT_FAILURE;
	}

	//Open the data file
	file_id = H5Fopen(args.filename, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", args.filename);
		return EXIT_FAILURE;
	}
	fprintf(stdout, " - Dataset file opened.\n");

	dataset_id = H5Dopen2(file_id, args.datasetname, H5P_DEFAULT);
	if (dataset_id < 1) {
		// Error creating file
		fprintf(stderr, "Dataset %s not found!\n", args.datasetname);
		return EXIT_FAILURE;
	}
	fprintf(stdout, " - Dataset opened.\n");

	/**
	 * Read dataset attributes
	 */
	if (read_attributes(dataset_id) != OK) {
		fprintf(stderr, "Error readings attributes from dataset");
		return EXIT_FAILURE;
	}

	// Allocate main buffer
	// https://vorpus.org/blog/why-does-calloc-exist/
	dataset = (unsigned long*) calloc(dimensions[0] * dimensions[1],
			sizeof(unsigned long));
	if (dataset == NULL) {
		fprintf(stderr, "Error allocating dataset");
		return EXIT_FAILURE;
	}

	// Fill dataset from hdf5 file
	herr_t status = H5Dread(dataset_id, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL,
	H5P_DEFAULT, dataset);
	if (status != 0) {
		fprintf(stderr, "Error reading the dataset data");
		return EXIT_FAILURE;
	}

	print_dataset(stdout, dataset, "Initial data");

	// Sort dataset
	qsort(dataset, n_observations, n_longs * sizeof(unsigned long),
			compare_lines);
	print_dataset(stdout, dataset, "Sorted data");

	// remove duplicates
	remove_duplicates(dataset);
	print_dataset(stdout, dataset, "Removed duplicates");

	// FIll class arrays
	n_items_per_class = (unsigned long*) calloc(n_classes,
			sizeof(unsigned long));
	if (n_items_per_class == NULL) {
		fprintf(stderr, "Error allocating n_items_per_class\n");
		return EXIT_FAILURE;
	}

	// TODO: replace by pointers so each class has n items
	// Right now we waste half the matrix space
	observations_per_class = (unsigned long**) calloc(
			n_classes * n_observations, sizeof(unsigned long*));

	fill_class_arrays(dataset, n_items_per_class, observations_per_class);

	for (unsigned int i = 0; i < n_classes; i++) {
		fprintf(stdout, "Class %d: %lu items\n", i, n_items_per_class[i]);
	}

	// Set JNSQ
	unsigned long max_jnsq = add_jnsqs(dataset);
	fprintf(stdout, "Max JNSQ: %lu\n", max_jnsq);

	// If we use more than n_bits_for_jnsq we're wasting space
	// TODO: move bits to the left to remove empty column
	// or write JNSQs backwards so the zero column(s) stay to the right/end
	// of the line and can be ignored
	unsigned int n_bits_for_jnsq = ceil(log2(max_jnsq + 1));

	// Update number of attributes to include the new JNSQs
	if (max_jnsq > 0) {
		n_attributes += n_bits_for_class;

		fprintf(stdout, "Wasted %d columns on jnsq!\n",
				n_bits_for_class - n_bits_for_jnsq);
	}

	print_dataset(stdout, dataset, "Data + jnsq");

	// Do LAD

	// Calculate cost array

	// Compare the lines of one class with all the others and get the number
	// of disagreements for each attribute.
	// Store the disagreement in the cost array.
	unsigned long *cost = calloc(n_attributes, sizeof(long));
	if (cost == NULL) {
		fprintf(stderr, "Error allocating cost array\n");
		return EXIT_FAILURE;
	}

	// Pointer to the line of class X
	unsigned long *a = NULL;

	// Pointer to the line of class Y
	unsigned long *b = NULL;

	unsigned long current_attribute = 0;

	char *blacklist = calloc(n_attributes, sizeof(char));

	// Calculate initial cost
	for (unsigned int i = 0; i < n_classes - 1; i++) {
		for (unsigned int j = i + 1; j < n_classes; j++) {

			for (unsigned long k = 0; k < n_items_per_class[i]; k++) {

				a = observations_per_class[i * n_classes + k];

				for (unsigned long l = 0; l < n_items_per_class[j]; l++) {

					b = observations_per_class[j * n_classes + l];

					current_attribute = 0;

					for (unsigned int m = 0;
							m < n_longs && current_attribute < n_attributes;
							m++) {
						for (int n = LONG_BITS - 1;
								n >= 0 && current_attribute < n_attributes;
								n--) {

							if (BIT_CHECK(a[m],n) != BIT_CHECK(b[m], n)) {
								// Disagreement
								// Add to cost
								cost[current_attribute]++;
							}

							current_attribute++;
						}
					}
					/*
					 printf("Doing %c[%lu] [", letras[i], k);
					 print_line(stdout, a);
					 printf("] x %c[%lu] [ ", letras[j], l);
					 print_line(stdout, b);
					 printf("] : ");

					 for (unsigned long i = 0; i < n_attributes; i++) {
					 fprintf(stdout, "%lu ", cost[i]);
					 }
					 fprintf(stdout, "\n");
					 */
				}
			}
		}
	}

//	printf("Initial cost : ");
//
//	for (unsigned long i = 0; i < n_attributes; i++) {
//		fprintf(stdout, "%lu ", cost[i]);
//	}
//	fprintf(stdout, "\n");

	unsigned long max_cost = 0;
	unsigned long to_blacklist = 0;

	do {
		max_cost = 0;
		to_blacklist = 0;

		for (unsigned long i = 0; i < n_attributes; i++) {
			if (cost[i] > max_cost) {
				max_cost = cost[i];
				to_blacklist = i;
			}
		}

		if (max_cost == 0) {
			// Nothing else to do here
			break;
		}

		// blacklist max_cost
		blacklist[to_blacklist] = 1;
		cost[to_blacklist] = 0;

		unsigned int n_to_blacklist = to_blacklist / LONG_BITS;
		unsigned int r_to_blacklist = LONG_BITS - 1 - to_blacklist % LONG_BITS;

		// remove lines that have this attribute set
		// We don't want to build the full disjoint matrix
		// So we check again which line combinations contributed to the
		// blacklisted attribute and remote their contribution from the cost
		//array

		for (unsigned int i = 0; i < n_classes - 1; i++) {
			for (unsigned int j = i + 1; j < n_classes; j++) {

				for (unsigned long k = 0; k < n_items_per_class[i]; k++) {

					a = observations_per_class[i * n_classes + k];
					if (a == NULL) {
						continue;
					}

					for (unsigned long l = 0; l < n_items_per_class[j]; l++) {

						b = observations_per_class[j * n_classes + l];
						if (b == NULL) {
							continue;
						}

						if (BIT_CHECK(a[n_to_blacklist],
								r_to_blacklist) != BIT_CHECK(b[n_to_blacklist], r_to_blacklist)) {
							// These lines contributed to the blacklisted attribute
							// Remove them from the cost array

							current_attribute = 0;

							for (unsigned int m = 0;
									m < n_longs
											&& current_attribute < n_attributes;
									m++) {
								for (int n = LONG_BITS - 1;
										n >= 0
												&& current_attribute
														< n_attributes; n--) {

									if ((blacklist[current_attribute] == 0)
											&& (BIT_CHECK(a[m],
													n) != BIT_CHECK(b[m], n))) {
										// Disagreement
										// Remove from cost

										// TODO: this if shouldn't be necessary
										// but I'm getting negative values here...
										if (cost[current_attribute] > 0) {
											cost[current_attribute]--;
										}
									}

									current_attribute++;
								}
							}
						}
					}
				}
			}
		}

//		printf("Cost : ");
//
//		for (unsigned long i = 0; i < n_attributes; i++) {
//			fprintf(stdout, "%lu ", cost[i]);
//		}
//		fprintf(stdout, "\n");
	} while (max_cost > 0);

	// Profit!
	printf("Solution: { ");
	for (unsigned long i = 0; i < n_attributes; i++) {
		if (blacklist[i] == 1) {
			printf("%lu ", i + 1);
		}
	}
	printf("}\n");

	free(blacklist);
	free(cost);

	free(n_items_per_class);
	free(observations_per_class);
	free(dataset);

	H5Dclose(dataset_id);
	H5Fclose(file_id);

	fprintf(stdout, "All done!\n");

	return EXIT_SUCCESS;
}
