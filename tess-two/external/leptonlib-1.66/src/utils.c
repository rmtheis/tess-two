/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/


/*
 *  utils.c
 *
 *       Error, warning and info procs; all invoked by macros
 *           l_int32    returnErrorInt()
 *           l_float32  returnErrorFloat()
 *           void      *returnErrorPtr()
 *           void       l_error()
 *           void       l_errorString()
 *           void       l_errorInt()
 *           void       l_errorFloat()
 *           void       l_warning()
 *           void       l_warningString()
 *           void       l_warningInt()
 *           void       l_warningInt2()
 *           void       l_warningFloat()
 *           void       l_warningFloat2()
 *           void       l_info()
 *           void       l_infoString()
 *           void       l_infoInt()
 *           void       l_infoInt2()
 *           void       l_infoFloat()
 *           void       l_infoFloat2()
 *
 *       Safe string procs
 *           char      *stringNew()
 *           l_int32    stringReplace()
 *           char      *stringJoin()
 *           char      *stringReverse()
 *           char      *strtokSafe()
 *           l_int32    stringSplitOnToken()
 *
 *       Find and replace string procs
 *           char      *stringRemoveChars()
 *           l_int32    stringFindSubstr()
 *           char      *stringReplaceSubstr()
 *           char      *stringReplaceEachSubstr()
 *           l_int32    arrayFindSequence()
 *
 *       Safe realloc
 *           void      *reallocNew()
 *
 *       Read and write between file and memory
 *           l_uint8   *arrayRead()
 *           l_uint8   *arrayReadStream()
 *           l_int32    nbytesInFile()
 *           l_int32    fnbytesInFile()
 *           l_int32    arrayWrite()
 *
 *       Copy in memory
 *           l_uint8   *arrayCopy()
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
 *       File opening
 *           FILE      *fopenReadStream()
 *
 *       File name operations
 *           l_int32    splitPathAtDirectory()
 *           l_int32    splitPathAtExtension()
 *           char      *genPathname()
 *           char      *genTempFilename()
 *           char      *mungePathnameForWindows()
 *           l_int32    extractNumberFromFilename()
 *
 *       Version number
 *           char      *getLeptonlibVersion()
 *           char      *getImagelibVersions()
 *
 *       Timing
 *           void       startTimer()
 *           l_float32  stopTimer()
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif   /* _MSC_VER */
#include "allheaders.h"

#ifdef _WIN32
#include <windows.h>
static const char sepchar = '\\';
#else
static const char sepchar = '/';
#endif


/*----------------------------------------------------------------------*
 *                 Error, warning and info message procs                *
 *                                                                      *
 *            ---------------------  N.B. ---------------------         *
 *                                                                      *
 *    (1) These functions all print messages to stderr.                 *
 *                                                                      *
 *    (2) They must be invoked only by macros, which are in             *
 *        environ.h, so that the print output can be disabled           *
 *        at compile time, using -DNO_CONSOLE_IO.                       *
 *                                                                      *
 *----------------------------------------------------------------------*/
/*!
 *  returnErrorInt()
 *
 *      Input:  msg (error message)
 *              procname
 *              ival (return val)
 *      Return: ival (typically 1)
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


/*!
 *  l_error()
 *
 *      Input: msg (error message)
 *             procname
 */
void
l_error(const char  *msg,
        const char  *procname)
{
    fprintf(stderr, "Error in %s: %s\n", procname, msg);
    return;
}


/*!
 *  l_errorString()
 *
 *      Input: msg (error message; must include '%s')
 *             procname
 *             str (embedded in error message via %s)
 */
void
l_errorString(const char  *msg,
              const char  *procname,
              const char  *str)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname || !str) {
        L_ERROR("msg, procname or str not defined in l_errorString()",
                procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_errorString()", procname);
        return;
    }

    sprintf(charbuf, "Error in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, str);

    FREE(charbuf);
    return;
}


/*!
 *  l_errorInt()
 *
 *      Input: msg (error message; must include '%d')
 *             procname
 *             ival (embedded in error message via %d)
 */
void
l_errorInt(const char  *msg,
           const char  *procname,
           l_int32      ival)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_errorInt()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_errorInt()", procname);
        return;
    }

    sprintf(charbuf, "Error in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, ival);

    FREE(charbuf);
    return;
}


/*!
 *  l_errorFloat()
 *
 *      Input: msg (error message; must include '%f')
 *             procname
 *             fval (embedded in error message via %f)
 */
void
l_errorFloat(const char  *msg,
             const char  *procname,
             l_float32    fval)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_errorFloat()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_errorFloat()", procname);
        return;
    }

    sprintf(charbuf, "Error in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, fval);

    FREE(charbuf);
    return;
}


/*!
 *  l_warning()
 *
 *      Input: msg (warning message)
 *             procname
 */
void
l_warning(const char  *msg,
          const char  *procname)
{
    fprintf(stderr, "Warning in %s: %s\n", procname, msg);
    return;
}


/*!
 *  l_warningString()
 *
 *      Input: msg (warning message; must include '%s')
 *             procname
 *             str (embedded in warning message via %s)
 */
void
l_warningString(const char  *msg,
                const char  *procname,
                const char  *str)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname || !str) {
        L_ERROR("msg, procname or str not defined in l_warningString()",
                procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_warningString()", procname);
        return;
    }

    sprintf(charbuf, "Warning in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, str);

    FREE(charbuf);
    return;
}


/*!
 *  l_warningInt()
 *
 *      Input: msg (warning message; must include '%d')
 *             procname
 *             ival (embedded in warning message via %d)
 */
void
l_warningInt(const char  *msg,
             const char  *procname,
             l_int32      ival)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_warningInt()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_warningInt()", procname);
        return;
    }

    sprintf(charbuf, "Warning in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, ival);

    FREE(charbuf);
    return;
}


/*!
 *  l_warningInt2()
 *
 *      Input: msg (warning message; must include '%d')
 *             procname
 *             ival1, ival2 (two args, embedded in message via %d)
 */
void
l_warningInt2(const char  *msg,
              const char  *procname,
              l_int32      ival1,
              l_int32      ival2)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_warningInt2()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_warningInt()", procname);
        return;
    }

    sprintf(charbuf, "Warning in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, ival1, ival2);

    FREE(charbuf);
    return;
}


/*!
 *  l_warningFloat()
 *
 *      Input: msg (warning message; must include '%f')
 *             procname
 *             fval (embedded in warning message via %f)
 */
void
l_warningFloat(const char  *msg,
               const char  *procname,
               l_float32    fval)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_warningFloat()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_warningFloat()", procname);
        return;
    }

    sprintf(charbuf, "Warning in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, fval);

    FREE(charbuf);
    return;
}


/*!
 *  l_warningFloat2()
 *
 *      Input: msg (warning message; must include '%f')
 *             procname
 *             fval1, fval2 (two args, embedded in message via %f)
 */
void
l_warningFloat2(const char  *msg,
                const char  *procname,
                l_float32    fval1,
                l_float32    fval2)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_warningFloat2()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_warningFloat()", procname);
        return;
    }

    sprintf(charbuf, "Warning in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, fval1, fval2);

    FREE(charbuf);
    return;
}


/*!
 *  l_info()
 *
 *      Input: msg (info message)
 *             procname
 */
void
l_info(const char  *msg,
       const char  *procname)
{
    fprintf(stderr, "Info in %s: %s\n", procname, msg);
    return;
}


/*!
 *  l_infoString()
 *
 *      Input: msg (info message; must include '%s')
 *             procname
 *             str (embedded in warning message via %s)
 */
void
l_infoString(const char  *msg,
             const char  *procname,
             const char  *str)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname || !str) {
        L_ERROR("msg, procname or str not defined in l_infoString()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_infoString()", procname);
        return;
    }

    sprintf(charbuf, "Info in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, str);

    FREE(charbuf);
    return;
}


/*!
 *  l_infoInt()
 *
 *      Input: msg (info message; must include '%d')
 *             procname
 *             ival (embedded in info message via %d)
 */
void
l_infoInt(const char  *msg,
          const char  *procname, 
          l_int32      ival)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_infoInt()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_infoInt()", procname);
        return;
    }

    sprintf(charbuf, "Info in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, ival);

    FREE(charbuf);
    return;
}


/*!
 *  l_infoInt2()
 *
 *      Input: msg (info message; must include two '%d')
 *             procname
 *             ival1, ival2 (two args, embedded in info message via %d)
 */
void
l_infoInt2(const char  *msg,
           const char  *procname, 
           l_int32      ival1,
           l_int32      ival2)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_infoInt2()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_infoInt2()", procname);
        return;
    }

    sprintf(charbuf, "Info in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, ival1, ival2);

    FREE(charbuf);
    return;
}


/*!
 *  l_infoFloat()
 *
 *      Input: msg (info message; must include '%f')
 *             procname
 *             fval (embedded in info message via %f)
 */
void
l_infoFloat(const char  *msg,
            const char  *procname, 
            l_float32    fval)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_infoFloat()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_infoFloat()", procname);
        return;
    }

    sprintf(charbuf, "Info in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, fval);

    FREE(charbuf);
    return;
}


/*!
 *  l_infoFloat2()
 *
 *      Input: msg (info message; must include two '%f')
 *             procname
 *             fval1, fval2 (two args, embedded in info message via %f)
 */
void
l_infoFloat2(const char  *msg,
             const char  *procname, 
             l_float32    fval1,
             l_float32    fval2)
{
l_int32  bufsize;
char    *charbuf;

    if (!msg || !procname) {
        L_ERROR("msg or procname not defined in l_infoFloat2()", procname);
        return;
    }

    bufsize = strlen(msg) + strlen(procname) + 128;
    if ((charbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL) {
        L_ERROR("charbuf not made in l_infoFloat()", procname);
        return;
    }

    sprintf(charbuf, "Info in %s: %s\n", procname, msg);
    fprintf(stderr, charbuf, fval1, fval2);

    FREE(charbuf);
    return;
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
char  *dest;

    PROCNAME("stringNew");

    if (!src)
        return (char *)ERROR_PTR("src not defined", procName, NULL);
    
    if ((dest = (char *)CALLOC(strlen(src) + 2, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("dest not made", procName, NULL);
    strcpy(dest, src);

    return dest;
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
 *      (3) If either or both strings are null, does the reasonable thing.
 */
l_int32
stringReplace(char       **pdest,
              const char  *src)
{
char  *scopy;

    PROCNAME("stringReplace");

    if (!pdest)
        return ERROR_INT("pdest not defined", procName, 1);

    if (*pdest)
        FREE(*pdest);
    
    if (src) {
        if ((scopy = (char *)CALLOC(strlen(src) + 2, sizeof(char))) == NULL)
            return ERROR_INT("scopy not made", procName, 1);
        strcpy(scopy, src);
        *pdest = scopy;
    }
    else
        *pdest = NULL;

    return 0;
}
    

/*!
 *  stringJoin()
 *
 *      Input:  src1 string (<optional>)
 *              src2 string (<optional>)
 *      Return: concatenated string, or null on error
 *
 *  Notes:
 *      (1) This is the safe version of strcat; it makes a new string.
 *      (2) It is not an error if either or both of the strings
 *          are empty, or if either or both the pointers are null.
 */
char *
stringJoin(const char  *src1, 
           const char  *src2)
{
char    *dest;
l_int32  srclen1, srclen2, destlen;

    PROCNAME("stringJoin");

    srclen1 = srclen2 = 0;
    if (src1)
        srclen1 = strlen(src1);
    if (src2)
        srclen2 = strlen(src2);
    destlen = srclen1 + srclen2 + 3;

    if ((dest = (char *)CALLOC(destlen, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("calloc fail for dest", procName, NULL);

    if (src1)
        strcpy(dest, src1);
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
    strncpy(substr, start + istart, nchars);

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
 *      (2) This searches for the first occurrence in 'data' of
 *          the first 'seqlen' bytes of 'sequence'.  The parameter 'seqlen'
 *          must not exceed the actual length of the 'sequence' byte array.
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
    if (!poffset || !pfound)
        return ERROR_INT("&offset and &found not both defined", procName, 1);

    *pfound = 0;
    *poffset = -1;
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
        *pfound = 1;
        *poffset = i;
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
 *              size of input data to be copied (bytes)
 *              size of data to be reallocated (bytes)
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

    if (!indata)   /* nonstandard usage */
    {
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
 *                       Reading bytes from file                      *
 *--------------------------------------------------------------------*/
/*!
 *  arrayRead()
 *
 *      Input:  filename
 *              &nbytes (<return> number of bytes read)
 *      Return: array, or null on error
 */
l_uint8 *
arrayRead(const char  *fname, 
           l_int32     *pnbytes)
{
l_uint8  *data;
FILE     *fp;

    PROCNAME("arrayRead");

    if (!fname)
        return (l_uint8 *)ERROR_PTR("fname not defined", procName, NULL);
    if (!pnbytes)
        return (l_uint8 *)ERROR_PTR("pnbytes not defined", procName, NULL);
    *pnbytes = 0;

    if ((fp = fopen(fname, "rb")) == NULL)
        return (l_uint8 *)ERROR_PTR("file stream not opened", procName, NULL);

    data = arrayReadStream(fp, pnbytes);
    fclose(fp);

    return data;
}


/*!
 *  arrayReadStream()
 *
 *      Input:  stream
 *              &nbytes (<return> number of bytes read)
 *      Return: null-terminated array, or null on error
 *              (reading 0 bytes is not an error)
 *
 *  Notes:
 *      (1) N.B.: as a side effect, this always re-positions the
 *          stream ptr to the beginning of the file.
 */
l_uint8 *
arrayReadStream(FILE     *fp, 
                l_int32  *pnbytes)
{
l_uint8  *data;

    PROCNAME("arrayReadStream");

    if (!fp)
        return (l_uint8 *)ERROR_PTR("stream not defined", procName, NULL);
    if (!pnbytes)
        return (l_uint8 *)ERROR_PTR("ptr to nbytes not defined",
                                    procName, NULL);

    *pnbytes = fnbytesInFile(fp);
    if ((data = (l_uint8 *)CALLOC(1, *pnbytes + 1)) == NULL)
        return (l_uint8 *)ERROR_PTR("CALLOC fail for data", procName, NULL);
    fread(data, *pnbytes, 1, fp);
    return data;
}


/*!
 *  nbytesInFile()
 *
 *      Input:  filename
 *      Return: nbytes in file; 0 on error
 */
l_int32
nbytesInFile(const char  *filename)
{
l_int32  nbytes;
FILE    *fp;

    PROCNAME("nbytesInFile");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 0);
    if ((fp = fopen(filename, "rb")) == NULL)
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
l_int32
fnbytesInFile(FILE  *fp)
{
l_int32  nbytes, pos;

    PROCNAME("fnbytesInFile");

    if (!fp)
        return ERROR_INT("stream not open", procName, 0);

    pos = ftell(fp);          /* initial position */
    fseek(fp, 0, SEEK_END);   /* EOF */
    nbytes = ftell(fp);
    fseek(fp, 0, pos);        /* back to initial position */
    return nbytes;
}


/*!
 *  arrayWrite()
 *
 *      Input:  filename (output)
 *              operation  ("w" for write; "a" for append)
 *              data  (binary data to be written)
 *              nbytes  (size of data array)
 *      Return: 0 if OK; 1 on error
 */
l_int32
arrayWrite(const char  *filename,
           const char  *operation,
           void        *data,
           l_int32      nbytes)
{
FILE  *fp;
char   actualOperation[20];

    PROCNAME("arrayWrite");

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

    strncpy(actualOperation, operation, 2);
    strncat(actualOperation, "b", 2);  /* for windows */
    if ((fp = fopen(filename, actualOperation)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    fwrite(data, 1, nbytes, fp);
    fclose(fp);

    return 0;
}


/*--------------------------------------------------------------------*
 *                            Copy in memory                          *
 *--------------------------------------------------------------------*/
/*!
 *  arrayCopy()
 *
 *      Input:  datas
 *              size (of data array)
 *      Return: datad (on heap), or null on error
 */
l_uint8 *
arrayCopy(l_uint8  *datas,
          size_t    size)
{
l_uint8  *datad;

    PROCNAME("arrayCopy");

    if (!datas)
        return (l_uint8 *)ERROR_PTR("datas not defined", procName, NULL);

    if ((datad = (l_uint8 *)CALLOC(size, sizeof(l_uint8))) == NULL)
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
 *      Input:  filename1 (copy this file)
 *              filename2 (to this file)
 *      Return: 0 if OK, 1 on error
 */
l_int32
fileCopy(const char  *filename1,
         const char  *filename2)
{
l_uint8  *data;
l_int32   nbytes, ret;

    PROCNAME("fileCopy");

    if ((data = arrayRead(filename1, &nbytes)) == NULL)
        return ERROR_INT("data not returned", procName, 1);
    ret = arrayWrite(filename2, "w", data, nbytes);
    FREE(data);
    return ret;
}


/*!
 *  fileConcatenate()
 *
 *      Input:  filename1
 *              filename2 (file to add to filename1)
 *      Return: 0 if OK, 1 on error
 */
l_int32
fileConcatenate(const char  *filename1,
                const char  *filename2)
{
l_uint8  *data;
l_int32   nbytes;

    PROCNAME("fileConcatenate");

    if (!filename1)
        return ERROR_INT("filename1 not defined", procName, 1);
    if (!filename2)
        return ERROR_INT("filename2 not defined", procName, 1);

    data = arrayRead(filename2, &nbytes);
    arrayWrite(filename1, "a", data, nbytes);
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

    if ((fp = fopen(filename, "a")) == NULL)
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
l_int32   i, same, nbytes1, nbytes2;
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

    if ((array1 = arrayRead(fname1, &nbytes1)) == NULL)
        return ERROR_INT("array1 not read", procName, 1);
    if ((array2 = arrayRead(fname2, &nbytes2)) == NULL)
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
 *                         Opening read stream                        *
 *--------------------------------------------------------------------*/
/*!
 *  fopenReadStream()
 *
 *      Input:  filename 
 *      Return: stream or null on error
 */
FILE *
fopenReadStream(const char  *filename)
{
char  *tail;
FILE  *fp;

    PROCNAME("fopenReadStream");

    if (!filename)
        return (FILE *)ERROR_PTR("filename not defined", procName, NULL);

        /* Try input filename */
    if ((fp = fopen(filename, "rb")))
        return fp;

        /* Else, strip directory and try locally */
    splitPathAtDirectory(filename, NULL, &tail);
    if ((fp = fopen(tail, "rb"))) {
        FREE(tail);
        return fp;
    }
    FREE(tail);

    return (FILE *)ERROR_PTR("file not found", procName, NULL);
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
 *  Note: (1) if you only want the tail, input null for
 *            the root directory ptr.
 *        (2) if you only want the root directory name,
 *            input null for the tail ptr.
 *        (3) This function makes decisions based only on the lexical
 *            structure of the input.  Examples:
 *              /usr/tmp/abc  -->  dir: /usr/tmp/   tail: abc
 *              /usr/tmp/  -->  dir: /usr/tmp/   tail: [empty string]
 *              /usr/tmp  -->  dir: /usr/   tail: tmp
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
    if ((lastslash = strrchr(cpathname, sepchar))) {
        if (ptail)
            *ptail = stringNew(lastslash + 1);
        if (pdir) {
            *(lastslash + 1) = '\0';
            *pdir = cpathname;
        }
        else
            FREE(cpathname);
    }
    else {  /* no directory */
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
 *            /usr/tmp/abc.jpg  -->  basename: /usr/tmp/abc   ext: .jpg
 *            /usr/tmp/.jpg  -->  basename: /usr/tmp/   tail: .jpg
 *            /usr/tmp.jpg/  -->  basename: /usr/tmp.jpg/   tail: [empty str]
 *            ./.jpg  -->  basename: ./   tail: .jpg
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
    }
    else {
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
 *  genPathname()
 *
 *      Input:  dir (directory name, with or without trailing '/')
 *              fname (file name within the directory)
 *      Return: full pathname, or null on error
 */
char *
genPathname(const char  *dir,
            const char  *fname)
{
char    *charbuf;
l_int32  dirlen, namelen, totlen;
    
    PROCNAME("genPathname");

    if (!dir)
        return (char *)ERROR_PTR("dir not defined", procName, NULL);
    if (!fname)
        return (char *)ERROR_PTR("fname not defined", procName, NULL);

    dirlen = strlen(dir);
    namelen = strlen(fname);
    totlen = dirlen + namelen + 20;
    if ((charbuf = (char *)CALLOC(totlen, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("charbuf not made", procName, NULL);

#ifdef _WIN32
    if (stringFindSubstr(dir, "/", NULL) > 0) {
        char *tempname;
        tempname = stringReplaceEachSubstr(dir, "/", "\\", NULL);
        strncpy(charbuf, tempname, strlen(tempname));
        FREE(tempname);
    }
    else {
        strncpy(charbuf, dir, dirlen);
    }
#else
    strncpy(charbuf, dir, dirlen);
#endif  /* _WIN32 */

    dirlen = strlen(charbuf);
    if (charbuf[dirlen - 1] != sepchar)  /* append sepchar */
        charbuf[dirlen] = sepchar;
    strncat(charbuf, fname, namelen);
    return charbuf;
}


/*! 
 *  genTempFilename()
 *
 *      Input:  dir (directory name; use '.' for local dir; no trailing '/')
 *              tail (<optional>  tailname, including extension if any;
 *                                can be null)
 *              usepid (1 to include pid in filename before the tail;
 *                      0 to omit the pid.
 *      Return: temp filename, or null on error
 *
 *  Notes:
 *      (1) This function is useful when there can be more than one
 *          process writing and reading temporary files.  It will not
 *          work properly when multiple threads from a single process call
 *          this function.  Furthermore, as with any function that
 *          provides easily guessed temporary filenames, it is not designed
 *          to be safe from an attack where the intruder is logged onto
 *          the server.
 *      (2) For windows, if the caller requests '/tmp', use GetTempPath()
 *          to select the actual directory.  This avoids using
 *          platform-conditional code wherever this is used.
 *      (3) When @usepid == 1, the output filename is:
 *              <dir>/<pid>_<tail>
 *          Otherwise it is simply
 *              <dir>/<tail>
 */
char *
genTempFilename(const char  *dir,
                const char  *tail,
                l_int32      usepid)
{
char     buf[256];
l_int32  pid;
    
    PROCNAME("genTempFilename");

    if (!dir)
        return (char *)ERROR_PTR("dir not defined", procName, NULL);

    if (usepid) pid = getpid();

#ifdef _WIN32
    {  /* do not assume /tmp exists */
    char  dirt[MAX_PATH];
    if (!strcmp(dir, "/tmp"))
        GetTempPath(sizeof(dirt), dirt);
    else
        snprintf(dirt, sizeof(dirt), "%s\\", dir);  /* add trailing '\' */
    if (usepid)
        snprintf(buf, sizeof(buf), "%s%d_", dirt, pid);
    else
        snprintf(buf, sizeof(buf), "%s", dirt);
    }
#else
    if (usepid)
        snprintf(buf, sizeof(buf), "%s/%d_", dir, pid);
    else
        snprintf(buf, sizeof(buf), "%s/", dir);
#endif

    return stringJoin(buf, tail);
}


/*! 
 *  mungePathnameForWindows()
 *
 *      Input:  namein (pathname)
 *      Return: nameout (in Windows, replace '/tmp')
 *
 *  Notes:
 *      (1) This returns a new string.  The caller is responsible
 *          for freeing it.
 *      (2) For windows, if the caller requests '/tmp', use GetTempPath()
 *          to select the actual directory.  This avoids using
 *          platform-conditional code wherever this is used.
 */
char *
mungePathnameForWindows(const char  *namein)
{
    PROCNAME("mungePathnameForWindows");

    if (!namein)
        return (char *)ERROR_PTR("namein not defined", procName, NULL);

#ifdef _WIN32
    {  /* do not assume /tmp exists */
    char   dirt[MAX_PATH];
    char  *tail, *nameout;
    if (strncmp(namein, "/tmp", 4) != 0)
        return stringNew(namein);
    GetTempPath(sizeof(dirt), dirt);
    if (strlen(namein) == 4)
        return stringNew(dirt);
    tail = stringNew(namein + 4);
    nameout = stringJoin(dirt, tail);
    FREE(tail);
    return nameout;
    }
#else
    return stringNew(namein);
#endif
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
 *                          Version number                             *
 *---------------------------------------------------------------------*/
/*!
 *  getLeptonlibVersion()
 *
 *      Return: string of version number (e.g., 'leptonlib-1.65')
 *
 *  Notes:
 *      (1) The caller has responsibility to free the memory.
 */
char *
getLeptonlibVersion()
{
    char *version = (char *)CALLOC(100, sizeof(char));

#ifdef _MSC_VER
  #ifdef _DLL
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
    char bitStr[] = " 32 bit";
  #elif _M_X64
    char bitStr[] = " 64 bit";
  #else
    char bitStr[] = ""
  #endif
    snprintf(version, 100, "leptonlib-%d.%d (%s, %s) [MSC v.%d %s %s%s]",
             LIBLEPT_MAJOR_VERSION, LIBLEPT_MINOR_VERSION,
             __DATE__, __TIME__, _MSC_VER, dllStr, debugStr, bitStr);

#else

    snprintf(version, 100, "leptonlib-%d.%d", LIBLEPT_MAJOR_VERSION,
             LIBLEPT_MINOR_VERSION);

#endif   /* _MSC_VER */
    return version;
}


/*---------------------------------------------------------------------*
 *                    Image Library Version number                     *
 *---------------------------------------------------------------------*/
/*! 
 *  getImagelibVersions()
 *
 *      Return: string of version numbers (e.g.,
 *               libgiff 4.1.6
 *               libjpeg 8b
 *               libpng 1.4.3
 *               libtiff 3.9.4
 *               zlib 1.2.5
 *
 *  Notes:
 *      (1) The caller has responsibility to free the memory.
 */
#if HAVE_LIBGIF
#include "gif_lib.h"
#endif

#if HAVE_LIBJPEG
#include "jpeglib.h"
#include "jerror.h"
#endif

#if HAVE_LIBPNG
#include "png.h"
#endif

#if HAVE_LIBTIFF
#include "tiffio.h"
#endif

#if HAVE_LIBZ
#include "zlib.h"
#endif

#define stringJoinInPlace(s1, s2) \
    tempStrP = stringJoin(s1,s2); FREE(s1); s1 = tempStrP;

char *
getImagelibVersions()
{
#if HAVE_LIBJPEG
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr err;
    char buffer[JMSG_LENGTH_MAX];
#endif
    char *tempStrP;
    char *versionNumP;
    char *nextTokenP;
    char *versionStrP = stringNew("");

#if HAVE_LIBGIF
    stringJoinInPlace(versionStrP, "libgiff 4.1.6\n");
    //strncat_s(version, 1000, GIF_LIB_VERSION, _TRUNCATE);
    //GIF_LIB_VERSION is just "4.1" so manually specify the full version.
#endif

#if HAVE_LIBJPEG
    cinfo.err = jpeg_std_error(&err);
    err.msg_code = JMSG_VERSION;
    (*err.format_message) ((j_common_ptr ) &cinfo, buffer);

    stringJoinInPlace(versionStrP, "libjpeg ");
    versionNumP = strtokSafe(buffer, " ", &nextTokenP);
    stringJoinInPlace(versionStrP, versionNumP);
    stringJoinInPlace(versionStrP, "\n");
    FREE(versionNumP);
#endif

#if HAVE_LIBPNG
    stringJoinInPlace(versionStrP, "libpng ");
    stringJoinInPlace(versionStrP, png_get_libpng_ver(NULL));
    stringJoinInPlace(versionStrP, "\n");
#endif

#if HAVE_LIBTIFF
    stringJoinInPlace(versionStrP, "libtiff ");
    versionNumP = strtokSafe((char *)TIFFGetVersion(), " \n", &nextTokenP);
    FREE(versionNumP);
    versionNumP = strtokSafe(NULL, " \n", &nextTokenP);
    FREE(versionNumP);
    versionNumP = strtokSafe(NULL, " \n", &nextTokenP);
    stringJoinInPlace(versionStrP, versionNumP);
    stringJoinInPlace(versionStrP, "\n");
    FREE(versionNumP);
#endif

#if HAVE_LIBZ
    stringJoinInPlace(versionStrP, "zlib ");
    stringJoinInPlace(versionStrP, zlibVersion());
    stringJoinInPlace(versionStrP, "\n");
#endif

    return versionStrP;
}


/*---------------------------------------------------------------------*
 *                           Timing procs                              *
 *---------------------------------------------------------------------*/
/*
 *  Example of use:
 *
 *      startTimer();
 *      ....
 *      fprintf(stderr, "Elapsed time = %7.3f sec\n", stopTimer());
 */
#ifndef _WIN32

#include <sys/time.h>
#include <sys/resource.h>

static struct rusage rusage_before;
static struct rusage rusage_after;

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

#else   /* _WIN32 : resource.h not implemented under Windows */

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
HANDLE    this_process;
FILETIME  start, stop, kernel, user;
    
    this_process = GetCurrentProcess();

    GetProcessTimes(this_process, &start, &stop, &kernel, &user);

    utime_after.LowPart  = user.dwLowDateTime;
    utime_after.HighPart = user.dwHighDateTime;

    return ((l_float32)(signed)(utime_after.QuadPart - utime_before.QuadPart) /
             10000000.0);
}

#endif
