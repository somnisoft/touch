/**
 * @file
 * @brief Test seams
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include "test.h"

/**
 * Error counter for @ref test_seam_futimens.
 */
int g_test_seam_err_ctr_futimens = -1;

/**
 * Error counter for @ref test_seam_malloc.
 */
int g_test_seam_err_ctr_malloc = -1;

/**
 * Error counter for @ref test_seam_mktime.
 */
int g_test_seam_err_ctr_mktime = -1;

/**
 * Error counter for @ref test_seam_setenv.
 */
int g_test_seam_err_ctr_setenv = -1;

/**
 * Error counter for @ref test_seam_strtod.
 */
int g_test_seam_err_ctr_strtod = -1;

/**
 * Error counter for @ref test_seam_utimensat.
 */
int g_test_seam_err_ctr_utimensat = -1;

/**
 * Decrement an error counter until it reaches -1.
 *
 * After a counter reaches -1, it will return a true response. This gets
 * used by the test suite to denote when to cause a function to fail. For
 * example, the unit test might need to cause the malloc() function to fail
 * after calling it a third time. In that case, the counter should initially
 * get set to 2 and will get decremented every time this function gets called.
 *
 * @param[in,out] err_ctr Error counter to decrement.
 * @retval        true    The counter has reached -1.
 * @retval        false   The counter has been decremented, but did not reach
 *                        -1 yet.
 */
static bool
test_seam_dec_err_ctr(int *const err_ctr){
  bool reached_end;

  reached_end = false;
  if(*err_ctr >= 0){
    *err_ctr -= 1;
    if(*err_ctr < 0){
      reached_end = true;
    }
  }
  return reached_end;
}

/**
 * Control when futimens() fails.
 *
 * @param[in] fd    File descriptor.
 * @param[in] times Access and modification times.
 * @retval    0     Successfully updated times.
 * @retval    -1    Failed to update times.
 */
int
test_seam_futimens(int fd,
                   const struct timespec times[2]){
  int rc;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_futimens)){
    errno = EINVAL;
    rc = -1;
  }
  else{
    rc = futimens(fd, times);
  }
  return rc;
}

/**
 * Control when malloc() fails.
 *
 * @param[in] size  Number of bytes to allocate.
 * @retval    void* Newly allocated memory.
 * @retval    NULL  Failed to allocate memory.
 */
void *
test_seam_malloc(size_t size){
  void *alloc;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_malloc)){
    errno = ENOMEM;
    alloc = NULL;
  }
  else{
    alloc = malloc(size);
  }
  return alloc;
}

/**
 * Control when mktime() fails.
 *
 * @param[in] timeptr Broken-down time structure.
 * @return            Time since the epoch, or (time_t)-1 if error.
 */
time_t
test_seam_mktime(struct tm *timeptr){
  time_t time_since_epoch;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_mktime)){
    errno = EOVERFLOW;
    time_since_epoch = (time_t)-1;
  }
  else{
    time_since_epoch = mktime(timeptr);
  }
  return time_since_epoch;
}

/**
 * Control when setenv() fails.
 *
 * @param[in] envname   Environment variable name.
 * @param[in] envval    Set environment variable to this value.
 * @param[in] overwrite Overwrite the existing environment variable value.
 * @retval    0         Successfully set environment variable.
 * @retval    -1        Failed to set environment variable.
 */
int
test_seam_setenv(const char *envname,
                 const char *envval,
                 int overwrite){
  int rc;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_setenv)){
    errno = EINVAL;
    rc = -1;
  }
  else{
    rc = setenv(envname, envval, overwrite);
  }
  return rc;
}

/**
 * Convert a string to a double value.
 *
 * @param[in]  nptr   String to convert to double.
 * @param[out] endptr Points to last character in @p nptr after parse.
 * @return            Double value or 0 if error (check errno).
 */
double
test_seam_strtod(const char *nptr,
                 char **endptr){
  double d;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_strtod)){
    errno = ERANGE;
    d = 0;
  }
  else{
    d = strtod(nptr, endptr);
  }
  return d;
}

/**
 * Control when utimensat() fails.
 *
 * @param[in] fd    Change @p path relative to this directory.
 * @param[in] path  Change times on this file.
 * @param[in] times Access and modification times.
 * @param[in] flag  Flags controlling the behavior of utimensat().
 * @retval    0     Successfully updated times.
 * @retval    -1    Failed to update times.
 */
int
test_seam_utimensat(int fd,
                    const char *path,
                    const struct timespec times[2],
                    int flag){
  int rc;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_utimensat)){
    errno = EINVAL;
    rc = -1;
  }
  else{
    rc = utimensat(fd, path, times, flag);
  }
  return rc;
}

