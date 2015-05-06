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
 *           char      *stringConcatNew()
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
 *           l_uint8   *l_binaryReadSelect()
 *           l_uint8   *l_binaryReadSelectStream()
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
 *           l_int32    lept_rm_match()
 *           l_int32    lept_rm()
 *           l_int32    lept_rmfile()
 *           l_int32    lept_cp()
 *
 *       General file name operations
 *           l_int32    splitPathAtDirectory()
 *           l_int32    splitPathAtExtension()
 *           char      *pathJoin()
 *           char      *appendSubdirectory()
 *
 *       Special file name operations
  *          l_int32    convertSepCharsInPath()
 *           char      *genPathname()
 *           l_int32    makeTempDirname()
 *           l_int32    modifyTrailingSlash()
 *           char      *genTempFilename()
 *           l_int32    extractNumberFromFilename()
 *
 *       File corruption operation
 *           l_int32    fileCorruptByDeletion()
 *           l_int32    fileCorruptByMutation()
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
 *  This is important:
 *  (1) With the exception of splitPathAtDirectory(), splitPathAtExtension()
  *     and genPathname(), all input pathnames must have unix separators.
 *  (2) On Windows, when you specify a read or write to "/tmp/...",
 *      the filename is rewritten to use the Windows temp directory:
 *         /tmp  ==>    <Temp>...    (windows)
 *  (3) This filename rewrite, along with the conversion from unix
 *      to windows pathnames, happens in genPathname().
 *  (4) Use fopenReadStream() and fopenWriteStream() to open files,
 *      because these use genPathname() to find the platform-dependent
 *      filenames.  Likewise for l_binaryRead() and l_binaryWrite().
 *  (5) For moving, copying and removing files and directories that are in
 *      subdirectories of /tmp, use the lept_*() file system shell wrappers:
 *         lept_mkdir(), lept_rmdir(), lept_mv(), lept_rm() and lept_cp().
 *  (6) Use the lept_*() C library wrappers.  These work properly on
 *      Windows, where the same DLL must perform complementary operations
 *      on file streams (open/close) and heap memory (malloc/free):
 *         lept_fopen(), lept_fclose(), lept_calloc() and lept_free().
 */

#include <string.h>
#include <time.h>
#ifdef _MSC_VER
#include <process.h>
#include <direct.h>
#else
#include <unistd.h>
#endif   /* _MSC_VER */
#include "allheaders.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>  /* for stat, mkdir(2) */
#include <sys/types.h>
#endif


#include <stddef.h>


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

    if (!src) {
        L_WARNING("src not defined\n", procName);
        return NULL;
    }

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
 *              src string (<optional> can be null)
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
 *              src string (<optional> can be null)
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
    return size;  /* didn't find a NUL byte */
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
 *  stringConcatNew()
 *
 *      Input:  first (first string in list)
 *              varargs  (NULL-terminated list of strings)
 *      Return: result (new string concatenating the input strings), or
 *                      NULL if first == NULL
 *
 *  Notes:
 *      (1) The last arg in the list of strings must be NULL.
 *      (2) Caller must free the returned string.
 */
char *
stringConcatNew(const char  *first, ...)
{
size_t       len;
char        *result, *ptr;
const char  *arg;
va_list      args;

    if (!first) return NULL;

        /* Find the length of the output string */
    va_start(args, first);
    len = strlen(first);
    while ((arg = va_arg(args, const char *)) != NULL)
        len += strlen(arg);
    va_end(args);
    result = (char *)CALLOC(len + 1, sizeof(char));

        /* Concatenate the args */
    va_start(args, first);
    ptr = result;
    arg = first;
    while (*arg)
        *ptr++ = *arg++;
    while ((arg = va_arg(args, const char *)) != NULL) {
        while (*arg)
            *ptr++ = *arg++;
    }
    va_end(args);
    return result;
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

    if (pcount) *pcount = 0;
    if (!src)
        return (char *)ERROR_PTR("src not defined", procName, NULL);
    if (!sub1)
        return (char *)ERROR_PTR("sub1 not defined", procName, NULL);
    if (!sub2)
        return (char *)ERROR_PTR("sub2 not defined", procName, NULL);

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
                      size_t          datalen,
                      const l_uint8  *sequence,
                      size_t          seqlen)
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
 *              &found (<return> 1 if sequence is found; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The byte arrays 'data' and 'sequence' are not C strings,
 *          as they can contain null bytes.  Therefore, for each
 *          we must give the length of the array.
 *      (2) This searches for the first occurrence in @data of @sequence,
 *          which consists of @seqlen bytes.  The parameter @seqlen
 *          must not exceed the actual length of the @sequence byte array.
 *      (3) If the sequence is not found, the offset will be 0, so you
 *          must check @found.
 */
l_int32
arrayFindSequence(const l_uint8  *data,
                  size_t          datalen,
                  const l_uint8  *sequence,
                  size_t          seqlen,
                  l_int32        *poffset,
                  l_int32        *pfound)
{
l_int32  i, j, found, lastpos;

    PROCNAME("arrayFindSequence");

    if (poffset) *poffset = 0;
    if (pfound) *pfound = FALSE;
    if (!data || !sequence)
        return ERROR_INT("data & sequence not both defined", procName, 1);
    if (!poffset || !pfound)
        return ERROR_INT("&offset and &found not defined", procName, 1);

    lastpos = datalen - seqlen + 1;
    found = FALSE;
    for (i = 0; i < lastpos; i++) {
        for (j = 0; j < seqlen; j++) {
            if (data[i + j] != sequence[j])
                 break;
            if (j == seqlen - 1)
                 found = TRUE;
        }
        if (found == TRUE)
            break;
    }

    if (found == TRUE) {
        *poffset = i;
        *pfound = TRUE;
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

    if (!pnbytes)
        return (l_uint8 *)ERROR_PTR("pnbytes not defined", procName, NULL);
    *pnbytes = 0;
    if (!filename)
        return (l_uint8 *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (l_uint8 *)ERROR_PTR("file stream not opened", procName, NULL);
    data = l_binaryReadStream(fp, pnbytes);
    fclose(fp);
    return data;
}


/*!
 *  l_binaryReadStream()
 *
 *      Input:  fp (stream opened to read; can be stdin)
 *              &nbytes (<return> number of bytes read)
 *      Return: null-terminated array, or null on error
 *              (reading 0 bytes is not an error)
 *
 *  Notes:
 *      (1) The returned array is terminated with a null byte so that it can
 *          be used to read ascii data from a file into a proper C string.
 *      (2) This can be used to capture data that is piped in via stdin,
 *          because it does not require seeking within the file.
 *      (3) For example, you can read an image from stdin into memory
 *          using shell redirection, with one of these shell commands:
 *             cat <imagefile> | readprog
 *             readprog < <imagefile>
 *          where readprog is:
 *             l_uint8 *data = l_binaryReadStream(stdin, &nbytes);
 *             Pix *pix = pixReadMem(data, nbytes);
 */
l_uint8 *
l_binaryReadStream(FILE    *fp,
                   size_t  *pnbytes)
{
l_uint8  *data;
l_int32   seekable, navail, nadd, nread;
BBUFFER  *bb;

    PROCNAME("l_binaryReadStream");

    if (!pnbytes)
        return (l_uint8 *)ERROR_PTR("&nbytes not defined", procName, NULL);
    *pnbytes = 0;
    if (!fp)
        return (l_uint8 *)ERROR_PTR("fp not defined", procName, NULL);

        /* Test if the stream is seekable, by attempting to seek to
         * the start of data.  This is a no-op.  If it is seekable, use
         * l_binaryReadSelectStream() to determine the size of the
         * data to be read in advance. */
    seekable = (ftell(fp) == 0) ? 1 : 0;
    if (seekable)
        return l_binaryReadSelectStream(fp, 0, 0, pnbytes);

        /* If it is not seekable, use the bbuffer to realloc memory
         * as needed during reading. */
    bb = bbufferCreate(NULL, 4096);
    while (1) {
        navail = bb->nalloc - bb->n;
        if (navail < 4096) {
             nadd = L_MAX(bb->nalloc, 4096);
             bbufferExtendArray(bb, nadd);
        }
        nread = fread((void *)(bb->array + bb->n), 1, 4096, fp);
        bb->n += nread;
        if (nread != 4096) break;
    }

        /* Copy the data to a new array sized for the data, because
         * the bbuffer array can be nearly twice the size we need. */
    if ((data = (l_uint8 *)CALLOC(bb->n + 1, sizeof(l_uint8))) != NULL) {
        memcpy(data, bb->array, bb->n);
        *pnbytes = bb->n;
    } else {
        L_ERROR("calloc fail for data\n", procName);
    }

    bbufferDestroy(&bb);
    return data;
}


/*!
 *  l_binaryReadSelect()
 *
 *      Input:  filename
 *              start (first byte to read)
 *              nbytes (number of bytes to read; use 0 to read to end of file)
 *              &nread (<return> number of bytes actually read)
 *      Return: data, or null on error
 *
 *  Notes:
 *      (1) The returned array is terminated with a null byte so that it can
 *          be used to read ascii data from a file into a proper C string.
 */
l_uint8 *
l_binaryReadSelect(const char  *filename,
                   size_t       start,
                   size_t       nbytes,
                   size_t      *pnread)
{
l_uint8  *data;
FILE     *fp;

    PROCNAME("l_binaryReadSelect");

    if (!pnread)
        return (l_uint8 *)ERROR_PTR("pnread not defined", procName, NULL);
    *pnread = 0;
    if (!filename)
        return (l_uint8 *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (l_uint8 *)ERROR_PTR("file stream not opened", procName, NULL);
    data = l_binaryReadSelectStream(fp, start, nbytes, pnread);
    fclose(fp);
    return data;
}


/*!
 *  l_binaryReadSelectStream()
 *
 *      Input:  stream
 *              start (first byte to read)
 *              nbytes (number of bytes to read; use 0 to read to end of file)
 *              &nread (<return> number of bytes actually read)
 *      Return: null-terminated array, or null on error
 *              (reading 0 bytes is not an error)
 *
 *  Notes:
 *      (1) The returned array is terminated with a null byte so that it can
 *          be used to read ascii data from a file into a proper C string.
 *          If the file to be read is empty and @start == 0, an array
 *          with a single null byte is returned.
 *      (2) Side effect: the stream pointer is re-positioned to the
 *          beginning of the file.
 */
l_uint8 *
l_binaryReadSelectStream(FILE    *fp,
                         size_t   start,
                         size_t   nbytes,
                         size_t  *pnread)
{
l_uint8  *data;
size_t    bytesleft, bytestoread, nread, filebytes;

    PROCNAME("l_binaryReadSelectStream");

    if (!pnread)
        return (l_uint8 *)ERROR_PTR("&nread not defined", procName, NULL);
    *pnread = 0;
    if (!fp)
        return (l_uint8 *)ERROR_PTR("stream not defined", procName, NULL);

        /* Verify and adjust the parameters if necessary */
    fseek(fp, 0, SEEK_END);  /* EOF */
    filebytes = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (start > filebytes) {
        L_ERROR("start = %lu but filebytes = %lu\n", procName,
                (unsigned long)start, (unsigned long)filebytes);
        return NULL;
    }
    if (filebytes == 0)  /* start == 0; nothing to read; return null byte */
        return (l_uint8 *)CALLOC(1, 1);
    bytesleft = filebytes - start;  /* greater than 0 */
    if (nbytes == 0) nbytes = bytesleft;
    bytestoread = (bytesleft >= nbytes) ? nbytes : bytesleft;

        /* Read the data */
    if ((data = (l_uint8 *)CALLOC(1, bytestoread + 1)) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for data", procName, NULL);
    fseek(fp, start, SEEK_SET);
    nread = fread(data, 1, bytestoread, fp);
    if (nbytes != nread)
        L_INFO("%lu bytes requested; %lu bytes read\n", procName,
               (unsigned long)nbytes, (unsigned long)nread);
    *pnread = nread;
    fseek(fp, 0, SEEK_SET);
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
 *      (1) This should be used whenever you want to run fopen() to
 *          read from a stream.  Never call fopen() directory.
 *      (2) This also handles pathname conversions for Windows; in
 *          particular the rewrite:
 *             /tmp --> <Temp>
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
 *      (1) This should be used whenever you want to run fopen() to
 *          write or append to a stream.  Never call fopen() directory.
 *      (2) This also handles pathname conversions for Windows; in
 *          particular the rewrite:
 *             /tmp --> <Temp>
 */
FILE *
fopenWriteStream(const char  *filename,
                 const char  *modestring)
{
char  *fname;
FILE  *fp;

    PROCNAME("fopenWriteStream");

    if (!filename)
        return (FILE *)ERROR_PTR("filename not defined", procName, NULL);

    fname = genPathname(filename, NULL);
    fp = fopen(fname, modestring);
    FREE(fname);
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
#if 0   /* TODO: REMOVE_THIS */
/*!
 *  lept_mkdir()
 *
 *      Input:  subdir (of /tmp or its equivalent on Windows)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This makes a subdirectory of the root temp directory.
 *          The root temp directory is:
 *            /tmp      (unix)
 *            <Temp>    (windows)
 */
l_int32
lept_mkdir(const char  *subdir)
{
char    *rootdir, *dir;
l_int32  ret;

    PROCNAME("lept_mkdir");

    if (!subdir)
        return ERROR_INT("subdir not defined", procName, 1);
    if ((strlen(subdir) == 0) || (subdir[0] == '.') || (subdir[0] == '/'))
        return ERROR_INT("subdir not an actual subdirectory", procName, 1);

    rootdir = genPathname("/tmp", NULL);
    if ((dir = appendSubdirectory(rootdir, subdir)) == NULL) {
        FREE(rootdir);
        return ERROR_INT("directory name not made", procName, 1);
    }

        /* Make the subdirectory */
#ifndef _WIN32
    ret = mkdir(dir, 0777);
#else
    l_uint32 attributes = GetFileAttributes(dir);
    ret = 0;
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        ret = (CreateDirectory(dir, NULL) ? 0 : 1);
    }
#endif  /* !_WIN32 */

    FREE(rootdir);
    FREE(dir);
    return ret;
}
#endif


/*!
 *  lept_mkdir()
 *
 *      Input:  subdir (of /tmp or its equivalent on Windows)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This makes a subdirectory of the root temp directory.
 *          The root temp directory is:
 *            /tmp      (unix)
 *            <Temp>    (windows)
 */
l_int32
lept_mkdir(const char  *subdir)
{
char     *dir;
l_int32   ret;
#ifdef  _WIN32
char     *newpath;
l_uint32  attributes;
#endif  /* _WIN32 */

    PROCNAME("lept_mkdir");

    if (!subdir)
        return ERROR_INT("subdir not defined", procName, 1);
    if ((strlen(subdir) == 0) || (subdir[0] == '.') || (subdir[0] == '/'))
        return ERROR_INT("subdir not an actual subdirectory", procName, 1);

    dir = pathJoin("/tmp", subdir);

        /* Make the subdirectory */
#ifndef _WIN32
    ret = mkdir(dir, 0777);
#else
        /* Make sure the tmp director exists */
    newpath = genPathname("/tmp", NULL);
    attributes = GetFileAttributes(newpath);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        ret = (CreateDirectory(newpath, NULL) ? 0 : 1);
    }
    FREE(newpath);

    newpath = genPathname(dir, NULL);
    ret = (CreateDirectory(newpath, NULL) ? 0 : 1);
    FREE(newpath);
#endif  /*  !_WIN32 */

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
 *      (1) This removes all files from the specified subdirectory of:
 *            /tmp      (unix)
 *            <Temp>    (windows)
 *          and then removes the directory.
 *      (2) The combination
 *            lept_rmdir(subdir);
 *            lept_mkdir(subdir);
 *          is guaranteed to give you an empty subdirectory.
 */
l_int32
lept_rmdir(const char  *subdir)
{
char    *rootdir, *dir, *fname, *fullname;
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

        /* Find the temp subdirectory */
    rootdir = genPathname("/tmp", NULL);
    dir = appendSubdirectory(rootdir, subdir);
    FREE(rootdir);
    if (!dir)
        return ERROR_INT("directory name not made", procName, 1);
    lept_direxists(dir, &exists);
    if (!exists) {  /* fail silently */
        FREE(dir);
        return 0;
    }

        /* List all the files */
    if ((sa = getFilenamesInDirectory(dir)) == NULL) {
        L_ERROR("directory %s does not exist!\n", procName, dir);
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
        ret = lept_rmfile(fullname);
        FREE(fullname);
    }
    newpath = genPathname(dir, NULL);
    ret = RemoveDirectory(newpath) ? 0 : 1;
    FREE(newpath);
#endif  /* !_WIN32 */

    sarrayDestroy(&sa);
    FREE(dir);
    return ret;
}


/*!
 *  lept_direxists()
 *
 *      Input:  dir
 *              &exists (<return> 1 if it exists; 0 otherwise)
 *      Return: void
 *
 *  Notes:
 *      (1) Always use unix pathname separators.
 *      (2) For windows, does automatic translation to <temp> subdirectory
 *          if the pathname begins with '/tmp'.
 */
void
lept_direxists(const char  *dir,
               l_int32     *pexists)
{
char  *realdir;

    if (!pexists) return;
    *pexists = 0;
    if (!dir) return;
    if ((realdir = genPathname(dir, NULL)) == NULL)
        return;

#ifndef _WIN32
    {
    struct stat s;
    l_int32 err = stat(realdir, &s);
    if (err != -1 && S_ISDIR(s.st_mode))
        *pexists = 1;
    }
#else  /* _WIN32 */
    l_uint32  attributes;
    attributes = GetFileAttributes(realdir);
    if (attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        *pexists = 1;
    }
#endif  /* _WIN32 */

    FREE(realdir);
    return;
}


/*!
 *  lept_rm_match()
 *
 *      Input:  subdir (<optional>  If NULL, the removed files are in /tmp)
 *              substr (<optional> pattern to match in filename)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This removes the matched files in /tmp or a subdirectory of /tmp.
 *          Use NULL for @subdir if the files are in /tmp.
 *      (2) If @substr == NULL, this removes all files in the directory.
 *          If @substr == "" (empty), this removes no files.
 *          If both @subdir == NULL and @substr == NULL, this removes
 *          all files in /tmp.
 *      (3) Use unix pathname separators.
 *      (4) On Windows, the file is in either <Temp>, or in a
 *          subdirectory, where <Temp> is the Windows temp dir.
 *          The name translation is: /tmp --> <Temp>.
 *      (5) Error conditions:
 *            * returns -1 if the directory is not found
 *            * returns the number of files (> 0) that it was unable to remove.
 */
l_int32
lept_rm_match(const char  *subdir,
              const char  *substr)
{
char    *path, *fname;
char     tempdir[256];
l_int32  i, n, ret;
SARRAY  *sa;

    PROCNAME("lept_rm_match");

    makeTempDirname(tempdir, 256, subdir);
    if ((sa = getSortedPathnamesInDirectory(tempdir, substr, 0, 0)) == NULL)
        return ERROR_INT("sa not made", procName, -1);
    n = sarrayGetCount(sa);
    if (n == 0) {
        L_WARNING("no matching files found\n", procName);
        return 0;
    }

    ret = 0;
    for (i = 0; i < n; i++) {
        fname = sarrayGetString(sa, i, L_NOCOPY);
        path = genPathname(fname, NULL);
        if (lept_rmfile(path) != 0) {
            L_ERROR("failed to remove %s\n", procName, path);
            ret++;
        }
        FREE(path);
    }
    sarrayDestroy(&sa);
    return ret;
}


/*!
 *  lept_rm()
 *
 *      Input:  subdir (<optional>  If NULL, the removed file is in /tmp)
 *              tail (filename without the directory)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This removes the named file in /tmp or a subdirectory of /tmp.
 *          Use NULL for @subdir if the file is in /tmp.
 *      (2) Use unix pathname separators.
 *      (3) On Windows, the file is in either <Temp>, or in a
 *          subdirectory, where <Temp> is the Windows temp dir.
 *          The name translation is: /tmp --> <Temp>.
 */
l_int32
lept_rm(const char  *subdir,
        const char  *tail)
{
char    *path;
char     newtemp[256];
l_int32  ret;

    PROCNAME("lept_rm");

    if (!tail || strlen(tail) == 0)
        return ERROR_INT("tail undefined or empty", procName, 1);

    makeTempDirname(newtemp, 256, subdir);
    path = genPathname(newtemp, tail);
    ret = lept_rmfile(path);
    FREE(path);
    return ret;
}


/*!
 *  lept_rmfile()
 *
 *      Input:  filepath (full path to file including the directory)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This removes the named file.
 *      (2) Use unix pathname separators.
 *      (3) Unlike the other lept_* functions in this section, this can remove
 *          any file. It is not restricted to files that are in /tmp or a
 *          subdirectory of it.
 */
l_int32
lept_rmfile(const char  *filepath)
{
l_int32  ret;

    PROCNAME("lept_rmfile");

    if (!filepath || strlen(filepath) == 0)
        return ERROR_INT("filepath undefined or empty", procName, 1);

#ifndef _WIN32
    ret = remove(filepath);
#else
        /* Set attributes to allow deletion of read-only files */
    SetFileAttributes(filepath, FILE_ATTRIBUTE_NORMAL);
    ret = DeleteFile(filepath) ? 0 : 1;
#endif  /* !_WIN32 */

    return ret;
}


/*!
 *  lept_mv()
 *
 *      Input:  srcfile
 *              newdir (<optional>; can be NULL)
 *              newtail (<optional>; can be NULL)
 *              &newpath (<optional return> of actual path; can be NULL)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This moves @srcfile to /tmp or to a subdirectory of /tmp.
 *      (2) @srcfile can either be a full path or relative to the
 *          current directory.
 *      (3) @newdir can either specify an existing subdirectory of /tmp
 *          or can be NULL.  In the latter case, the file will be written
 *          into /tmp.
 *      (4) @newtail can either specify a filename tail or, if NULL,
 *          the filename is taken from the tail of @srcfile.
 *      (5) For debugging, the computed newpath can be returned.  It must
 *          be freed by the caller.
 *      (6) Reminders:
 *            (a) use unix pathname separators
 *            (b) on windows, there is a name translation from
 *                /tmp  -->  <Temp>
 *      (7) Examples:
 *          * newdir = NULL, newtail = NULL   ==> /tmp/src-tail
 *          * newdir = NULL, newtail = abc    ==> /tmp/abc
 *          * newdir = def, newtail = NULL    ==> /tmp/def/src-tail
 *          * newdir = def, newtail = abc     ==> /tmp/def/abc
 */
l_int32
lept_mv(const char  *srcfile,
        const char  *newdir,
        const char  *newtail,
        char       **pnewpath)
{
char    *srcpath, *newpath, *dir, *srctail;
char     newtemp[256];
l_int32  ret;

    PROCNAME("lept_mv");

    if (!srcfile)
        return ERROR_INT("srcfile not defined", procName, 1);

       /* Get canonical src pathname */
    splitPathAtDirectory(srcfile, &dir, &srctail);
    srcpath = genPathname(dir, srctail);
    FREE(dir);

        /* Require output pathname to be in /tmp/ or a subdirectory */
    makeTempDirname(newtemp, 256, newdir);
    if (!newtail || newtail[0] == '\0')
        newpath = genPathname(newtemp, srctail);
    else
        newpath = genPathname(newtemp, newtail);
    FREE(srctail);

        /* Overwrite any existing file at 'newpath' */
#ifndef _WIN32
    ret = fileCopy(srcpath, newpath);
    if (!ret)
        remove(srcpath);
#else
    ret = MoveFileEx(srcpath, newpath,
                     MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) ? 0 : 1;
#endif

    FREE(srcpath);
    if (pnewpath)
        *pnewpath = newpath;
    else
        FREE(newpath);
    return ret;
}


/*!
 *  lept_cp()
 *
 *      Input:  srcfile
 *              newdir (<optional>; can be NULL)
 *              newtail (<optional>; can be NULL)
 *              &newpath (<optional return> of actual path; can be NULL)
 *      Return: 0 on success, non-zero on failure
 *
 *  Notes:
 *      (1) This copies @srcfile to /tmp or to a subdirectory of /tmp.
 *      (2) @srcfile can either be a full path or relative to the
 *          current directory.
 *      (3) @newdir can either specify an existing subdirectory of /tmp,
 *          or can be NULL.  In the latter case, the file will be written
 *          into /tmp.
 *      (4) @newtail can either specify a filename tail or, if NULL,
 *          the filename is taken from the tail of @srcfile.
 *      (5) For debugging, the computed newpath can be returned.  It must
 *          be freed by the caller.
 *      (6) Reminders:
 *            (a) use unix pathname separators
 *            (b) on windows, there is an additional name translation from
 *                /tmp  -->  <Temp>
 *      (7) Examples:
 *          * newdir = NULL, newtail = NULL   ==> /tmp/src-tail
 *          * newdir = NULL, newtail = abc    ==> /tmp/abc
 *          * newdir = def, newtail = NULL    ==> /tmp/def/src-tail
 *          * newdir = def, newtail = abc     ==> /tmp/def/abc
 *
 */
l_int32
lept_cp(const char  *srcfile,
        const char  *newdir,
        const char  *newtail,
        char       **pnewpath)
{
char    *srcpath, *newpath, *dir, *srctail;
char     newtemp[256];
l_int32  ret;

    PROCNAME("lept_cp");

    if (!srcfile)
        return ERROR_INT("srcfile not defined", procName, 1);

       /* Get canonical src pathname */
    splitPathAtDirectory(srcfile, &dir, &srctail);
    srcpath = genPathname(dir, srctail);
    FREE(dir);

        /* Require output pathname to be in /tmp or a subdirectory */
    makeTempDirname(newtemp, 256, newdir);
    if (!newtail || newtail[0] == '\0')
        newpath = genPathname(newtemp, srctail);
    else
        newpath = genPathname(newtemp, newtail);
    FREE(srctail);

        /* Overwrite any existing file at 'newpath' */
#ifndef _WIN32
    ret = fileCopy(srcpath, newpath);
#else
    ret = CopyFile(srcpath, newpath, FALSE) ? 0 : 1;
#endif

    FREE(srcpath);
    if (pnewpath)
        *pnewpath = newpath;
    else
        FREE(newpath);
    return ret;
}


/*--------------------------------------------------------------------*
 *                     General file name operations                   *
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
 *      (4) The input can have either forward (unix) or backward (win)
 *          slash separators.  The output has unix separators.
 *          Note that Win32 pathname functions generally accept both
 *          slash forms, but the windows command line interpreter
 *          only accepts backward slashes, because forward slashes are
 *          used to demarcate switches (vs. dashes in unix).
 */
l_int32
splitPathAtDirectory(const char  *pathname,
                     char       **pdir,
                     char       **ptail)
{
char  *cpathname, *lastslash;

    PROCNAME("splitPathAtDirectory");

    if (!pdir && !ptail)
        return ERROR_INT("null input for both strings", procName, 1);
    if (pdir) *pdir = NULL;
    if (ptail) *ptail = NULL;
    if (!pathname)
        return ERROR_INT("pathname not defined", procName, 1);

    cpathname = stringNew(pathname);
    convertSepCharsInPath(cpathname, UNIX_PATH_SEPCHAR);
    lastslash = strrchr(cpathname, '/');
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
 *      (4) The input can have either forward (unix) or backward (win)
 *          slash separators.  The output has unix separators.
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
 *  appendSubdirectory()
 *
 *      Input:  dir
 *              subdir
 *      Return: concatenated directory path without trailing slash,
 *              or null on error
 *
 *  Notes:
 *      (1) Use unix pathname separators
 *      (2) Allocates a new string:  <dir>/<subdir>
 */
char *
appendSubdirectory(const char  *dir,
                   const char  *subdir)
{
char   *newdir;
size_t  len1, len2, len3, len4;

    PROCNAME("appendSubdirectory");

    if (!dir || !subdir)
        return (char *)ERROR_PTR("dir and subdir not both defined",
                                 procName, NULL);

    len1 = strlen(dir);
    len2 = strlen(subdir);
    len3 = len1 + len2 + 6;
    newdir = (char *)CALLOC(len3, 1);
    strncat(newdir, dir, len3);  /* add dir */
    if (newdir[len1 - 1] != '/')  /* add '/' if necessary */
        newdir[len1] = '/';
    if (subdir[0] == '/')  /* add subdir, stripping leading '/' */
        strncat(newdir, subdir + 1, len3);
    else
        strncat(newdir, subdir, len3);
    len4 = strlen(newdir);
    if (newdir[len4 - 1] == '/')  /* strip trailing '/' */
        newdir[len4 - 1] = '\0';

    return newdir;
}


/*--------------------------------------------------------------------*
 *                     Special file name operations                   *
 *--------------------------------------------------------------------*/
/*!
 *  convertSepCharsInPath()
 *
 *      Input:  path
 *              type (UNIX_PATH_SEPCHAR, WIN_PATH_SEPCHAR)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) In-place conversion.
 *      (2) Type is the resulting type:
 *            * UNIX_PATH_SEPCHAR:  '\\' ==> '/'
 *            * WIN_PATH_SEPCHAR:   '/' ==> '\\'
 *      (3) Virtually all path operations in leptonica use unix separators.
 */
l_int32
convertSepCharsInPath(char    *path,
                      l_int32  type)
{
l_int32  i;
size_t   len;

    PROCNAME("convertSepCharsInPath");
    if (!path)
        return ERROR_INT("path not defined", procName, 1);
    if (type != UNIX_PATH_SEPCHAR && type != WIN_PATH_SEPCHAR)
        return ERROR_INT("invalid type", procName, 1);

    len = strlen(path);
    if (type == UNIX_PATH_SEPCHAR) {
        for (i = 0; i < len; i++) {
            if (path[i] == '\\')
                path[i] = '/';
        }
    } else {  /* WIN_PATH_SEPCHAR */
        for (i = 0; i < len; i++) {
            if (path[i] == '/')
                path[i] = '\\';
        }
    }
    return 0;
}


/*!
 *  genPathname()
 *
 *      Input:  dir (<optional> directory or full path name, with or without
 *                   trailing '/')
 *              fname (<optional> file name within a directory)
 *      Return: pathname (either a directory or full path), or null on error
 *
 *  Notes:
 *      (1) This function generates actual paths in the following ways:
 *            * from two sub-parts (e.g., a directory and a file name).
 *            * from a single path full path, placed in @dir, with
 *              @fname == NULL.
 *            * from the name of a file in the local directory placed in
 *              @fname, with @dir == NULL.
 *            * if in a "/tmp" directory and on windows, the windows
 *              temp directory is used.
 *      (2) The name translation for "/tmp" on windows is:
 *               /tmp  -->   <Temp>   (windows)
 *      (3) There are four cases for the input:
 *          (a) @dir is a directory and @fname is defined: result is a full path
 *          (b) @dir is a directory and @fname is null: result is a directory
 *          (c) @dir is a full path and @fname is null: result is a full path
 *          (d) @dir is null or an empty string: start in the current dir;
 *              result is a full path
 *      (4) In all cases, the resulting pathname is not terminated with a slash
 *      (5) The caller is responsible for freeing the returned pathname.
 */
char *
genPathname(const char  *dir,
            const char  *fname)
{
char    *cdir, *pathout;
l_int32  dirlen, namelen, size;

    PROCNAME("genPathname");

    if (!dir && !fname)
        return (char *)ERROR_PTR("no input", procName, NULL);

        /* Handle the case where we start from the current directory */
    if (!dir || dir[0] == '\0') {
        if ((cdir = getcwd(NULL, 0)) == NULL)
            return (char *)ERROR_PTR("no current dir found", procName, NULL);
    } else {
        cdir = stringNew(dir);
    }

        /* Convert to unix path separators, and remove the trailing
         * slash in the directory, except when dir == "/"  */
    convertSepCharsInPath(cdir, UNIX_PATH_SEPCHAR);
    dirlen = strlen(cdir);
    if (cdir[dirlen - 1] == '/' && dirlen != 1) {
        cdir[dirlen - 1] = '\0';
        dirlen--;
    }

    namelen = (fname) ? strlen(fname) : 0;
    size = dirlen + namelen + 256;
    if ((pathout = (char *)CALLOC(size, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("pathout not made", procName, NULL);

        /* First handle the @dir (which may be a full pathname) */
#ifdef _WIN32
    if (strncmp(cdir, "/tmp", 4) != 0) {  /* not in /tmp; OK as is */
        stringCopy(pathout, cdir, dirlen);
    } else {  /* in /tmp */
            /* Start with the temp dir */
        char  dirt[MAX_PATH];
        GetTempPath(sizeof(dirt), dirt);  /* get the windows temp directory */
        stringCopy(pathout, dirt, strlen(dirt) - 1);

            /* Add the rest of cdir */
        if (dirlen > 4)
            stringCat(pathout, size, cdir + 4);
    }
#else  /* unix */
    stringCopy(pathout, cdir, dirlen);
#endif  /* _WIN32 */

       /* Now handle fname */
    if (fname && strlen(fname) > 0) {
        dirlen = strlen(pathout);
        pathout[dirlen] = '/';
        strncat(pathout, fname, namelen);
    }

    FREE(cdir);
    return pathout;
}


/*!
 *  makeTempDirname()
 *
 *      Input:  result (preallocated on stack or heap and passed in)
 *              nbytes (size of @result array, in bytes)
 *              subdir (<optional>; can be NULL or an empty string)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This generates the directory path for output temp files,
 *          written into @result, with unix separators.
 *      (2) Caller allocates @result, large enough to hold
 *          <temp>/<subdir>, where <temp> is "/tmp" on unix
 *          and some other path on windows, determined by the windows
 *          function GenTempPath().
 *      (3) Usage example:
 *           char  result[256];
 *           makeTempDirname(result, 256, "golden");
 */
l_int32
makeTempDirname(char        *result,
                size_t       nbytes,
                const char  *subdir)
{
size_t  len;

    PROCNAME("makeTempDirname");

    if (!result)
        return ERROR_INT("result not defined", procName, 1);
    if (subdir && ((subdir[0] == '.') || (subdir[0] == '/')))
        return ERROR_INT("subdir not an actual subdirectory", procName, 1);

        /* Start with <temp> directory */
#ifdef _WIN32
    char  dirt[MAX_PATH];
    GetTempPath(sizeof(dirt), dirt);
    snprintf(result, nbytes, "%s", dirt);
#else
    snprintf(result, nbytes, "%s", "/tmp");
#endif  /* _WIN32 */

        /* Optionally add input subdirectory */
    if (subdir) {
        len = strlen(result);
        strncat(result, "/", nbytes - len);
        strncat(result, subdir, nbytes - len - 1);
    }

    return 0;
}


/*!
 *  modifyTrailingSlash()
 *
 *      Input:  path (preallocated on stack or heap and passed in)
 *              nbytes (size of @path array, in bytes)
 *              flag (L_ADD_TRAIL_SLASH or L_REMOVE_TRAIL_SLASH)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This carries out the requested action if necessary.
 */
l_int32
modifyTrailingSlash(char    *path,
                    size_t   nbytes,
                    l_int32  flag)
{
char    lastchar;
size_t  len;

    PROCNAME("modifyTrailingSlash");

    if (!path)
        return ERROR_INT("path not defined", procName, 1);
    if (flag != L_ADD_TRAIL_SLASH && flag != L_REMOVE_TRAIL_SLASH)
        return ERROR_INT("invalid flag", procName, 1);

    len = strlen(path);
    lastchar = path[len - 1];
    if (flag == L_ADD_TRAIL_SLASH && lastchar != '/' && len < nbytes - 2) {
        path[len] = '/';
        path[len + 1] = '\0';
    } else if (flag == L_REMOVE_TRAIL_SLASH && lastchar == '/') {
        path[len - 1] = '\0';
    }
    return 0;
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
 *      (1) This makes a filename that is as unique as desired, and which
 *          can optionally include both the time and pid in the name.
 *      (2) Use unix-style pathname separators ('/').
 *      (3) Specifying the root directory (@dir == "/") is invalid.
 *      (4) Specifying a @tail containing '/' is invalid.
 *      (5) The most general form (@usetime = @usepid = 1) is:
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
 *      (6) N.B. The caller is responsible for freeing the returned filename.
 *          For windows, to avoid C-runtime boundary crossing problems
 *          when using DLLs, you must use lept_free() to free the name.
 *      (7) For windows, if the caller requests the directory '/tmp',
 *          this uses GetTempPath() to select the actual directory,
 *          avoiding platform-conditional code in use.  We represent
 *          the Windows temp directory by <Temp>.
 *      (8) Set @usetime = @usepid = 1 when
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
char    *newpath;
l_int32  i, buflen, usec, pid, emptytail;

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

    newpath = genPathname(dir, NULL);
    if (usetime && usepid)
        snprintf(buf, buflen, "%s/%d_%d_", newpath, usec, pid);
    else if (usetime)
        snprintf(buf, buflen, "%s/%d_", newpath, usec);
    else if (usepid)
        snprintf(buf, buflen, "%s/%d_", newpath, pid);
    else
        snprintf(buf, buflen, "%s/", newpath);
    FREE(newpath);

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
 *                       File corruption operations                    *
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
 *      (1) @loc and @size are expressed as a fraction of the file size.
 *      (2) This makes a copy of the data in @filein, where bytes in the
 *          specified region have deleted.
 *      (3) If (@loc + @size) >= 1.0, this deletes from the position
 *          represented by @loc to the end of the file.
 *      (4) It is useful for testing robustness of I/O wrappers when the
 *          data is corrupted, by simulating data corruption by deletion.
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


/*!
 *  fileCorruptByMutation()
 *
 *      Input:  filein
 *              loc (fractional location of start of randomization)
 *              size (fractional size of randomization)
 *              fileout (corrupted file)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) @loc and @size are expressed as a fraction of the file size.
 *      (2) This makes a copy of the data in @filein, where bytes in the
 *          specified region have been replaced by random data.
 *      (3) If (@loc + @size) >= 1.0, this modifies data from the position
 *          represented by @loc to the end of the file.
 *      (4) It is useful for testing robustness of I/O wrappers when the
 *          data is corrupted, by simulating data corruption.
 */
l_int32
fileCorruptByMutation(const char  *filein,
                      l_float32    loc,
                      l_float32    size,
                      const char  *fileout)
{
l_int32   i, locb, sizeb;
size_t    bytes;
l_uint8  *data;

    PROCNAME("fileCorruptByMutation");

    if (!filein || !fileout)
        return ERROR_INT("filein and fileout not both specified", procName, 1);
    if (loc < 0.0 || loc >= 1.0)
        return ERROR_INT("loc must be in [0.0 ... 1.0)", procName, 1);
    if (size <= 0.0)
        return ERROR_INT("size must be > 0.0", procName, 1);
    if (loc + size > 1.0)
        size = 1.0 - loc;

    data = l_binaryRead(filein, &bytes);
    locb = (l_int32)(loc * bytes + 0.5);
    locb = L_MIN(locb, bytes - 1);
    sizeb = (l_int32)(size * bytes + 0.5);
    sizeb = L_MAX(1, sizeb);
    sizeb = L_MIN(sizeb, bytes - locb);  /* >= 1 */
    L_INFO("Randomizing %d bytes at location %d\n", procName, sizeb, locb);

        /* Make an array of random bytes and do the substitution */
    for (i = 0; i < sizeb; i++) {
        data[locb + i] =
            (l_uint8)(255.9 * ((l_float64)rand() / (l_float64)RAND_MAX));
    }

    l_binaryWrite(fileout, "w", data, bytes);
    FREE(data);
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
