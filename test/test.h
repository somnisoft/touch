/**
 * @file
 * @brief Test touch.
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */
#ifndef TOUCH_TEST_H
#define TOUCH_TEST_H

#include <sys/stat.h>
#include <time.h>

int
touch_main(int argc,
           char *const argv[]);

int
test_seam_futimens(int fd,
                   const struct timespec times[2]);

void *
test_seam_malloc(size_t size);

time_t
test_seam_mktime(struct tm *timeptr);

int
test_seam_setenv(const char *envname,
                 const char *envval,
                 int overwrite);

double
test_seam_strtod(const char *nptr,
                 char **endptr);

int
test_seam_utimensat(int fd,
                    const char *path,
                    const struct timespec times[2],
                    int flag);

extern int g_test_seam_err_ctr_futimens;
extern int g_test_seam_err_ctr_malloc;
extern int g_test_seam_err_ctr_mktime;
extern int g_test_seam_err_ctr_setenv;
extern int g_test_seam_err_ctr_strtod;
extern int g_test_seam_err_ctr_utimensat;

#endif /* TOUCH_TEST_H */

