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
 *  sudoku.c
 *
 *      Solve a sudoku by brute force search
 *
 *      Read input data from file or string
 *          l_int32         *sudokuReadFile()
 *          l_int32         *sudokuReadString()
 *
 *      Create/destroy
 *          L_SUDOKU        *sudokuCreate()
 *          void             sudokuDestroy()
 *
 *      Solve the puzzle
 *          l_int32          sudokuSolve()
 *          static l_int32   sudokuValidState()
 *          static l_int32   sudokuNewGuess()
 *          static l_int32   sudokuTestState()
 *
 *      Test for uniqueness
 *          l_int32          sudokuTestUniqueness()
 *          static l_int32   sudokuCompareState()
 *          static l_int32  *sudokuRotateArray()
 *
 *      Generation
 *          L_SUDOKU        *sudokuGenerate()
 *
 *      Output
 *          l_int32          sudokuOutput()
 *
 *  Solving sudokus is a somewhat addictive pastime.  The rules are
 *  simple but it takes just enough concentration to make it rewarding
 *  when you find a number.  And you get 50 to 60 such rewards each time
 *  you complete one.  The downside is that you could have been doing
 *  something more creative, like keying out a new plant, staining
 *  the deck, or even writing a computer program to discourage your
 *  wife from doing sudokus.
 *
 *  My original plan for the sudoku solver was somewhat grandiose.
 *  The program would model the way a person solves the problem.
 *  It would examine each empty position and determine how many possible
 *  numbers could fit.  The empty positions would be entered in a priority
 *  queue keyed on the number of possible numbers that could fit.
 *  If there existed a position where only a single number would work,
 *  it would greedily take it.  Otherwise it would consider a
 *  positions that could accept two and make a guess, with backtracking
 *  if an impossible state were reached.  And so on.
 *
 *  Then one of my colleagues announced she had solved the problem
 *  by brute force and it was fast.  At that point the original plan was
 *  dead in the water, because the two top requirements for a leptonica
 *  algorithm are (1) as simple as possible and (2) fast.  The brute
 *  force approach starts at the UL corner, and in succession at each
 *  blank position it finds the first valid number (testing in
 *  sequence from 1 to 9).  When no number will fit a blank position
 *  it backtracks, choosing the next valid number in the previous
 *  blank position.
 *
 *  This is an inefficient method for pruning the space of solutions
 *  (imagine backtracking from the LR corner back to the UL corner
 *  and starting over with a new guess), but it nevertheless gets
 *  the job done quickly.  I have made no effort to optimize
 *  it, because it is fast: a 5-star (highest difficulty) sudoku might
 *  require a million guesses and take 0.05 sec.  (This BF implementation
 *  does about 20M guesses/sec at 3 GHz.)
 *
 *  Proving uniqueness of a sudoku solution is tricker than finding
 *  a solution (or showing that no solution exists).  A good indication
 *  that a solution is unique is if we get the same result solving
 *  by brute force when the puzzle is also rotated by 90, 180 and 270
 *  degrees.  If there are multiple solutions, it seems unlikely
 *  that you would get the same solution four times in a row, using a
 *  brute force method that increments guesses and scans LR/TB.
 *  The function sudokuTestUniqueness() does this.
 *
 *  And given a function that can determine uniqueness, it is
 *  easy to generate valid sudokus.  We provide sudokuGenerate(),
 *  which starts with some valid initial solution, and randomly
 *  removes numbers, stopping either when a minimum number of non-zero
 *  elements are left, or when it becomes difficult to remove another
 *  element without destroying the uniqueness of the solution.
 *
 *  For further reading, see the Wikipedia articles:
 *     (1) http://en.wikipedia.org/wiki/Algorithmics_of_sudoku
 *     (2) http://en.wikipedia.org/wiki/Sudoku
 *
 *  How many 9x9 sudokus are there?  Here are the numbers.
 *   - From ref(1), there are about 6 x 10^27 "latin squares", where
 *     each row and column has all 9 digits.
 *   - There are 7.2 x 10^21 actual solutions, having the added
 *     constraint in each of the 9 3x3 squares.  (The constraint
 *     reduced the number by the fraction 1.2 x 10^(-6).)
 *   - There are a mere 5.5 billion essentially different solutions (EDS),
 *     when symmetries (rotation, reflection, permutation and relabelling)
 *     are removed.
 *   - Thus there are 1.3 x 10^12 solutions that can be derived by
 *     symmetry from each EDS.  Can we account for these?
 *   - Sort-of.  From an EDS, you can derive (3!)^8 = 1.7 million solutions
 *     by simply permuting rows and columns.  (Do you see why it is
 *     not (3!)^6 ?)
 *   - Also from an EDS, you can derive 9! solutions by relabelling,
 *     and 4 solutions by rotation, for a total of 1.45 million solutions
 *     by relabelling and rotation.  Then taking the product, by symmetry
 *     we can derive 1.7M x 1.45M = 2.45 trillion solutions from each EDS.
 *     (Something is off by about a factor of 2 -- close enough.)
 *
 *  Another interesting fact is that there are apparently 48K EDS sudokus
 *  (with unique solutions) that have only 17 givens.  No sudokus are known
 *  with less than 17, but there exists no proof that this is the minimum.
 */

#include "allheaders.h"


static l_int32 sudokuValidState(l_int32  *state);
static l_int32 sudokuNewGuess(L_SUDOKU  *sud);
static l_int32 sudokuTestState(l_int32  *state, l_int32  index);
static l_int32 sudokuCompareState(L_SUDOKU  *sud1, L_SUDOKU  *sud2,
                                  l_int32  quads, l_int32  *psame);
static l_int32 *sudokuRotateArray(l_int32  *array, l_int32  quads);

    /* An example of a valid solution */
static const char valid_solution[] = "3 8 7 2 6 4 1 9 5 "
                                     "2 6 5 8 9 1 4 3 7 "
                                     "1 4 9 5 3 7 6 8 2 "
                                     "5 2 3 7 1 6 8 4 9 "
                                     "7 1 6 9 4 8 2 5 3 "
                                     "8 9 4 3 5 2 7 1 6 "
                                     "9 7 2 1 8 5 3 6 4 "
                                     "4 3 1 6 7 9 5 2 8 "
                                     "6 5 8 4 2 3 9 7 1 ";


/*---------------------------------------------------------------------*
 *               Read input data from file or string                   *
 *---------------------------------------------------------------------*/
/*!
 *  sudokuReadFile()
 *
 *      Input:  filename (of formatted sudoku file)
 *      Return: array (of 81 numbers), or null on error
 *
 *  Notes:
 *      (1) The file format has:
 *          * any number of comment lines beginning with '#'
 *          * a set of 9 lines, each having 9 digits (0-9) separated
 *            by a space
 */
l_int32 *
sudokuReadFile(const char  *filename)
{
char     *str, *strj;
l_uint8  *data;
l_int32   i, j, nlines, val, index, error;
l_int32  *array;
size_t    size;
SARRAY   *saline, *sa1, *sa2;

    PROCNAME("sudokuReadFile");

    if (!filename)
        return (l_int32 *)ERROR_PTR("filename not defined", procName, NULL);
    data = l_binaryRead(filename, &size);
    sa1 = sarrayCreateLinesFromString((char *)data, 0);
    sa2 = sarrayCreate(9);

        /* Filter out the comment lines; verify that there are 9 data lines */
    nlines = sarrayGetCount(sa1);
    for (i = 0; i < nlines; i++) {
        str = sarrayGetString(sa1, i, L_NOCOPY);
        if (str[0] != '#')
            sarrayAddString(sa2, str, L_COPY);
    }
    FREE(data);
    sarrayDestroy(&sa1);
    nlines = sarrayGetCount(sa2);
    if (nlines != 9) {
        sarrayDestroy(&sa2);
        L_ERROR("file has %d lines\n", procName, nlines);
        return (l_int32 *)ERROR_PTR("invalid file", procName, NULL);
    }

        /* Read the data into the array, verifying that each data
         * line has 9 numbers. */
    error = FALSE;
    array = (l_int32 *)CALLOC(81, sizeof(l_int32));
    for (i = 0, index = 0; i < 9; i++) {
        str = sarrayGetString(sa2, i, L_NOCOPY);
        saline = sarrayCreateWordsFromString(str);
        if (sarrayGetCount(saline) != 9) {
            error = TRUE;
            sarrayDestroy(&saline);
            break;
        }
        for (j = 0; j < 9; j++) {
            strj = sarrayGetString(saline, j, L_NOCOPY);
            if (sscanf(strj, "%d", &val) != 1)
                error = TRUE;
            else
                array[index++] = val;
        }
        sarrayDestroy(&saline);
        if (error) break;
    }
    sarrayDestroy(&sa2);

    if (error) {
        FREE(array);
        return (l_int32 *)ERROR_PTR("invalid data", procName, NULL);
    }

    return array;
}


/*!
 *  sudokuReadString()
 *
 *      Input:  str (of input data)
 *      Return: array (of 81 numbers), or null on error
 *
 *  Notes:
 *      (1) The string is formatted as 81 single digits, each separated
 *          by 81 spaces.
 */
l_int32 *
sudokuReadString(const char  *str)
{
l_int32   i;
l_int32  *array;

    PROCNAME("sudokuReadString");

    if (!str)
        return (l_int32 *)ERROR_PTR("str not defined", procName, NULL);

        /* Read in the initial solution */
    array = (l_int32 *)CALLOC(81, sizeof(l_int32));
    for (i = 0; i < 81; i++) {
        if (sscanf(str + 2 * i, "%d ", &array[i]) != 1)
            return (l_int32 *)ERROR_PTR("invalid format", procName, NULL);
    }

    return array;
}


/*---------------------------------------------------------------------*
 *                        Create/destroy sudoku                        *
 *---------------------------------------------------------------------*/
/*!
 *  sudokuCreate()
 *
 *      Input:  array (of 81 numbers, 9 rows of 9 numbers each)
 *      Return: l_sudoku, or null on error
 *
 *  Notes:
 *      (1) The input array has 0 for the unknown values, and 1-9
 *          for the known initial values.  It is generated from
 *          a file using sudokuReadInput(), which checks that the file
 *          data has 81 numbers in 9 rows.
 */
L_SUDOKU *
sudokuCreate(l_int32  *array)
{
l_int32    i, val, locs_index;
L_SUDOKU  *sud;

    PROCNAME("sudokuCreate");

    if (!array)
        return (L_SUDOKU *)ERROR_PTR("array not defined", procName, NULL);

    locs_index = 0;  /* into locs array */
    if ((sud = (L_SUDOKU *)CALLOC(1, sizeof(L_SUDOKU))) == NULL)
        return (L_SUDOKU *)ERROR_PTR("sud not made", procName, NULL);
    if ((sud->locs = (l_int32 *)CALLOC(81, sizeof(l_int32))) == NULL)
        return (L_SUDOKU *)ERROR_PTR("su state array not made", procName, NULL);
    if ((sud->init = (l_int32 *)CALLOC(81, sizeof(l_int32))) == NULL)
        return (L_SUDOKU *)ERROR_PTR("su init array not made", procName, NULL);
    if ((sud->state = (l_int32 *)CALLOC(81, sizeof(l_int32))) == NULL)
        return (L_SUDOKU *)ERROR_PTR("su state array not made", procName, NULL);
    for (i = 0; i < 81; i++) {
        val = array[i];
        sud->init[i] = val;
        sud->state[i] = val;
        if (val == 0)
            sud->locs[locs_index++] = i;
    }
    sud->num = locs_index;
    sud->failure = FALSE;
    sud->finished = FALSE;
    return sud;
}


/*!
 *  sudokuDestroy()
 *
 *      Input:  &l_sudoku (<to be nulled>)
 *      Return: void
 */
void
sudokuDestroy(L_SUDOKU  **psud)
{
L_SUDOKU  *sud;

    PROCNAME("sudokuDestroy");

    if (psud == NULL) {
        L_WARNING("ptr address is NULL\n", procName);
        return;
    }
    if ((sud = *psud) == NULL)
        return;

    FREE(sud->locs);
    FREE(sud->init);
    FREE(sud->state);
    FREE(sud);

    *psud = NULL;
    return;
}


/*---------------------------------------------------------------------*
 *                           Solve the puzzle                          *
 *---------------------------------------------------------------------*/
/*!
 *  sudokuSolve()
 *
 *      Input:  l_sudoku (starting in initial state)
 *      Return: 1 on success, 0 on failure to solve (note reversal of
 *              typical unix returns)
 */
l_int32
sudokuSolve(L_SUDOKU  *sud)
{
    PROCNAME("sudokuSolve");

    if (!sud)
        return ERROR_INT("sud not defined", procName, 0);

    if (!sudokuValidState(sud->init))
        return ERROR_INT("initial state not valid", procName, 0);

    while (1) {
        if (sudokuNewGuess(sud))
            break;
        if (sud->finished == TRUE)
            break;
    }

    if (sud->failure == TRUE) {
        fprintf(stderr, "Failure after %d guesses\n", sud->nguess);
        return 0;
    }

    fprintf(stderr, "Solved after %d guesses\n", sud->nguess);
    return 1;
}


/*!
 *  sudokuValidState()
 *
 *      Input:  state (array of size 81)
 *      Return: 1 if valid, 0 if invalid
 *
 *  Notes:
 *      (1) This can be used on either the initial state (init)
 *          or on the current state (state) of the l_soduku.
 *          All values of 0 are ignored.
 */
static l_int32
sudokuValidState(l_int32  *state)
{
l_int32  i;

    PROCNAME("sudokuValidState");

    if (!state)
        return ERROR_INT("state not defined", procName, 0);

    for (i = 0; i < 81; i++) {
        if (!sudokuTestState(state, i))
            return 0;
    }

    return 1;
}


/*!
 *  sudokuNewGuess()
 *
 *      Input:  l_sudoku
 *      Return: 0 if OK; 1 if no solution is possible
 *
 *  Notes:
 *      (1) This attempts to increment the number in the current
 *          location.  If it can't, it backtracks (sets the number
 *          in the current location to zero and decrements the
 *          current location).  If it can, it tests that number,
 *          and if the number is valid, moves forward to the next
 *          empty location (increments the current location).
 *      (2) If there is no solution, backtracking will eventually
 *          exhaust possibilities for the first location.
 */
static l_int32
sudokuNewGuess(L_SUDOKU  *sud)
{
l_int32   index, val, valid;
l_int32  *locs, *state;

    locs = sud->locs;
    state = sud->state;
    index = locs[sud->current];  /* 0 to 80 */
    val = state[index];
    if (val == 9) {  /* backtrack or give up */
        if (sud->current == 0) {
            sud->failure = TRUE;
            return 1;
        }
        state[index] = 0;
        sud->current--;
    } else {  /* increment current value and test */
        sud->nguess++;
        state[index]++;
        valid = sudokuTestState(state, index);
        if (valid) {
            if (sud->current == sud->num - 1) {  /* we're done */
                sud->finished = TRUE;
                return 0;
            } else {  /* advance to next position */
                sud->current++;
            }
        }
    }

    return 0;
}


/*!
 *  sudokuTestState()
 *
 *      Input:  state (current state: array of 81 values)
 *              index (into state element that we are testing)
 *      Return: 1 if valid; 0 if invalid (no error checking)
 */
static l_int32
sudokuTestState(l_int32  *state,
                l_int32   index)
{
l_int32  i, j, val, row, rowstart, rowend, col;
l_int32  blockrow, blockcol, blockstart, rowindex, locindex;

    if ((val = state[index]) == 0)  /* automatically valid */
        return 1;

        /* Test row.  Test val is at (x, y) = (index % 9, index / 9)  */
    row = index / 9;
    rowstart = 9 * row;
    for (i = rowstart; i < index; i++) {
        if (state[i] == val)
            return 0;
    }
    rowend = rowstart + 9;
    for (i = index + 1; i < rowend; i++) {
        if (state[i] == val)
            return 0;
    }

        /* Test column */
    col = index % 9;
    for (j = col; j < index; j += 9) {
        if (state[j] == val)
            return 0;
    }
    for (j = index + 9; j < 81; j += 9) {
        if (state[j] == val)
            return 0;
    }

        /* Test local 3x3 block */
    blockrow = 3 * (row / 3);
    blockcol = 3 * (col / 3);
    blockstart = 9 * blockrow + blockcol;
    for (i = 0; i < 3; i++) {
        rowindex = blockstart + 9 * i;
        for (j = 0; j < 3; j++) {
            locindex = rowindex + j;
            if (index == locindex) continue;
            if (state[locindex] == val)
                return 0;
        }
    }

    return 1;
}


/*---------------------------------------------------------------------*
 *                         Test for uniqueness                         *
 *---------------------------------------------------------------------*/
/*!
 *  sudokuTestUniqueness()
 *
 *      Input:  array (of 81 numbers, 9 lines of 9 numbers each)
 *              &punique (<return> 1 if unique, 0 if not)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This applies the brute force method to all four 90 degree
 *          rotations.  If there is more than one solution, it is highly
 *          unlikely that all four results will be the same;
 *          consequently, if they are the same, the solution is
 *          most likely to be unique.
 */
l_int32
sudokuTestUniqueness(l_int32  *array,
                     l_int32  *punique)
{
l_int32    same1, same2, same3;
l_int32   *array1, *array2, *array3;
L_SUDOKU  *sud, *sud1, *sud2, *sud3;

    PROCNAME("sudokuTestUniqueness");

    if (!punique)
        return ERROR_INT("&unique not defined", procName, 1);
    *punique = 0;
    if (!array)
        return ERROR_INT("array not defined", procName, 1);

    sud = sudokuCreate(array);
    sudokuSolve(sud);
    array1 = sudokuRotateArray(array, 1);
    sud1 = sudokuCreate(array1);
    sudokuSolve(sud1);
    array2 = sudokuRotateArray(array, 2);
    sud2 = sudokuCreate(array2);
    sudokuSolve(sud2);
    array3 = sudokuRotateArray(array, 3);
    sud3 = sudokuCreate(array3);
    sudokuSolve(sud3);

    sudokuCompareState(sud, sud1, 1, &same1);
    sudokuCompareState(sud, sud2, 2, &same2);
    sudokuCompareState(sud, sud3, 3, &same3);
    *punique = (same1 && same2 && same3);

    sudokuDestroy(&sud);
    sudokuDestroy(&sud1);
    sudokuDestroy(&sud2);
    sudokuDestroy(&sud3);
    FREE(array1);
    FREE(array2);
    FREE(array3);
    return 0;
}


/*!
 *  sudokuCompareState()
 *
 *      Input:  sud1, sud2
 *              quads (rotation of sud2 input with respect to sud1,
 *                    in units of 90 degrees cw)
 *              &same (<return> 1 if all 4 results are identical; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The input to sud2 has been rotated by @quads relative to the
 *          input to sud1.  Therefore, we must rotate the solution to
 *          sud1 by the same amount before comparing it to the
 *          solution to sud2.
 */
static l_int32
sudokuCompareState(L_SUDOKU  *sud1,
                   L_SUDOKU  *sud2,
                   l_int32    quads,
                   l_int32   *psame)
{
l_int32   i, same;
l_int32  *array;

    PROCNAME("sudokuCompareState");

    if (!psame)
        return ERROR_INT("&same not defined", procName, 1);
    *psame = 0;
    if (!sud1)
        return ERROR_INT("sud1 not defined", procName, 1);
    if (!sud2)
        return ERROR_INT("sud1 not defined", procName, 1);
    if (quads < 1 || quads > 3)
        return ERROR_INT("valid quads in {1,2,3}", procName, 1);

    same = TRUE;
    if ((array = sudokuRotateArray(sud1->state, quads)) == NULL)
        return ERROR_INT("array not made", procName, 1);
    for (i = 0; i < 81; i++) {
        if (array[i] != sud2->state[i]) {
            same = FALSE;
            break;
        }
    }
    *psame = same;
    FREE(array);
    return 0;
}


/*!
 *  sudokuRotateArray()
 *
 *      Input:  array (of 81 numbers; 9 lines of 9 numbers each)
 *              quads (1-3; number of 90 degree cw rotations)
 *      Return: rarray (rotated array), or null on error
 */
static l_int32 *
sudokuRotateArray(l_int32  *array,
                  l_int32   quads)
{
l_int32   i, j, sindex, dindex;
l_int32  *rarray;

    PROCNAME("sudokuRotateArray");

    if (!array)
        return (l_int32 *)ERROR_PTR("array not defined", procName, NULL);
    if (quads < 1 || quads > 3)
        return (l_int32 *)ERROR_PTR("valid quads in {1,2,3}", procName, NULL);

    rarray = (l_int32 *)CALLOC(81, sizeof(l_int32));
    if (quads == 1) {
        for (j = 0, dindex = 0; j < 9; j++) {
             for (i = 8; i >= 0; i--) {
                 sindex = 9 * i + j;
                 rarray[dindex++] = array[sindex];
             }
        }
    } else if (quads == 2) {
        for (i = 8, dindex = 0; i >= 0; i--) {
             for (j = 8; j >= 0; j--) {
                 sindex = 9 * i + j;
                 rarray[dindex++] = array[sindex];
             }
        }
    } else {  /* quads == 3 */
        for (j = 8, dindex = 0; j >= 0; j--) {
             for (i = 0; i < 9; i++) {
                 sindex = 9 * i + j;
                 rarray[dindex++] = array[sindex];
             }
        }
    }

    return rarray;
}


/*---------------------------------------------------------------------*
 *                              Generation                             *
 *---------------------------------------------------------------------*/
/*!
 *  sudokuGenerate()
 *
 *      Input:  array (of 81 numbers, 9 rows of 9 numbers each)
 *              seed (random number)
 *              minelems (min non-zero elements allowed; <= 80)
 *              maxtries (max tries to remove a number and get a valid sudoku)
 *      Return: l_sudoku, or null on error
 *
 *  Notes:
 *      (1) This is a brute force generator.  It starts with a completed
 *          sudoku solution and, by removing elements (setting them to 0),
 *          generates a valid (unique) sudoku initial condition.
 *      (2) The process stops when either @minelems, the minimum
 *          number of non-zero elements, is reached, or when the
 *          number of attempts to remove the next element exceeds @maxtries.
 *      (3) No sudoku is known with less than 17 nonzero elements.
 */
L_SUDOKU *
sudokuGenerate(l_int32  *array,
               l_int32   seed,
               l_int32   minelems,
               l_int32   maxtries)
{
l_int32    index, sector, nzeros, removefirst, tries, val, oldval, unique;
L_SUDOKU  *sud, *testsud;

    PROCNAME("sudokuGenerate");

    if (!array)
        return (L_SUDOKU *)ERROR_PTR("array not defined", procName, NULL);
    if (minelems > 80)
        return (L_SUDOKU *)ERROR_PTR("minelems must be < 81", procName, NULL);

        /* Remove up to 30 numbers at random from the solution.
         * Test if the solution is valid -- the initial 'solution' may
         * have been invalid.  Then test if the sudoku with 30 zeroes
         * is unique -- it almost always will be. */
    srand(seed);
    nzeros = 0;
    sector = 0;
    removefirst = L_MIN(30, 81 - minelems);
    while (nzeros < removefirst) {
        genRandomIntegerInRange(9, 0, &val);
        index = 27 * (sector / 3) + 3 * (sector % 3) +
                9 * (val / 3) + (val % 3);
        if (array[index] == 0) continue;
        array[index] = 0;
        nzeros++;
        sector++;
        sector %= 9;
    }
    testsud = sudokuCreate(array);
    sudokuSolve(testsud);
    if (testsud->failure) {
        sudokuDestroy(&testsud);
        L_ERROR("invalid initial solution\n", procName);
        return NULL;
    }
    sudokuTestUniqueness(testsud->init, &unique);
    sudokuDestroy(&testsud);
    if (!unique) {
        L_ERROR("non-unique result with 30 zeroes\n", procName);
        return NULL;
    }

        /* Remove more numbers, testing at each removal for uniqueness. */
    tries = 0;
    sector = 0;
    while (1) {
        if (tries > maxtries) break;
        if (81 - nzeros <= minelems) break;

        if (tries == 0) {
            fprintf(stderr, "Trying %d zeros\n", nzeros);
            tries = 1;
        }

            /* Choose an element to be zeroed.  We choose one
             * at random in succession from each of the nine sectors. */
        genRandomIntegerInRange(9, 0, &val);
        index = 27 * (sector / 3) + 3 * (sector % 3) +
                9 * (val / 3) + (val % 3);
        sector++;
        sector %= 9;
        if (array[index] == 0) continue;

            /* Save the old value in case we need to revert */
        oldval = array[index];

            /* Is there a solution?  If not, try again. */
        array[index] = 0;
        testsud = sudokuCreate(array);
        sudokuSolve(testsud);
        if (testsud->failure == TRUE) {
            sudokuDestroy(&testsud);
            array[index] = oldval;  /* revert */
            tries++;
            continue;
        }

            /* Is the solution unique?  If not, try again. */
        sudokuTestUniqueness(testsud->init, &unique);
        sudokuDestroy(&testsud);
        if (!unique) {  /* revert and try again */
            array[index] = oldval;
            tries++;
        } else {  /* accept this */
            tries = 0;
            fprintf(stderr, "Have %d zeros\n", nzeros);
            nzeros++;
        }
    }
    fprintf(stderr, "Final: nelems = %d\n", 81 - nzeros);

        /* Show that we can recover the solution */
    sud = sudokuCreate(array);
    sudokuOutput(sud, L_SUDOKU_INIT);
    sudokuSolve(sud);
    sudokuOutput(sud, L_SUDOKU_STATE);

    return sud;
}


/*---------------------------------------------------------------------*
 *                               Output                                *
 *---------------------------------------------------------------------*/
/*!
 *  sudokuOutput()
 *
 *      Input:  l_sudoku (at any stage)
 *              arraytype (L_SUDOKU_INIT, L_SUDOKU_STATE)
 *      Return: void
 *
 *  Notes:
 *      (1) Prints either the initial array or the current state
 *          of the solution.
 */
l_int32
sudokuOutput(L_SUDOKU  *sud,
             l_int32    arraytype)
{
l_int32   i, j;
l_int32  *array;

    PROCNAME("sudokuOutput");

    if (!sud)
        return ERROR_INT("sud not defined", procName, 1);
    if (arraytype == L_SUDOKU_INIT)
        array = sud->init;
    else if (arraytype == L_SUDOKU_STATE)
        array = sud->state;
    else
        return ERROR_INT("invalid arraytype", procName, 1);

    for (i = 0; i < 9; i++) {
        for (j = 0; j < 9; j++)
            fprintf(stderr, "%d ", array[9 * i + j]);
        fprintf(stderr, "\n");
    }

    return 0;
}
