/**
 * @file
 * @brief Print file time
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * Print access and modification times for a list of files.
 *
 * This software has been placed into the public domain using CC0.
 */

#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

/**
 * Print file tm info.
 *
 * @param[in] tm_type Local or UTC indicator.
 * @param[in] tm      Local or UTC time.
 */
static void
file_time_tm(const char *const tm_type,
             const struct tm *const tm){
  printf("%s time\n"
         "Second  : %d\n"
         "Minute  : %d\n"
         "Hour    : %d\n"
         "Day     : %d\n"
         "Month   : %d\n"
         "Year    : %d\n"
         "Weekday : %d\n"
         "Year Day: %d\n"
         "DST     : %d\n",
         tm_type,
         tm->tm_sec,
         tm->tm_min,
         tm->tm_hour,
         tm->tm_mday,
         tm->tm_mon,
         tm->tm_year,
         tm->tm_wday,
         tm->tm_yday,
         tm->tm_isdst);
}

/**
 * Print file stat info.
 *
 * @param[in] time_type Access or modification indicator.
 * @param[in] timespec  Access/modification time.
 */
static void
file_time_stat(const char *const time_type,
               const struct timespec *const timespec){
  struct tm *tm_local;
  struct tm *tm_utc;

  printf("%s time\n", time_type);
  printf("seconds: %lu\n", timespec->tv_sec);
  printf("nanoseconds: %lu\n", timespec->tv_nsec);
  tm_local = localtime(&timespec->tv_sec);
  assert(tm_local);
  file_time_tm("local", tm_local);
  tm_utc = gmtime(&timespec->tv_sec);
  assert(tm_utc);
  file_time_tm("utc", tm_utc);
}

/**
 * Query the file access and modification timestamps.
 *
 * @param[in] path Path to file.
 */
static void
file_time_path(const char *const path){
  struct stat sb;

  assert(stat(path, &sb) == 0);

  puts(path);
  file_time_stat("access", &sb.st_atim);
  file_time_stat("modification", &sb.st_mtim);
}

/**
 * Print file times from a list of files.
 *
 * Usage:
 * file-time file...
 *
 * @param[in] argc Number of arguments in @p argv.
 * @param[in] argv Argument list.
 * @retval    0    Successfully queried and printed file times.
 */
int
main(int argc,
     char *argv[]){
  argc -= 1;
  argv += 1;
  assert(argc > 0);
  while(argc){
    file_time_path(argv[0]);
    argc -= 1;
    argv += 1;
  }
  return 0;
}

