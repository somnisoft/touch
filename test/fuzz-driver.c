/**
 * @file
 * @brief Fuzz testing.
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

/**
 * Number of arguments sent to the touch program.
 */
#define ARGC (4)

/**
 * Fuzz test the date_time string.
 *
 * Usage: fuzz-driver
 *
 * @retval 0 All tests passed.
 */
int
main(void){
  const char *const PATH_TMP_FILE = "/tmp/touch-fuzz.txt";
  char *argv[ARGC];
  FILE *fp;
  char *date_time;
  size_t bufsz;
  size_t buflen;
  size_t bytes_read;
  size_t i;
  size_t arg_alloc_sz;

  fp = stdin;
  date_time = NULL;
  bufsz = 0;
  buflen = 0;
  do{
    bufsz += 1000;
    date_time = realloc(date_time, bufsz);
    assert(date_time);
    bytes_read = fread(&date_time[buflen], 1, bufsz - buflen, fp);
    buflen += bytes_read;
    assert(ferror(fp) == 0);
  } while(!feof(fp));
  date_time[buflen] = '\0';

  argv[0] = "touch";
  argv[1] = "-d";
  argv[2] = date_time;
  argv[3] = PATH_TMP_FILE;
  touch_main(ARGC, argv);
  free(date_time);
  return 0;
}

