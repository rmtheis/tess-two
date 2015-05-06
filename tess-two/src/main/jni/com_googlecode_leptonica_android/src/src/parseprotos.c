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
 * parseprotos.c
 *
 *       char             *parseForProtos()
 *
 *    Static helpers
 *       static l_int32    getNextNonCommentLine()
 *       static l_int32    getNextNonBlankLine()
 *       static l_int32    getNextNonDoubleSlashLine()
 *       static l_int32    searchForProtoSignature()
 *       static char      *captureProtoSignature()
 *       static char      *cleanProtoSignature()
 *       static l_int32    skipToEndOfFunction()
 *       static l_int32    skipToMatchingBrace()
 *       static l_int32    skipToSemicolon()
 *       static l_int32    getOffsetForCharacter()
 *       static l_int32    getOffsetForMatchingRP()
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  L_BUF_SIZE = 512;    /* max token size */

static l_int32 getNextNonCommentLine(SARRAY *sa, l_int32 start, l_int32 *pnext);
static l_int32 getNextNonBlankLine(SARRAY *sa, l_int32 start, l_int32 *pnext);
static l_int32 getNextNonDoubleSlashLine(SARRAY *sa, l_int32 start,
            l_int32 *pnext);
static l_int32 searchForProtoSignature(SARRAY *sa, l_int32 begin,
            l_int32 *pstart, l_int32 *pstop, l_int32 *pcharindex,
            l_int32 *pfound);
static char * captureProtoSignature(SARRAY *sa, l_int32 start, l_int32 stop,
            l_int32 charindex);
static char * cleanProtoSignature(char *str);
static l_int32 skipToEndOfFunction(SARRAY *sa, l_int32 start,
            l_int32 charindex, l_int32 *pnext);
static l_int32 skipToMatchingBrace(SARRAY *sa, l_int32 start,
            l_int32 lbindex, l_int32 *prbline, l_int32 *prbindex);
static l_int32 skipToSemicolon(SARRAY *sa, l_int32 start,
            l_int32 charindex, l_int32 *pnext);
static l_int32 getOffsetForCharacter(SARRAY *sa, l_int32 start, char tchar,
            l_int32 *psoffset, l_int32 *pboffset, l_int32 *ptoffset);
static l_int32 getOffsetForMatchingRP(SARRAY *sa, l_int32 start,
            l_int32 soffsetlp, l_int32 boffsetlp, l_int32 toffsetlp,
            l_int32 *psoffset, l_int32 *pboffset, l_int32 *ptoffset);


/*
 *  parseForProtos()
 *
 *      Input:  filein (output of cpp)
 *              prestring (<optional> string that prefaces each decl;
 *                        use NULL to omit)
 *      Return: parsestr (string of function prototypes), or NULL on error
 *
 *  Notes:
 *      (1) We parse the output of cpp:
 *              cpp -ansi <filein>
 *          Three plans were attempted, with success on the third.
 *      (2) Plan 1.  A cursory examination of the cpp output indicated that
 *          every function was preceeded by a cpp comment statement.
 *          So we just need to look at statements beginning after comments.
 *          Unfortunately, this is NOT the case.  Some functions start
 *          without cpp comment lines, typically when there are no
 *          comments in the source that immediately precede the function.
 *      (3) Plan 2.  Consider the keywords in the language that start
 *          parts of the cpp file.  Some, like 'typedef', 'enum',
 *          'union' and 'struct', are followed after a while by '{',
 *          and eventually end with '}, plus an optional token and a
 *          final ';'  Others, like 'extern' and 'static', are never
 *          the beginnings of global function definitions.   Function
 *          prototypes have one or more sets of '(' followed eventually
 *          by a ')', and end with ';'.  But function definitions have
 *          tokens, followed by '(', more tokens, ')' and then
 *          immediately a '{'.  We would generate a prototype from this
 *          by adding a ';' to all tokens up to the ')'.  So we use
 *          these special tokens to decide what we are parsing.  And
 *          whenever a function definition is found and the prototype
 *          extracted, we skip through the rest of the function
 *          past the corresponding '}'.  This token ends a line, and
 *          is often on a line of its own.  But as it turns out,
 *          the only keyword we need to consider is 'static'.
 *      (4) Plan 3.  Consider the parentheses and braces for various
 *          declarations.  A struct, enum, or union has a pair of
 *          braces followed by a semicolon.  They cannot have parentheses
 *          before the left brace, but a struct can have lots of parentheses
 *          within the brace set.  A function prototype has no braces.
 *          A function declaration can have sets of left and right
 *          parentheses, but these are followed by a left brace.
 *          So plan 3 looks at the way parentheses and braces are
 *          organized.  Once the beginning of a function definition
 *          is found, the prototype is extracted and we search for
 *          the ending right brace.
 *      (5) To find the ending right brace, it is necessary to do some
 *          careful parsing.  For example, in this file, we have
 *          left and right braces as characters, and these must not
 *          be counted.  Somewhat more tricky, the file fhmtauto.c
 *          generates code, and includes a right brace in a string.
 *          So we must not include braces that are in strings.  But how
 *          do we know if something is inside a string?  Keep state,
 *          starting with not-inside, and every time you hit a double quote
 *          that is not escaped, toggle the condition.  Any brace
 *          found in the state of being within a string is ignored.
 *      (6) When a prototype is extracted, it is put in a canonical
 *          form (i.e., cleaned up).  Finally, we check that it is
 *          not static and save it.  (If static, it is ignored).
 *      (7) The @prestring for unix is NULL; it is included here so that
 *          you can use Microsoft's declaration for importing or
 *          exporting to a dll.  See environ.h for examples of use.
 *          Here, we set: @prestring = "LEPT_DLL ".  Note in particular
 *          the space character that will separate 'LEPT_DLL' from
 *          the standard unix prototype that follows.
 */
char *
parseForProtos(const char *filein,
               const char *prestring)
{
char    *strdata, *str, *newstr, *parsestr, *secondword;
l_int32  start, next, stop, charindex, found;
size_t   nbytes;
SARRAY  *sa, *saout, *satest;

    PROCNAME("parseForProtos");

    if (!filein)
        return (char *)ERROR_PTR("filein not defined", procName, NULL);

        /* Read in the cpp output into memory, one string for each
         * line in the file, omitting blank lines.  */
    strdata = (char *)l_binaryRead(filein, &nbytes);
    sa = sarrayCreateLinesFromString(strdata, 0);

    saout = sarrayCreate(0);
    next = 0;
    while (1) {  /* repeat after each non-static prototype is extracted */
        searchForProtoSignature(sa, next, &start, &stop, &charindex, &found);
        if (!found)
            break;
/*        fprintf(stderr, "  start = %d, stop = %d, charindex = %d\n",
                start, stop, charindex); */
        str = captureProtoSignature(sa, start, stop, charindex);

            /* Make sure that the signature found by cpp is neither
             * static nor extern.  We get 'extern' declarations from
             * header files, and with some versions of cpp running on
             * #include <sys/stat.h> we get something of the form:
             *    extern ... (( ... )) ... ( ... ) { ...
             * For this, the 1st '(' is the lp, the 2nd ')' is the rp,
             * and there is a lot of garbage between the rp and the lb.
             * It is easiest to simply reject any signature that starts
             * with 'extern'.  Note also that an 'extern' token has been
             * prepended to each prototype, so the 'static' or
             * 'extern' keywords we are looking for, if they exist,
             * would be the second word. */
        satest = sarrayCreateWordsFromString(str);
        secondword = sarrayGetString(satest, 1, 0);
        if (strcmp(secondword, "static") &&  /* not static */
            strcmp(secondword, "extern")) {  /* not extern */
            if (prestring) {  /* prepend it to the prototype */
                newstr = stringJoin(prestring, str);
                sarrayAddString(saout, newstr, L_INSERT);
                FREE(str);
            } else {
                sarrayAddString(saout, str, L_INSERT);
            }
        } else {
            FREE(str);
        }
        sarrayDestroy(&satest);

        skipToEndOfFunction(sa, stop, charindex, &next);
        if (next == -1) break;
    }

        /* Flatten into a string with newlines between prototypes */
    parsestr = sarrayToString(saout, 1);
    FREE(strdata);
    sarrayDestroy(&sa);
    sarrayDestroy(&saout);

    return parsestr;
}


/*
 *  getNextNonCommentLine()
 *
 *      Input:  sa (output from cpp, by line)
 *              start (starting index to search)
 *              &next (<return> index of first uncommented line after
 *                     the start line)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Skips over all consecutive comment lines, beginning at 'start'
 *      (2) If all lines to the end are '#' comments, return next = -1
 */
static l_int32
getNextNonCommentLine(SARRAY  *sa,
                      l_int32  start,
                      l_int32 *pnext)
{
char    *str;
l_int32  i, n;

    PROCNAME("getNextNonCommentLine");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pnext)
        return ERROR_INT("&pnext not defined", procName, 1);

        /* Init for situation where this line and all following are comments */
    *pnext = -1;

    n = sarrayGetCount(sa);
    for (i = start; i < n; i++) {
        if ((str = sarrayGetString(sa, i, 0)) == NULL)
            return ERROR_INT("str not returned; shouldn't happen", procName, 1);
        if (str[0] != '#') {
            *pnext = i;
            return 0;
        }
    }

    return 0;
}


/*
 *  getNextNonBlankLine()
 *
 *      Input:  sa (output from cpp, by line)
 *              start (starting index to search)
 *              &next (<return> index of first nonblank line after
 *                     the start line)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Skips over all consecutive blank lines, beginning at 'start'
 *      (2) A blank line has only whitespace characters (' ', '\t', '\n', '\r')
 *      (3) If all lines to the end are blank, return next = -1
 */
static l_int32
getNextNonBlankLine(SARRAY  *sa,
                    l_int32  start,
                    l_int32 *pnext)
{
char    *str;
l_int32  i, j, n, len;

    PROCNAME("getNextNonBlankLine");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pnext)
        return ERROR_INT("&pnext not defined", procName, 1);

        /* Init for situation where this line and all following are blank */
    *pnext = -1;

    n = sarrayGetCount(sa);
    for (i = start; i < n; i++) {
        if ((str = sarrayGetString(sa, i, 0)) == NULL)
            return ERROR_INT("str not returned; shouldn't happen", procName, 1);
        len = strlen(str);
        for (j = 0; j < len; j++) {
            if (str[j] != ' ' && str[j] != '\t'
                && str[j] != '\n' && str[j] != '\r') {  /* non-blank */
                *pnext = i;
                return 0;
            }
        }
    }

    return 0;
}


/*
 *  getNextNonDoubleSlashLine()
 *
 *      Input:  sa (output from cpp, by line)
 *              start (starting index to search)
 *              &next (<return> index of first uncommented line after
 *                     the start line)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Skips over all consecutive '//' lines, beginning at 'start'
 *      (2) If all lines to the end start with '//', return next = -1
 */
static l_int32
getNextNonDoubleSlashLine(SARRAY  *sa,
                          l_int32  start,
                          l_int32 *pnext)
{
char    *str;
l_int32  i, n, len;

    PROCNAME("getNextNonDoubleSlashLine");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pnext)
        return ERROR_INT("&pnext not defined", procName, 1);

        /* Init for situation where this line and all following
         * start with '//' */
    *pnext = -1;

    n = sarrayGetCount(sa);
    for (i = start; i < n; i++) {
        if ((str = sarrayGetString(sa, i, 0)) == NULL)
            return ERROR_INT("str not returned; shouldn't happen", procName, 1);
        len = strlen(str);
        if (len < 2 || str[0] != '/' || str[1] != '/') {
            *pnext = i;
            return 0;
        }
    }

    return 0;
}


/*
 *  searchForProtoSignature()
 *
 *      Input:  sa (output from cpp, by line)
 *              begin (beginning index to search)
 *              &start (<return> starting index for function definition)
 *              &stop (<return> index of line on which proto is completed)
 *              &charindex (<return> char index of completing ')' character)
 *              &found (<return> 1 if valid signature is found; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If this returns found == 0, it means that there are no
 *          more function definitions in the file.  Caller must check
 *          this value and exit the loop over the entire cpp file.
 *      (2) This follows plan 3 (see above).  We skip comment and blank
 *          lines at the beginning.  Then we don't check for keywords.
 *          Instead, find the relative locations of the first occurrences
 *          of these four tokens: left parenthesis (lp), right
 *          parenthesis (rp), left brace (lb) and semicolon (sc).
 *      (3) The signature of a function definition looks like this:
 *               .... '(' .... ')' '{'
 *          where the lp and rp must both precede the lb, with only
 *          whitespace between the rp and the lb.  The '....'
 *          are sets of tokens that have no braces.
 *      (4) If a function definition is found, this returns found = 1,
 *          with 'start' being the first line of the definition and
 *          'charindex' being the position of the ')' in line 'stop'
 *          at the end of the arg list.
 */
static l_int32
searchForProtoSignature(SARRAY   *sa,
                        l_int32   begin,
                        l_int32  *pstart,
                        l_int32  *pstop,
                        l_int32  *pcharindex,
                        l_int32  *pfound)
{
l_int32  next, rbline, rbindex, scline;
l_int32  soffsetlp, soffsetrp, soffsetlb, soffsetsc;
l_int32  boffsetlp, boffsetrp, boffsetlb, boffsetsc;
l_int32  toffsetlp, toffsetrp, toffsetlb, toffsetsc;

    PROCNAME("searchForProtoSignature");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pstart)
        return ERROR_INT("&start not defined", procName, 1);
    if (!pstop)
        return ERROR_INT("&stop not defined", procName, 1);
    if (!pcharindex)
        return ERROR_INT("&charindex not defined", procName, 1);
    if (!pfound)
        return ERROR_INT("&found not defined", procName, 1);

    *pfound = FALSE;

    while (1) {

            /* Skip over sequential '#' comment lines */
        getNextNonCommentLine(sa, begin, &next);
        if (next == -1) return 0;
        if (next != begin) {
            begin = next;
            continue;
        }

            /* Skip over sequential blank lines */
        getNextNonBlankLine(sa, begin, &next);
        if (next == -1) return 0;
        if (next != begin) {
            begin = next;
            continue;
        }

            /* Skip over sequential lines starting with '//' */
        getNextNonDoubleSlashLine(sa, begin, &next);
        if (next == -1) return 0;
        if (next != begin) {
            begin = next;
            continue;
        }

            /* Search for specific character sequence patterns; namely
             * a lp, a matching rp, a lb and a semicolon.
             * Abort the search if no lp is found. */
        getOffsetForCharacter(sa, next, '(', &soffsetlp, &boffsetlp,
                              &toffsetlp);
        if (soffsetlp == -1)
            break;
        getOffsetForMatchingRP(sa, next, soffsetlp, boffsetlp, toffsetlp,
                               &soffsetrp, &boffsetrp, &toffsetrp);
        getOffsetForCharacter(sa, next, '{', &soffsetlb, &boffsetlb,
                              &toffsetlb);
        getOffsetForCharacter(sa, next, ';', &soffsetsc, &boffsetsc,
                              &toffsetsc);

            /* We've found a lp.  Now weed out the case where a matching
             * rp and a lb are not both found. */
        if (soffsetrp == -1 || soffsetlb == -1)
            break;

            /* Check if a left brace occurs before a left parenthesis;
             * if so, skip it */
        if (toffsetlb < toffsetlp) {
            skipToMatchingBrace(sa, next + soffsetlb, boffsetlb,
                &rbline, &rbindex);
            skipToSemicolon(sa, rbline, rbindex, &scline);
            begin = scline + 1;
            continue;
        }

            /* Check if a semicolon occurs before a left brace or
             * a left parenthesis; if so, skip it */
        if ((soffsetsc != -1) &&
            (toffsetsc < toffsetlb || toffsetsc < toffsetlp)) {
            skipToSemicolon(sa, next, 0, &scline);
            begin = scline + 1;
            continue;
        }

            /* OK, it should be a function definition.  We haven't
             * checked that there is only white space between the
             * rp and lb, but we've only seen problems with two
             * extern inlines in sys/stat.h, and this is handled
             * later by eliminating any prototype beginning with 'extern'. */
        *pstart = next;
        *pstop = next + soffsetrp;
        *pcharindex = boffsetrp;
        *pfound = TRUE;
        break;
    }

    return 0;
}


/*
 *  captureProtoSignature()
 *
 *      Input:  sa (output from cpp, by line)
 *              start (starting index to search; never a comment line)
 *              stop (index of line on which pattern is completed)
 *              charindex (char index of completing ')' character)
 *      Return: cleanstr (prototype string), or NULL on error
 *
 *  Notes:
 *      (1) Return all characters, ending with a ';' after the ')'
 */
static char *
captureProtoSignature(SARRAY  *sa,
                      l_int32  start,
                      l_int32  stop,
                      l_int32  charindex)
{
char    *str, *newstr, *protostr, *cleanstr;
SARRAY  *sap;
l_int32  i;

    PROCNAME("captureProtoSignature");

    if (!sa)
        return (char *)ERROR_PTR("sa not defined", procName, NULL);

    sap = sarrayCreate(0);
    for (i = start; i < stop; i++) {
        str = sarrayGetString(sa, i, 1);
        sarrayAddString(sap, str, 0);
    }
    str = sarrayGetString(sa, stop, 1);
    str[charindex + 1] = '\0';
    newstr = stringJoin(str, ";");
    sarrayAddString(sap, newstr, 0);
    FREE(str);
    protostr = sarrayToString(sap, 2);
    sarrayDestroy(&sap);
    cleanstr = cleanProtoSignature(protostr);
    FREE(protostr);

    return cleanstr;
}


/*
 *  cleanProtoSignature()
 *
 *      Input:  instr (input prototype string)
 *      Return: cleanstr (clean prototype string), or NULL on error
 *
 *  Notes:
 *      (1) Adds 'extern' at beginning and regularizes spaces
 *          between tokens.
 */
static char *
cleanProtoSignature(char *instr)
{
char    *str, *cleanstr;
char     buf[L_BUF_SIZE];
char     externstring[] = "extern";
l_int32  i, j, nwords, nchars, index, len;
SARRAY  *sa, *saout;

    PROCNAME("cleanProtoSignature");

    if (!instr)
        return (char *)ERROR_PTR("instr not defined", procName, NULL);

    sa = sarrayCreateWordsFromString(instr);
    nwords = sarrayGetCount(sa);
    saout = sarrayCreate(0);
    sarrayAddString(saout, externstring, 1);
    for (i = 0; i < nwords; i++) {
        str = sarrayGetString(sa, i, 0);
        nchars = strlen(str);
        index = 0;
        for (j = 0; j < nchars; j++) {
            if (index > L_BUF_SIZE - 6)
                return (char *)ERROR_PTR("token too large", procName, NULL);
            if (str[j] == '(') {
                buf[index++] = ' ';
                buf[index++] = '(';
                buf[index++] = ' ';
            } else if (str[j] == ')') {
                buf[index++] = ' ';
                buf[index++] = ')';
            } else {
                buf[index++] = str[j];
            }
        }
        buf[index] = '\0';
        sarrayAddString(saout, buf, 1);
    }

        /* Flatten to a prototype string with spaces added after
         * each word, and remove the last space */
    cleanstr = sarrayToString(saout, 2);
    len = strlen(cleanstr);
    cleanstr[len - 1] = '\0';

    sarrayDestroy(&sa);
    sarrayDestroy(&saout);
    return cleanstr;
}


/*
 *  skipToEndOfFunction()
 *
 *      Input:  sa (output from cpp, by line)
 *              start (index of starting line with left bracket to search)
 *              lbindex (starting char index for left bracket)
 *              &next (index of line following the ending '}' for function
 *      Return: 0 if OK, 1 on error
 */
static l_int32
skipToEndOfFunction(SARRAY   *sa,
                    l_int32   start,
                    l_int32   lbindex,
                    l_int32  *pnext)
{
l_int32  end, rbindex;
l_int32 soffsetlb, boffsetlb, toffsetlb;

    PROCNAME("skipToEndOfFunction");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pnext)
        return ERROR_INT("&next not defined", procName, 1);

    getOffsetForCharacter(sa, start, '{', &soffsetlb, &boffsetlb,
                &toffsetlb);
    skipToMatchingBrace(sa, start + soffsetlb, boffsetlb, &end, &rbindex);
    if (end == -1) {  /* shouldn't happen! */
        *pnext = -1;
        return 1;
    }

    *pnext = end + 1;
    return 0;
}


/*
 *  skipToMatchingBrace()
 *
 *      Input:  sa (output from cpp, by line)
 *              start (index of starting line with left bracket to search)
 *              lbindex (starting char index for left bracket)
 *              &stop (index of line with the matching right bracket)
 *              &rbindex (char index of matching right bracket)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If the matching right brace is not found, returns
 *          stop = -1.  This shouldn't happen.
 */
static l_int32
skipToMatchingBrace(SARRAY   *sa,
                    l_int32   start,
                    l_int32   lbindex,
                    l_int32  *pstop,
                    l_int32  *prbindex)
{
char    *str;
l_int32  i, j, jstart, n, sumbrace, found, instring, nchars;

    PROCNAME("skipToMatchingBrace");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pstop)
        return ERROR_INT("&stop not defined", procName, 1);
    if (!prbindex)
        return ERROR_INT("&rbindex not defined", procName, 1);

    instring = 0;  /* init to FALSE; toggle on double quotes */
    *pstop = -1;
    n = sarrayGetCount(sa);
    sumbrace = 1;
    found = FALSE;
    for (i = start; i < n; i++) {
        str = sarrayGetString(sa, i, 0);
        jstart = 0;
        if (i == start)
            jstart = lbindex + 1;
        nchars = strlen(str);
        for (j = jstart; j < nchars; j++) {
                /* Toggle the instring state every time you encounter
                 * a double quote that is NOT escaped. */
            if (j == jstart && str[j] == '\"')
                instring = 1 - instring;
            if (j > jstart && str[j] == '\"' && str[j-1] != '\\')
                instring = 1 - instring;
                /* Record the braces if they are neither a literal character
                 * nor within a string. */
            if (str[j] == '{' && str[j+1] != '\'' && !instring) {
                sumbrace++;
            } else if (str[j] == '}' && str[j+1] != '\'' && !instring) {
                sumbrace--;
                if (sumbrace == 0) {
                    found = TRUE;
                    *prbindex = j;
                    break;
                }
            }
        }
        if (found) {
            *pstop = i;
            return 0;
        }
    }

    return ERROR_INT("matching right brace not found", procName, 1);
}


/*
 *  skipToSemicolon()
 *
 *      Input:  sa (output from cpp, by line)
 *              start (index of starting line to search)
 *              charindex (starting char index for search)
 *              &next (index of line containing the next ';')
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If the semicolon isn't found, returns next = -1.
 *          This shouldn't happen.
 *      (2) This is only used in contexts where the semicolon is
 *          not within a string.
 */
static l_int32
skipToSemicolon(SARRAY   *sa,
                l_int32   start,
                l_int32   charindex,
                l_int32  *pnext)
{
char    *str;
l_int32  i, j, n, jstart, nchars, found;

    PROCNAME("skipToSemicolon");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pnext)
        return ERROR_INT("&next not defined", procName, 1);

    *pnext = -1;
    n = sarrayGetCount(sa);
    found = FALSE;
    for (i = start; i < n; i++) {
        str = sarrayGetString(sa, i, 0);
        jstart = 0;
        if (i == start)
            jstart = charindex + 1;
        nchars = strlen(str);
        for (j = jstart; j < nchars; j++) {
            if (str[j] == ';') {
                found = TRUE;;
                break;
            }
        }
        if (found) {
            *pnext = i;
            return 0;
        }
    }

    return ERROR_INT("semicolon not found", procName, 1);
}


/*
 *  getOffsetForCharacter()
 *
 *      Input:  sa (output from cpp, by line)
 *              start (starting index in sa to search; never a comment line)
 *              tchar (we are searching for the first instance of this)
 *              &soffset (<return> offset in strings from start index)
 *              &boffset (<return> offset in bytes within string in which
 *                        the character is first found)
 *              &toffset (<return> offset in total bytes from beginning of
 *                        string indexed by 'start' to the location where
 *                        the character is first found)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) We are searching for the first instance of 'tchar', starting
 *          at the beginning of the string indexed by start.
 *      (2) If the character is not found, soffset is returned as -1,
 *          and the other offsets are set to very large numbers.  The
 *          caller must check the value of soffset.
 *      (3) This is only used in contexts where it is not necessary to
 *          consider if the character is inside a string.
 */
static l_int32
getOffsetForCharacter(SARRAY   *sa,
                      l_int32   start,
                      char      tchar,
                      l_int32  *psoffset,
                      l_int32  *pboffset,
                      l_int32  *ptoffset)
{
char    *str;
l_int32  i, j, n, nchars, totchars, found;

    PROCNAME("getOffsetForCharacter");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!psoffset)
        return ERROR_INT("&soffset not defined", procName, 1);
    if (!pboffset)
        return ERROR_INT("&boffset not defined", procName, 1);
    if (!ptoffset)
        return ERROR_INT("&toffset not defined", procName, 1);

    *psoffset = -1;  /* init to not found */
    *pboffset = 100000000;
    *ptoffset = 100000000;

    n = sarrayGetCount(sa);
    found = FALSE;
    totchars = 0;
    for (i = start; i < n; i++) {
        if ((str = sarrayGetString(sa, i, 0)) == NULL)
            return ERROR_INT("str not returned; shouldn't happen", procName, 1);
        nchars = strlen(str);
        for (j = 0; j < nchars; j++) {
            if (str[j] == tchar) {
                found = TRUE;
                break;
            }
        }
        if (found)
            break;
        totchars += nchars;
    }

    if (found) {
        *psoffset = i - start;
        *pboffset = j;
        *ptoffset = totchars + j;
    }

    return 0;
}


/*
 *  getOffsetForMatchingRP()
 *
 *      Input:  sa (output from cpp, by line)
 *              start (starting index in sa to search; never a comment line)
 *              soffsetlp (string offset to first LP)
 *              boffsetlp (byte offset within string to first LP)
 *              toffsetlp (total byte offset to first LP)
 *              &soffset (<return> offset in strings from start index)
 *              &boffset (<return> offset in bytes within string in which
 *                        the matching RP is found)
 *              &toffset (<return> offset in total bytes from beginning of
 *                        string indexed by 'start' to the location where
 *                        the matching RP is found);
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) We are searching for the matching right parenthesis (RP) that
 *          corresponds to the first LP found beginning at the string
 *          indexed by start.
 *      (2) If the matching RP is not found, soffset is returned as -1,
 *          and the other offsets are set to very large numbers.  The
 *          caller must check the value of soffset.
 *      (3) This is only used in contexts where it is not necessary to
 *          consider if the character is inside a string.
 *      (4) We must do this because although most arg lists have a single
 *          left and right parenthesis, it is possible to construct
 *          more complicated prototype declarations, such as those
 *          where functions are passed in.  The C++ rules for prototypes
 *          are strict, and require that for functions passed in as args,
 *          the function name arg be placed in parenthesis, as well
 *          as its arg list, thus incurring two extra levels of parentheses.
 */
static l_int32
getOffsetForMatchingRP(SARRAY   *sa,
                       l_int32   start,
                       l_int32   soffsetlp,
                       l_int32   boffsetlp,
                       l_int32   toffsetlp,
                       l_int32  *psoffset,
                       l_int32  *pboffset,
                       l_int32  *ptoffset)
{
char    *str;
l_int32  i, j, n, nchars, totchars, leftmatch, firstline, jstart, found;

    PROCNAME("getOffsetForMatchingRP");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!psoffset)
        return ERROR_INT("&soffset not defined", procName, 1);
    if (!pboffset)
        return ERROR_INT("&boffset not defined", procName, 1);
    if (!ptoffset)
        return ERROR_INT("&toffset not defined", procName, 1);

    *psoffset = -1;  /* init to not found */
    *pboffset = 100000000;
    *ptoffset = 100000000;

    n = sarrayGetCount(sa);
    found = FALSE;
    totchars = toffsetlp;
    leftmatch = 1;  /* count of (LP - RP); we're finished when it goes to 0. */
    firstline = start + soffsetlp;
    for (i = firstline; i < n; i++) {
        if ((str = sarrayGetString(sa, i, 0)) == NULL)
            return ERROR_INT("str not returned; shouldn't happen", procName, 1);
        nchars = strlen(str);
        jstart = 0;
        if (i == firstline)
            jstart = boffsetlp + 1;
        for (j = jstart; j < nchars; j++) {
            if (str[j] == '(')
                leftmatch++;
            else if (str[j] == ')')
                leftmatch--;
            if (leftmatch == 0) {
                found = TRUE;
                break;
            }
        }
        if (found)
            break;
        if (i == firstline)
            totchars += nchars - boffsetlp;
        else
            totchars += nchars;
    }

    if (found) {
        *psoffset = i - start;
        *pboffset = j;
        *ptoffset = totchars + j;
    }

    return 0;
}
