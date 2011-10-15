#ifndef  LEPTONICA__STDIO_H
#define  LEPTONICA__STDIO_H

#ifndef  BUILD_HOST

#include <stdio.h>
#include <stdint.h>

typedef struct cookie_io_functions_t {
  ssize_t (*read)(void *cookie, char *buf, size_t n);
  ssize_t (*write)(void *cookie, const char *buf, size_t n);
  int (*seek)(void *cookie, off_t *pos, int whence);
  int (*close)(void *cookie);
} cookie_io_functions_t;

FILE *fopencookie(void *cookie, const char *mode, cookie_io_functions_t functions);

FILE *fmemopen(void *buf, size_t size, const char *mode);

FILE *open_memstream(char **buf, size_t *size);

#endif

#endif  /* LEPTONICA__STDIO_H */
