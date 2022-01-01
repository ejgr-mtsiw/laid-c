/*
 ============================================================================
 Name        : dataset_lines.c
 Author      : Eduardo Ribeiro
 Description : Structure to store info about the lines of the dataset
 ============================================================================
 */
#include "dataset_lines.h"

/**
 * Prepares the dataset_lines structure
 */
void setup_dataset_lines(dataset_lines *dataset_lines, unsigned long n_observations) {
	dataset_lines->first = NULL;
	dataset_lines->last = NULL;
	dataset_lines->n_lines = 0;

	// Allocate dataset lines
	dataset_lines->lines = (dataset_line*) malloc(sizeof(dataset_line) * n_observations);
}

/**
 * Checks if a line already is present and if true if it is a duplicate
 * or an inconsistent one (and updates line inconsistency)
 */
int check_duplicate_inconsistent(const dataset_lines *dataset_lines, dataset_line *new_line,
		const unsigned long n_attributes) {

	// Check for duplicates and inconsistencies
	if (dataset_lines->first != NULL) {
		for (dataset_line *current = dataset_lines->first; current <= dataset_lines->last; current++) {
			if (has_same_attributes(new_line, current, n_attributes)) {
				// Check class
				if (has_same_class(new_line, current)) {
					// It's a duplicate
					fprintf(stdout, "Duplicate!\n");
					print_line(new_line, n_attributes);
					print_line(current, n_attributes);
					return DATASET_LINE_DUPLICATE;
				} else {
					// It's an inconsistent observation
					new_line->inconsistency++;

					fprintf(stdout, "Inconsistency!\n");
					print_line(new_line, n_attributes);
					print_line(current, n_attributes);
				}
			}
		}
	}

	if (new_line->inconsistency == 0) {
		return DATASET_LINE_DISTINCT;
	}

	return DATASET_LINE_INCONSISTENT;
}

/**
 * Adds a line to the dataset
 */
void add_line(dataset_lines *dataset_lines, const dataset_line *new_line) {

	if (dataset_lines->first == NULL) {
		dataset_lines->first = dataset_lines->lines;
		dataset_lines->last = dataset_lines->lines;
	} else {
		dataset_lines->last++;
	}

	dataset_line_copy(dataset_lines->last, new_line);

	dataset_lines->n_lines++;
}
