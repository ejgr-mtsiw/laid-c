/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Basic implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

//
#define DEBUG
//
#include "clargs.h"
#include "bit_utils.h"
#include "cost.h"
#include "dataset.h"
#include "globals.h"
#include "hdf5.h"
#include "jnsq.h"
#include <stdio.h>
#include <stdlib.h>

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

	/**
	 * Array to store the costs for each atribute
	 */
	unsigned long *cost = NULL;

	/**
	 * Array to store the solution attributes
	 * Also works as a sort of blacklist: every attribute that's part
	 * of the solution is ignored in the next round of calculating costs
	 */
	unsigned char *solution = NULL;

	/**
	 * Number of attributes blacklisted
	 */
	//unsigned long n_blacklisted_attributes = 0;

	/**
	 * Array to store the blacklisted attributes
	 * This array is for fast checking the blaclisted attributes: the solution
	 * is a list of all the attributes selected this one stores a flag
	 * indicating if the blacklisted[i] attribute is blacklisted (1) or not (0)
	 */
	//unsigned char *blacklisted = NULL;


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
	fprintf(stdout, "Dataset file opened\n");

	dataset_id = H5Dopen2(file_id, args.datasetname, H5P_DEFAULT);
	if (dataset_id < 1) {
		// Error creating file
		fprintf(stderr, "Dataset %s not found!\n", args.datasetname);
		return EXIT_FAILURE;
	}
	fprintf(stdout, "Dataset opened\n");

	/**
	 * Read dataset attributes
	 */
	if (read_attributes(dataset_id) != OK) {
		fprintf(stderr, "Error readings attributes from dataset\n");
		return EXIT_FAILURE;
	}

	fprintf(stdout, " - classes = %d \n", g_n_classes);
	fprintf(stdout, " - observations = %lu \n", g_n_observations);
	fprintf(stdout, " - attributes = %lu \n", g_n_attributes);

	// Allocate main buffer
	// https://vorpus.org/blog/why-does-calloc-exist/
	dataset = (unsigned long*) calloc(g_dimensions[0] * g_dimensions[1],
			sizeof(unsigned long));
	if (dataset == NULL) {
		fprintf(stderr, "Error allocating dataset\n");
		return EXIT_FAILURE;
	}

	// Fill dataset from hdf5 file
	herr_t status = H5Dread(dataset_id, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL,
	H5P_DEFAULT, dataset);
	if (status != 0) {
		fprintf(stderr, "Error reading the dataset data\n");
		return EXIT_FAILURE;
	}

	DEBUG_PRINT_DATASET(stdout, "Initial data", dataset, WITH_EXTRA_BITS)

	// Sort dataset
	qsort(dataset, g_n_observations, g_n_longs * sizeof(unsigned long),
			compare_lines);

	DEBUG_PRINT_DATASET(stdout, "Sorted data", dataset, WITH_EXTRA_BITS)

	// remove duplicates
	unsigned long old_n = g_n_observations;
	fprintf(stdout, "Removing duplicates:\n");
	remove_duplicates(dataset);
	fprintf(stdout, " - %lu duplicate(s) removed\n", old_n - g_n_observations);

	DEBUG_PRINT_DATASET(stdout, "Unique observations", dataset, WITH_EXTRA_BITS)

	// FIll class arrays
	fprintf(stdout, "Checking classes:\n");

	n_items_per_class = (unsigned long*) calloc(g_n_classes,
			sizeof(unsigned long));
	if (n_items_per_class == NULL) {
		fprintf(stderr, "Error allocating n_items_per_class\n");
		return EXIT_FAILURE;
	}

	// TODO: should we replace the static matrix by pointers so each
	// class has n items? Right now we waste at least half the matrix space
	observations_per_class = (unsigned long**) calloc(
			g_n_classes * g_n_observations, sizeof(unsigned long*));

	fill_class_arrays(dataset, n_items_per_class, observations_per_class);

	for (unsigned int i = 0; i < g_n_classes; i++) {
		fprintf(stdout, " - class %d: %lu item(s)\n", i, n_items_per_class[i]);
	}

	// Set JNSQ
	fprintf(stdout, "Setting up JNSQ attributes:\n");
	unsigned int max_jnsq = add_jnsqs(dataset);
	fprintf(stdout, " - Max JNSQ: %d\n", max_jnsq);

	// Update number of attributes to include the new JNSQs
	if (max_jnsq > 0) {
		// How many bits are needed for jnsq attributes
		unsigned int n_bits_for_jnsq = ceil(log2(max_jnsq + 1));

		g_n_attributes += n_bits_for_jnsq;

		fprintf(stdout, " - Added %d columns for JNSQ!\n", n_bits_for_jnsq);
	}

	DEBUG_PRINT_DATASET(stdout, "Data + jnsq\n", dataset, WITHOUT_EXTRA_BITS)

	// Do LAD

	// Allocate cost array
	cost = calloc(g_n_attributes, sizeof(long));
	if (cost == NULL) {
		fprintf(stderr, "Error allocating cost array\n");
		return EXIT_FAILURE;
	}

	/**
	 * Allocate solution/blacklist array
	 */
	solution = calloc(g_n_attributes, sizeof(unsigned char));

	/**
	 * Allocate blacklisted array
	 */
	//blacklisted = calloc(g_n_attributes, sizeof(unsigned char));

	// Compare the lines of one class with all the others and get the number
	// of disagreements for each attribute.
	// Store the disagreement in the cost array.

	/*
	 //DEBUG
	 for (unsigned int i = 0; i < g_n_classes; i++) {
	 for (unsigned long n_i = i * g_n_observations;
	 n_i < i * g_n_observations + n_items_per_class[i]; n_i++) {
	 print_line(stdout, observations_per_class[n_i], WITHOUT_EXTRA_BITS);
	 printf(" %d\n", i);
	 }
	 }
	 */

	// Calculate initial cost
	fprintf(stdout, "Running main loop:\n");

	calculate_initial_cost(observations_per_class, n_items_per_class, cost);

	DEBUG_PRINT_COST(stdout, "Initial cost",cost)

	/**
	 * Max cost in the array.
	 * Program will end when this is 0
	 */
	unsigned long max_cost = 0;

	/**
	 * Attribute to add to blacklist/solution in this round
	 * It's the attribute with the highest cost
	 */
	unsigned long attribute_to_blacklist = 0;

	do {

		// Select attribute to blacklist / add to solution
		max_cost = 0;
		attribute_to_blacklist = 0;

		for (unsigned long i = 0; i < g_n_attributes; i++) {
			if (cost[i] > max_cost) {
				max_cost = cost[i];
				attribute_to_blacklist = i;
			}
		}

		printf(" - Max. cost : %lu\n", max_cost);

		if (max_cost == 0) {
			// Nothing else to do here: we have a solution that covers the
			// full disjoint matrix
			break;
		}

		// Update solution and blacklist attribute with max cost
		solution[attribute_to_blacklist] = 1;
		cost[attribute_to_blacklist] = 0;

		printf("  - Blacklisted: %lu\n", attribute_to_blacklist + 1);

		// Calculate the cost of all the lines in the disjoint matrix that had
		// the blacklisted atribute set, and subtract it from the current cost
		decrease_cost_blacklisted_attribute(observations_per_class,
				n_items_per_class, attribute_to_blacklist, solution, cost);

		DEBUG_PRINT_COST(stdout, "Current cost", cost);

	} while (max_cost > 0);

	// Profit!
	fprintf(stdout, "Solution: { ");
	for (unsigned long i = 0; i < g_n_attributes; i++) {
		if (solution[i]) {
			// This attribute is set so it's part of the solution
			printf("%lu ", i + 1);
		}
	}
	printf("}\n");

	// Cleanup
	free(solution);
	free(cost);

	free(n_items_per_class);
	free(observations_per_class);
	free(dataset);

	H5Dclose(dataset_id);
	H5Fclose(file_id);

	fprintf(stdout, "All done!\n");

	return EXIT_SUCCESS;
}
