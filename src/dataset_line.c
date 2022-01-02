/*
 ============================================================================
 Name        : dataset_line.c
 Author      : Eduardo Ribeiro
 Description : Structure to store info about one line of the dataset
 ============================================================================
 */

#include "dataset_line.h"

/**
 * Extracts the class value from a line read from the hdf5 dataset
 */
unsigned int get_class(const unsigned long *buffer, const unsigned long n_attributes, unsigned int n_bits_for_classes) {

	// Number of longs filled only with attributes
	int n_longs = n_attributes / LONG_BITS;

	// Check how many attributes remain
	int remaining_attributes = n_attributes % LONG_BITS;

	// Class starts here
	int at = LONG_BITS - remaining_attributes - n_bits_for_classes;

	unsigned int class_id = (unsigned int) get_bits(buffer[n_longs], at, n_bits_for_classes);

	return class_id;
}

/**
 * Copies one dataset_line info to another
 */
void dataset_line_copy(dataset_line *to, const dataset_line *from) {

	to->class_id = from->class_id;
	to->inconsistency = from->inconsistency;
	to->data = from->data;
}

/**
 * Replaces the class bits with jnsq bits
 * It's ok, because the number of jnsq bits is always <= bits needed for class
 * And we don't need the class anymore because we extracted it to the
 * dataset_line structure
 */
void set_jnsq(dataset_line *new_line, const unsigned long n_attributes, const unsigned int bits_for_jnsq) {

	// Number of longs filled only with attributes
	unsigned int n_longs = n_attributes / LONG_BITS;

	// Check how many attributes remain
	unsigned int remaining_attributes = n_attributes % LONG_BITS;

	// Jnsq starts after this bit
	unsigned int jnsq_start = LONG_BITS - remaining_attributes - bits_for_jnsq;

	unsigned long last_long = new_line->data[n_longs];

	last_long = set_bits(last_long, new_line->inconsistency, jnsq_start, bits_for_jnsq);

	new_line->data[n_longs] = last_long;
}

/**
 * Checks if the dataset lines have the same attributes
 */
int has_same_attributes(const dataset_line *a, const dataset_line *b, const unsigned long n_attributes) {

	// Number of longs filled only with attributes
	int n_longs = n_attributes / LONG_BITS;

	// We only have attributes here so we can fast check
	for (int i = 0; i < n_longs - 1; i++) {
		if (a->data[i] != b->data[i]) {
			// They're differente
			return 0;
		}
	}

	// Check the remaining attributes
	unsigned int remaining_attributes = n_attributes % LONG_BITS;

	if (remaining_attributes == 0) {
		// Nothing more to check
		return 1;
	}

	unsigned long mask = 0xffffffffffffffff;
	mask >>= remaining_attributes;
	mask = ~mask;

	unsigned long la = a->data[n_longs] & mask;
	unsigned long lb = b->data[n_longs] & mask;

	if (la != lb) {
		// They're differente
		return 0;
	}

	// Every attribute is the same
	return 1;
}

/**
 * Checks if the dataset lines have the same class
 */
int has_same_class(const dataset_line *a, const dataset_line *b) {
	return a->class_id == b->class_id;
}

/**
 * Prints a line to stdout
 */
void print_line(const dataset_line *line, const unsigned long n_attributes) {

	// Number of longs that have attributes
	unsigned int n_longs = n_attributes / LONG_BITS + (n_attributes % LONG_BITS != 0);

	// Current attribute
	unsigned long column = 0;

	for (unsigned int i = 0; i < n_longs; i++) {
		for (unsigned int j = 0; j < LONG_BITS; j++) {
			if (column < n_attributes) {
				if (j % 8 == 0) {
					printf(" ");
				}

				printf("%d", CHECK_BIT(line->data[i], LONG_BITS - 1 - j));
			} else {
				// done!
				break;
			}
			column++;
		}

		if (i == 0 && n_longs > 2) {
			// Too many to write, let's jump to the end
			i = n_longs - 2;
			printf(" ... ");
		}
	}

	printf(" | ");
	printf("i: %d", line->inconsistency);

	printf(" | ");
	printf("c: %d\n", line->class_id);
}
