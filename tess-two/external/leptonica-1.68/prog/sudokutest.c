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
 * sudokutest.c
 *
 *   Tests sudoku solver and generator.
 */

#include "allheaders.h"

static const char *startsol = "3 8 7 2 6 4 1 9 5 "
                              "2 6 5 8 9 1 4 3 7 "
                              "1 4 9 5 3 7 6 8 2 "
                              "5 2 3 7 1 6 8 4 9 "
                              "7 1 6 9 4 8 2 5 3 "
                              "8 9 4 3 5 2 7 1 6 "
                              "9 7 2 1 8 5 3 6 4 "
                              "4 3 1 6 7 9 5 2 8 "
                              "6 5 8 4 2 3 9 7 1";

main(int    argc,
     char **argv)
{
l_int32      unique;
l_int32     *array;
L_SUDOKU    *sud;
static char  mainName[] = "sudokutest";

    if (argc != 1 && argc != 2)
	return ERROR_INT(" Syntax: sudokutest [filein]", mainName, 1);

    if (argc == 1) {
            /* Generate a new sudoku by element elimination */
        array = sudokuReadString(startsol);
        sud = sudokuGenerate(array, 3693, 28, 7);
        sudokuDestroy(&sud);
        lept_free(array);
        return 0;
    }

        /* Solve the input sudoku */
    if ((array = sudokuReadFile(argv[1])) == NULL)
        return ERROR_INT("invalid input", mainName, 1);
    if ((sud = sudokuCreate(array)) == NULL)
        return ERROR_INT("sud not made", mainName, 1);
    sudokuOutput(sud, L_SUDOKU_INIT);
    startTimer();
    sudokuSolve(sud);
    fprintf(stderr, "Time: %7.3f sec\n", stopTimer());
    sudokuOutput(sud, L_SUDOKU_STATE);
    sudokuDestroy(&sud);

        /* Test for uniqueness */
    sudokuTestUniqueness(array, &unique);
    if (unique)
        fprintf(stderr, "Sudoku is unique\n");
    else
        fprintf(stderr, "Sudoku is NOT unique\n");
    lept_free(array);

    return 0;
}


