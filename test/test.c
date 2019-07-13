/**
 * @file
 * @brief Test touch
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test.h"

/**
 * Path to send to touch program to create and modify times.
 */
#define PATH_TMP_FILE "/tmp/test-touch.txt"

/**
 * Path to file that does not exist and do not have permissions to create.
 */
#define PATH_NOEXIST  "/noexist.txt"

/**
 * Path to file used as a reference.
 */
#define PATH_REF_FILE "/etc/hosts"

/**
 * Number of arguments in @ref g_argv.
 */
static int
g_argc;

/**
 * Argument list to pass to touch program.
 */
static char **
g_argv;

/**
 * Remove @ref PATH_TMP_FILE.
 */
static void
test_remove_tmp_file(void){
  remove(PATH_TMP_FILE);
}

/**
 * Check if a file exists.
 *
 * @param[in] path  File path.
 * @retval    true  File exists.
 * @retval    false File does not exist.
 */
static bool
test_file_exists(const char *const path){
  bool exists;

  if(access(path, F_OK) == 0){
    exists = true;
  }
  else{
    exists = false;
  }
  return exists;
}

/**
 * Check and assert time values in tm structure.
 *
 * @param[in] tm            Broken-down time structure.
 * @param[in] expect_year   Expected year.
 * @param[in] expect_month  Expected month.
 * @param[in] expect_day    Expected day of month.
 * @param[in] expect_hour   Expected hour.
 * @param[in] expect_minute Expected minute.
 * @param[in] expect_second Expected second.
 */
static void
test_assert_tm(const struct tm *const tm,
               const int expect_year,
               const int expect_month,
               const int expect_day,
               const int expect_hour,
               const int expect_minute,
               const int expect_second){
  assert(tm->tm_year + 1900 == expect_year);
  assert(tm->tm_mon + 1  == expect_month);
  assert(tm->tm_mday == expect_day);
  assert(tm->tm_hour == expect_hour);
  assert(tm->tm_min  == expect_minute);
  assert(tm->tm_sec  == expect_second);
}

/**
 * Call @ref touch_main with the global argument parameters.
 *
 * @param[in] a                  See @ref TOUCH_FLAG_ACCESS_TIME.
 * @param[in] c                  See @ref TOUCH_FLAG_NO_CREATE.
 * @param[in] m                  See @ref TOUCH_FLAG_MOD_TIME.
 * @param[in] ref_file           See @ref TOUCH_FLAG_REF_FILE.
 * @param[in] time               See @ref TOUCH_FLAG_TIME.
 * @param[in] date_time          See @ref TOUCH_FLAG_DATE_TIME.
 * @param[in] expect_exit_status Expected exit code from @ref touch_main.
 * @param[in] file_list          List of files to touch. Terminate list
 *                               using NULL.
 */
static void
test_touch_main(const bool a,
                const bool c,
                const bool m,
                const char *const ref_file,
                const char *const time,
                const char *const date_time,
                const int expect_exit_status,
                const char *const file_list, ...){
  int rc;
  va_list ap;
  const char *file;

  g_argc = 0;
  strcpy(g_argv[g_argc++], "touch");
  if(a){
    strcpy(g_argv[g_argc++], "-a");
  }
  if(c){
    strcpy(g_argv[g_argc++], "-c");
  }
  if(m){
    strcpy(g_argv[g_argc++], "-m");
  }
  if(ref_file){
    strcpy(g_argv[g_argc++], "-r");
    strcpy(g_argv[g_argc++], ref_file);
  }
  if(time){
    strcpy(g_argv[g_argc++], "-t");
    strcpy(g_argv[g_argc++], time);
  }
  if(date_time){
    strcpy(g_argv[g_argc++], "-d");
    strcpy(g_argv[g_argc++], date_time);
  }
  va_start(ap, file_list);
  for(file = file_list; file; file = va_arg(ap, const char *const)){
    strcpy(g_argv[g_argc++], file);
  }
  va_end(ap);
  optind = 0;
  rc = touch_main(g_argc, g_argv);
  assert(rc == expect_exit_status);
}

/**
 * Run the touch command and return the time info of the touched file.
 *
 * @param[in]  ref_file  See @ref TOUCH_FLAG_REF_FILE.
 * @param[in]  time      See @ref TOUCH_FLAG_TIME.
 * @param[in]  date_time See @ref TOUCH_FLAG_DATE_TIME.
 * @param[out] tm_local  Local time after running touch.
 * @param[out] tm_utc    UTC time after running touch.
 * @param[out] tv_nsec   Nanoseconds after running touch.
 */
static void
test_touch_main_get_result(const char *const ref_file,
                           const char *const time,
                           const char *const date_time,
                           struct tm **const tm_local,
                           struct tm **const tm_utc,
                           long *const tv_nsec){
  struct stat sb;

  test_remove_tmp_file();
  test_touch_main(false,
                  false,
                  false,
                  ref_file,
                  time,
                  date_time,
                  EXIT_SUCCESS,
                  PATH_TMP_FILE,
                  NULL);
  assert(stat(PATH_TMP_FILE, &sb) == 0);
  test_remove_tmp_file();
  assert(memcmp(&sb.st_atim, &sb.st_mtim, sizeof(sb.st_atim)) == 0);
  if(tm_local){
    *tm_local = localtime(&sb.st_atim.tv_sec);
    assert(*tm_local);
  }
  if(tm_utc){
    *tm_utc = gmtime(&sb.st_atim.tv_sec);
    assert(*tm_utc);
  }
  if(tv_nsec){
    *tv_nsec = sb.st_atim.tv_nsec;
  }
}

/**
 * Test cases from POSIX examples section.
 */
static void
test_touch_posix_examples(void){
  struct tm *tm;
  long tv_nsec;
  struct stat sb_ref;
  struct stat sb_tmp;

  /* Local time. */
  test_touch_main_get_result(NULL,
                             NULL,
                             "2007-11-12T10:15:30",
                             &tm,
                             NULL,
                             &tv_nsec);
  test_assert_tm(tm, 2007, 11, 12, 10, 15, 30);
  assert(tv_nsec == 0);

  /* UTC time. */
  test_touch_main_get_result(NULL,
                             NULL,
                             "2007-11-12T10:15:30Z",
                             NULL,
                             &tm,
                             &tv_nsec);
  test_assert_tm(tm, 2007, 11, 12, 10, 15, 30);
  assert(tv_nsec == 0);

  /* Local time - fractional second. */
  test_touch_main_get_result(NULL,
                             NULL,
                             "2007-11-12T10:15:30,002",
                             &tm,
                             NULL,
                             &tv_nsec);
  test_assert_tm(tm, 2007, 11, 12, 10, 15, 30);
  assert(tv_nsec == 2000000);

  /* UTC time - fractional second. */
  test_touch_main_get_result(NULL,
                             NULL,
                             "2007-11-12T10:15:30,002Z",
                             NULL,
                             &tm,
                             &tv_nsec);
  test_assert_tm(tm, 2007, 11, 12, 10, 15, 30);
  assert(tv_nsec == 2000000);

  /* Time without second specifier. */
  test_touch_main_get_result(NULL,
                             "200711121015",
                             NULL,
                             &tm,
                             NULL,
                             &tv_nsec);
  test_assert_tm(tm, 2007, 11, 12, 10, 15, 0);
  assert(tv_nsec == 0);

  /* Time with second specifier. */
  test_touch_main_get_result(NULL,
                             "200711121015.30",
                             NULL,
                             &tm,
                             NULL,
                             &tv_nsec);
  test_assert_tm(tm, 2007, 11, 12, 10, 15, 30);
  assert(tv_nsec == 0);

  /* Time without century specifier. */
  test_touch_main_get_result(NULL,
                             "0711121015.30",
                             NULL,
                             &tm,
                             NULL,
                             &tv_nsec);
  test_assert_tm(tm, 2007, 11, 12, 10, 15, 30);
  assert(tv_nsec == 0);

  /* Update file with access time of reference file. */
  assert(stat(PATH_REF_FILE, &sb_ref) == 0);
  test_remove_tmp_file();
  test_touch_main(true,
                  false,
                  false,
                  PATH_REF_FILE,
                  NULL,
                  NULL,
                  EXIT_SUCCESS,
                  PATH_TMP_FILE,
                  NULL);
  assert(stat(PATH_TMP_FILE, &sb_tmp) == 0);
  assert(memcmp(&sb_ref.st_atim, &sb_tmp.st_atim, sizeof(sb_ref.st_atim)) == 0);
  assert(memcmp(&sb_ref.st_mtim, &sb_tmp.st_mtim, sizeof(sb_ref.st_mtim)) != 0);
  test_remove_tmp_file();
}

/**
 * Test scenarios with [-r ref_file].
 */
static void
test_touch_ref_file_all(void){
  struct stat sb_ref;
  struct stat sb_tmp;

  assert(stat(PATH_REF_FILE, &sb_ref) == 0);
  test_remove_tmp_file();

  test_touch_main(false,
                  false,
                  false,
                  PATH_REF_FILE,
                  NULL,
                  NULL,
                  EXIT_SUCCESS,
                  PATH_TMP_FILE,
                  NULL);
  assert(stat(PATH_TMP_FILE, &sb_tmp) == 0);
  assert(memcmp(&sb_ref.st_atim, &sb_tmp.st_atim, sizeof(sb_ref.st_atim)) == 0);
  assert(memcmp(&sb_ref.st_mtim, &sb_tmp.st_mtim, sizeof(sb_ref.st_mtim)) == 0);
  test_remove_tmp_file();

  /* Update access and modification time (-a & -m). */
  test_touch_main(true,
                  false,
                  true,
                  PATH_REF_FILE,
                  NULL,
                  NULL,
                  EXIT_SUCCESS,
                  PATH_TMP_FILE,
                  NULL);
  assert(stat(PATH_TMP_FILE, &sb_tmp) == 0);
  assert(memcmp(&sb_ref.st_atim, &sb_tmp.st_atim, sizeof(sb_ref.st_atim)) == 0);
  assert(memcmp(&sb_ref.st_mtim, &sb_tmp.st_mtim, sizeof(sb_ref.st_mtim)) == 0);
  test_remove_tmp_file();

  /* Only update access time (-a). */
  test_touch_main(true,
                  false,
                  false,
                  PATH_REF_FILE,
                  NULL,
                  NULL,
                  EXIT_SUCCESS,
                  PATH_TMP_FILE,
                  NULL);
  assert(stat(PATH_TMP_FILE, &sb_tmp) == 0);
  assert(memcmp(&sb_ref.st_atim, &sb_tmp.st_atim, sizeof(sb_ref.st_atim)) == 0);
  assert(memcmp(&sb_ref.st_mtim, &sb_tmp.st_mtim, sizeof(sb_ref.st_mtim)) != 0);
  test_remove_tmp_file();

  /* Only update modification time (-m). */
  test_touch_main(false,
                  false,
                  true,
                  PATH_REF_FILE,
                  NULL,
                  NULL,
                  EXIT_SUCCESS,
                  PATH_TMP_FILE,
                  NULL);
  assert(stat(PATH_TMP_FILE, &sb_tmp) == 0);
  assert(memcmp(&sb_ref.st_atim, &sb_tmp.st_atim, sizeof(sb_ref.st_atim)) != 0);
  assert(memcmp(&sb_ref.st_mtim, &sb_tmp.st_mtim, sizeof(sb_ref.st_mtim)) == 0);
  test_remove_tmp_file();

  /* Reference file does not exist. */
  test_touch_main(false,
                  false,
                  false,
                  PATH_NOEXIST,
                  NULL,
                  NULL,
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);
}

/**
 * Test scenarios with [-d date_time].
 */
static void
test_touch_date_time_all(void){
  struct tm *tm;
  long tv_nsec;

  /* Use ' ' instead of 'T'. */
  test_touch_main_get_result(NULL,
                             NULL,
                             "2019-01-01 09:05:00",
                             &tm,
                             NULL,
                             NULL);
  test_assert_tm(tm, 2019, 1, 1, 9, 5, 0);

  test_touch_main_get_result(NULL,
                             NULL,
                             "2007-11-12T10:15:30,12345",
                             &tm,
                             NULL,
                             &tv_nsec);
  test_assert_tm(tm, 2007, 11, 12, 10, 15, 30);
  assert(tv_nsec == 123450000);

  test_touch_main_get_result(NULL,
                             NULL,
                             "2007-11-12T10:15:30,000",
                             &tm,
                             NULL,
                             &tv_nsec);
  test_assert_tm(tm, 2007, 11, 12, 10, 15, 30);
  assert(tv_nsec == 0);

  /* Date string too small. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "2007-11-12T10:15:3",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);

  /* Date string too long. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "2007-11-12T10:15:30Z1",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);

  /* strptime: Failed to parse date string. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "200a-11-12T10:15:30",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);

  /* setenv: Failed to set TZ environment variable. */
  g_test_seam_err_ctr_setenv = 0;
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "2007-11-12T10:15:30Z",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);
  g_test_seam_err_ctr_setenv = -1;

  /* mktime.*/
  g_test_seam_err_ctr_mktime = 0;
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "2007-11-12T10:15:30Z",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);
  g_test_seam_err_ctr_mktime = -1;

  /* Invalid frac. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "2007-11-12T10:15:30,a002",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);

  /* malloc: Failed to allocate frac_str. */
  g_test_seam_err_ctr_malloc = 0;
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "2007-11-12T10:15:30,002",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);
  g_test_seam_err_ctr_malloc = -1;

  /* strtod: Failed to parse frac. */
  g_test_seam_err_ctr_strtod = 0;
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "2007-11-12T10:15:30,002",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);
  g_test_seam_err_ctr_strtod = -1;
}

/**
 * Test scenarios with [-t time].
 */
static void
test_touch_time_all(void){
  /* Time string too small. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  "071112101",
                  NULL,
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);

  /* Time string too large. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  "200711121015.301",
                  NULL,
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);

  /* mktime.*/
  g_test_seam_err_ctr_mktime = 0;
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  "200711121015.30",
                  NULL,
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);
  g_test_seam_err_ctr_mktime = -1;
}

/**
 * Update time on a directory.
 */
static void
test_touch_directory(void){
  struct stat sb;
  struct tm *tm;
  const char *const PATH_TMP_DIR = "/tmp/test-touch-dir";

  assert(mkdir(PATH_TMP_DIR, S_IRUSR | S_IWUSR) == 0);
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "2019-01-01T09:05:00",
                  EXIT_SUCCESS,
                  PATH_TMP_DIR,
                  NULL);
  assert(stat(PATH_TMP_DIR, &sb) == 0);
  assert(rmdir(PATH_TMP_DIR) == 0);
  assert(memcmp(&sb.st_atim, &sb.st_mtim, sizeof(sb.st_atim)) == 0);
  tm = localtime(&sb.st_atim.tv_sec);
  assert(tm);
  test_assert_tm(tm, 2019, 1, 1, 9, 5, 0);
}

/**
 * Touch multiple files at once.
 */
static void
test_touch_multi_files(void){
  const char *const PATH_TMP_FILE_2 = "/tmp/test-touch-2.txt";
  struct stat sb_tmp_1;
  struct stat sb_tmp_2;

  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  "2019-01-01T09:05:00",
                  EXIT_SUCCESS,
                  PATH_TMP_FILE,
                  PATH_TMP_FILE_2,
                  NULL);
  assert(stat(PATH_TMP_FILE, &sb_tmp_1) == 0);
  assert(stat(PATH_TMP_FILE_2, &sb_tmp_2) == 0);
  assert(memcmp(&sb_tmp_1.st_atim,
                &sb_tmp_2.st_atim,
                sizeof(sb_tmp_1.st_atim)) == 0);
  assert(memcmp(&sb_tmp_1.st_mtim,
                &sb_tmp_2.st_mtim,
                sizeof(sb_tmp_1.st_mtim)) == 0);
  test_remove_tmp_file();
  assert(remove(PATH_TMP_FILE_2) == 0);
}

/**
 * Run all test cases for touch.
 */
static void
test_touch_all(void){
  test_remove_tmp_file();

  /* Invalid argument. */
  g_argc = 0;
  strcpy(g_argv[g_argc++], "touch");
  strcpy(g_argv[g_argc++], "-z");
  assert(touch_main(g_argc, g_argv) == EXIT_FAILURE);

  /* Missing file operand. */
  test_touch_main(false, false, false, NULL, NULL, NULL, EXIT_FAILURE, NULL);

  /* -r and -t provided. */
  test_touch_main(false,
                  false,
                  false,
                  PATH_REF_FILE,
                  "200711121015",
                  NULL,
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);

  /* -r and -d provided. */
  test_touch_main(false,
                  false,
                  false,
                  PATH_REF_FILE,
                  NULL,
                  "2007-11-12T10:15:30",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);

  /* -t and -d provided. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  "200711121015",
                  "2007-11-12T10:15:30",
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);

  /* Try to touch an existing file without the appropriate permissions. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  NULL,
                  EXIT_FAILURE,
                  "/etc/hosts",
                  NULL);

  /* Fail to update file after creat - futimens. */
  g_test_seam_err_ctr_futimens = 0;
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  NULL,
                  EXIT_FAILURE,
                  PATH_TMP_FILE,
                  NULL);
  g_test_seam_err_ctr_futimens = -1;
  test_remove_tmp_file();

  /* Unable to open file because we lack permission. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  NULL,
                  EXIT_FAILURE,
                  "/etc/shadow",
                  NULL);

  /* Unable to creat file because we lack directory permission. */
  test_touch_main(false,
                  false,
                  false,
                  NULL,
                  NULL,
                  NULL,
                  EXIT_FAILURE,
                  PATH_NOEXIST,
                  NULL);

  /* Do not create file if it does not exist. */
  test_touch_main(false,
                  true,
                  false,
                  NULL,
                  NULL,
                  NULL,
                  EXIT_SUCCESS,
                  PATH_TMP_FILE,
                  NULL);
  assert(test_file_exists(PATH_TMP_FILE) == false);
  test_remove_tmp_file();

  test_touch_posix_examples();
  test_touch_ref_file_all();
  test_touch_date_time_all();
  test_touch_time_all();
  test_touch_directory();
  test_touch_multi_files();
}

/**
 * Test touch program.
 *
 * Usage: test
 *
 * @retval 0 All tests passed.
 */
int
main(void){
  const size_t MAX_ARGS = 20;
  const size_t MAX_ARG_LEN = 255;
  size_t i;

  g_argv = malloc(MAX_ARGS * sizeof(g_argv));
  assert(g_argv);
  for(i = 0; i < MAX_ARGS; i++){
    g_argv[i] = malloc(MAX_ARG_LEN * sizeof(*g_argv));
    assert(g_argv[i]);
  }
  test_touch_all();
  for(i = 0; i < MAX_ARGS; i++){
    free(g_argv[i]);
  }
  free(g_argv);
  return 0;
}

