/**
 * @file
 * @brief Test seams
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */
#ifndef TOUCH_TEST_SEAMS_H
#define TOUCH_TEST_SEAMS_H

#include "test.h"

/*
 * Redefine these functions to internal test seams.
 */
#undef clock_gettime
#undef futimens
#undef malloc
#undef mktime
#undef setenv
#undef strtod
#undef utimensat

/**
 * Inject a test seam to replace futimens().
 */
#define futimens      test_seam_futimens

/**
 * Inject a test seam to replace malloc().
 */
#define malloc        test_seam_malloc

/**
 * Inject a test seam to replace mktime().
 */
#define mktime        test_seam_mktime

/**
 * Inject a test seam to replace setenv().
 */
#define setenv        test_seam_setenv

/**
 * Inject a test seam to replace strtod().
 */
#define strtod        test_seam_strtod

/**
 * Inject a test seam to replace utimensat().
 */
#define utimensat     test_seam_utimensat

#endif /* TOUCH_TEST_SEAMS_H */

