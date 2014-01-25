/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/


/*
 *  utils.c
 *
 *       Control of error, warning and info messages
 *           l_int32    setMsgSeverity()
 *
 *       Error return functions, invoked by macros
 *           l_int32    returnErrorInt()
 *           l_float32  returnErrorFloat()
 *           void      *returnErrorPtr()
 *
 *       Safe string procs
 *           char      *stringNew()
 *           l_int32    stringCopy()
 *           l_int32    stringReplace()
 *           l_int32    stringLength()
 *           l_int32    stringCat()
 *           char      *stringJoin()
 *           char      *stringReverse()
 *           char      *strtokSafe()
 *           l_int32    stringSplitOnToken()
 *
 *       Find and replace string and array procs
 *           char      *stringRemoveChars()
 *           l_int32    stringFindSubstr()
 *           char      *stringReplaceSubstr()
 *           char      *stringReplaceEachSubstr()
 *           L_DNA     *arrayFindEachSequence()
 *           l_int32    arrayFindSequence()
 *
 *       Safe realloc
 *           void      *reallocNew()
 *
 *       Read and write between file and memory
 *           l_uint8   *l_binaryRead()
 *           l_uint8   *l_binaryReadStream()
 *           l_int32    l_binaryWrite()
 *           l_int32    nbytesInFile()
 *           l_int32    fnbytesInFile()
 *
 *       Copy in memory
 *           l_uint8   *l_binaryCopy()
 *
 *       File copy operations
 *           l_int32    fileCopy()
 *           l_int32    fileConcatenate()
 *           l_int32    fileAppendString()
 *
 *       Test files for equivalence
 *           l_int32    filesAreIdentical()
 *
 *       Byte-swapping data conversion
 *           l_uint16   convertOnBigEnd16()
 *           l_uint32   convertOnBigEnd32()
 *           l_uint16   convertOnLittleEnd16()
 *           l_uint32   convertOnLittleEnd32()
 *
 *       Cross-platform functions for opening file streams
 *           FILE      *fopenReadStream()
 *           FILE      *fopenWriteStream()
 *
 *       Cross-platform functions that avoid C-runtime boundary crossing
 *       with Windows DLLs
 *           FILE      *lept_fopen()
 *           l_int32    lept_fclose()
 *           void       lept_calloc()
 *           void       lept_free()
 *
 *       Cross-platform file system operations in temp directories
 *           l_int32    lept_mkdir()
 *           l_int32    lept_rmdir()
 *           l_int32    lept_direxists()
 *           l_int32    lept_mv()
 *           l_int32    lept_rm()
 *           l_int32    lept_cp()
 *
 *       File name operations
 *           l_int32    splitPathAtDirectory()
 *           l_int32    splitPathAtExtension()
 *           char      *pathJoin()
 *           char      *genPathname()
 *           char      *genTempFilename()
 *           l_int32    extractNumberFromFilename()
 *
 *       File corruption operation
 *           l_int32    fileCorruptByDeletion()
 *
 *       Generate random integer in given range
 *           l_int32    genRandomIntegerInRange()
 *
 *       Simple math function
 *           l_int32    lept_roundftoi()
 *
 *       Gray code conversion
 *           l_uint32   convertBinaryToGrayCode()
 *           l_uint32   convertGrayToBinaryCode()
 *
 *       Leptonica version number
 *           char      *getLeptonicaVersion()
 *
 *       Timing
 *           void       startTimer()
 *           l_float32  stopTimer()
 *           L_TIMER    startTimerNested()
 *           l_float32  stopTimerNested()
 *           void       l_getCurrentTime()
 *           void       l_getFormattedDate()
 *
 *  Notes on cross-platform development
 *  -----------------------------------
 *  This is important if your applications must work on Windows.
 *  (1) With the exception of splitPathAtDirectory() and
 *      splitPathAtExtension(), all input pathnames must have unix separators.
 *  (2) The conversion from unix to windows pathnames happens in genPathname().
 *  (3) Use fopenReadStream() and fopenWriteStream() to open files,
 *      because these use genPathname() to find the platform-dependent
 *      filenames.  Likewise for l_binaryRead() and l_binaryWrite().
 *  (4) For moving, copying and removing files and directories that are in
 *      /tmp or subdirectories of /tmp, use the lept_*() file system
 *      shell wrappers:
 *         lept_mkdir(), lept_rmdir(), lept_mv(), lept_rm() and lept_cp().
 *  (5) Use the lept_*() C library wrappers.  These work properly on
 *      Windows, where the same DLL must perform complementary operations
 *      on file streams (open/close) and heap memory (malloc/free):
 *         lept_fopen(), lept_fclose(), lept_calloc() and lept_free().
 */

#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <dirent.h>     /* unix only */
#endif  /* ! _WIN32 */
#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif   /* _MSC_VER */
#include "allheaders.h"

#ifdef _WIN32
#include <windows.h>
static const char path_sepchar = '\\';
#else
#include <sys/stat.h>  /* for mkdir(2) */
#include <sys/types.h>
static const char path_sepchar = '/';
#endif


    /* Global for controlling message output at runtime */
LEPT_DLL l_int32  LeptMsgSeverity = DEFAULT_SEVERITY;


/*----------------------------------------------------------------------*
 *                Control of error, warning and info messages           *
 *----------------------------------------------------------------------*/
/*!
 *  setMsgSeverity()
 *
 *      Input:  newsev
 *      Return: oldsev
 *
 *  Notes:
 *      (1) setMsgSeverity() allows the user to specify the desired
 *          message severity threshold.  Messages of equal or greater
 *          severity will be output.  The previous message severity is
 *          returned when the new severity is set.
 *      (2) If L_SEVERITY_EXTERNAL is passed, then the severity will be
 *          obtained from the LEPT_MSG_SEVERITY environment variable.
 *          If the environmental variable is not set, a warning is issued.
 */
l_int32
setMsgSeverity(l_int32  newsev)
{
l_int32  oldsev;
char    *envsev;

    PROCNAME("setMsgSeverity");

    oldsev = LeptMsgSeverity;
    if (newsev == L_SEVERITY_EXTERNAL) {
        envsev = getenv("LEPT_MSG_SEVERITY");
        if (envsev) {
            LeptMsgSeverity = atoi(envsev);
            L_INFO("message severity set to external\n", procName);
        } else {
            L_WARNING("environment var LEPT_MSG_SEVERITY not defined\n",
                      procName);
        }
    } else {
        LeptMsgSeverity = newsev;
        L_INFO("message severity set to %d\n", procName, newsev);
    }

    return oldsev;
}


/*----------------------------------------------------------------------*
 *                Error return functions, invoked by macros             *
 *                                                                      *
 *    (1) These error functions print messages to stderr and allow      *
 *        exit from the function that called them.                      *
 *    (2) They must be invoked only by the macros ERROR_INT,            *
 *        ERROR_FLOAT and ERROR_PTR, which are in environ.h             *
 *    (3) The print output can be disabled at compile time, either      *
 *        by using -DNO_CONSOLE_IO or by setting LeptMsgSeverity.       *
 *----------------------------------------------------------------------*/
/*!
 *  returnErrorInt()
 *
 *      Input:  msg (error message)
 *              procname
 *              ival (return val)
 *      Return: ival (typically 1 for an error return)
 */
l_int32
returnErrorInt(const char  *msg,
               const char  *procname,
               l_int32      ival)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return ival;
}


/*!
 *  returnErrorFloat()
 *
 *      Input:  msg (error message)
 *              procname
 *              fval (return val)
 *      Return: fval
 */
l_float32
returnErrorFloat(const char  *msg,
                 const char  *procname,
                 l_float32    fval)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return fval;
}


/*!
 *  returnErrorPtr()
 *
 *      Input:  msg (error message)
 *              procname
 *              pval  (return val)
 *      Return: pval (typically null)
 */
void *
returnErrorPtr(const char  *msg,
               const char  *procname,
               void        *pval)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return pval;
}


/*--------------------------------------------------------------------*
 *                       Safe string operations                       *
 *--------------------------------------------------------------------*/
/*!
 *  stringNew()
 *
 *      Input:  src string
 *      Return: dest copy of src string, or null on error
 */
char *
stringNew(const char  *src)
{
l_int32  len;
char    *dest;

    PROCNAME("stringNew");

    if (!src)
        return (char *)ERROR_PTR("src not defined", procName, NULL);

    len = strlen(src);
    if ((dest = (char *)CALLOC(len + 1, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("dest not made", procName, NULL);

    stringCopy(dest, src, len);
    return dest;
}


/*!
 *  stringCopy()
 *
 *      Input:  dest (existing byte buffer)
 *              src string (can be null)
 *              n (max number of characters to copy)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Relatively safe wrapper for strncpy, that checks the input,
 *          and does not complain if @src is null or @n < 1.
 *          If @n < 1, this is a no-op.
 *      (2) @dest needs to be at least @n bytes in size.
 *      (3) We don't call strncpy() because valgrind complains about
 *          use of uninitialized values.
 */
l_int32
stringCopy(char        *dest,
           const char  *src,
           l_int32      n)
{
l_int32  i;

    PROCNAME("stringCopy");

    if (!dest)
        return ERROR_INT("dest not defined", procName, 1);
    if (!src || n < 1)
        return 0;

        /* Implementation of strncpy that valgrind doesn't complain about */
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return 0;
}


/*!
 *  stringReplace()
 *
 *      Input:  &dest string (<return> copy)
 *              src string
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Frees any existing dest string
 *      (2) Puts a copy of src string in the dest
 *      (3) If either or both strings are null, does something reasonable.
 */
l_int32
stringReplace(char       **pdest,
              const char  *src)
{
char    *scopy;
l_int32  len;

    PROCNAME("stringReplace");

    if (!pdest)
        return ERROR_INT("pdest not defined", procName, 1);

    if (*pdest)
        FREE(*pdest);

    if (src) {
        len = strlen(src);
        if ((scopy = (char *)CALLOC(len + 1, sizeof(char))) == NULL)
            return ERROR_INT("scopy not made", procName, 1);
        stringCopy(scopy, src, len);
        *pdest = scopy;
    } else {
        *pdest = NULL;
    }

    return 0;
}


/*!
 *  stringLength()
 *
 *      Input:  src string (can be null or null-terminated string)
 *              size (size of src buffer)
 *      Return: length of src in bytes.
 *
 *  Notes:
 *      (1) Safe implementation of strlen that only checks size bytes
 *          for trailing NUL.
 *      (2) Valid returned string lengths are between 0 and size - 1.
 *          If size bytes are checked without finding a NUL byte, then
 *          an error is indicated by returning size.
 */
l_int32
stringLength(const char  *src,
             size_t       size)
{
l_int32  i;

    PROCNAME("stringLength");

    if (!src)
        return ERROR_INT("src not defined", procName, 0);
    if (size < 1)
        return 0;

    for (i = 0; i < size; i++) {
        if (src[i] == '\0')
            return i;
    }
    return size;
}


/*!
 *  stringCat()
 *
 *      Input:  dest (null-terminated byte buffer)
 *              size (size of dest)
 *              src string (can be null or null-terminated string)
 *      Return: number of bytes added to dest; -1 on error
 *
 *  Notes:
 *      (1) Alternative implementation of strncat, that checks the input,
 *          is easier to use (since the size of the dest buffer is specified
 *          rather than the number of bytes to copy), and does not complain
 *          if @src is null.
 *      (2) Never writes past end of dest.
 *      (3) If it can't append src (an error), it does nothing.
 *      (4) N.B. The order of 2nd and 3rd args is reversed from that in
 *          strncat, as in the Windows function strcat_s().
 */
l_int32
stringCat(char        *dest,
          size_t       size,
          const char  *src)
{
l_int32  i, n;
l_int32  lendest, lensrc;

    PROCNAME("stringCat");

    if (!dest)
        return ERROR_INT("dest not defined", procName, -1);
    if (size < 1)
        return ERROR_INT("size < 1; too small", procName, -1);
    if (!src)
        return 0;

    lendest = stringLength(dest, size);
    if (lendest == size)
        return ERROR_INT("no terminating nul byte", procName, -1);
    lensrc = stringLength(src, size);
    if (lensrc == 0)
        return 0;
    n = (lendest + lensrc > size - 1 ? size - lendest - 1 : lensrc);
    if (n < 1)
        return ERROR_INT("dest too small for append", procName, -1);

    for (i = 0; i < n; i++)
        dest[lendest + i] = src[i];
    dest[lendest + n] = '\0';
    return n;
}


/*!
 *  stringJoin()
 *
 *      Input:  src1 string (<optional> can be null)
 *              src2 string (<optional> can be null)
 *      Return: concatenated string, or null on error
 *
 *  Notes:
 *      (1) This is a safe version of strcat; it makes a new string.
 *      (2) It is not an error if either or both of the strings
 *          are empty, or if either or both of the pointers are null.
 */
char *
stringJoin(const char  *src1,
           const char  *src2)
{
char    *dest;
l_int32  srclen1, srclen2, destlen;

    PROCNAME("stringJoin");

    srclen1 = (src1) ? strlen(src1) : 0;
    srclen2 = (src2) ? strlen(src2) : 0;
    destlen = srclen1 + srclen2 + 3;

    if ((dest = (char *)CALLOC(destlen, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("calloc fail for dest", procName, NULL);

    if (src1)
        stringCopy(dest, src1, srclen1);
    if (src2)
        strncat(dest, src2, srclen2);
    return dest;
}


/*!
 *  stringReverse()
 *
 *      Input:  src (string)
 *      Return: dest (newly-allocated reversed string)
 */
char *
stringReverse(const char  *src)
{
char    *dest;
l_int32  i, len;

    PROCNAME("stringReverse");

    if (!src)
        return (char *)ERROR_PTR("src not defined", procName, NULL);
    len = strlen(src);
    if ((dest = (char *)CALLOC(len + 1, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("calloc fail for dest", procName, NULL);
    for (i = 0; i < len; i++)
        dest[i] = src[len - 1 - i];

    return dest;
}


/*!
 *  strtokSafe()
 *
 *      Input:  cstr (input string to be sequentially parsed;
 *                    use NULL after the first call)
 *              seps (a string of character separators)
 *              &saveptr (<return> ptr to the next char after
 *                        the last encountered separator)
 *      Return: substr (a new string that is copied from the previous
 *                      saveptr up to but not including the next
 *                      separator character), or NULL if end of cstr.
 *
 *  Notes:
 *      (1) This is a thread-safe implementation of strtok.
 *      (2) It has the same interface as strtok_r.
 *      (3) It differs from strtok_r in usage in two respects:
 *          (a) the input string is not altered
 *          (b) each returned substring is newly allocated and must
 *              be freed after use.
 *      (4) Let me repeat that.  This is "safe" because the input
 *          string is not altered and because each returned string
 *          is newly allocated on the heap.
 *      (5) It is here because, surprisingly, some C libraries don't
 *          include strtok_r.
 *      (6) Important usage points:
 *          - Input the string to be parsed on the first invocation.
 *          - Then input NULL after that; the value returned in saveptr
 *            is used in all subsequent calls.
 *      (7) This is only slightly slower than strtok_k.
 */
char *
strtokSafe(char        *cstr,
           const char  *seps,
           char       **psaveptr)
{
char     nextc;
char    *start, *substr;
l_int32  istart, i, j, nchars;

    PROCNAME("strtokSafe");

    if (!seps)
        return (char *)ERROR_PTR("seps not defined", procName, NULL);
    if (!psaveptr)
        return (char *)ERROR_PTR("&saveptr not defined", procName, NULL);

    if (!cstr)
        start = *psaveptr;
    else
        start = cstr;
    if (!start)  /* nothing to do */
        return NULL;

        /* First time, scan for the first non-sep character */
    istart = 0;
    if (cstr) {
        for (istart = 0;; istart++) {
            if ((nextc = start[istart]) == '\0') {
                *psaveptr = NULL;  /* in case caller doesn't check ret value */
                return NULL;
            }
            if (!strchr(seps, nextc))
                break;
        }
    }

        /* Scan through, looking for a sep character; if none is
         * found, 'i' will be at the end of the string. */
    for (i = istart;; i++) {
        if ((nextc = start[i]) == '\0')
            break;
        if (strchr(seps, nextc))
            break;
    }

        /* Save the substring */
    nchars = i - istart;
    substr = (char *)CALLOC(nchars + 1, sizeof(char));
    stringCopy(substr, start + istart, nchars);

        /* Look for the next non-sep character.
         * If this is the last substring, return a null saveptr. */
    for (j = i;; j++) {
        if ((nextc = start[j]) == '\0') {
            *psaveptr = NULL;  /* no more non-sep characters */
            break;
        }
        if (!strchr(seps, nextc)) {
            *psaveptr = start + j;  /* start here on next call */
                break;
        }
    }

    return substr;
}


/*!
 *  stringSplitOnToken()
 *
 *      Input:  cstr (input string to be split; not altered)
 *              seps (a string of character separators)
 *              &head (<return> ptr to copy of the input string, up to
 *                     the first separator token encountered)
 *              &tail (<return> ptr to copy of the part of the input string
 *                     starting with the first non-separator character
 *                     that occurs after the first separator is found)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The input string is not altered; all split parts are new strings.
 *      (2) The split occurs around the first consecutive sequence of
 *          tokens encountered.
 *      (3) The head goes from the beginning of the string up to
 *          but not including the first token found.
 *      (4) The tail contains the second part of the string, starting
 *          with the first char in that part that is NOT a token.
 *      (5) If no separator token is found, 'head' contains a copy
 *          of the input string and 'tail' is null.
 */
l_int32
stringSplitOnToken(char        *cstr,
                   const char  *seps,
                   char       **phead,
                   char       **ptail)
{
char  *saveptr;

    PROCNAME("stringSplitOnToken");

    if (!phead)
        return ERROR_INT("&head not defined", procName, 1);
    if (!ptail)
        return ERROR_INT("&tail not defined", procName, 1);
    *phead = *ptail = NULL;
    if (!cstr)
        return ERROR_INT("cstr not defined", procName, 1);
    if (!seps)
        return ERROR_INT("seps not defined", procName, 1);

    *phead = strtokSafe(cstr, seps, &saveptr);
    if (saveptr)
        *ptail = stringNew(saveptr);
    return 0;
}


/*--------------------------------------------------------------------*
 *                       Find and replace procs                       *
 *--------------------------------------------------------------------*/
/*!
 *  stringRemoveChars()
 *
 *      Input:  src (input string; can be of zero length)
 *              remchars  (string of chars to be removed from src)
 *      Return: dest (string with specified chars removed), or null on error
 */
char *
stringRemoveChars(const char  *src,
                  const char  *remchars)
{
char     ch;
char    *dest;
l_int32  nsrc, i, k;

    PROCNAME("stringRemoveChars");

    if (!src)
        return (char *)ERROR_PTR("src not defined", procName, NULL);
    if (!remchars)
        return stringNew(src);

    if ((dest = (char *)CALLOC(strlen(src) + 1, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("dest not made", procName, NULL);
    nsrc = strlen(src);
    for (i = 0, k = 0; i < nsrc; i++) {
        ch = src[i];
        if (!strchr(remchars, ch))
            dest[k++] = ch;
    }

    return dest;
}


/*!
 *  stringFindSubstr()
 *
 *      Input:  src (input string; can be of zero length)
 *              sub (substring to be searched for)
 *              &loc (<return optional> location of substring in src)
 *      Return: 1 if found; 0 if not found or on error
 *
 *  Notes:
 *      (1) This is a wrapper around strstr().
 *      (2) Both @src and @sub must be defined, and @sub must have
 *          length of at least 1.
 *      (3) If the substring is not found and loc is returned, it has
 *          the value -1.
 */
l_int32
stringFindSubstr(const char  *src,
                 const char  *sub,
                 l_int32     *ploc)
{
char  *ptr;

    PROCNAME("stringFindSubstr");

    if (!src)
        return ERROR_INT("src not defined", procName, 0);
    if (!sub)
        return ERROR_INT("sub not defined", procName, 0);
    if (ploc) *ploc = -1;
    if (strlen(sub) == 0)
        return ERROR_INT("substring length 0", procName, 0);
    if (strlen(src) == 0)
        return 0;

    if ((ptr = (char *)strstr(src, sub)) == NULL)  /* not found */
        return 0;

    if (ploc)
        *ploc = ptr - src;
    return 1;
}


/*!
 *  stringReplaceSubstr()
 *
 *      Input:  src (input string; can be of zero length)
 *              sub1 (substring to be replaced)
 *              sub2 (substring to put in; can be "")
 *              &found (<return optional> 1 if sub1 is found; 0 otherwise)
 *              &loc (<return optional> location of ptr after replacement)
 *      Return: dest (string with substring replaced), or null if the
 *              substring not found or on error.
 *
 *  Notes:
 *      (1) Replaces the first instance.
 *      (2) To only remove sub1, use "" for sub2
 *      (3) Returns a new string if sub1 and sub2 are the same.
 *      (4) The optional loc is input as the byte offset within the src
 *          from which the search starts, and after the search it is the
 *          char position in the string of the next character after
 *          the substituted string.
 *      (5) N.B. If ploc is not null, loc must always be initialized.
 *          To search the string from the beginning, set loc = 0.
 */
char *
stringReplaceSubstr(const char  *src,
                    const char  *sub1,
                    const char  *sub2,
                    l_int32     *pfound,
                    l_int32     *ploc)
{
char    *ptr, *dest;
l_int32  nsrc, nsub1, nsub2, len, npre, loc;

    PROCNAME("stringReplaceSubstr");

    if (!src)
        return (char *)ERROR_PTR("src not defined", procName, NULL);
    if (!sub1)
        return (char *)ERROR_PTR("sub1 not defined", procName, NULL);
    if (!sub2)
        return (char *)ERROR_PTR("sub2 not defined", procName, NULL);

    if (pfound)
        *pfound = 0;
    if (ploc)
        loc = *ploc;
    else
        loc = 0;
    if ((ptr = (char *)strstr(src + loc, sub1)) == NULL) {
        return NULL;
    }

    if (pfound)
        *pfound = 1;
    nsrc = strlen(src);
    nsub1 = strlen(sub1);
    nsub2 = strlen(sub2);
    len = nsrc + nsub2 - nsub1;
    if ((dest = (char *)CALLOC(len + 1, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("dest not made", procName, NULL);
    npre = ptr - src;
    memcpy(dest, src, npre);
    strcpy(dest + npre, sub2);
    strcpy(dest + npre + nsub2, ptr + nsub1);
    if (ploc)
        *ploc = npre + nsub2;

    return dest;
}


/*!
 *  stringReplaceEachSubstr()
 *
 *      Input:  src (input string; can be of zero length)
 *              sub1 (substring to be replaced)
 *              sub2 (substring to put in; can be "")
 *              &count (<optional return > the number of times that sub1
 *                      is found in src; 0 if not found)
 *      Return: dest (string with substring replaced), or null if the
 *              substring not found or on error.
 *
 *  Notes:
 *      (1) Replaces every instance.
 *      (2) To only remove each instance of sub1, use "" for sub2
 *      (3) Returns NULL if sub1 and sub2 are the same.
 */
char *
stringReplaceEachSubstr(const char  *src,
                        const char  *sub1,
                        const char  *sub2,
                        l_int32     *pcount)
{
char    *currstr, *newstr;
l_int32  loc;

    PROCNAME("stringReplaceEachSubstr");

    if (!src)
        return (char *)ERROR_PTR("src not defined", procName, NULL);
    if (!sub1)
        return (char *)ERROR_PTR("sub1 not defined", procName, NULL);
    if (!sub2)
        return (char *)ERROR_PTR("sub2 not defined", procName, NULL);

    if (pcount)
        *pcount = 0;
    loc = 0;
    if ((newstr = stringReplaceSubstr(src, sub1, sub2, NULL, &loc)) == NULL)
        return NULL;

    if (pcount)
        (*pcount)++;
    while (1) {
        currstr = newstr;
        newstr = stringReplaceSubstr(currstr, sub1, sub2, NULL, &loc);
        if (!newstr)
            return currstr;
        FREE(currstr);
        if (pcount)
            (*pcount)++;
    }
}


/*!
 *  arrayFindEachSequence()
 *
 *      Input:  data (byte array)
 *              datalen (length of data, in bytes)
 *              sequence (subarray of bytes to find in data)
 *              seqlen (length of sequence, in bytes)
 *      Return: dna of offsets where the sequence is found, or null if
 *              none are found or on error
 *
 *  Notes:
 *      (1) The byte arrays @data and @sequence are not C strings,
 *          as they can contain null bytes.  Therefore, for each
 *          we must give the length of the array.
 *      (2) This finds every occurrence in @data of @sequence.
 */
L_DNA *
arrayFindEachSequence(const l_uint8  *data,
                      l_int32         datalen,
                      const l_uint8  *sequence,
                      l_int32         seqlen)
{
l_int32  start, offset, realoffset, found;
L_DNA   *da;

    PROCNAME("arrayFindEachSequence");

    if (!data || !sequence)
        return (L_DNA *)ERROR_PTR("data & sequence not both defined",
                                  procName, NULL);

    da = l_dnaCreate(0);
    start = 0;
    while (1) {
        arrayFindSequence(data + start, datalen - start, sequence, seqlen,
                          &offset, &found);
        if (found == FALSE)
            break;

        realoffset = start + offset;
        l_dnaAddNumber(da, realoffset);
        start = realoffset + seqlen;
        if (start >= datalen)
            break;
    }

    if (l_dnaGetCount(da) == 0)
        l_dnaDestroy(&da);
    return da;
}


/*!
 *  arrayFindSequence()
 *
 *      Input:  data (byte array)
 *              datalen (length of data, in bytes)
 *              sequence (subarray of bytes to find in data)
 *              seqlen (length of sequence, in bytes)
 *              &offset (return> offset from beginning of
 *                       data where the sequence begins)
 *              &found (<optional return> 1 if sequence is found; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The byte arrays 'data' and 'sequence' are not C strings,
 *          as they can contain null bytes.  Therefore, for each
 *          we must give the length of the array.
 *      (2) This searches for the first occurrence in @data of @sequence,
 *          which consists of @seqlen bytes.  The parameter @seqlen
 *          must not exceed the actual length of the @sequence byte array.
 *      (3) If the sequence is not found, the offset will be set to -1.
 */
l_int32
arrayFindSequence(const l_uint8  *data,
                  l_int32         datalen,
                  const l_uint8  *sequence,
                  l_int32         seqlen,
                  l_int32        *poffset,
                  l_int32        *pfound)
{
l_int32  i, j, found, lastpos;

    PROCNAME("arrayFindSequence");

    if (!data || !sequence)
        return ERROR_INT("data & sequence not both defined", procName, 1);
    if (!poffset)
        return ERROR_INT("&offset not defined", procName, 1);

    *poffset = -1;
    if (pfound) *pfound = 0;
    lastpos = datalen - seqlen + 1;
    found = 0;
    for (i = 0; i < lastpos; i++) {
        for (j = 0; j < seqlen; j++) {
            if (data[i + j] != sequence[j])
                 break;
            if (j == seqlen - 1)
                 found = 1;
        }
        if (found)
            break;
    }

    if (found) {
        *poffset = i;
        if (pfound) *pfound = 1;
    }

    return 0;
}


/*--------------------------------------------------------------------*
 *                             Safe realloc                           *
 *--------------------------------------------------------------------*/
/*!
 *  reallocNew()
 *
 *      Input:  &indata (<optional>; nulls indata)
 *              oldsize (size of input data to be copied, in bytes)
 *              newsize (size of data to be reallocated in bytes)
 *      Return: ptr to new data, or null on error
 *
 *  Action: !N.B. (3) and (4)!
 *      (1) Allocates memory, initialized to 0
 *      (2) Copies as much of the input data as possible
 *          to the new block, truncating the copy if necessary
 *      (3) Frees the input data
 *      (4) Zeroes the input data ptr
 *
 *  Notes:
 *      (1) If newsize <=0, just frees input data and nulls ptr
 *      (2) If input ptr is null, just callocs new memory
 *      (3) This differs from realloc in that it always allocates
 *          new memory (if newsize > 0) and initializes it to 0,
 *          it requires the amount of old data to be copied,
 *          and it takes the address of the input ptr and
 *          nulls the handle.
 */
void *
reallocNew(void   **pindata,
           l_int32  oldsize,
           l_int32  newsize)
{
l_int32  minsize;
void    *indata;
void    *newdata;

    PROCNAME("reallocNew");

    if (!pindata)
        return ERROR_PTR("input data not defined", procName, NULL);
    indata = *pindata;

    if (newsize <= 0) {   /* nonstandard usage */
        if (indata) {
            FREE(indata);
            *pindata = NULL;
        }
        return NULL;
    }

    if (!indata) {  /* nonstandard usage */
        if ((newdata = (void *)CALLOC(1, newsize)) == NULL)
            return ERROR_PTR("newdata not made", procName, NULL);
        return newdata;
    }

        /* Standard usage */
    if ((newdata = (void *)CALLOC(1, newsize)) == NULL)
        return ERROR_PTR("newdata not made", procName, NULL);
    minsize = L_MIN(oldsize, newsize);
    memcpy((char *)newdata, (char *)indata, minsize);

    FREE(indata);
    *pindata = NULL;

    return newdata;
}



/*--------------------------------------------------------------------*
 *                 Read and write between file and memory             *
 *--------------------------------------------------------------------*/
/*!
 *  l_binaryRead()
 *
 *      Input:  filename
 *              &nbytes (<return> number of bytes read)
 *      Return: data, or null on error
 */
l_uint8 *
l_binaryRead(const char  *filename,
             size_t      *pnbytes)
{
l_uint8  *data;
FILE     *fp;

    PROCNAME("l_binaryRead");

    if (!filename)
        return (l_uint8 *)ERROR_PTR("filename not defined", procName, NULL);
    if (!pnbytes)
        return (l_uint8 *)ERROR_PTR("pnbytes not defined", procName, NULL);
    *pnbytes = 0;

    if ((fp = fopenReadStream(filename)) == NULL)
        return (l_uint8 *)ERROR_PTR("file stream not opened", procName, NULL);

    data = l_binaryReadStream(fp, pnbytes);
    fclose(fp);
    return data;
}


/*!
 *  l_binaryReadStream()
 *
 *      Input:  stream
 *              &nbytes (<return> number of bytes read)
 *      Return: null-terminated array, or null on error
 *              (reading 0 bytes is not an error)
 *
 *  Notes:
 *      (1) The returned array is terminated with a null byte so that
 *          it can be used to read ascii data into a proper C string.
 *      (2) Side effect: this re-positions the stream ptr to the
 *          beginning of the file.
 */
l_uint8 *
l_binaryReadStream(FILE    *fp,
                   size_t  *pnbytes)
{
l_int32   ignore;
l_uint8  *data;

    PROCNAME("l_binaryReadStream");

    if (!pnbytes)
        return (l_uint8 *)ERROR_PTR("&nbytes not defined", procName, NULL);
    *pnbytes = 0;
    if (!fp)
        return (l_uint8 *)ERROR_PTR("stream not defined", procName, NULL);

    *pnbytes = fnbytesInFile(fp);
    if ((data = (l_uint8 *)CALLOC(1, *pnbytes + 1)) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for data", procName, NULL);
    ignore = fread(data, 1, *pnbytes, fp);
    return data;
}


/*!
 *  l_binaryWrite()
 *
 *      Input:  filename (output)
 *              operation  ("w" for write; "a" for append)
 *              data  (binary data to be written)
 *              nbytes  (size of data array)
 *      Return: 0 if OK; 1 on error
 */
l_int32
l_binaryWrite(const char  *filename,
              const char  *operation,
              void        *data,
              size_t       nbytes)
{
char   actualOperation[20];
FILE  *fp;

    PROCNAME("l_binaryWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!operation)
        return ERROR_INT("operation not defined", procName, 1);
    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    if (nbytes <= 0)
        return ERROR_INT("nbytes must be > 0", procName, 1);

    if (!strcmp(operation, "w") && !strcmp(operation, "a"))
        return ERROR_INT("operation not one of {'w','a'}", procName, 1);

        /* The 'b' flag to fopen() is ignored for all POSIX
         * conforming systems.  However, Windows needs the 'b' flag. */
    stringCopy(actualOperation, operation, 2);
    strncat(actualOperation, "b", 2);

    if ((fp = fopenWriteStream(filename, actualOperation)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    fwrite(data, 1, nbytes, fp);
    fclose(fp);
    return 0;
}


/*!
 *  nbytesInFile()
 *
 *      Input:  filename
 *      Return: nbytes in file; 0 on error
 */
size_t
nbytesInFile(const char  *filename)
{
size_t  nbytes;
FILE   *fp;

    PROCNAME("nbytesInFile");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 0);
    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("stream not opened", procName, 0);
    nbytes = fnbytesInFile(fp);
    fclose(fp);
    return nbytes;
}


/*!
 *  fnbytesInFile()
 *
 *      Input:  file stream
 *      Return: nbytes in file; 0 on error
 */
size_t
fnbytesInFile(FILE  *fp)
{
size_t  nbytes, pos;

    PROCNAME("fnbytesInFile");

    if (!fp)
        return ERROR_INT("stream not open", procName, 0);

    pos = ftell(fp);          /* initial position */
    fseek(fp, 0, SEEK_END);   /* EOF */
    nbytes = ftell(fp);
    fseek(fp, pos, SEEK_SET);        /* back to initial position */
    return nbytes;
}


/*--------------------------------------------------------------------*
 *                            Copy in memory                          *
 *--------------------------------------------------------------------*/
/*!
 *  l_binaryCopy()
 *
 *      Input:  datas
 *              size (of data array)
 *      Return: datad (on heap), or null on error
 *
 *  Notes:
 *      (1) We add 4 bytes to the zeroed output because in some cases
 *          (e.g., string handling) it is important to have the data
 *          be null terminated.  This guarantees that after the memcpy,
 *          the result is automatically null terminated.
 */
l_uint8 *
l_binaryCopy(l_uint8  *datas,
             size_t    size)
{
l_uint8  *datad;

    PROCNAME("l_binaryCopy");

    if (!datas)
        return (l_uint8 *)ERROR_PTR("datas not defined", procName, NULL);

    if ((datad = (l_uint8 *)CALLOC(size + 4, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("datad not made", procName, NULL);
    memcpy(datad, datas, size);
    return datad;
}


/*--------------------------------------------------------------------*
 *                         File copy operations                       *
 *--------------------------------------------------------------------*/
/*!
 *  fileCopy()
 *
 *      Input:  srcfile (copy this file)
 *              newfile (to this file)
 *      Return: 0 if OK, 1 on error
 */
l_int32
fileCopy(const char  *srcfile,
         const char  *newfile)
{
l_int32   ret;
size_t    nbytes;
l_uint8  *data;

    PROCNAME("fileCopy");

    if (!srcfile)
        return ERROR_INT("srcfile not defined", procName, 1);
    if (!newfile)
        return ERROR_INT("newfile not defined", procName, 1);

    if ((data = l_binaryRead(srcfile, &nbytes)) == NULL)
        return ERROR_INT("data not returned", procName, 1);
    ret = l_binaryWrite(newfile, "w", data, nbytes);
    FREE(data);
    return ret;
}


/*!
 *  fileConcatenate()
 *
 *      Input:  srcfile (file to append)
 *              destfile (file to add to)
 *      Return: 0 if OK, 1 on error
 */
l_int32
fileConcatenate(const char  *srcfile,
                const char  *destfile)
{
size_t    nbytes;
l_uint8  *data;

    PROCNAME("fileConcatenate");

    if (!srcfile)
        return ERROR_INT("srcfile not defined", procName, 1);
    if (!destfile)
        return ERROR_INT("destfile not defined", procName, 1);

    data = l_binaryRead(srcfile, &nbytes);
    l_binaryWrite(destfile, "a", data, nbytes);
    FREE(data);
    return 0;
}


/*!
 *  fileAppendString()
 *
 *      Input:  filename
 *              str (string to append to file)
 *      Return: 0 if OK, 1 on error
 */
l_int32
fileAppendString(const char  *filename,
                 const char  *str)
{
FILE  *fp;

    PROCNAME("fileAppendString");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!str)
        return ERROR_INT("str not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "a")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    fprintf(fp, "%s", str);
    fclose(fp);
    return 0;
}


/*--------------------------------------------------------------------*
 *                      Test files for equivalence                    *
 *--------------------------------------------------------------------*/
/*!
 *  filesAreIdentical()
 *
 *      Input:  fname1
 *              fname2
 *              &same (<return> 1 if identical; 0 if different)
 *      Return: 0 if OK, 1 on error
 */
l_int32
filesAreIdentical(const char  *fname1,
                  const char  *fname2,
                  l_int32     *psame)
{
l_int32   i, same;
size_t    nbytes1, nbytes2;
l_uint8  *array1, *array2;

    PROCNAME("filesAreIdentical");

    if (!psame)
        return ERROR_INT("&same not defined", procName, 1);
    *psame = 0;
    if (!fname1 || !fname2)
        return ERROR_INT("both names not defined", procName, 1);

    nbytes1 = nbytesInFile(fname1);
    nbytes2 = nbytesInFile(fname2);
    if (nbytes1 != nbytes2)
        return 0;

    if ((array1 = l_binaryRead(fname1, &nbytes1)) == NULL)
        return ERROR_INT("array1 not read", procName, 1);
    if ((array2 = l_binaryRead(fname2, &nbytes2)) == NULL)
        return ERROR_INT("array2 not read", procName, 1);
    same = 1;
    for (i = 0; i < nbytes1; i++) {
        if (array1[i] != array2[i]) {
            same = 0;
            break;
        }
    }
    FREE(array1);
    FREE(array2);
    *psame = same;

    return 0;
}


/*--------------------------------------------------------------------------*
 *   16 and 32 bit byte-swapping on big endian and little  endian machines  *
 *                                                                          *
 *   These are typically used for I/O conversions:                          *
 *      (1) endian conversion for data that was read from a file            *
 *      (2) endian conversion on data before it is written to a file        *
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------*
 *                        16-bit byte swapping                        *
 *--------------------------------------------------------------------*/
#ifdef L_BIG_ENDIAN

l_uint16
convertOnBigEnd16(l_uint16  shortin)
{
    return ((shortin << 8) | (shortin >> 8));
}

l_uint16
convertOnLittleEnd16(l_uint16  shortin)
{
    return  shortin;
}

#else     /* L_LITTLE_ENDIAN */

l_uint16
convertOnLittleEnd16(l_uint16  shortin)
{
    return ((shortin << 8) | (shortin >> 8));
}

l_uint16
convertOnBigEnd16(l_uint16  shortin)
{
    return  shortin;
}

#endif  /* L_BIG_ENDIAN */


/*--------------------------------------------------------------------*
 *                        32-bit byte swapping                        *
 *--------------------------------------------------------------------*/
#ifdef L_BIG_ENDIAN

l_uint32
convertOnBigEnd32(l_uint32  wordin)
{
    return ((wordin << 24) | ((wordin << 8) & 0x00ff0000) |
            ((wordin >> 8) & 0x0000ff00) | (wordin >> 24));
}

l_uint32
convertOnLittleEnd32(l_uint32  wordin)
{
    return wordin;
}

#else  /*  L_LITTLE_ENDIAN */

l_uint32
convertOnLittleEnd32(l_uint32  wordin)
{
    return ((wordin << 24) | ((wordin << 8) & 0x00ff0000) |
            ((wordin >> 8) & 0x0000ff00) | (wordin >> 24));
}

l_uint32
convertOnBigEnd32(l_uint32  wordin)
{
    return wordin;
}

#endif  /* L_BIG_ENDIAN */



/*--------------------------------------------------------------------*
 *                        Opening file streams                        *
 *--------------------------------------------------------------------*/
/*!
 *  fopenReadStream()
 *
 *      Input:  filename
 *      Return: stream, or null on error
 *
 *  Notes:
 *      (1) This wrapper also handles pathname conversions for Windows.
 *          It should be used whenever you want to run fopen() to
 *          read from a stream.
 */
FILE *
fopenReadStream(const char  *filename)
{
char  *fname, *tail;
FILE  *fp;

    PROCNAME("fopenReadStream");

    if (!filename)
        return (FILE *)ERROR_PTR("filename not defined", procName, NULL);

        /* Try input filename */
    fname = genPathname(filename, NULL);
    fp = fopen(fname, "rb");
    FREE(fname);
    if (fp) return fp;

        /* Else, strip directory and try locally */
    splitPathAtDirectory(filename, NULL, &tail);
    fp = fopen(tail, "rb");
    FREE(tail);

    if (!fp)
        return (FILE *)ERROR_PTR("file not found", procName, NULL);
    return fp;
}


/*!
 *  fopenWriteStream()
 *
 *      Input:  filename
 *              modestring
 *      Return: stream, or null on error
 *
 *  Notes:
 *      (1) This wrapper also handles pathname conversions for Windows.
 *          It should be used whenever you want to run fopen() to
 *          write or append to a stream.
 */
FILE *
fopenWriteStream(const char  *filename,
                 const char  *modestring)
{
FILE  *fp;

    PROCNAME("fopenWriteStream");

    if (!filename)
        return (FILE *)ERROR_PTR("filename not defined", procName, NULL);

#ifdef _WIN32
    {
    char  *fname;
        fname = genPathname(filename, NULL);
        fp = fopen(fname, modestring);
        FREE(fname);
    }
#else
    fp = fopen(filename, modestring);
#endif  /* _WIN32 */

    if (!fp)
        return (FILE *)ERROR_PTR("stream not opened", procName, NULL);
    return fp;
}


/*--------------------------------------------------------------------*
 *      Functions to avoid C-runtime boundary crossing with dlls      *
 *--------------------------------------------------------------------*/
/*
 *  Problems arise when pointers to streams and data are passed
 *  between two Windows DLLs that have been generated with different
 *  C runtimes.  To avoid this, leptonica provides wrappers for
 *  several C library calls.
 */
/*!
 *  lept_fopen()
 *
 *      Input:  filename
 *              mode (same as for fopen(); e.g., "rb")
 *      Return: stream or null on error
 *
 *  Notes:
 *      (1) This must be used by any application that passes
 *          a file handle to a leptonica Windows DLL.
 */
FILE *
lept_fopen(const char  *filename,
           const char  *mode)
{
    PROCNAME("lept_fopen");

    if (!filename)
        return (FILE *)ERROR_PTR("filename not defined", procName, NULL);
    if (!mode)
        return (FILE *)ERROR_PTR("mode not defined", procName, NULL);

    if (stringFindSubstr(mode, "r", NULL))
        return fopenReadStream(filename);
    else
        return fopenWriteStream(filename, mode);
}


/*!
 *  lept_fclose()
 *
 *      Input:  fp (stream handle)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This should be used by any application that accepts
 *          a file handle generated by a leptonica Windows DLL.
 */
l_int32
lept_fclose(FILE *fp)
{
    PROCNAME("lept_fclose");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);

    return fclose(fp);
}


/*!
 *  lept_calloc()
 *
 *      Input:  nmemb (number of members)
 *              size (of each member)
 *      Return: void ptr, or null on error
 *
 *  Notes:
 *      (1) For safety with windows DLLs, this can be used in conjunction
 *          with lept_free() to avoid C-runtime boundary problems.
 *          Just use these two functions throughout your application.
 */
void *
lept_calloc(size_t  nmemb,
            size_t  size)
{
    if (nmemb <= 0 || size <= 0)
        return NULL;
    return CALLOC(nmemb, size);
}


/*!
 *  lept_free()
 *
 *      Input:  void ptr
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This should be used by any application that accepts
 *          heap data allocated by a leptonica Windows DLL.
 */
void
lept_free(void *ptr)
{
    if (!ptr) return;
    FREE(ptr);
    return;
}


/*--------------------------------------------------------------------*
 *                Cross-platform file system operations               *
 *         [ These only write to /tmp or its subdirectories ]         *
 *--------------------------------------------------------------------*/
/*!
 *  lept_mkdir()
 *
 *      Input:  subdir (of /tmp or its equivalent on Windows)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This makes a subdirectory of /tmp/.
 *      (2) Use unix pathname separators.
 *      (3) On Windows, it makes a subdirectory of <Temp>/leptonica,
 *          where <Temp> is the Windows temp dir.  The name translation is:
 *                 /tmp  -->   <Temp>/leptonica
 */
l_int32
lept_mkdir(const char  *subdir)
{
char     *dir;
l_int32   ret;
#ifdef  _WIN32
char     *newpath;
l_uint32  attributes;
#endif  /* !_WIN32 */

    PROCNAME("lept_mkdir");

    if (!subdir)
        return ERROR_INT("subdir not defined", procName, 1);
    if ((strlen(subdir) == 0) || (subdir[0] == '.') || (subdir[0] == '/'))
        return ERROR_INT("subdir not an actual subdirectory", procName, 1);

    dir = pathJoin("/tmp", subdir);

#ifndef _WIN32
    ret = mkdir(dir, 0777);
#else
        /* Make sure the leptonica subdir exists in tmp dir */
    newpath = genPathname("/tmp", NULL);
    attributes = GetFileAttributes(newpath);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        ret = (CreateDirectory(newpath, NULL) ? 0 : 1);
    }
    FREE(newpath);

    newpath = genPathname(dir, NULL);
    ret = (CreateDirectory(newpath, NULL) ? 0 : 1);
    FREE(newpath);
#endif  /* !_WIN32 */

    FREE(dir);
    return ret;
}


/*!
 *  lept_rmdir()
 *
 *      Input:  subdir (of /tmp or its equivalent on Windows)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) On unix, this removes all the files in the named
 *          subdirectory of /tmp.  It then removes the subdirectory.
 *      (2) Use unix pathname separators.
 *      (3) On Windows, the affected directory is a subdirectory
 *          of <Temp>/leptonica, where <Temp> is the Windows temp dir.
 */
l_int32
lept_rmdir(const char  *subdir)
{
char    *dir, *fname, *fullname;
l_int32  exists, ret, i, nfiles;
SARRAY  *sa;
#ifdef _WIN32
char    *newpath;
#endif  /* _WIN32 */

    PROCNAME("lept_rmdir");

    if (!subdir)
        return ERROR_INT("subdir not defined", procName, 1);
    if ((strlen(subdir) == 0) || (subdir[0] == '.') || (subdir[0] == '/'))
        return ERROR_INT("subdir not an actual subdirectory", procName, 1);

    if ((dir = pathJoin("/tmp", subdir)) == NULL)
        return ERROR_INT("dir not made", procName, 1);
    lept_direxists(dir, &exists);
    if (!exists) {  /* fail silently */
        FREE(dir);
        return 0;
    }

        /* List all the files in temp subdir */
    if ((sa = getFilenamesInDirectory(dir)) == NULL) {
        L_ERROR("directory %s does not exist!!\n", procName, dir);
        FREE(dir);
        return 1;
    }
    nfiles = sarrayGetCount(sa);

#ifndef _WIN32
    for (i = 0; i < nfiles; i++) {
        fname = sarrayGetString(sa, i, L_NOCOPY);
        fullname = genPathname(dir, fname);
        remove(fullname);
        FREE(fullname);
    }
    ret = remove(dir);
#else
    for (i = 0; i < nfiles; i++) {
        fname = sarrayGetString(sa, i, L_NOCOPY);
        fullname = genPathname(dir, fname);
        ret = DeleteFile(fullname);
        FREE(fullname);
    }
    newpath = genPathname(dir, NULL);
    ret = (RemoveDirectory(newpath) ? 0 : 1);
    FREE(newpath);
#endif  /* !_WIN32 */

    sarrayDestroy(&sa);
    FREE(dir);
    return ret;
}


/*!
 *  lept_direxists()
 *
 *      Input:  dirname
 *              &exists (<return> 1 on success, 0 on failure)
 *      Return: void
 *
 *  Notes:
 *      (1) For Windows, use windows pathname separators.
 */
void
lept_direxists(const char  *dirname,
               l_int32     *pexists)
{
    if (!pexists) return;
    *pexists = 0;
    if (!dirname) return;

#ifndef _WIN32
    {
    DIR  *pdir = opendir(dirname);
        if (pdir) {
            *pexists = 1;
            closedir(pdir);
        }
    }
#else  /* _WIN32 */
    {
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA  ffd;
        hFind = FindFirstFileA(dirname, &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            *pexists = 1;
            FindClose(hFind);
        }
    }
#endif  /* _WIN32 */

    return;
}


/*!
 *  lept_rm()
 *
 *      Input:  subdir (can be NULL, in which case the removed file is
 *                      in /tmp)
 *              filename (with or without the directory)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This removes the named file in /tmp or a subdirectory of /tmp.
 *          If the file is in /tmp, use NULL for the subdir.
 *      (2) @filename can include directories in the path, but they are ignored.
 *      (3) Use unix pathname separators.
 *      (4) On Windows, the file is in either <Temp>/leptonica, or
 *          a subdirectory of this, where <Temp> is the Windows temp dir.
 *          The name translation is: /tmp  -->   <Temp>/leptonica
 */
l_int32
lept_rm(const char  *subdir,
        const char  *filename)
{
char    *dir, *pathname, *tail;
l_int32  ret;
#ifdef _WIN32
char    *newpath;
#endif  /* _WIN32 */

    PROCNAME("lept_rm");

    if (!filename || strlen(filename) == 0)
        return ERROR_INT("filename undefined or empty", procName, 1);

    splitPathAtDirectory(filename, NULL, &tail);
    if (subdir) {
        dir = pathJoin("/tmp", subdir);
        pathname = pathJoin(dir, tail);
        FREE(dir);
    } else {
        pathname = pathJoin("/tmp", tail);
    }
    FREE(tail);
    if (!pathname)
        return ERROR_INT("pathname not made", procName, 1);

#ifndef _WIN32
    ret = remove(pathname);
#else
    newpath = genPathname(pathname, NULL);
    if (!newpath) {
        FREE(pathname);
        return ERROR_INT("newpath not made", procName, 1);
    }
    ret = (DeleteFile(newpath) ? 0 : 1);
    FREE(newpath);
#endif  /* !_WIN32 */

    FREE(pathname);
    return ret;
}


/*!
 *  lept_mv()
 *
 *      Input:  srcfile, newfile
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This moves a srcfile to /tmp or to a subdirectory of /tmp.
 *      (2) The input srcfile name is the complete pathname.
 *          The input newfile is either in /tmp or a subdirectory
 *          of /tmp, and newfile can be specified either as the
 *          full path or without the leading '/tmp'.
 *      (3) Use unix pathname separators.
 *      (4) On Windows, the source and target filename are altered
 *          internally if necessary to conform to the Windows temp file.
 *          The name translation is: /tmp  -->   <Temp>/leptonica
 */
l_int32
lept_mv(const char  *srcfile,
        const char  *newfile)
{
char     *newfileplus;
l_int32   ret;
#ifndef _WIN32
char     *command;
l_int32   nbytes;
#else
char     *srcpath, *newpath, *tail;
l_uint32  attributes;
#endif  /* !_WIN32 */

    PROCNAME("lept_mv");

    if (!srcfile || !newfile)
        return ERROR_INT("srcfile and newfile not both defined", procName, 1);
    if (strncmp(newfile, "/tmp/", 5))
        newfileplus = pathJoin("/tmp", newfile);
    else
        newfileplus = stringNew(newfile);

#ifndef _WIN32
    nbytes = strlen(srcfile) + strlen(newfileplus) + 10;
    command = (char *)CALLOC(nbytes, sizeof(char));
    snprintf(command, nbytes, "mv %s %s", srcfile, newfileplus);
    ret = system(command);
    FREE(command);
#else
    srcpath = genPathname(srcfile, NULL);
    newpath = genPathname(newfileplus, NULL);
    attributes = GetFileAttributes(newpath);
    if (attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if (splitPathAtDirectory(srcpath, NULL, &tail)) {
            FREE(srcpath);
            FREE(newpath);
            return ERROR_INT("Unable to split source filename into root & tail",
                             procName, 1);
        }
        FREE(newpath);
        newpath = genPathname(newfileplus, tail);
        FREE(tail);
    }

        /* New file overwritten if it already exists */
    ret = (MoveFileEx(srcpath, newpath,
                      MOVEFILE_COPY_ALLOWED |
                      MOVEFILE_REPLACE_EXISTING) ? 0 : 1);
    FREE(srcpath);
    FREE(newpath);
#endif  /* !_WIN32 */

    FREE(newfileplus);
    return ret;
}


/*!
 *  lept_cp()
 *
 *      Input:  srcfile
 *              newfile
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This copies a file to /tmp or a subdirectory of /tmp.
 *      (2) The input srcfile name is the complete pathname.
 *          The input newfile is either in /tmp or a subdirectory
 *          of /tmp, and newfile can be specified either as the
 *          full path or without the leading '/tmp'.
 *      (3) Use unix pathname separators.
 *      (4) On Windows, the source and target filename are altered
 *          internally if necessary to conform to the Windows temp file.
 *      (5) Alternatively, you can use fileCopy().  This avoids
 *          forking a new process and has no restrictions on the
 *          destination directory.
 */
l_int32
lept_cp(const char  *srcfile,
        const char  *newfile)
{
char     *newfileplus;
l_int32   ret;
#ifndef _WIN32
char     *command;
l_int32   nbytes;
#else
char     *srcpath, *newpath, *tail;
l_uint32  attributes;
#endif  /* !_WIN32 */

    PROCNAME("lept_cp");

    if (!srcfile || !newfile)
        return ERROR_INT("srcfile and newfile not both defined", procName, 1);
    if (strncmp(newfile, "/tmp/", 5))
        newfileplus = pathJoin("/tmp", newfile);
    else
        newfileplus = stringNew(newfile);

#ifndef _WIN32
    nbytes = strlen(srcfile) + strlen(newfileplus) + 10;
    command = (char *)CALLOC(nbytes, sizeof(char));
    snprintf(command, nbytes, "cp %s %s", srcfile, newfile);
    ret = system(command);
    FREE(command);
#else
    srcpath = genPathname(srcfile, NULL);
    newpath = genPathname(newfileplus, NULL);
    attributes = GetFileAttributes(newpath);
    if (attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if (splitPathAtDirectory(srcpath, NULL, &tail)) {
            FREE(srcpath);
            FREE(newpath);
            return ERROR_INT("Unable to split source filename into root & tail",
                             procName, 1);
        }
        FREE(newpath);
        newpath = genPathname(newfileplus, tail);
        FREE(tail);
    }

        /* New file overwritten if it already exists */
    ret = (CopyFile(srcpath, newpath, FALSE) ? 0 : 1);
    FREE(srcpath);
    FREE(newpath);
#endif  /* !_WIN32 */

    FREE(newfileplus);
    return ret;
}


/*--------------------------------------------------------------------*
 *                         File name operations                       *
 *--------------------------------------------------------------------*/
/*!
 *  splitPathAtDirectory()
 *
 *      Input:  pathname  (full path; can be a directory)
 *              &dir  (<optional return> root directory name of
 *                     input path, including trailing '/')
 *              &tail (<optional return> path tail, which is either
 *                     the file name within the root directory or
 *                     the last sub-directory in the path)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If you only want the tail, input null for the root directory ptr.
 *      (2) If you only want the root directory name, input null for the
 *          tail ptr.
 *      (3) This function makes decisions based only on the lexical
 *          structure of the input.  Examples:
 *            /usr/tmp/abc  -->  dir: /usr/tmp/       tail: abc
 *            /usr/tmp/     -->  dir: /usr/tmp/       tail: [empty string]
 *            /usr/tmp      -->  dir: /usr/           tail: tmp
 *            abc           -->  dir: [empty string]  tail: abc
 *      (4) For unix, the input pathname must have unix directory
 *          separators (i.e., forward slashes).  For windows, we allow
 *          both forward and backward slash separators, because Win32
 *          pathname functions generally accept either slash form.
 *          Note, however, that the windows command line interpreter
 *          only accepts backward slashes, because forward slashes are
 *          used to demarcate switches (vs. dashes in unix).
 */
l_int32
splitPathAtDirectory(const char  *pathname,
                     char       **pdir,
                     char       **ptail)
{
char  *cpathname, *lastslash, *fwdslash;

    PROCNAME("splitPathAtDirectory");

    if (!pdir && !ptail)
        return ERROR_INT("null input for both strings", procName, 1);
    if (pdir) *pdir = NULL;
    if (ptail) *ptail = NULL;
    if (!pathname)
        return ERROR_INT("pathname not defined", procName, 1);

    cpathname = stringNew(pathname);
    lastslash = strrchr(cpathname, path_sepchar);
#ifdef _WIN32  /* allow both types of slash for win32 */
    if ((fwdslash = strrchr(cpathname, '/')) != NULL) {
        if (lastslash)
            lastslash = L_MAX(lastslash, fwdslash);
        else
            lastslash = fwdslash;
    }
#endif  /* _WIN32 */
    if (lastslash) {
        if (ptail)
            *ptail = stringNew(lastslash + 1);
        if (pdir) {
            *(lastslash + 1) = '\0';
            *pdir = cpathname;
        } else {
            FREE(cpathname);
        }
    } else {  /* no directory */
        if (pdir)
            *pdir = stringNew("");
        if (ptail)
            *ptail = cpathname;
        else
            FREE(cpathname);
    }

    return 0;
}


/*!
 *  splitPathAtExtension()
 *
 *      Input:  pathname (full path; can be a directory)
 *              &basename (<optional return> pathname not including the
 *                        last dot and characters after that)
 *              &extension (<optional return> path extension, which is
 *                        the last dot and the characters after it.  If
 *                        there is no extension, it returns the empty string)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If you only want the extension, input null for the basename ptr.
 *      (2) If you only want the basename without extension, input null
 *          for the extension ptr.
 *      (3) This function makes decisions based only on the lexical
 *          structure of the input.  Examples:
 *            /usr/tmp/abc.jpg  -->  basename: /usr/tmp/abc    ext: .jpg
 *            /usr/tmp/.jpg     -->  basename: /usr/tmp/       ext: .jpg
 *            /usr/tmp.jpg/     -->  basename: /usr/tmp.jpg/   ext: [empty str]
 *            ./.jpg            -->  basename: ./              ext: .jpg
 *      (4) N.B. The input pathname must have unix directory separators
 *          for unix and windows directory separators for windows.
 */
l_int32
splitPathAtExtension(const char  *pathname,
                     char       **pbasename,
                     char       **pextension)
{
char  *tail, *dir, *lastdot;
char   empty[4] = "";

    PROCNAME("splitPathExtension");

    if (!pbasename && !pextension)
        return ERROR_INT("null input for both strings", procName, 1);
    if (pbasename) *pbasename = NULL;
    if (pextension) *pextension = NULL;
    if (!pathname)
        return ERROR_INT("pathname not defined", procName, 1);

        /* Split out the directory first */
    splitPathAtDirectory(pathname, &dir, &tail);

        /* Then look for a "." in the tail part.
         * This way we ignore all "." in the directory. */
    if ((lastdot = strrchr(tail, '.'))) {
        if (pextension)
            *pextension = stringNew(lastdot);
        if (pbasename) {
            *lastdot = '\0';
            *pbasename = stringJoin(dir, tail);
        }
    } else {
        if (pextension)
            *pextension = stringNew(empty);
        if (pbasename)
            *pbasename = stringNew(pathname);
    }
    FREE(dir);
    FREE(tail);
    return 0;
}


/*!
 *  pathJoin()
 *
 *      Input:  dir (<optional> can be null)
 *              fname (<optional> can be null)
 *      Return: specially concatenated path, or null on error
 *
 *  Notes:
 *      (1) Use unix-style pathname separators ('/').
 *      (2) @fname can be the entire path, or part of the path containing
 *          at least one directory, or a tail without a directory, or null.
 *      (3) It produces a path that strips multiple slashes to a single
 *          slash, joins @dir and @fname by a slash, and has no trailing
 *          slashes (except in the cases where @dir == "/" and
 *          @fname == NULL, or v.v.).
 *      (4) If both @dir and @fname are null, produces an empty string.
 *      (5) Neither @dir nor @fname can begin with '.'.
 *      (6) The result is not canonicalized or tested for correctness:
 *          garbage in (e.g., /&%), garbage out.
 *      (7) Examples:
 *             //tmp// + //abc/  -->  /tmp/abc
 *             tmp/ + /abc/      -->  tmp/abc
 *             tmp/ + abc/       -->  tmp/abc
 *             /tmp/ + ///       -->  /tmp
 *             /tmp/ + NULL      -->  /tmp
 *             // + /abc//       -->  /abc
 *             // + NULL         -->  /
 *             NULL + /abc/def/  -->  /abc/def
 *             NULL + abc//      -->  abc
 *             NULL + //         -->  /
 *             NULL + NULL       -->  (empty string)
 *             "" + ""           -->  (empty string)
 *             "" + /            -->  /
 *             ".." + /etc/foo   -->  NULL
 *             /tmp + ".."       -->  NULL
 */
char *
pathJoin(const char  *dir,
         const char  *fname)
{
char     *slash = (char *)"/";
char     *str, *dest;
l_int32   i, n1, n2, emptydir;
size_t    size;
SARRAY   *sa1, *sa2;
L_BYTEA  *ba;

    PROCNAME("pathJoin");

    if (!dir && !fname)
        return stringNew("");
    if (dir && dir[0] == '.')
        return (char *)ERROR_PTR("dir starts with '.'", procName, NULL);
    if (fname && fname[0] == '.')
        return (char *)ERROR_PTR("fname starts with '.'", procName, NULL);

    sa1 = sarrayCreate(0);
    sa2 = sarrayCreate(0);
    ba = l_byteaCreate(4);

        /* Process @dir */
    if (dir && strlen(dir) > 0) {
        if (dir[0] == '/')
            l_byteaAppendString(ba, slash);
        sarraySplitString(sa1, dir, "/");  /* removes all slashes */
        n1 = sarrayGetCount(sa1);
        for (i = 0; i < n1; i++) {
            str = sarrayGetString(sa1, i, L_NOCOPY);
            l_byteaAppendString(ba, str);
            l_byteaAppendString(ba, slash);
        }
    }

        /* Special case to add leading slash: dir NULL or empty string  */
    emptydir = dir && strlen(dir) == 0;
    if ((!dir || emptydir) && fname && strlen(fname) > 0 && fname[0] == '/')
        l_byteaAppendString(ba, slash);

        /* Process @fname */
    if (fname && strlen(fname) > 0) {
        sarraySplitString(sa2, fname, "/");
        n2 = sarrayGetCount(sa2);
        for (i = 0; i < n2; i++) {
            str = sarrayGetString(sa2, i, L_NOCOPY);
            l_byteaAppendString(ba, str);
            l_byteaAppendString(ba, slash);
        }
    }

        /* Remove trailing slash */
    dest = (char *)l_byteaCopyData(ba, &size);
    if (size > 1 && dest[size - 1] == '/')
        dest[size - 1] = '\0';

    sarrayDestroy(&sa1);
    sarrayDestroy(&sa2);
    l_byteaDestroy(&ba);
    return dest;
}


/*!
 *  genPathname()
 *
 *      Input:  dir (directory name, with or without trailing '/')
 *              fname (<optional> file name within the directory)
 *      Return: pathname (either a directory or full path), or null on error
 *
 *  Notes:
 *      (1) Use unix-style pathname separators ('/').
 *      (2) This function can be used in several ways:
 *            * to generate a full path from a directory and a file name
 *            * to convert a unix pathname to a windows pathname
 *            * to convert from the unix '/tmp' directory to the
 *              equivalent windows temp directory.
 *          The windows name translation is:
 *                   /tmp  -->   <Temp>/leptonica
 *      (3) There are three cases for the input:
 *          (a) @dir is a directory and @fname is null: result is a directory
 *          (b) @dir is a full path and @fname is null: result is a full path
 *          (c) @dir is a directory and @fname is defined: result is a full path
 *      (4) In all cases, the resulting pathname is not terminated with a slash
 *      (5) The caller is responsible for freeing the pathname.
 */
char *
genPathname(const char  *dir,
            const char  *fname)
{
char    *cdir, *pathout;
l_int32  dirlen, namelen, size;

    PROCNAME("genPathname");

    if (!dir)
        return (char *)ERROR_PTR("dir not defined", procName, NULL);

        /* Remove trailing slash in dir, except when dir == "/"  */
    cdir = stringNew(dir);
    dirlen = strlen(cdir);
    if (cdir[dirlen - 1] == '/' && dirlen != 1) {
        cdir[dirlen - 1] = '\0';
        dirlen--;
    }

    namelen = (fname) ? strlen(fname) : 0;
    size = dirlen + namelen + 256;
    if ((pathout = (char *)CALLOC(size, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("pathout not made", procName, NULL);

#ifdef _WIN32
    {
        char  dirt[MAX_PATH];
        if (stringFindSubstr(cdir, "/", NULL) > 0) {
            char    *tempdir;
            l_int32  tdirlen;
            tempdir = stringReplaceEachSubstr(cdir, "/", "\\", NULL);
            tdirlen = strlen(tempdir);
            if (strncmp(dir, "/tmp", 4) == 0) {  /* get temp directory */
                GetTempPath(sizeof(dirt), dirt);
                stringCopy(pathout, dirt, strlen(dirt) - 1);
                stringCat(pathout, size, "\\leptonica");
                if (tdirlen > 4)
                    stringCat(pathout, size, tempdir + 4);

                    /* Set an extra null byte.  Otherwise, when setting
                       path_sepchar later, no trailing null byte remains. */
                pathout[strlen(pathout) + 1] = '\0';
            } else {
                stringCopy(pathout, tempdir, tdirlen);
            }
            FREE(tempdir);
        } else {  /* no '/' characters; OK as is */
            stringCopy(pathout, cdir, dirlen);
        }
    }
#else
    stringCopy(pathout, cdir, dirlen);
#endif  /* _WIN32 */

    if (fname && strlen(fname) > 0) {
        dirlen = strlen(pathout);
        pathout[dirlen] = path_sepchar;  /* append path_sepchar */
        strncat(pathout, fname, namelen);
    }
    FREE(cdir);
    return pathout;
}


/*!
 *  genTempFilename()
 *
 *      Input:  dir (directory name; use '.' for local dir;
 *                   no trailing '/' and @dir == "/" is invalid)
 *              tail (<optional>  tailname, including extension if any;
 *                    can be null or empty but can't contain '/')
 *              usetime (1 to include current time in microseconds in
 *                       the filename; 0 to omit.
 *              usepid (1 to include pid in filename; 0 to omit.
 *      Return: temp filename, or null on error
 *
 *  Notes:
 *      (1) Use unix-style pathname separators ('/').
 *      (2) Specifying the root directory (@dir == "/") is invalid.
 *      (3) Specifying a @tail containing '/' is invalid.
 *      (4) The most general form (@usetime = @usepid = 1) is:
 *              <dir>/<usec>_<pid>_<tail>
 *          When @usetime = 1, @usepid = 0, the output filename is:
 *              <dir>/<usec>_<tail>
 *          When @usepid = 0, @usepid = 1, the output filename is:
 *              <dir>/<pid>_<tail>
 *          When @usetime = @usepid = 0, the output filename is:
 *              <dir>/<tail>
 *          Note: It is not valid to have @tail = null or empty and have
 *          both @usetime = @usepid = 0.  That is, there must be
 *          some non-empty tail name.
 *      (5) N.B. The caller is responsible for freeing the returned filename.
 *          For windows, to avoid C-runtime boundary crossing problems
 *          when using DLLs, you must use lept_free() to free the name.
 *      (6) For windows, if the caller requests the directory '/tmp',
 *          this uses GetTempPath() to select the actual directory,
 *          avoiding platform-conditional code in use.  The directory
 *          selected is <Temp>/leptonica, where <Temp> is the Windows
 *          temp directory.
 *      (7) Set @usetime = @usepid = 1 when
 *          (a) more than one process is writing and reading temp files, or
 *          (b) multiple threads from a single process call this function, or
 *          (c) there is the possiblity of an attack where the intruder
 *              is logged onto the server and might try to guess filenames.
 */
char *
genTempFilename(const char  *dir,
                const char  *tail,
                l_int32      usetime,
                l_int32      usepid)
{
char     buf[256];
l_int32  i, buflen, usec, pid, emptytail;
#ifdef _WIN32
char    *newpath;
l_uint32 attributes;
l_int32  ret;
#endif  /* !_WIN32 */

    PROCNAME("genTempFilename");

    if (!dir)
        return (char *)ERROR_PTR("dir not defined", procName, NULL);
    if (dir && strlen(dir) == 1 && dir[0] == '/')
        return (char *)ERROR_PTR("dir == '/' not permitted", procName, NULL);
    if (tail && strlen(tail) > 0 && stringFindSubstr(tail, "/", NULL))
        return (char *)ERROR_PTR("tail can't contain '/'", procName, NULL);
    emptytail = tail && (strlen(tail) == 0);
    if (!usetime && !usepid && (!tail || emptytail))
        return (char *)ERROR_PTR("name can't be a directory", procName, NULL);

    if (usepid) pid = getpid();
    buflen = sizeof(buf);
    for (i = 0; i < buflen; i++)
        buf[i] = 0;
    l_getCurrentTime(NULL, &usec);

#ifdef _WIN32
    {  /* do not assume /tmp exists */
    char  dirt[MAX_PATH];
    if (!strcmp(dir, "/tmp")) {
        GetTempPath(sizeof(dirt), dirt);
        stringCat(dirt, sizeof(dirt), "leptonica\\");

            /* Make sure the leptonica subdir exists in tmp dir */
        newpath = genPathname("/tmp", NULL);
        attributes = GetFileAttributes(newpath);
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            ret = (CreateDirectory(newpath, NULL) ? 0 : 1);
        }
        FREE(newpath);
    } else {
        snprintf(dirt, sizeof(dirt), "%s\\", dir);  /* add trailing '\' */
    }

    if (usetime && usepid)
        snprintf(buf, buflen, "%s%d_%d_", dirt, usec, pid);
    else if (usetime)
        snprintf(buf, buflen, "%s%d_", dirt, usec);
    else if (usepid)
        snprintf(buf, buflen, "%s%d_", dirt, pid);
    else
        snprintf(buf, buflen, "%s", dirt);
    }
#else
    if (usetime && usepid)
        snprintf(buf, buflen, "%s/%d_%d_", dir, usec, pid);
    else if (usetime)
        snprintf(buf, buflen, "%s/%d_", dir, usec);
    else if (usepid)
        snprintf(buf, buflen, "%s/%d_", dir, pid);
    else
        snprintf(buf, buflen, "%s/", dir);
#endif

    return stringJoin(buf, tail);
}


/*!
 *  extractNumberFromFilename()
 *
 *      Input:  fname
 *              numpre (number of characters before the digits to be found)
 *              numpost (number of characters after the digits to be found)
 *      Return: num (number embedded in the filename); -1 on error or if
 *                   not found
 *
 *  Notes:
 *      (1) The number is to be found in the basename, which is the
 *          filename without either the directory or the last extension.
 *      (2) When a number is found, it is non-negative.  If no number
 *          is found, this returns -1, without an error message.  The
 *          caller needs to check.
 */
l_int32
extractNumberFromFilename(const char  *fname,
                          l_int32      numpre,
                          l_int32      numpost)
{
char    *tail, *basename;
l_int32  len, nret, num;

    PROCNAME("extractNumberFromFilename");

    if (!fname)
        return ERROR_INT("fname not defined", procName, -1);

    splitPathAtDirectory(fname, NULL, &tail);
    splitPathAtExtension(tail, &basename, NULL);
    FREE(tail);

    len = strlen(basename);
    if (numpre + numpost > len - 1) {
        FREE(basename);
        return ERROR_INT("numpre + numpost too big", procName, -1);
    }

    basename[len - numpost] = '\0';
    nret = sscanf(basename + numpre, "%d", &num);
    FREE(basename);

    if (nret == 1)
        return num;
    else
        return -1;  /* not found */
}


/*---------------------------------------------------------------------*
 *                       File corruption operation                     *
 *---------------------------------------------------------------------*/
/*!
 *  fileCorruptByDeletion()
 *
 *      Input:  filein
 *              loc (fractional location of start of deletion)
 *              size (fractional size of deletion)
 *              fileout (corrupted file)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is useful for testing robustness of I/O wrappers with image
 *          file corruption.
 *      (2) Deletion size adjusts automatically to avoid array transgressions.
 */
l_int32
fileCorruptByDeletion(const char  *filein,
                      l_float32    loc,
                      l_float32    size,
                      const char  *fileout)
{
l_int32   i, locb, sizeb, rembytes;
size_t    inbytes, outbytes;
l_uint8  *datain, *dataout;

    PROCNAME("fileCorruptByDeletion");

    if (!filein || !fileout)
        return ERROR_INT("filein and fileout not both specified", procName, 1);
    if (loc < 0.0 || loc >= 1.0)
        return ERROR_INT("loc must be in [0.0 ... 1.0)", procName, 1);
    if (size <= 0.0)
        return ERROR_INT("size must be > 0.0", procName, 1);
    if (loc + size > 1.0)
        size = 1.0 - loc;

    datain = l_binaryRead(filein, &inbytes);
    locb = (l_int32)(loc * inbytes + 0.5);
    locb = L_MIN(locb, inbytes - 1);
    sizeb = (l_int32)(size * inbytes + 0.5);
    sizeb = L_MAX(1, sizeb);
    sizeb = L_MIN(sizeb, inbytes - locb);  /* >= 1 */
    L_INFO("Removed %d bytes at location %d\n", procName, sizeb, locb);
    rembytes = inbytes - locb - sizeb;  /* >= 0; to be copied, after excision */

    outbytes = inbytes - sizeb;
    dataout = (l_uint8 *)CALLOC(outbytes, 1);
    for (i = 0; i < locb; i++)
        dataout[i] = datain[i];
    for (i = 0; i < rembytes; i++)
        dataout[locb + i] = datain[locb + sizeb + i];
    l_binaryWrite(fileout, "w", dataout, outbytes);

    FREE(datain);
    FREE(dataout);
    return 0;
}


/*---------------------------------------------------------------------*
 *                Generate random integer in given range               *
 *---------------------------------------------------------------------*/
/*!
 *  genRandomIntegerInRange()
 *
 *      Input:  range (size of range; must be >= 2)
 *              seed (use 0 to skip; otherwise call srand)
 *              val (<return> random integer in range {0 ... range-1}
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) For example, to choose a rand integer between 0 and 99,
 *          use @range = 100.
 */
l_int32
genRandomIntegerInRange(l_int32   range,
                        l_int32   seed,
                        l_int32  *pval)
{
    PROCNAME("genRandomIntegerInRange");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0;
    if (range < 2)
        return ERROR_INT("range must be >= 2", procName, 1);

    if (seed > 0) srand(seed);
    *pval = (l_int32)((l_float64)range *
                       ((l_float64)rand() / (l_float64)RAND_MAX));
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Simple math function                        *
 *---------------------------------------------------------------------*/
/*!
 *  lept_roundftoi()
 *
 *      Input:  fval
 *      Return: value rounded to int
 *
 *  Notes:
 *      (1) For fval >= 0, fval --> round(fval) == floor(fval + 0.5)
 *          For fval < 0, fval --> -round(-fval))
 *          This is symmetric around 0.
 *          e.g., for fval in (-0.5 ... 0.5), fval --> 0
 */
l_int32
lept_roundftoi(l_float32  fval)
{
    return (fval >= 0.0) ? (l_int32)(fval + 0.5) : (l_int32)(fval - 0.5);
}


/*---------------------------------------------------------------------*
 *                         Gray code conversion                        *
 *---------------------------------------------------------------------*/
/*!
 *  convertBinaryToGrayCode()
 *
 *      Input:  val
 *      Return: gray code value
 *
 *  Notes:
 *      (1) Gray code values corresponding to integers differ by
 *          only one bit transition between successive integers.
 */
l_uint32
convertBinaryToGrayCode(l_uint32 val)
{
    return (val >> 1) ^ val;
}


/*!
 *  convertGrayCodeToBinary()
 *
 *      Input:  gray code value
 *      Return: binary value
 */
l_uint32
convertGrayCodeToBinary(l_uint32 val)
{
l_uint32  shift;

    for (shift = 1; shift < 32; shift <<= 1)
        val ^= val >> shift;
    return val;
}


/*---------------------------------------------------------------------*
 *                       Leptonica version number                      *
 *---------------------------------------------------------------------*/
/*!
 *  getLeptonicaVersion()
 *
 *      Return: string of version number (e.g., 'leptonica-1.68')
 *
 *  Notes:
 *      (1) The caller has responsibility to free the memory.
 */
char *
getLeptonicaVersion()
{
    char *version = (char *)CALLOC(100, sizeof(char));

#ifdef _MSC_VER
  #ifdef _USRDLL
    char dllStr[] = "DLL";
  #else
    char dllStr[] = "LIB";
  #endif
  #ifdef _DEBUG
    char debugStr[] = "Debug";
  #else
    char debugStr[] = "Release";
  #endif
  #ifdef _M_IX86
    char bitStr[] = " x86";
  #elif _M_X64
    char bitStr[] = " x64";
  #else
    char bitStr[] = "";
  #endif
    snprintf(version, 100, "leptonica-%d.%d (%s, %s) [MSC v.%d %s %s%s]",
             LIBLEPT_MAJOR_VERSION, LIBLEPT_MINOR_VERSION,
             __DATE__, __TIME__, _MSC_VER, dllStr, debugStr, bitStr);

#else

    snprintf(version, 100, "leptonica-%d.%d", LIBLEPT_MAJOR_VERSION,
             LIBLEPT_MINOR_VERSION);

#endif   /* _MSC_VER */
    return version;
}


/*---------------------------------------------------------------------*
 *                           Timing procs                              *
 *---------------------------------------------------------------------*/
#ifndef _WIN32

#include <sys/time.h>
#include <sys/resource.h>

static struct rusage rusage_before;
static struct rusage rusage_after;

/*!
 *  startTimer(), stopTimer()
 *
 *  Example of usage:
 *
 *      startTimer();
 *      ....
 *      fprintf(stderr, "Elapsed time = %7.3f sec\n", stopTimer());
 */
void
startTimer(void)
{
    getrusage(RUSAGE_SELF, &rusage_before);
}

l_float32
stopTimer(void)
{
l_int32  tsec, tusec;

    getrusage(RUSAGE_SELF, &rusage_after);

    tsec = rusage_after.ru_utime.tv_sec - rusage_before.ru_utime.tv_sec;
    tusec = rusage_after.ru_utime.tv_usec - rusage_before.ru_utime.tv_usec;
    return (tsec + ((l_float32)tusec) / 1000000.0);
}


/*!
 *  startTimerNested(), stopTimerNested()
 *
 *  Example of usage:
 *
 *      L_TIMER  t1 = startTimerNested();
 *      ....
 *      L_TIMER  t2 = startTimerNested();
 *      ....
 *      fprintf(stderr, "Elapsed time 2 = %7.3f sec\n", stopTimerNested(t2));
 *      ....
 *      fprintf(stderr, "Elapsed time 1 = %7.3f sec\n", stopTimerNested(t1));
 */
L_TIMER
startTimerNested(void)
{
struct rusage  *rusage_start;

    rusage_start = (struct rusage *)CALLOC(1, sizeof(struct rusage));
    getrusage(RUSAGE_SELF, rusage_start);
    return rusage_start;
}

l_float32
stopTimerNested(L_TIMER  rusage_start)
{
l_int32        tsec, tusec;
struct rusage  rusage_stop;

    getrusage(RUSAGE_SELF, &rusage_stop);

    tsec = rusage_stop.ru_utime.tv_sec -
           ((struct rusage *)rusage_start)->ru_utime.tv_sec;
    tusec = rusage_stop.ru_utime.tv_usec -
           ((struct rusage *)rusage_start)->ru_utime.tv_usec;
    FREE(rusage_start);
    return (tsec + ((l_float32)tusec) / 1000000.0);
}


/*!
 *  l_getCurrentTime()
 *
 *      Input:  &sec (<optional return> in seconds since birth of Unix)
 *              &usec (<optional return> in microseconds since birth of Unix)
 *      Return: void
 */
void
l_getCurrentTime(l_int32  *sec,
                 l_int32  *usec)
{
struct timeval tv;

    gettimeofday(&tv, NULL);
    if (sec) *sec = (l_int32)tv.tv_sec;
    if (usec) *usec = (l_int32)tv.tv_usec;
    return;
}


#else   /* _WIN32 : resource.h not implemented under Windows */

    /* Note: if division by 10^7 seems strange, the time is expressed
     * as the number of 100-nanosecond intervals that have elapsed
     * since 12:00 A.M. January 1, 1601.  */

static ULARGE_INTEGER utime_before;
static ULARGE_INTEGER utime_after;

void
startTimer(void)
{
HANDLE    this_process;
FILETIME  start, stop, kernel, user;

    this_process = GetCurrentProcess();

    GetProcessTimes(this_process, &start, &stop, &kernel, &user);

    utime_before.LowPart  = user.dwLowDateTime;
    utime_before.HighPart = user.dwHighDateTime;
}

l_float32
stopTimer(void)
{
HANDLE     this_process;
FILETIME   start, stop, kernel, user;
ULONGLONG  hnsec;  /* in units of hecto-nanosecond (100 ns) intervals */

    this_process = GetCurrentProcess();

    GetProcessTimes(this_process, &start, &stop, &kernel, &user);

    utime_after.LowPart  = user.dwLowDateTime;
    utime_after.HighPart = user.dwHighDateTime;
    hnsec = utime_after.QuadPart - utime_before.QuadPart;
    return (l_float32)(signed)hnsec / 10000000.0;
}

L_TIMER
startTimerNested(void)
{
HANDLE           this_process;
FILETIME         start, stop, kernel, user;
ULARGE_INTEGER  *utime_start;

    this_process = GetCurrentProcess();

    GetProcessTimes (this_process, &start, &stop, &kernel, &user);

    utime_start = (ULARGE_INTEGER *)CALLOC(1, sizeof(ULARGE_INTEGER));
    utime_start->LowPart  = user.dwLowDateTime;
    utime_start->HighPart = user.dwHighDateTime;
    return utime_start;
}

l_float32
stopTimerNested(L_TIMER  utime_start)
{
HANDLE          this_process;
FILETIME        start, stop, kernel, user;
ULARGE_INTEGER  utime_stop;
ULONGLONG       hnsec;  /* in units of 100 ns intervals */

    this_process = GetCurrentProcess ();

    GetProcessTimes (this_process, &start, &stop, &kernel, &user);

    utime_stop.LowPart  = user.dwLowDateTime;
    utime_stop.HighPart = user.dwHighDateTime;
    hnsec = utime_stop.QuadPart - ((ULARGE_INTEGER *)utime_start)->QuadPart;
    FREE(utime_start);
    return (l_float32)(signed)hnsec / 10000000.0;
}

void
l_getCurrentTime(l_int32  *sec,
                 l_int32  *usec)
{
ULARGE_INTEGER  utime, birthunix;
FILETIME        systemtime;
LONGLONG        birthunixhnsec = 116444736000000000;  /*in units of 100 ns */
LONGLONG        usecs;

    GetSystemTimeAsFileTime(&systemtime);
    utime.LowPart  = systemtime.dwLowDateTime;
    utime.HighPart = systemtime.dwHighDateTime;

    birthunix.LowPart = (DWORD) birthunixhnsec;
    birthunix.HighPart = birthunixhnsec >> 32;

    usecs = (LONGLONG) ((utime.QuadPart - birthunix.QuadPart) / 10);

    if (sec) *sec = (l_int32) (usecs / 1000000);
    if (usec) *usec = (l_int32) (usecs % 1000000);
    return;
}

#endif


/*!
 *  l_getFormattedDate()
 *
 *      Input:  (none)
 *      Return: formatted date string, or null on error
 */
char *
l_getFormattedDate()
{
char        buf[64];
time_t      tmp1;
struct tm  *tmp2;

    tmp1 = time(NULL);
    tmp2 = localtime(&tmp1);
    strftime(buf, sizeof(buf), "%y%m%d%H%M%S", tmp2);
    return stringNew(buf);
}

