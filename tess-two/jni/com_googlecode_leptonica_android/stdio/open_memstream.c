/* Copyright (C) 2007 Eric Blake
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 *
 * Modifications for Android written Jul 2009 by Alan Viverette
 */

/*
FUNCTION
<<open_memstream>>---open a write stream around an arbitrary-length string

INDEX
	open_memstream

ANSI_SYNOPSIS
	#include <stdio.h>
	FILE *open_memstream(char **restrict <[buf]>,
			     size_t *restrict <[size]>);

DESCRIPTION
<<open_memstream>> creates a seekable <<FILE>> stream that wraps an
arbitrary-length buffer, created as if by <<malloc>>.  The current
contents of *<[buf]> are ignored; this implementation uses *<[size]>
as a hint of the maximum size expected, but does not fail if the hint
was wrong.  The parameters <[buf]> and <[size]> are later stored
through following any call to <<fflush>> or <<fclose>>, set to the
current address and usable size of the allocated string; although
after fflush, the pointer is only valid until another stream operation
that results in a write.  Behavior is undefined if the user alters
either *<[buf]> or *<[size]> prior to <<fclose>>.

The stream is write-only, since the user can directly read *<[buf]>
after a flush; see <<fmemopen>> for a way to wrap a string with a
readable stream.  The user is responsible for calling <<free>> on
the final *<[buf]> after <<fclose>>.

Any time the stream is flushed, a NUL byte is written at the current
position (but is not counted in the buffer length), so that the string
is always NUL-terminated after at most *<[size]> bytes.  However, data
previously written beyond the current stream offset is not lost, and
the NUL byte written during a flush is restored to its previous value
when seeking elsewhere in the string.

RETURNS
The return value is an open FILE pointer on success.  On error,
<<NULL>> is returned, and <<errno>> will be set to EINVAL if <[buf]>
or <[size]> is NULL, ENOMEM if memory could not be allocated, or
EMFILE if too many streams are already open.

PORTABILITY
This function is being added to POSIX 200x, but is not in POSIX 2001.

Supporting OS subroutines required: <<sbrk>>.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include "extrastdio.h"

/* Describe details of an open memstream.  */
typedef struct memstream {
  void *storage; /* storage to free on close */
  char **pbuf; /* pointer to the current buffer */
  size_t *psize; /* pointer to the current size, smaller of pos or eof */
  size_t pos; /* current position */
  size_t eof; /* current file size */
  size_t max; /* current malloc buffer size, always > eof */
  char saved; /* saved character that lived at *psize before NUL */
} memstream;

/* Write up to non-zero N bytes of BUF into the stream described by COOKIE,
   returning the number of bytes written or EOF on failure.  */
static int
memwrite(void *cookie, const char *buf, int n)
{
  memstream *c = (memstream *) cookie;
  char *cbuf = *c->pbuf;

  /* size_t is unsigned, but off_t is signed.  Don't let stream get so
     big that user cannot do ftello.  */
  if (sizeof (off_t) == sizeof (size_t) && (ssize_t) (c->pos + n) < 0)
    {
      return EOF;
    }
  /* Grow the buffer, if necessary.  Choose a geometric growth factor
     to avoid quadratic realloc behavior, but use a rate less than
     (1+sqrt(5))/2 to accomodate malloc overhead.  Overallocate, so
     that we can add a trailing \0 without reallocating.  The new
     allocation should thus be max(prev_size*1.5, c->pos+n+1). */
  if (c->pos + n >= c->max)
    {
      size_t newsize = c->max * 3 / 2;
      if (newsize < c->pos + n + 1)
	newsize = c->pos + n + 1;
      cbuf = realloc (cbuf, newsize);
      if (! cbuf)
	return EOF; /* errno already set to ENOMEM */
      *c->pbuf = cbuf;
      c->max = newsize;
    }
  /* If we have previously done a seek beyond eof, ensure all
     intermediate bytes are NUL.  */
  if (c->pos > c->eof)
    memset (cbuf + c->eof, '\0', c->pos - c->eof);
  memcpy (cbuf + c->pos, buf, n);
  c->pos += n;
  /* If the user has previously written further, remember what the
     trailing NUL is overwriting.  Otherwise, extend the stream.  */
  if (c->pos > c->eof)
    c->eof = c->pos;
  else
    c->saved = cbuf[c->pos];
  cbuf[c->pos] = '\0';
  *c->psize = c->pos;
  return n;
}

/* Seek to position POS relative to WHENCE within stream described by
   COOKIE; return resulting position or fail with EOF.  */
static fpos_t
memseek(void *cookie, fpos_t pos, int whence)
{
  memstream *c = (memstream *) cookie;
  off_t offset = (off_t) pos;

  if (whence == SEEK_CUR)
    offset += c->pos;
  else if (whence == SEEK_END)
    offset += c->eof;
  if (offset < 0)
    {
      offset = -1;
    }
  else if ((size_t) offset != offset)
    {
      offset = -1;
    }
  else
    {
      if (c->pos < c->eof)
	{
	  (*c->pbuf)[c->pos] = c->saved;
	  c->saved = '\0';
	}
      c->pos = offset;
      if (c->pos < c->eof)
	{
	  c->saved = (*c->pbuf)[c->pos];
	  (*c->pbuf)[c->pos] = '\0';
	  *c->psize = c->pos;
	}
      else
	*c->psize = c->eof;
    }
  return (fpos_t) offset;
}

/* Reclaim resources used by stream described by COOKIE.  */
static int
memclose(void *cookie)
{
  memstream *c = (memstream *) cookie;
  char *buf;

  /* Be nice and try to reduce any unused memory.  */
  buf = realloc (*c->pbuf, *c->psize + 1);
  if (buf)
    *c->pbuf = buf;
  free (c->storage);
  return 0;
}

/* Open a memstream that tracks a dynamic buffer in BUF and SIZE.
   Return the new stream, or fail with NULL.  */
FILE *
open_memstream(char **buf, size_t *size)
{
  FILE *fp;
  memstream *c;
  int flags;

  if (!buf || !size)
    {
      return NULL;
    }
  if ((fp = (FILE *) __sfp ()) == NULL)
    return NULL;
  if ((c = (memstream *) malloc (sizeof *c)) == NULL)
    {
      fp->_flags = 0;		/* release */
      return NULL;
    }

  /* Use *size as a hint for initial sizing, but bound the initial
     malloc between 64 bytes (same as asprintf, to avoid frequent
     mallocs on small strings) and 64k bytes (to avoid overusing the
     heap if *size was garbage).  */
  c->max = *size;
  if (c->max < 64)
    c->max = 64;
  else if (c->max > 64 * 1024)
    c->max = 64 * 1024;
  *size = 0;
  *buf = malloc (c->max);
  if (!*buf)
    {
      fp->_flags = 0;		/* release */
      free (c);
      return NULL;
    }
  **buf = '\0';

  c->storage = c;
  c->pbuf = buf;
  c->psize = size;
  c->eof = 0;
  c->saved = '\0';

  fp->_file = -1;
  fp->_flags = __SWR;
  fp->_cookie = c;
  fp->_read = NULL;
  fp->_write = memwrite;
  fp->_seek = memseek;
  fp->_close = memclose;

  return fp;
}

