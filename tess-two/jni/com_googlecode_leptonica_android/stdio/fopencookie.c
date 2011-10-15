/* Copyright (C) 2007 Eric Blake
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 *
 * Modifications for Android written Jul 2009 by Alan Viverette
 */

/*
FUNCTION
<<fopencookie>>---open a stream with custom callbacks

INDEX
	fopencookie

ANSI_SYNOPSIS
	#include <stdio.h>
	typedef ssize_t (*cookie_read_function_t)(void *_cookie, char *_buf,
						  size_t _n);
	typedef ssize_t (*cookie_write_function_t)(void *_cookie,
						   const char *_buf, size_t _n);
	typedef int (*cookie_seek_function_t)(void *_cookie, off_t *_off,
					      int _whence);
	typedef int (*cookie_close_function_t)(void *_cookie);
	FILE *fopencookie(const void *<[cookie]>, const char *<[mode]>,
			  cookie_io_functions_t <[functions]>);

DESCRIPTION
<<fopencookie>> creates a <<FILE>> stream where I/O is performed using
custom callbacks.  The callbacks are registered via the structure:

.	typedef struct
.	{
.		cookie_read_function_t	*read;
.		cookie_write_function_t *write;
.		cookie_seek_function_t	*seek;
.		cookie_close_function_t *close;
.	} cookie_io_functions_t;

The stream is opened with <[mode]> treated as in <<fopen>>.  The
callbacks <[functions.read]> and <[functions.write]> may only be NULL
when <[mode]> does not require them.

<[functions.read]> should return -1 on failure, or else the number of
bytes read (0 on EOF).  It is similar to <<read>>, except that
<[cookie]> will be passed as the first argument.

<[functions.write]> should return -1 on failure, or else the number of
bytes written.  It is similar to <<write>>, except that <[cookie]>
will be passed as the first argument.

<[functions.seek]> should return -1 on failure, and 0 on success, with
*<[_off]> set to the current file position.  It is a cross between
<<lseek>> and <<fseek>>, with the <[_whence]> argument interpreted in
the same manner.  A NULL <[functions.seek]> makes the stream behave
similarly to a pipe in relation to stdio functions that require
positioning.

<[functions.close]> should return -1 on failure, or 0 on success.  It
is similar to <<close>>, except that <[cookie]> will be passed as the
first argument.  A NULL <[functions.close]> merely flushes all data
then lets <<fclose>> succeed.  A failed close will still invalidate
the stream.

Read and write I/O functions are allowed to change the underlying
buffer on fully buffered or line buffered streams by calling
<<setvbuf>>.  They are also not required to completely fill or empty
the buffer.  They are not, however, allowed to change streams from
unbuffered to buffered or to change the state of the line buffering
flag.  They must also be prepared to have read or write calls occur on
buffers other than the one most recently specified.

RETURNS
The return value is an open FILE pointer on success.  On error,
<<NULL>> is returned, and <<errno>> will be set to EINVAL if a
function pointer is missing or <[mode]> is invalid, ENOMEM if the
stream cannot be created, or EMFILE if too many streams are already
open.

PORTABILITY
This function is a newlib extension, copying the prototype from Linux.
It is not portable.  See also the <<funopen>> interface from BSD.

Supporting OS subroutines required: <<sbrk>>.
*/

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include "extrastdio.h"

typedef struct fccookie {
  void *cookie;
  FILE *fp;
  ssize_t	(*readfn)(void *, char *, size_t);
  ssize_t	(*writefn)(void *, const char *, size_t);
  int		(*seekfn)(void *, off_t *, int);
  int		(*closefn)(void *);
} fccookie;

static int
fcread(void *cookie, char *buf, int n)
{
  int result;
  fccookie *c = (fccookie *) cookie;
  result = c->readfn (c->cookie, buf, n);
  return result;
}

static int
fcwrite(void *cookie, const char *buf, int n)
{
  int result;
  fccookie *c = (fccookie *) cookie;
  if (c->fp->_flags & __SAPP && c->fp->_seek)
    {
      c->fp->_seek (cookie, 0, SEEK_END);
    }
  result = c->writefn (c->cookie, buf, n);
  return result;
}

static fpos_t
fcseek(void *cookie, fpos_t pos, int whence)
{
  fccookie *c = (fccookie *) cookie;
  off_t offset = (off_t) pos;

  c->seekfn (c->cookie, &offset, whence);

  return (fpos_t) offset;
}

static int
fcclose(void *cookie)
{
  int result = 0;
  fccookie *c = (fccookie *) cookie;
  if (c->closefn)
    {
      result = c->closefn (c->cookie);
    }
  free (c);
  return result;
}

FILE *
fopencookie(void *cookie, const char *mode, cookie_io_functions_t functions)
{
  FILE *fp;
  fccookie *c;
  int flags;
  int dummy;

  if ((flags = __sflags (mode, &dummy)) == 0)
    return NULL;
  if (((flags & (__SRD | __SRW)) && !functions.read)
      || ((flags & (__SWR | __SRW)) && !functions.write))
    {
      return NULL;
    }
  if ((fp = (FILE *) __sfp ()) == NULL)
    return NULL;
  if ((c = (fccookie *) malloc (sizeof *c)) == NULL)
    {
      fp->_flags = 0;
      return NULL;
    }

  fp->_file = -1;
  fp->_flags = flags;
  c->cookie = cookie;
  c->fp = fp;
  fp->_cookie = c;
  c->readfn = functions.read;
  fp->_read = fcread;
  c->writefn = functions.write;
  fp->_write = fcwrite;
  c->seekfn = functions.seek;
  fp->_seek = functions.seek ? fcseek : NULL;
  c->closefn = functions.close;
  fp->_close = fcclose;

  return fp;
}


