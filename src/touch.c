/**
 * @file
 * @brief Touch
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * Update file access/modification times.
 *
 * This software has been placed into the public domain using CC0.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef TOUCH_TEST
/**
 * Declare some functions with extern linkage, allowing the test suite to call
 * those functions.
 */
# define TOUCH_LINKAGE extern
# include "../test/seams.h"
#else /* !(TOUCH_TEST) */
/**
 * Define all functions as static when not testing.
 */
# define TOUCH_LINKAGE static
#endif /* TOUCH_TEST */

/**
 * Maximum buffer size allowed in the -t strptime format.
 *
 * %C%y%m%d%H%M.%S
 * 1234567890123456
 *         10    16
 */
#define MAX_TIME_FMT_LEN (16)

/**
 * Maximum buffer size allowed in the -d strptime format.
 *
 * %Y-%m-%dT%T
 * 123456789012
 *         10 12
 */
#define MAX_DATE_TIME_FMT_LEN (12)

/**
 * Maximum length for long type (nanoseconds) in fractional specifier.
 */
static size_t
MAX_FRAC_CHAR_LEN;

/**
 * @defgroup touch_flag touch flags
 *
 * Option flags when running touch.
 */

/**
 * Change file access time.
 *
 * Do not change modification time unless @ref TOUCH_FLAG_MOD_TIME (-m) also
 * set.
 *
 * This flag corresponds to argument -a.
 *
 * @ingroup touch_flag
 */
#define TOUCH_FLAG_ACCESS_TIME (1 << 0)

/**
 * Do not create the file if it does not already exist.
 *
 * This flag corresponds to argument -c.
 *
 * @ingroup touch_flag
 */
#define TOUCH_FLAG_NO_CREATE   (1 << 1)

/**
 * Change file modification time.
 *
 * Do not change access time unless @ref TOUCH_FLAG_ACCESS_TIME (-a) also set.
 *
 * This flag corresponds to argument -m.
 *
 * @ingroup touch_flag
 */
#define TOUCH_FLAG_MOD_TIME    (1 << 2)

/**
 * Use the time from a reference file.
 *
 * This flag corresponds to argument -r.
 *
 * @ingroup touch_flag
 */
#define TOUCH_FLAG_REF_FILE    (1 << 3)

/**
 * Use the time specified in the following format.
 *
 * [[CC]YY]MMDDhhmm[.SS]
 *
 * This flag corresponds to argument -t.
 *
 * @ingroup touch_flag
 */
#define TOUCH_FLAG_TIME        (1 << 4)

/**
 * Use the date_time specified in the following format.
 *
 * YYYY-MM-DDThh:mm:SS[[.|,]frac][tz]
 *
 * This flag corresopnds to argument -d.
 *
 * @ingroup touch_flag
 */
#define TOUCH_FLAG_DATE_TIME   (1 << 5)

/**
 * Touch program context.
 */
struct touch{
  /**
   * See @ref touch_flag.
   */
  unsigned int flags;

  /**
   * Exit status set to one of the following values.
   *   - EXIT_SUCCESS
   *   - EXIT_FAILURE
   */
  int status_code;

  /**
   * Access and modifications times to set.
   */
  struct timespec time_am[2];
};

/**
 * Print an error message to STDERR and set an error status code.
 *
 * @param[in,out] touch     See @ref touch.
 * @param[in]     errno_msg Include a standard message describing errno.
 * @param[in]     fmt       Format string used by vwarnx.
 */
static void
touch_warn(struct touch *const touch,
            const bool errno_msg,
            const char *const fmt, ...){
  va_list ap;

  touch->status_code = EXIT_FAILURE;
  va_start(ap, fmt);
  if(errno_msg){
    vwarn(fmt, ap);
  }
  else{
    vwarnx(fmt, ap);
  }
  va_end(ap);
}

/**
 * Parse the frac part of a date_time string.
 *
 * @param[in,out] touch      See @ref touch.
 * @param[in,out] time_parse Pointer to next character in parse string. This
 *                           pointer will get updated to point to the last
 *                           character after the frac.
 */
static void
touch_parse_frac(struct touch *const touch,
                 const char **time_parse){
  /*
   * Convert the fractional second part to nanoseconds stored in tv_nsec.
   *                          3  2  1
   *                          ||||||||||
   */
  const long CONV_FRAC_NSEC = 1000000000;
  char *ep;
  char *frac_str;
  size_t i;
  double frac;
  long tv_nsec;

  if(**time_parse == '.' || **time_parse == ','){
    frac_str = malloc(MAX_FRAC_CHAR_LEN + 1);
    if(frac_str == NULL){
      touch_warn(touch, true, "malloc: frac_str");
    }
    else{
      *time_parse += 1;
      frac_str[0] = '.';
      for(i = 1; isdigit((*time_parse)[i - 1]) && i < MAX_FRAC_CHAR_LEN; i++){
        frac_str[i] = (*time_parse)[i - 1];
      }
      frac_str[i] = '\0';
      errno = 0;
      frac = strtod(frac_str, &ep);
      if(errno != 0){
        touch_warn(touch, true, "failed to parse frac");
      }
      else{
        *time_parse += (ep - frac_str - 1);
        tv_nsec = (long)(frac * CONV_FRAC_NSEC);
        touch->time_am[0].tv_nsec = tv_nsec;
        touch->time_am[1].tv_nsec = tv_nsec;
      }
      free(frac_str);
    }
  }
}

/**
 * Parse a date time string [-d date_string] in the following format.
 *
 * YYYY-MM-DDThh:mm:SS[[.|,]frac][tz]
 *
 * frac = nanoseconds
 * tz = blank or Z
 *
 * @param[in,out] touch         See @ref touch.
 * @param[in]     date_time_str String with the format described above.
 */
static void
touch_parse_date_time(struct touch *const touch,
                      const char *const date_time_str){
  /*
   * YYYY-MM-DDThh:mm:SS
   * 1234567890123456789
   *         10       19
   */
  const size_t MIN_DATE_TIME = 19;
  /*
   * frac = maximum length for long type (nanoseconds)
   * YYYY-MM-DDThh:mm:SS,fracZ
   * 12345678901234567890 + ceil(log10(LONG_MAX)) + 1
   *         10        20 + ceil(log10(LONG_MAX)) + 1
   */
  size_t MAX_DATE_TIME;
  /*
   * YYYY-MM-DDThh:mm:SS[.frac][tz]
   *           ^
   * 01234567890
   *          10
   */
  const size_t DATE_TIME_T_POS = 10;
  struct tm tm = {0};
  size_t slen;
  char fmt[MAX_DATE_TIME_FMT_LEN];
  const char *time_parse;
  time_t tv_sec;

  MAX_FRAC_CHAR_LEN = ((size_t)(log10(LONG_MAX) + 1));
  MAX_DATE_TIME = 20 + MAX_FRAC_CHAR_LEN + 1;

  slen = strlen(date_time_str);
  if(slen < MIN_DATE_TIME ||
     slen >= MAX_DATE_TIME ||
     (date_time_str[DATE_TIME_T_POS] != 'T' &&
      date_time_str[DATE_TIME_T_POS] != ' ')){
    touch_warn(touch, false, "invalid date_time");
  }
  else{
    /*
     * Set the position of the time indicator in the format string.
     *           012345678
     *           ||||||||| */
    strcpy(fmt, "%Y-%m-%d %T");
    fmt[8] = date_time_str[DATE_TIME_T_POS];
    tm.tm_isdst = -1;
    time_parse = strptime(date_time_str, fmt, &tm);
    if(time_parse == NULL){
      touch_warn(touch, true, "failed to parse date_time");
    }
    else{
      touch_parse_frac(touch, &time_parse);
      if(*time_parse == 'Z'){
        if(setenv("TZ", "UTC", 1) != 0){
          touch_warn(touch, true, "failed to set TZ");
        }
        time_parse += 1;
      }
      if(touch->status_code != 0 || *time_parse != '\0'){
        touch_warn(touch, false, "failed to parse date_time");
      }
      else{
        tv_sec = mktime(&tm);
        if(tv_sec == (time_t)-1){
          touch_warn(touch, true, "mktime");
        }
        touch->time_am[0].tv_sec = tv_sec;
        touch->time_am[1].tv_sec = tv_sec;
      }
    }
  }
}

/**
 * Parse a time string [-t time] in the following format.
 *
 * [[CC]YY]MMDDhhmm[.SS]
 *
 * @param[in,out] touch    See @ref touch.
 * @param[in]     time_str String with the format described above.
 */
static void
touch_parse_time(struct touch *const touch,
                 const char *const time_str){
  /*
   * MMDDhhmm
   * 12345678
   *        8
   */
  const size_t MIN_TIME_LEN = 8;
  /*
   * CCYYMMDDhhmm.SS
   * 123456789012345
   *         10   15
   */
  const size_t MAX_TIME_LEN = 15;
  struct tm tm;
  char fmt[MAX_TIME_FMT_LEN];
  size_t slen;
  size_t slen_minus_sec;
  const char *time_parse;
  char *fmt_append;
  bool has_dot;
  time_t tv_sec;

  fmt[0] = '\0';
  memset(&tm, 0, sizeof(tm));

  slen = strlen(time_str);
  if(slen < MIN_TIME_LEN || slen > MAX_TIME_LEN){
    touch_warn(touch, false, "invalid time string");
  }
  else{
    slen_minus_sec = slen;
    if(strchr(time_str, '.')){
      has_dot = true;
      slen_minus_sec -= 3;
    }
    else{
      has_dot = false;
    }
    fmt_append = fmt;
    if(slen_minus_sec == 12){
      fmt_append = stpcpy(fmt_append, "%C");
    }
    if(slen_minus_sec >= 10){
      fmt_append = stpcpy(fmt_append, "%y");
    }
    fmt_append = stpcpy(fmt_append, "%m%d%H%M");
    if(has_dot){
      stpcpy(fmt_append, ".%S");
    }
    time_parse = strptime(time_str, fmt, &tm);
    if(time_parse == NULL || *time_parse != '\0'){
      touch_warn(touch, true, "invalid time string");
    }
    else{
      tv_sec = mktime(&tm);
      if(tv_sec == (time_t)-1){
        touch_warn(touch, true, "mktime");
      }
      touch->time_am[0].tv_sec = tv_sec;
      touch->time_am[1].tv_sec = tv_sec;
    }
  }
}

/**
 * Get the access/modification time of an existing reference file.
 *
 * @param[in,out] touch See @ref touch.
 * @param[in]     path  Path to reference file.
 */
static void
touch_get_time_ref_file(struct touch *const touch,
                        const char *const path){
  struct stat sb;

  if(stat(path, &sb) < 0){
    touch_warn(touch, true, "stat reference file: %s", path);
  }
  else{
    touch->time_am[0] = sb.st_atim;
    touch->time_am[1] = sb.st_mtim;
  }
}

/**
 * Touch a file.
 *
 * @param[in,out] touch See @ref touch.
 * @param[in]     path  Path to a file to touch.
 */
static void
touch_path(struct touch *const touch,
           const char *const path){
  int fd;
  const mode_t cm = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

  fd = open(path, O_RDONLY);
  if(fd < 0){
    if(errno != ENOENT){
      touch_warn(touch, true, "open: %s", path);
    }
    else{
      if(!(touch->flags & TOUCH_FLAG_NO_CREATE)){
        fd = creat(path, cm);
        if(fd < 0){
          touch_warn(touch, true, "creat: %s", path);
        }
        else{
          if(futimens(fd, touch->time_am) != 0){
            touch_warn(touch, true, "futimens: %s", path);
          }
        }
      }
    }
  }
  else{
    if(utimensat(AT_FDCWD, path, touch->time_am, 0) != 0){
      touch_warn(touch, true, "utimensat on: %s", path);
    }
  }
  if(fd >= 0){
    close(fd);
  }
}

/**
 * Check if only one of -r, -t, and -d arguments have been provided if any.
 *
 * @param[in] touch See @ref touch.
 * @retval    true  Valid arguments.
 * @retval    false Invalid arguments.
 */
static bool
touch_ensure_args_mutually_exclusive(const struct touch *const touch){
  bool exclusive;
  unsigned int flags_exclusive;

  flags_exclusive = touch->flags & (TOUCH_FLAG_REF_FILE |
                                    TOUCH_FLAG_TIME     |
                                    TOUCH_FLAG_DATE_TIME);
  if(flags_exclusive                        &&
     flags_exclusive != TOUCH_FLAG_REF_FILE &&
     flags_exclusive != TOUCH_FLAG_TIME     &&
     flags_exclusive != TOUCH_FLAG_DATE_TIME){
    exclusive = false;
  }
  else{
    exclusive = true;
  }
  return exclusive;
}

/**
 * Main entry point for touch program.
 *
 * Usage:
 * touch [-acm] [-d date_time|-r ref_file|-t time] file...
 *
 * @param[in]     argc         Number of arguments in @p argv.
 * @param[in,out] argv         Argument list.
 * @retval        EXIT_SUCCESS Successful.
 * @retval        EXIT_FAILURE Error occurred.
 */
TOUCH_LINKAGE int
touch_main(int argc,
           char *const argv[]){
  struct touch touch;
  int i;
  int c;

  memset(&touch, 0, sizeof(touch));
  while((c = getopt(argc, argv, "acd:mr:t:")) != -1){
    switch(c){
    case 'a':
      touch.flags |= TOUCH_FLAG_ACCESS_TIME;
      break;
    case 'c':
      touch.flags |= TOUCH_FLAG_NO_CREATE;
      break;
    case 'd':
      touch_parse_date_time(&touch, optarg);
      touch.flags |= TOUCH_FLAG_DATE_TIME;
      break;
    case 'm':
      touch.flags |= TOUCH_FLAG_MOD_TIME;
      break;
    case 'r':
      touch_get_time_ref_file(&touch, optarg);
      touch.flags |= TOUCH_FLAG_REF_FILE;
      break;
    case 't':
      touch_parse_time(&touch, optarg);
      touch.flags |= TOUCH_FLAG_TIME;
      break;
    default:
      touch.status_code = EXIT_FAILURE;
      break;
    }
  }
  argc -= optind;
  argv += optind;

  if(argc < 1){
    touch_warn(&touch, false, "file... argument required");
  }
  else if(touch_ensure_args_mutually_exclusive(&touch) == false){
    touch_warn(&touch, false, "-r, -t, and -d mutually exclusive");
  }
  else if(touch.status_code == 0){
    /* Use current time if the user did not specify a time. */
    if(touch.time_am[0].tv_sec == 0){
      touch.time_am[0].tv_nsec = UTIME_NOW;
      touch.time_am[1].tv_nsec = UTIME_NOW;
    }
    if(!(touch.flags & TOUCH_FLAG_ACCESS_TIME) &&
       !(touch.flags & TOUCH_FLAG_MOD_TIME)){
      /* Update both access and modification times. */
    }
    else if(!(touch.flags & TOUCH_FLAG_ACCESS_TIME)){
      touch.time_am[0].tv_nsec = UTIME_OMIT;
    }
    else if(!(touch.flags & TOUCH_FLAG_MOD_TIME)){
      touch.time_am[1].tv_nsec = UTIME_OMIT;
    }
    for(i = 0; i < argc; i++){
      touch_path(&touch, argv[i]);
    }
  }
  return touch.status_code;
}

#ifndef TOUCH_TEST
/**
 * Main program entry point.
 *
 * @param[in]     argc See @ref touch_main.
 * @param[in,out] argv See @ref touch_main.
 * @return             See @ref touch_main.
 */
int
main(const int argc,
     char *const argv[]){
  return touch_main(argc, argv);
}
#endif /* TOUCH_TEST */

