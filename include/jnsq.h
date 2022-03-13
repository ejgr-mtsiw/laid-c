/*
 ============================================================================
 Name        : jnsq.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage JNSQ
 ============================================================================
 */

#ifndef JNSQ_H
#define JNSQ_H

#include "bit_utils.h"
#include "dataset.h"
#include "globals.h"
#include <stdint.h>

/**
 * Replaces the class bits with jnsq bits
 * It's ok, because the number of jnsq bits is always <= bits needed for class
 * And we don't need the class anymore because we extracted it to the
 * dataset_line structure.
 * And we fill the bits in reverse jnsq=1 with 3bits = 1 0 0
 * So the extra bits are zeroed and can be ignored in the calculations
 *
 * Also inconsistency = class
 * See Apolónia, J., & Cavique, L. (2019). Seleção de Atributos de Dados
 * Inconsistentes em ambiente HDF5+ Python na cloud INCD. Revista de
 * Ciências da Computação, 85-112.
 */
void set_jnsq_bits(uint_fast64_t *line, uint_fast8_t inconsistency);

/**
 * Compares 2 lines and updates jnsq on to_update if needed and updates
 * inconsistency level
 */
void update_jnsq(uint_fast64_t *to_update, const uint_fast64_t *to_compare,
		uint_fast8_t *inconsistency);

/**
 * Adds the JNSQs attributes to the dataset.
 * Returns max inconsistency found
 */
uint_fast8_t add_jnsqs(uint_fast64_t *dataset);

#endif
