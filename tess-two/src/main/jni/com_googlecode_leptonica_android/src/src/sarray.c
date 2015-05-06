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
 *   sarray.c
 *
 *      Create/Destroy/Copy
 *          SARRAY    *sarrayCreate()
 *          SARRAY    *sarrayCreateInitialized()
 *          SARRAY    *sarrayCreateWordsFromString()
 *          SARRAY    *sarrayCreateLinesFromString()
 *          void      *sarrayDestroy()
 *          SARRAY    *sarrayCopy()
 *          SARRAY    *sarrayClone()
 *
 *      Add/Remove string
 *          l_int32    sarrayAddString()
 *          static l_int32  sarrayExtendArray()
 *          char      *sarrayRemoveString()
 *          l_int32    sarrayReplaceString()
 *          l_int32    sarrayClear()
 *
 *      Accessors
 *          l_int32    sarrayGetCount()
 *          char     **sarrayGetArray()
 *          char      *sarrayGetString()
 *          l_int32    sarrayGetRefcount()
 *          l_int32    sarrayChangeRefcount()
 *
 *      Conversion back to string
 *          char      *sarrayToString()
 *          char      *sarrayToStringRange()
 *
 *      Concatenate 2 sarrays
 *          l_int32    sarrayConcatenate()
 *          l_int32    sarrayAppendRange()
 *
 *      Pad an sarray to be the same size as another sarray
 *          l_int32    sarrayPadToSameSize()
 *
 *      Convert word sarray to (formatted) line sarray
 *          SARRAY    *sarrayConvertWordsToLines()
 *
 *      Split string on separator list
 *          SARRAY    *sarraySplitString()
 *
 *      Filter sarray
 *          SARRAY    *sarraySelectBySubstring()
 *          SARRAY    *sarraySelectByRange()
 *          l_int32    sarrayParseRange()
 *
 *      Sort
 *          SARRAY    *sarraySort()
 *          SARRAY    *sarraySortByIndex()
 *          l_int32    stringCompareLexical()
 *
 *      Serialize for I/O
 *          SARRAY    *sarrayRead()
 *          SARRAY    *sarrayReadStream()
 *          l_int32    sarrayWrite()
 *          l_int32    sarrayWriteStream()
 *          l_int32    sarrayAppend()
 *
 *      Directory filenames
 *          SARRAY    *getNumberedPathnamesInDirectory()
 *          SARRAY    *getSortedPathnamesInDirectory()
 *          SARRAY    *convertSortedToNumberedPathnames()
 *          SARRAY    *getFilenamesInDirectory()
 *
 *      These functions are important for efficient manipulation
 *      of string data, and they have found widespread use in
 *      leptonica.  For example:
 *         (1) to generate text files: e.g., PostScript and PDF
 *             wrappers around sets of images
 *         (2) to parse text files: e.g., extracting prototypes
 *             from the source to generate allheaders.h
 *         (3) to generate code for compilation: e.g., the fast
 *             dwa code for arbitrary structuring elements.
 *
 *      Comments on usage:
 *
 *          The user is responsible for correctly disposing of strings
 *          that have been extracted from sarrays:
 *            - When you want a string from an Sarray to inspect it, or
 *              plan to make a copy of it later, use sarrayGetString()
 *              with copyflag = 0.  In this case, you must neither free
 *              the string nor put it directly in another array.
 *              We provide the copyflag constant L_NOCOPY, which is 0,
 *              for this purpose:
 *                 str-not-owned = sarrayGetString(sa, index, L_NOCOPY);
 *              To extract a copy of a string, use:
 *                 str-owned = sarrayGetString(sa, index, L_COPY);
 *
 *            - When you want to insert a string that is in one
 *              array into another array (always leaving the first
 *              array intact), you have two options:
 *                 (1) use copyflag = L_COPY to make an immediate copy,
 *                     which you must then add to the second array
 *                     by insertion; namely,
 *                       str-owned = sarrayGetString(sa, index, L_COPY);
 *                       sarrayAddString(sa, str-owned, L_INSERT);
 *                 (2) use copyflag = L_NOCOPY to get another handle to
 *                     the string, in which case you must add
 *                     a copy of it to the second string array:
 *                       str-not-owned = sarrayGetString(sa, index, L_NOCOPY);
 *                       sarrayAddString(sa, str-not-owned, L_COPY).
 *
 *              In all cases, when you use copyflag = L_COPY to extract
 *              a string from an array, you must either free it
 *              or insert it in an array that will be freed later.
 */

#include <string.h>
#ifndef _WIN32
#include <dirent.h>     /* unix only */
#endif  /* ! _WIN32 */
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 50;     /* n'importe quoi */
static const l_int32  L_BUF_SIZE = 512;

    /* Static function */
static l_int32 sarrayExtendArray(SARRAY *sa);


/*--------------------------------------------------------------------------*
 *                   String array create/destroy/copy/extend                *
 *--------------------------------------------------------------------------*/
/*!
 *  sarrayCreate()
 *
 *      Input:  size of string ptr array to be alloc'd
 *              (use 0 for default)
 *      Return: sarray, or null on error
 */
SARRAY *
sarrayCreate(l_int32  n)
{
SARRAY  *sa;

    PROCNAME("sarrayCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((sa = (SARRAY *)CALLOC(1, sizeof(SARRAY))) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    if ((sa->array = (char **)CALLOC(n, sizeof(char *))) == NULL)
        return (SARRAY *)ERROR_PTR("ptr array not made", procName, NULL);

    sa->nalloc = n;
    sa->n = 0;
    sa->refcount = 1;
    return sa;
}


/*!
 *  sarrayCreateInitialized()
 *
 *      Input:  n (size of string ptr array to be alloc'd)
 *              initstr (string to be initialized on the full array)
 *      Return: sarray, or null on error
 */
SARRAY *
sarrayCreateInitialized(l_int32  n,
                        char    *initstr)
{
l_int32  i;
SARRAY  *sa;

    PROCNAME("sarrayCreateInitialized");

    if (n <= 0)
        return (SARRAY *)ERROR_PTR("n must be > 0", procName, NULL);
    if (!initstr)
        return (SARRAY *)ERROR_PTR("initstr not defined", procName, NULL);

    sa = sarrayCreate(n);
    for (i = 0; i < n; i++)
        sarrayAddString(sa, initstr, L_COPY);
    return sa;
}


/*!
 *  sarrayCreateWordsFromString()
 *
 *      Input:  string
 *      Return: sarray, or null on error
 *
 *  Notes:
 *      (1) This finds the number of word substrings, creates an sarray
 *          of this size, and puts copies of each substring into the sarray.
 */
SARRAY *
sarrayCreateWordsFromString(const char  *string)
{
char     separators[] = " \n\t";
l_int32  i, nsub, size, inword;
SARRAY  *sa;

    PROCNAME("sarrayCreateWordsFromString");

    if (!string)
        return (SARRAY *)ERROR_PTR("textstr not defined", procName, NULL);

        /* Find the number of words */
    size = strlen(string);
    nsub = 0;
    inword = FALSE;
    for (i = 0; i < size; i++) {
        if (inword == FALSE &&
           (string[i] != ' ' && string[i] != '\t' && string[i] != '\n')) {
           inword = TRUE;
           nsub++;
        } else if (inword == TRUE &&
           (string[i] == ' ' || string[i] == '\t' || string[i] == '\n')) {
           inword = FALSE;
        }
    }

    if ((sa = sarrayCreate(nsub)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    sarraySplitString(sa, string, separators);

    return sa;
}


/*!
 *  sarrayCreateLinesFromString()
 *
 *      Input:  string
 *              blankflag  (0 to exclude blank lines; 1 to include)
 *      Return: sarray, or null on error
 *
 *  Notes:
 *      (1) This finds the number of line substrings, each of which
 *          ends with a newline, and puts a copy of each substring
 *          in a new sarray.
 *      (2) The newline characters are removed from each substring.
 */
SARRAY *
sarrayCreateLinesFromString(char    *string,
                            l_int32  blankflag)
{
l_int32  i, nsub, size, startptr;
char    *cstring, *substring;
SARRAY  *sa;

    PROCNAME("sarrayCreateLinesFromString");

    if (!string)
        return (SARRAY *)ERROR_PTR("textstr not defined", procName, NULL);

        /* find the number of lines */
    size = strlen(string);
    nsub = 0;
    for (i = 0; i < size; i++) {
        if (string[i] == '\n')
            nsub++;
    }

    if ((sa = sarrayCreate(nsub)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);

    if (blankflag) {  /* keep blank lines as null strings */
            /* Make a copy for munging */
        if ((cstring = stringNew(string)) == NULL)
            return (SARRAY *)ERROR_PTR("cstring not made", procName, NULL);
            /* We'll insert nulls like strtok */
        startptr = 0;
        for (i = 0; i < size; i++) {
            if (cstring[i] == '\n') {
                cstring[i] = '\0';
                if (i > 0 && cstring[i - 1] == '\r')
                    cstring[i - 1] = '\0';  /* also remove Windows CR */
                if ((substring = stringNew(cstring + startptr)) == NULL)
                    return (SARRAY *)ERROR_PTR("substring not made",
                                                procName, NULL);
                sarrayAddString(sa, substring, L_INSERT);
/*                fprintf(stderr, "substring = %s\n", substring); */
                startptr = i + 1;
            }
        }
        if (startptr < size) {  /* no newline at end of last line */
            if ((substring = stringNew(cstring + startptr)) == NULL)
                return (SARRAY *)ERROR_PTR("substring not made",
                                            procName, NULL);
            sarrayAddString(sa, substring, L_INSERT);
/*            fprintf(stderr, "substring = %s\n", substring); */
        }
        FREE(cstring);
    } else {  /* remove blank lines; use strtok */
        sarraySplitString(sa, string, "\r\n");
    }

    return sa;
}


/*!
 *  sarrayDestroy()
 *
 *      Input:  &sarray <to be nulled>
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the sarray.
 *      (2) Always nulls the input ptr.
 */
void
sarrayDestroy(SARRAY  **psa)
{
l_int32  i;
SARRAY  *sa;

    PROCNAME("sarrayDestroy");

    if (psa == NULL) {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }
    if ((sa = *psa) == NULL)
        return;

    sarrayChangeRefcount(sa, -1);
    if (sarrayGetRefcount(sa) <= 0) {
        if (sa->array) {
            for (i = 0; i < sa->n; i++) {
                if (sa->array[i])
                    FREE(sa->array[i]);
            }
            FREE(sa->array);
        }
        FREE(sa);
    }

    *psa = NULL;
    return;
}


/*!
 *  sarrayCopy()
 *
 *      Input:  sarray
 *      Return: copy of sarray, or null on error
 */
SARRAY *
sarrayCopy(SARRAY  *sa)
{
l_int32  i;
SARRAY  *csa;

    PROCNAME("sarrayCopy");

    if (!sa)
        return (SARRAY *)ERROR_PTR("sa not defined", procName, NULL);

    if ((csa = sarrayCreate(sa->nalloc)) == NULL)
        return (SARRAY *)ERROR_PTR("csa not made", procName, NULL);

    for (i = 0; i < sa->n; i++)
        sarrayAddString(csa, sa->array[i], L_COPY);

    return csa;
}


/*!
 *  sarrayClone()
 *
 *      Input:  sarray
 *      Return: ptr to same sarray, or null on error
 */
SARRAY *
sarrayClone(SARRAY  *sa)
{
    PROCNAME("sarrayClone");

    if (!sa)
        return (SARRAY *)ERROR_PTR("sa not defined", procName, NULL);
    sarrayChangeRefcount(sa, 1);
    return sa;
}


/*!
 *  sarrayAddString()
 *
 *      Input:  sarray
 *              string  (string to be added)
 *              copyflag (L_INSERT, L_COPY)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Legacy usage decrees that we always use 0 to insert a string
 *          directly and 1 to insert a copy of the string.  The
 *          enums for L_INSERT and L_COPY agree with this convention,
 *          and will not change in the future.
 *      (2) See usage comments at the top of this file.
 */
l_int32
sarrayAddString(SARRAY  *sa,
                char    *string,
                l_int32  copyflag)
{
l_int32  n;

    PROCNAME("sarrayAddString");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!string)
        return ERROR_INT("string not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY)
        return ERROR_INT("invalid copyflag", procName, 1);

    n = sarrayGetCount(sa);
    if (n >= sa->nalloc)
        sarrayExtendArray(sa);

    if (copyflag == L_INSERT)
        sa->array[n] = string;
    else  /* L_COPY */
        sa->array[n] = stringNew(string);
    sa->n++;

    return 0;
}


/*!
 *  sarrayExtendArray()
 *
 *      Input:  sarray
 *      Return: 0 if OK, 1 on error
 */
static l_int32
sarrayExtendArray(SARRAY  *sa)
{
    PROCNAME("sarrayExtendArray");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);

    if ((sa->array = (char **)reallocNew((void **)&sa->array,
                              sizeof(char *) * sa->nalloc,
                              2 * sizeof(char *) * sa->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    sa->nalloc *= 2;
    return 0;
}


/*!
 *  sarrayRemoveString()
 *
 *      Input:  sarray
 *              index (of string within sarray)
 *      Return: removed string, or null on error
 */
char *
sarrayRemoveString(SARRAY  *sa,
                   l_int32  index)
{
char    *string;
char   **array;
l_int32  i, n, nalloc;

    PROCNAME("sarrayRemoveString");

    if (!sa)
        return (char *)ERROR_PTR("sa not defined", procName, NULL);

    if ((array = sarrayGetArray(sa, &nalloc, &n)) == NULL)
        return (char *)ERROR_PTR("array not returned", procName, NULL);

    if (index < 0 || index >= n)
        return (char *)ERROR_PTR("array index out of bounds", procName, NULL);

    string = array[index];

        /* If removed string is not at end of array, shift
         * to fill in, maintaining original ordering.
         * Note: if we didn't care about the order, we could
         * put the last string array[n - 1] directly into the hole.  */
    for (i = index; i < n - 1; i++)
        array[i] = array[i + 1];

    sa->n--;
    return string;
}


/*!
 *  sarrayReplaceString()
 *
 *      Input:  sarray
 *              index (of string within sarray to be replaced)
 *              newstr (string to replace existing one)
 *              copyflag (L_INSERT, L_COPY)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This destroys an existing string and replaces it with
 *          the new string or a copy of it.
 *      (2) By design, an sarray is always compacted, so there are
 *          never any holes (null ptrs) in the ptr array up to the
 *          current count.
 */
l_int32
sarrayReplaceString(SARRAY  *sa,
                    l_int32  index,
                    char    *newstr,
                    l_int32  copyflag)
{
char    *str;
l_int32  n;

    PROCNAME("sarrayReplaceString");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    n = sarrayGetCount(sa);
    if (index < 0 || index >= n)
        return ERROR_INT("array index out of bounds", procName, 1);
    if (!newstr)
        return ERROR_INT("newstr not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY)
        return ERROR_INT("invalid copyflag", procName, 1);

    FREE(sa->array[index]);
    if (copyflag == L_INSERT)
        str = newstr;
    else  /* L_COPY */
        str = stringNew(newstr);
    sa->array[index] = str;
    return 0;
}


/*!
 *  sarrayClear()
 *
 *      Input:  sarray
 *      Return: 0 if OK; 1 on error
 */
l_int32
sarrayClear(SARRAY  *sa)
{
l_int32  i;

    PROCNAME("sarrayClear");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    for (i = 0; i < sa->n; i++) {  /* free strings and null ptrs */
        FREE(sa->array[i]);
        sa->array[i] = NULL;
    }
    sa->n = 0;
    return 0;
}


/*----------------------------------------------------------------------*
 *                               Accessors                              *
 *----------------------------------------------------------------------*/
/*!
 *  sarrayGetCount()
 *
 *      Input:  sarray
 *      Return: count, or 0 if no strings or on error
 */
l_int32
sarrayGetCount(SARRAY  *sa)
{
    PROCNAME("sarrayGetCount");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 0);
    return sa->n;
}


/*!
 *  sarrayGetArray()
 *
 *      Input:  sarray
 *              &nalloc  (<optional return> number allocated string ptrs)
 *              &n  (<optional return> number allocated strings)
 *      Return: ptr to string array, or null on error
 *
 *  Notes:
 *      (1) Caution: the returned array is not a copy, so caller
 *          must not destroy it!
 */
char **
sarrayGetArray(SARRAY   *sa,
               l_int32  *pnalloc,
               l_int32  *pn)
{
char  **array;

    PROCNAME("sarrayGetArray");

    if (!sa)
        return (char **)ERROR_PTR("sa not defined", procName, NULL);

    array = sa->array;
    if (pnalloc) *pnalloc = sa->nalloc;
    if (pn) *pn = sa->n;

    return array;
}


/*!
 *  sarrayGetString()
 *
 *      Input:  sarray
 *              index   (to the index-th string)
 *              copyflag  (L_NOCOPY or L_COPY)
 *      Return: string, or null on error
 *
 *  Notes:
 *      (1) Legacy usage decrees that we always use 0 to get the
 *          pointer to the string itself, and 1 to get a copy of
 *          the string.
 *      (2) See usage comments at the top of this file.
 *      (3) To get a pointer to the string itself, use for copyflag:
 *             L_NOCOPY or 0 or FALSE
 *          To get a copy of the string, use for copyflag:
 *             L_COPY or 1 or TRUE
 *          The const values of L_NOCOPY and L_COPY are guaranteed not
 *          to change.
 */
char *
sarrayGetString(SARRAY  *sa,
                l_int32  index,
                l_int32  copyflag)
{
    PROCNAME("sarrayGetString");

    if (!sa)
        return (char *)ERROR_PTR("sa not defined", procName, NULL);
    if (index < 0 || index >= sa->n)
        return (char *)ERROR_PTR("index not valid", procName, NULL);
    if (copyflag != L_NOCOPY && copyflag != L_COPY)
        return (char *)ERROR_PTR("invalid copyflag", procName, NULL);

    if (copyflag == L_NOCOPY)
        return sa->array[index];
    else  /* L_COPY */
        return stringNew(sa->array[index]);
}


/*!
 *  sarrayGetRefCount()
 *
 *      Input:  sarray
 *      Return: refcount, or UNDEF on error
 */
l_int32
sarrayGetRefcount(SARRAY  *sa)
{
    PROCNAME("sarrayGetRefcount");

    if (!sa)
        return ERROR_INT("sa not defined", procName, UNDEF);
    return sa->refcount;
}


/*!
 *  sarrayChangeRefCount()
 *
 *      Input:  sarray
 *              delta (change to be applied)
 *      Return: 0 if OK, 1 on error
 */
l_int32
sarrayChangeRefcount(SARRAY  *sa,
                     l_int32  delta)
{
    PROCNAME("sarrayChangeRefcount");

    if (!sa)
        return ERROR_INT("sa not defined", procName, UNDEF);
    sa->refcount += delta;
    return 0;
}


/*----------------------------------------------------------------------*
 *                      Conversion to string                           *
 *----------------------------------------------------------------------*/
/*!
 *  sarrayToString()
 *
 *      Input:  sarray
 *              addnlflag (flag: 0 adds nothing to each substring
 *                               1 adds '\n' to each substring
 *                               2 adds ' ' to each substring)
 *      Return: dest string, or null on error
 *
 *  Notes:
 *      (1) Concatenates all the strings in the sarray, preserving
 *          all white space.
 *      (2) If addnlflag != 0, adds either a '\n' or a ' ' after
 *          each substring.
 *      (3) This function was NOT implemented as:
 *            for (i = 0; i < n; i++)
 *                     strcat(dest, sarrayGetString(sa, i, L_NOCOPY));
 *          Do you see why?
 */
char *
sarrayToString(SARRAY  *sa,
               l_int32  addnlflag)
{
    PROCNAME("sarrayToString");

    if (!sa)
        return (char *)ERROR_PTR("sa not defined", procName, NULL);

    return sarrayToStringRange(sa, 0, 0, addnlflag);
}


/*!
 *  sarrayToStringRange()
 *
 *      Input: sarray
 *             first  (index of first string to use; starts with 0)
 *             nstrings (number of strings to append into the result; use
 *                       0 to append to the end of the sarray)
 *             addnlflag (flag: 0 adds nothing to each substring
 *                              1 adds '\n' to each substring
 *                              2 adds ' ' to each substring)
 *      Return: dest string, or null on error
 *
 *  Notes:
 *      (1) Concatenates the specified strings inthe sarray, preserving
 *          all white space.
 *      (2) If addnlflag != 0, adds either a '\n' or a ' ' after
 *          each substring.
 *      (3) If the sarray is empty, this returns a string with just
 *          the character corresponding to @addnlflag.
 */
char *
sarrayToStringRange(SARRAY  *sa,
                    l_int32  first,
                    l_int32  nstrings,
                    l_int32  addnlflag)
{
char    *dest, *src, *str;
l_int32  n, i, last, size, index, len;

    PROCNAME("sarrayToStringRange");

    if (!sa)
        return (char *)ERROR_PTR("sa not defined", procName, NULL);
    if (addnlflag != 0 && addnlflag != 1 && addnlflag != 2)
        return (char *)ERROR_PTR("invalid addnlflag", procName, NULL);

    n = sarrayGetCount(sa);

        /* Empty sa; return char corresponding to addnlflag only */
    if (n == 0) {
        if (first == 0) {
            if (addnlflag == 0)
                return stringNew("");
            if (addnlflag == 1)
                return stringNew("\n");
            else  /* addnlflag == 2) */
                return stringNew(" ");
        } else {
            return (char *)ERROR_PTR("first not valid", procName, NULL);
        }
    }

    if (first < 0 || first >= n)
        return (char *)ERROR_PTR("first not valid", procName, NULL);
    if (nstrings == 0 || (nstrings > n - first))
        nstrings = n - first;  /* no overflow */
    last = first + nstrings - 1;

    size = 0;
    for (i = first; i <= last; i++) {
        if ((str = sarrayGetString(sa, i, L_NOCOPY)) == NULL)
            return (char *)ERROR_PTR("str not found", procName, NULL);
        size += strlen(str) + 2;
    }

    if ((dest = (char *)CALLOC(size + 1, sizeof(char))) == NULL)
        return (char *)ERROR_PTR("dest not made", procName, NULL);

    index = 0;
    for (i = first; i <= last; i++) {
        src = sarrayGetString(sa, i, L_NOCOPY);
        len = strlen(src);
        memcpy(dest + index, src, len);
        index += len;
        if (addnlflag == 1) {
            dest[index] = '\n';
            index++;
        } else if (addnlflag == 2) {
            dest[index] = ' ';
            index++;
        }
    }

    return dest;
}


/*----------------------------------------------------------------------*
 *                      Concatenate 2 sarrays                           *
 *----------------------------------------------------------------------*/
/*!
 *  sarrayConcatenate()
 *
 *      Input:  sa1  (to be added to)
 *              sa2  (append to sa1)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Copies of the strings in sarray2 are added to sarray1.
 */
l_int32
sarrayConcatenate(SARRAY  *sa1,
                  SARRAY  *sa2)
{
char    *str;
l_int32  n, i;

    PROCNAME("sarrayConcatenate");

    if (!sa1)
        return ERROR_INT("sa1 not defined", procName, 1);
    if (!sa2)
        return ERROR_INT("sa2 not defined", procName, 1);

    n = sarrayGetCount(sa2);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sa2, i, L_NOCOPY);
        sarrayAddString(sa1, str, L_COPY);
    }

    return 0;
}


/*!
 *  sarrayAppendRange()
 *
 *      Input:  sa1  (to be added to)
 *              sa2  (append specified range of strings in sa2 to sa1)
 *              start (index of first string of sa2 to append)
 *              end (index of last string of sa2 to append; -1 to end of array)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Copies of the strings in sarray2 are added to sarray1.
 *      (2) The [start ... end] range is truncated if necessary.
 *      (3) Use end == -1 to append to the end of sa2.
 */
l_int32
sarrayAppendRange(SARRAY  *sa1,
                  SARRAY  *sa2,
                  l_int32  start,
                  l_int32  end)
{
char    *str;
l_int32  n, i;

    PROCNAME("sarrayAppendRange");

    if (!sa1)
        return ERROR_INT("sa1 not defined", procName, 1);
    if (!sa2)
        return ERROR_INT("sa2 not defined", procName, 1);

    if (start < 0)
        start = 0;
    n = sarrayGetCount(sa2);
    if (end < 0 || end >= n)
        end = n - 1;
    if (start > end)
        return ERROR_INT("start > end", procName, 1);

    for (i = start; i <= end; i++) {
        str = sarrayGetString(sa2, i, L_NOCOPY);
        sarrayAddString(sa1, str, L_COPY);
    }

    return 0;
}


/*----------------------------------------------------------------------*
 *          Pad an sarray to be the same size as another sarray         *
 *----------------------------------------------------------------------*/
/*!
 *  sarrayPadToSameSize()
 *
 *      Input:  sa1, sa2
 *              padstring
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If two sarrays have different size, this adds enough
 *          instances of @padstring to the smaller so that they are
 *          the same size.  It is useful when two or more sarrays
 *          are being sequenced in parallel, and it is necessary to
 *          find a valid string at each index.
 */
l_int32
sarrayPadToSameSize(SARRAY  *sa1,
                    SARRAY  *sa2,
                    char    *padstring)
{
l_int32  i, n1, n2;

    PROCNAME("sarrayPadToSameSize");

    if (!sa1 || !sa2)
        return ERROR_INT("both sa1 and sa2 not defined", procName, 1);

    n1 = sarrayGetCount(sa1);
    n2 = sarrayGetCount(sa2);
    if (n1 < n2) {
        for (i = n1; i < n2; i++)
            sarrayAddString(sa1, padstring, L_COPY);
    } else if (n1 > n2) {
        for (i = n2; i < n1; i++)
            sarrayAddString(sa2, padstring, L_COPY);
    }

    return 0;
}


/*----------------------------------------------------------------------*
 *                   Convert word sarray to line sarray                 *
 *----------------------------------------------------------------------*/
/*!
 *  sarrayConvertWordsToLines()
 *
 *      Input:  sa  (sa of individual words)
 *              linesize  (max num of chars in each line)
 *      Return: saout (sa of formatted lines), or null on error
 *
 *  This is useful for re-typesetting text to a specific maximum
 *  line length.  The individual words in the input sarray
 *  are concatenated into textlines.  An input word string of zero
 *  length is taken to be a paragraph separator.  Each time
 *  such a string is found, the current line is ended and
 *  a new line is also produced that contains just the
 *  string of zero length ("").  When the output sarray
 *  of lines is eventually converted to a string with newlines
 *  (typically) appended to each line string, the empty
 *  strings are just converted to newlines, producing the visible
 *  paragraph separation.
 *
 *  What happens when a word is larger than linesize?
 *  We write it out as a single line anyway!  Words preceding
 *  or following this long word are placed on lines preceding
 *  or following the line with the long word.  Why this choice?
 *  Long "words" found in text documents are typically URLs, and
 *  it's often desirable not to put newlines in the middle of a URL.
 *  The text display program (e.g., text editor) will typically
 *  wrap the long "word" to fit in the window.
 */
SARRAY *
sarrayConvertWordsToLines(SARRAY  *sa,
                          l_int32  linesize)
{
char    *wd, *strl;
char     emptystring[] = "";
l_int32  n, i, len, totlen;
SARRAY  *sal, *saout;

    PROCNAME("sarrayConvertWordsToLines");

    if (!sa)
        return (SARRAY *)ERROR_PTR("sa not defined", procName, NULL);

    if ((saout = sarrayCreate(0)) == NULL)
        return (SARRAY *)ERROR_PTR("saout not defined", procName, NULL);

    n = sarrayGetCount(sa);
    totlen = 0;
    sal = NULL;
    for (i = 0; i < n; i++) {
        if (!sal) {
            if ((sal = sarrayCreate(0)) == NULL)
                return (SARRAY *)ERROR_PTR("sal not made", procName, NULL);
        }
        wd = sarrayGetString(sa, i, L_NOCOPY);
        len = strlen(wd);
        if (len == 0) {  /* end of paragraph: end line & insert blank line */
            if (totlen > 0) {
                strl = sarrayToString(sal, 2);
                sarrayAddString(saout, strl, L_INSERT);
            }
            sarrayAddString(saout, emptystring, L_COPY);
            sarrayDestroy(&sal);
            totlen = 0;
        } else if (totlen == 0 && len + 1 > linesize) {  /* long word! */
            sarrayAddString(saout, wd, L_COPY);  /* copy to one line */
        } else if (totlen + len + 1 > linesize) {  /* end line & start new */
            strl = sarrayToString(sal, 2);
            sarrayAddString(saout, strl, L_INSERT);
            sarrayDestroy(&sal);
            if ((sal = sarrayCreate(0)) == NULL)
                return (SARRAY *)ERROR_PTR("sal not made", procName, NULL);
            sarrayAddString(sal, wd, L_COPY);
            totlen = len + 1;
        } else {  /* add to current line */
            sarrayAddString(sal, wd, L_COPY);
            totlen += len + 1;
        }
    }
    if (totlen > 0) {   /* didn't end with blank line; output last line */
        strl = sarrayToString(sal, 2);
        sarrayAddString(saout, strl, L_INSERT);
        sarrayDestroy(&sal);
    }

    return saout;

}


/*----------------------------------------------------------------------*
 *                    Split string on separator list                    *
 *----------------------------------------------------------------------*/
/*
 *  sarraySplitString()
 *
 *      Input:  sa (to append to; typically empty initially)
 *              str (string to split; not changed)
 *              separators (characters that split input string)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) This uses strtokSafe().  See the notes there in utils.c.
 */
l_int32
sarraySplitString(SARRAY      *sa,
                  const char  *str,
                  const char  *separators)
{
char  *cstr, *substr, *saveptr;

    PROCNAME("sarraySplitString");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!str)
        return ERROR_INT("str not defined", procName, 1);
    if (!separators)
        return ERROR_INT("separators not defined", procName, 1);

    cstr = stringNew(str);  /* preserves const-ness of input str */
    substr = strtokSafe(cstr, separators, &saveptr);
    if (substr)
        sarrayAddString(sa, substr, L_INSERT);
    while ((substr = strtokSafe(NULL, separators, &saveptr)))
        sarrayAddString(sa, substr, L_INSERT);
    FREE(cstr);

    return 0;
}


/*----------------------------------------------------------------------*
 *                              Filter sarray                           *
 *----------------------------------------------------------------------*/
/*!
 *  sarraySelectBySubstring()
 *
 *      Input:  sain (input sarray)
 *              substr (<optional> substring for matching; can be NULL)
 *      Return: saout (output sarray, filtered with substring) or null on error
 *
 *  Notes:
 *      (1) This selects all strings in sain that have substr as a substring.
 *          Note that we can't use strncmp() because we're looking for
 *          a match to the substring anywhere within each filename.
 *      (2) If substr == NULL, returns a copy of the sarray.
 */
SARRAY *
sarraySelectBySubstring(SARRAY      *sain,
                        const char  *substr)
{
char    *str;
l_int32  n, i, offset, found;
SARRAY  *saout;

    PROCNAME("sarraySelectBySubstring");

    if (!sain)
        return (SARRAY *)ERROR_PTR("sain not defined", procName, NULL);

    n = sarrayGetCount(sain);
    if (!substr || n == 0)
        return sarrayCopy(sain);

    saout = sarrayCreate(n);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sain, i, L_NOCOPY);
        arrayFindSequence((l_uint8 *)str, strlen(str), (l_uint8 *)substr,
                          strlen(substr), &offset, &found);
        if (found)
            sarrayAddString(saout, str, L_COPY);
    }

    return saout;
}


/*!
 *  sarraySelectByRange()
 *
 *      Input:  sain (input sarray)
 *              first (index of first string to be selected)
 *              last (index of last string to be selected; use 0 to go to the
 *                    end of the sarray)
 *      Return: saout (output sarray), or null on error
 *
 *  Notes:
 *      (1) This makes @saout consisting of copies of all strings in @sain
 *          in the index set [first ... last].  Use @last == 0 to get all
 *          strings from @first to the last string in the sarray.
 */
SARRAY *
sarraySelectByRange(SARRAY  *sain,
                    l_int32  first,
                    l_int32  last)
{
char    *str;
l_int32  n, i;
SARRAY  *saout;

    PROCNAME("sarraySelectByRange");

    if (!sain)
        return (SARRAY *)ERROR_PTR("sain not defined", procName, NULL);
    if (first < 0) first = 0;
    n = sarrayGetCount(sain);
    if (last <= 0) last = n - 1;
    if (last >= n) {
        L_WARNING("@last > n - 1; setting to n - 1\n", procName);
        last = n - 1;
    }
    if (first > last)
        return (SARRAY *)ERROR_PTR("first must be >= last", procName, NULL);

    saout = sarrayCreate(0);
    for (i = first; i <= last; i++) {
        str = sarrayGetString(sain, i, L_COPY);
        sarrayAddString(saout, str, L_INSERT);
    }

    return saout;
}


/*!
 *  sarrayParseRange()
 *
 *      Input:  sa (input sarray)
 *              start (index to start range search)
 *             &actualstart (<return> index of actual start; may be > 'start')
 *             &end (<return> index of end)
 *             &newstart (<return> index of start of next range)
 *              substr (substring for matching at beginning of string)
 *              loc (byte offset within the string for the pattern; use
 *                   -1 if the location does not matter);
 *      Return: 0 if valid range found; 1 otherwise
 *
 *  Notes:
 *      (1) This finds the range of the next set of strings in SA,
 *          beginning the search at 'start', that does NOT have
 *          the substring 'substr' either at the indicated location
 *          in the string or anywhere in the string.  The input
 *          variable 'loc' is the specified offset within the string;
 *          use -1 to indicate 'anywhere in the string'.
 *      (2) Always check the return value to verify that a valid range
 *          was found.
 *      (3) If a valid range is not found, the values of actstart,
 *          end and newstart are all set to the size of sa.
 *      (4) If this is the last valid range, newstart returns the value n.
 *          In use, this should be tested before calling the function.
 *      (5) Usage example.  To find all the valid ranges in a file
 *          where the invalid lines begin with two dashes, copy each
 *          line in the file to a string in an sarray, and do:
 *             start = 0;
 *             while (!sarrayParseRange(sa, start, &actstart, &end, &start,
 *                    "--", 0))
 *                 fprintf(stderr, "start = %d, end = %d\n", actstart, end);
 */
l_int32
sarrayParseRange(SARRAY      *sa,
                 l_int32      start,
                 l_int32     *pactualstart,
                 l_int32     *pend,
                 l_int32     *pnewstart,
                 const char  *substr,
                 l_int32      loc)
{
char    *str;
l_int32  n, i, offset, found;

    PROCNAME("sarrayParseRange");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pactualstart || !pend || !pnewstart)
        return ERROR_INT("not all range addresses defined", procName, 1);
    n = sarrayGetCount(sa);
    *pactualstart = *pend = *pnewstart = n;
    if (!substr)
        return ERROR_INT("substr not defined", procName, 1);

        /* Look for the first string without the marker */
    if (start < 0 || start >= n)
        return 1;
    for (i = start; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        arrayFindSequence((l_uint8 *)str, strlen(str), (l_uint8 *)substr,
                          strlen(substr), &offset, &found);
        if (loc < 0) {
            if (!found) break;
        } else {
            if (!found || offset != loc) break;
        }
    }
    start = i;
    if (i == n)  /* couldn't get started */
        return 1;

        /* Look for the last string without the marker */
    *pactualstart = start;
    for (i = start + 1; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        arrayFindSequence((l_uint8 *)str, strlen(str), (l_uint8 *)substr,
                          strlen(substr), &offset, &found);
        if (loc < 0) {
            if (found) break;
        } else {
            if (found && offset == loc) break;
        }
    }
    *pend = i - 1;
    start = i;
    if (i == n)  /* no further range */
        return 0;

        /* Look for the first string after *pend without the marker.
         * This will start the next run of strings, if it exists. */
    for (i = start; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        arrayFindSequence((l_uint8 *)str, strlen(str), (l_uint8 *)substr,
                          strlen(substr), &offset, &found);
        if (loc < 0) {
            if (!found) break;
        } else {
            if (!found || offset != loc) break;
        }
    }
    if (i < n)
        *pnewstart = i;

    return 0;
}


/*----------------------------------------------------------------------*
 *                                   Sort                               *
 *----------------------------------------------------------------------*/
/*!
 *  sarraySort()
 *
 *      Input:  saout (output sarray; can be NULL or equal to sain)
 *              sain (input sarray)
 *              sortorder (L_SORT_INCREASING or L_SORT_DECREASING)
 *      Return: saout (output sarray, sorted by ascii value), or null on error
 *
 *  Notes:
 *      (1) Set saout = sain for in-place; otherwise, set naout = NULL.
 *      (2) Shell sort, modified from K&R, 2nd edition, p.62.
 *          Slow but simple O(n logn) sort.
 */
SARRAY *
sarraySort(SARRAY  *saout,
           SARRAY  *sain,
           l_int32  sortorder)
{
char   **array;
char    *tmp;
l_int32  n, i, j, gap;

    PROCNAME("sarraySort");

    if (!sain)
        return (SARRAY *)ERROR_PTR("sain not defined", procName, NULL);

        /* Make saout if necessary; otherwise do in-place */
    if (!saout)
        saout = sarrayCopy(sain);
    else if (sain != saout)
        return (SARRAY *)ERROR_PTR("invalid: not in-place", procName, NULL);
    array = saout->array;  /* operate directly on the array */
    n = sarrayGetCount(saout);

        /* Shell sort */
    for (gap = n/2; gap > 0; gap = gap / 2) {
        for (i = gap; i < n; i++) {
            for (j = i - gap; j >= 0; j -= gap) {
                if ((sortorder == L_SORT_INCREASING &&
                     stringCompareLexical(array[j], array[j + gap])) ||
                    (sortorder == L_SORT_DECREASING &&
                     stringCompareLexical(array[j + gap], array[j])))
                {
                    tmp = array[j];
                    array[j] = array[j + gap];
                    array[j + gap] = tmp;
                }
            }
        }
    }

    return saout;
}


/*!
 *  sarraySortByIndex()
 *
 *      Input:  sain
 *              naindex (na that maps from the new sarray to the input sarray)
 *      Return: saout (sorted), or null on error
 */
SARRAY *
sarraySortByIndex(SARRAY  *sain,
                  NUMA    *naindex)
{
char    *str;
l_int32  i, n, index;
SARRAY  *saout;

    PROCNAME("sarraySortByIndex");

    if (!sain)
        return (SARRAY *)ERROR_PTR("sain not defined", procName, NULL);
    if (!naindex)
        return (SARRAY *)ERROR_PTR("naindex not defined", procName, NULL);

    n = sarrayGetCount(sain);
    saout = sarrayCreate(n);
    for (i = 0; i < n; i++) {
        numaGetIValue(naindex, i, &index);
        str = sarrayGetString(sain, index, L_COPY);
        sarrayAddString(saout, str, L_INSERT);
    }

    return saout;
}


/*!
 *  stringCompareLexical()
 *
 *      Input:  str1
 *              str2
 *      Return: 1 if str1 > str2 (lexically); 0 otherwise
 *
 *  Notes:
 *      (1) If the lexical values are identical, return a 0, to
 *          indicate that no swapping is required to sort the strings.
 */
l_int32
stringCompareLexical(const char *str1,
                     const char *str2)
{
l_int32  i, len1, len2, len;

    PROCNAME("sarrayCompareLexical");

    if (!str1)
        return ERROR_INT("str1 not defined", procName, 1);
    if (!str2)
        return ERROR_INT("str2 not defined", procName, 1);

    len1 = strlen(str1);
    len2 = strlen(str2);
    len = L_MIN(len1, len2);

    for (i = 0; i < len; i++) {
        if (str1[i] == str2[i])
            continue;
        if (str1[i] > str2[i])
            return 1;
        else
            return 0;
    }

    if (len1 > len2)
        return 1;
    else
        return 0;
}


/*----------------------------------------------------------------------*
 *                           Serialize for I/O                          *
 *----------------------------------------------------------------------*/
/*!
 *  sarrayRead()
 *
 *      Input:  filename
 *      Return: sarray, or null on error
 */
SARRAY *
sarrayRead(const char  *filename)
{
FILE    *fp;
SARRAY  *sa;

    PROCNAME("sarrayRead");

    if (!filename)
        return (SARRAY *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (SARRAY *)ERROR_PTR("stream not opened", procName, NULL);

    if ((sa = sarrayReadStream(fp)) == NULL) {
        fclose(fp);
        return (SARRAY *)ERROR_PTR("sa not read", procName, NULL);
    }

    fclose(fp);
    return sa;
}


/*!
 *  sarrayReadStream()
 *
 *      Input:  stream
 *      Return: sarray, or null on error
 *
 *  Notes:
 *      (1) We store the size of each string along with the string.
 *      (2) This allows a string to have embedded newlines.  By reading
 *          the entire string, as determined by its size, we are
 *          not affected by any number of embedded newlines.
 */
SARRAY *
sarrayReadStream(FILE  *fp)
{
char    *stringbuf;
l_int32  i, n, size, index, bufsize, version, ignore;
SARRAY  *sa;

    PROCNAME("sarrayReadStream");

    if (!fp)
        return (SARRAY *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nSarray Version %d\n", &version) != 1)
        return (SARRAY *)ERROR_PTR("not an sarray file", procName, NULL);
    if (version != SARRAY_VERSION_NUMBER)
        return (SARRAY *)ERROR_PTR("invalid sarray version", procName, NULL);
    if (fscanf(fp, "Number of strings = %d\n", &n) != 1)
        return (SARRAY *)ERROR_PTR("error on # strings", procName, NULL);

    if ((sa = sarrayCreate(n)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    bufsize = L_BUF_SIZE + 1;
    if ((stringbuf = (char *)CALLOC(bufsize, sizeof(char))) == NULL)
        return (SARRAY *)ERROR_PTR("stringbuf not made", procName, NULL);

    for (i = 0; i < n; i++) {
            /* Get the size of the stored string */
        if (fscanf(fp, "%d[%d]:", &index, &size) != 2)
            return (SARRAY *)ERROR_PTR("error on string size", procName, NULL);
            /* Expand the string buffer if necessary */
        if (size > bufsize - 5) {
            FREE(stringbuf);
            bufsize = (l_int32)(1.5 * size);
            stringbuf = (char *)CALLOC(bufsize, sizeof(char));
        }
            /* Read the stored string, plus leading spaces and trailing \n */
        if (fread(stringbuf, 1, size + 3, fp) != size + 3)
            return (SARRAY *)ERROR_PTR("error reading string", procName, NULL);
            /* Remove the \n that was added by sarrayWriteStream() */
        stringbuf[size + 2] = '\0';
            /* Copy it in, skipping the 2 leading spaces */
        sarrayAddString(sa, stringbuf + 2, L_COPY);
    }
    ignore = fscanf(fp, "\n");

    FREE(stringbuf);
    return sa;
}


/*!
 *  sarrayWrite()
 *
 *      Input:  filename
 *              sarray
 *      Return: 0 if OK; 1 on error
 */
l_int32
sarrayWrite(const char  *filename,
            SARRAY      *sa)
{
FILE  *fp;

    PROCNAME("sarrayWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);

    if (sarrayWriteStream(fp, sa))
        return ERROR_INT("sa not written to stream", procName, 1);

    fclose(fp);
    return 0;
}


/*!
 *  sarrayWriteStream()
 *
 *      Input:  stream
 *              sarray
 *      Returns 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This appends a '\n' to each string, which is stripped
 *          off by sarrayReadStream().
 */
l_int32
sarrayWriteStream(FILE    *fp,
                  SARRAY  *sa)
{
l_int32  i, n, len;

    PROCNAME("sarrayWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);

    n = sarrayGetCount(sa);
    fprintf(fp, "\nSarray Version %d\n", SARRAY_VERSION_NUMBER);
    fprintf(fp, "Number of strings = %d\n", n);
    for (i = 0; i < n; i++) {
        len = strlen(sa->array[i]);
        fprintf(fp, "  %d[%d]:  %s\n", i, len, sa->array[i]);
    }
    fprintf(fp, "\n");

    return 0;
}


/*!
 *  sarrayAppend()
 *
 *      Input:  filename
 *              sarray
 *      Return: 0 if OK; 1 on error
 */
l_int32
sarrayAppend(const char  *filename,
             SARRAY      *sa)
{
FILE  *fp;

    PROCNAME("sarrayAppend");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "a")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);

    if (sarrayWriteStream(fp, sa))
        return ERROR_INT("sa not appended to stream", procName, 1);

    fclose(fp);
    return 0;
}


/*---------------------------------------------------------------------*
 *                           Directory filenames                       *
 *---------------------------------------------------------------------*/
/*!
 *  getNumberedPathnamesInDirectory()
 *
 *      Input:  directory name
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              numpre (number of characters in name before number)
 *              numpost (number of characters in name after the number,
 *                       up to a dot before an extension)
 *              maxnum (only consider page numbers up to this value)
 *      Return: sarray of numbered pathnames, or NULL on error
 *
 *  Notes:
 *      (1) Returns the full pathnames of the numbered filenames in
 *          the directory.  The number in the filename is the index
 *          into the sarray.  For indices for which there are no filenames,
 *          an empty string ("") is placed into the sarray.
 *          This makes reading numbered files very simple.  For example,
 *          the image whose filename includes number N can be retrieved using
 *               pixReadIndexed(sa, N);
 *      (2) If @substr is not NULL, only filenames that contain
 *          the substring can be included.  If @substr is NULL,
 *          all matching filenames are used.
 *      (3) If no numbered files are found, it returns an empty sarray,
 *          with no initialized strings.
 *      (4) It is assumed that the page number is contained within
 *          the basename (the filename without directory or extension).
 *          @numpre is the number of characters in the basename
 *          preceeding the actual page number; @numpost is the number
 *          following the page number, up to either the end of the
 *          basename or a ".", whichever comes first.
 *      (5) This is useful when all filenames contain numbers that are
 *          not necessarily consecutive.  0-padding is not required.
 *      (6) To use a O(n) matching algorithm, the largest page number
 *          is found and two internal arrays of this size are created.
 *          This maximum is constrained not to exceed @maxsum,
 *          to make sure that an unrealistically large number is not
 *          accidentally used to determine the array sizes.
 */
SARRAY *
getNumberedPathnamesInDirectory(const char  *dirname,
                                const char  *substr,
                                l_int32      numpre,
                                l_int32      numpost,
                                l_int32      maxnum)
{
l_int32  nfiles;
SARRAY  *sa, *saout;

    PROCNAME("getNumberedPathnamesInDirectory");

    if (!dirname)
        return (SARRAY *)ERROR_PTR("dirname not defined", procName, NULL);

    if ((sa = getSortedPathnamesInDirectory(dirname, substr, 0, 0)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    if ((nfiles = sarrayGetCount(sa)) == 0)
        return sarrayCreate(1);

    saout = convertSortedToNumberedPathnames(sa, numpre, numpost, maxnum);
    sarrayDestroy(&sa);
    return saout;
}


/*!
 *  getSortedPathnamesInDirectory()
 *
 *      Input:  directory name
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              first (0-based)
 *              nfiles (use 0 for all to the end)
 *      Return: sarray of sorted pathnames, or NULL on error
 *
 *  Notes:
 *      (1) Use @substr to filter filenames in the directory.  If
 *          @substr == NULL, this takes all files.
 *      (2) The files in the directory, after optional filtering by
 *          the substring, are lexically sorted in increasing order.
 *          Use @first and @nfiles to select a contiguous set of files.
 *      (3) The full pathnames are returned for the requested sequence.
 *          If no files are found after filtering, returns an empty sarray.
 */
SARRAY *
getSortedPathnamesInDirectory(const char  *dirname,
                              const char  *substr,
                              l_int32      first,
                              l_int32      nfiles)
{
char    *fname, *fullname;
l_int32  i, n, last;
SARRAY  *sa, *safiles, *saout;

    PROCNAME("getSortedPathnamesInDirectory");

    if (!dirname)
        return (SARRAY *)ERROR_PTR("dirname not defined", procName, NULL);

    if ((sa = getFilenamesInDirectory(dirname)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    safiles = sarraySelectBySubstring(sa, substr);
    sarrayDestroy(&sa);
    n = sarrayGetCount(safiles);
    if (n == 0) {
        L_WARNING("no files found\n", procName);
        return safiles;
    }

    sarraySort(safiles, safiles, L_SORT_INCREASING);

    first = L_MIN(L_MAX(first, 0), n - 1);
    if (nfiles == 0)
        nfiles = n - first;
    last = L_MIN(first + nfiles - 1, n - 1);

    saout = sarrayCreate(last - first + 1);
    for (i = first; i <= last; i++) {
        fname = sarrayGetString(safiles, i, L_NOCOPY);
        fullname = genPathname(dirname, fname);
        sarrayAddString(saout, fullname, L_INSERT);
    }

    sarrayDestroy(&safiles);
    return saout;
}


/*!
 *  convertSortedToNumberedPathnames()
 *
 *      Input:  sorted pathnames (including zero-padded integers)
 *              numpre (number of characters in name before number)
 *              numpost (number of characters in name after the number,
 *                       up to a dot before an extension)
 *              maxnum (only consider page numbers up to this value)
 *      Return: sarray of numbered pathnames, or NULL on error
 *
 *  Notes:
 *      (1) Typically, numpre = numpost = 0; e.g., when the filename
 *          just has a number followed by an optional extension.
 */
SARRAY *
convertSortedToNumberedPathnames(SARRAY   *sa,
                                 l_int32   numpre,
                                 l_int32   numpost,
                                 l_int32   maxnum)
{
char    *fname, *str;
l_int32  i, nfiles, num, index;
SARRAY  *saout;

    PROCNAME("convertSortedToNumberedPathnames");

    if (!sa)
        return (SARRAY *)ERROR_PTR("sa not defined", procName, NULL);
    if ((nfiles = sarrayGetCount(sa)) == 0)
        return sarrayCreate(1);

        /* Find the last file in the sorted array that has a number
         * that (a) matches the count pattern and (b) does not
         * exceed @maxnum.  @maxnum sets an upper limit on the size
         * of the sarray.  */
    num = 0;
    for (i = nfiles - 1; i >= 0; i--) {
      fname = sarrayGetString(sa, i, L_NOCOPY);
      num = extractNumberFromFilename(fname, numpre, numpost);
      if (num < 0) continue;
      num = L_MIN(num + 1, maxnum);
      break;
    }

    if (num <= 0)  /* none found */
        return sarrayCreate(1);

        /* Insert pathnames into the output sarray.
         * Ignore numbers that are out of the range of sarray. */
    saout = sarrayCreateInitialized(num, (char *)"");
    for (i = 0; i < nfiles; i++) {
      fname = sarrayGetString(sa, i, L_NOCOPY);
      index = extractNumberFromFilename(fname, numpre, numpost);
      if (index < 0 || index >= num) continue;
      str = sarrayGetString(saout, index, L_NOCOPY);
      if (str[0] != '\0')
          L_WARNING("\n  Multiple files with same number: %d\n",
                    procName, index);
      sarrayReplaceString(saout, index, fname, L_COPY);
    }

    return saout;
}


/*!
 *  getFilenamesInDirectory()
 *
 *      Input:  directory name
 *      Return: sarray of file names, or NULL on error
 *
 *  Notes:
 *      (1) The versions compiled under unix and cygwin use the POSIX C
 *          library commands for handling directories.  For windows,
 *          there is a separate implementation.
 *      (2) It returns an array of filename tails; i.e., only the part of
 *          the path after the last slash.
 *      (3) Use of the d_type field of dirent is not portable:
 *          "According to POSIX, the dirent structure contains a field
 *          char d_name[] of unspecified size, with at most NAME_MAX
 *          characters preceding the terminating null character.  Use
 *          of other fields will harm the portability of your programs."
 *      (4) As a consequence of (3), we note several things:
 *           - MINGW doesn't have a d_type member.
 *           - Older versions of gcc (e.g., 2.95.3) return DT_UNKNOWN
 *             for d_type from all files.
 *          On these systems, this function will return directories
 *          (except for '.' and '..', which are eliminated using
 *          the d_name field).
 */

#ifndef _WIN32

SARRAY *
getFilenamesInDirectory(const char  *dirname)
{
char           *realdir, *name;
l_int32         len;
SARRAY         *safiles;
DIR            *pdir;
struct dirent  *pdirentry;

    PROCNAME("getFilenamesInDirectory");

    if (!dirname)
        return (SARRAY *)ERROR_PTR("dirname not defined", procName, NULL);

    realdir = genPathname(dirname, NULL);
    pdir = opendir(realdir);
    FREE(realdir);
    if (!pdir)
        return (SARRAY *)ERROR_PTR("pdir not opened", procName, NULL);
    if ((safiles = sarrayCreate(0)) == NULL)
        return (SARRAY *)ERROR_PTR("safiles not made", procName, NULL);
    while ((pdirentry = readdir(pdir))) {

        /* It's nice to ignore directories.  For this it is necessary to
         * define _BSD_SOURCE in the CC command, because the DT_DIR
         * flag is non-standard.  */
#if !defined(__SOLARIS__)
        if (pdirentry->d_type == DT_DIR)
            continue;
#endif

            /* Filter out "." and ".." if they're passed through */
        name = pdirentry->d_name;
        len = strlen(name);
        if (len == 1 && name[len - 1] == '.') continue;
        if (len == 2 && name[len - 1] == '.' && name[len - 2] == '.') continue;
        sarrayAddString(safiles, name, L_COPY);
    }
    closedir(pdir);

    return safiles;
}

#else  /* _WIN32 */

    /* http://msdn2.microsoft.com/en-us/library/aa365200(VS.85).aspx */
#include <windows.h>

SARRAY *
getFilenamesInDirectory(const char  *dirname)
{
char             *pszDir;
char             *realdir;
HANDLE            hFind = INVALID_HANDLE_VALUE;
SARRAY           *safiles;
WIN32_FIND_DATAA  ffd;

    PROCNAME("getFilenamesInDirectory");

    if (!dirname)
        return (SARRAY *)ERROR_PTR("dirname not defined", procName, NULL);

    realdir = genPathname(dirname, NULL);
    pszDir = stringJoin(realdir, "\\*");
    FREE(realdir);

    if (strlen(pszDir) + 1 > MAX_PATH) {
        FREE(pszDir);
        return (SARRAY *)ERROR_PTR("dirname is too long", procName, NULL);
    }

    if ((safiles = sarrayCreate(0)) == NULL) {
        FREE(pszDir);
        return (SARRAY *)ERROR_PTR("safiles not made", procName, NULL);
    }

    hFind = FindFirstFileA(pszDir, &ffd);
    if (INVALID_HANDLE_VALUE == hFind) {
        sarrayDestroy(&safiles);
        FREE(pszDir);
        return (SARRAY *)ERROR_PTR("hFind not opened", procName, NULL);
    }

    while (FindNextFileA(hFind, &ffd) != 0) {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)  /* skip dirs */
            continue;
        convertSepCharsInPath(ffd.cFileName, UNIX_PATH_SEPCHAR);
        sarrayAddString(safiles, ffd.cFileName, L_COPY);
    }

    FindClose(hFind);
    FREE(pszDir);
    return safiles;
}

#endif  /* _WIN32 */
