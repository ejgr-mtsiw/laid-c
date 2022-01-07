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

// These are global variables that I would like to reduce/remove in future
/**
 * Dataset dimensions
 */
extern hsize_t g_dimensions[2];

/**
 * Number of attributes
 */
extern unsigned long g_n_attributes;

/**
 * Number of observations
 */
extern unsigned long g_n_observations;

/**
 * Number of classes
 */
extern unsigned int g_n_classes;

/**
 * Number of bits used to store the class
 */
extern unsigned int g_n_bits_for_class;

/**
 * Number of longs needed to store one line of the dataset
 */
extern unsigned int g_n_longs;

#endif
