/*
 ============================================================================
 Name        : globals.h
 Author      : Eduardo Ribeiro
 Description : Global variables definitions
 ============================================================================
 */

#ifndef GLOBALS_H__
#define GLOBALS_H__

#include "hdf5.h"
#include <stdint.h>

//
#define DEBUG 1

#define status_t uint_fast8_t

#define NOK 0
#define OK 1

#define NOT_BLACKLISTED 0
#define BLACKLISTED 1

// These are global variables that I would like to reduce/remove in future
/**
 * Dataset dimensions
 */
extern hsize_t g_dimensions[2];

/**
 * Number of attributes
 */
extern uint_fast32_t g_n_attributes;

/**
 * Number of observations
 */
extern uint_fast32_t g_n_observations;

/**
 * Number of classes
 */
extern uint_fast8_t g_n_classes;

/**
 * Number of bits used to store the class
 */
extern uint_fast8_t g_n_bits_for_class;

/**
 * Number of longs needed to store one line of the dataset
 */
extern uint_fast32_t g_n_longs;

#endif
