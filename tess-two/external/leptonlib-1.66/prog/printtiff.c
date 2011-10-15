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
 * printtiff.c
 *
 *   Syntax:  printtiff filein [printer]
 *
 *   Prints a multipage tiff file to a printer.  If the tiff is
 *   at standard fax resolution, it expands the vertical size
 *   by a factor of two before encapsulating in ccittg4 encoded
 *   PostScript.  The PostScript file is left in /tmp, and
 *   erased (deleted, removed, unlinked) on the next invocation.
 *
 *   If the printer is not specified, this just writes the PostScript
 *   file into /tmp.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   TEMP_PS       "/tmp/junk_printtiff.ps"
#define   FILL_FACTOR   0.95

main(int    argc,
     char **argv)
{
char           *filein, *printer;
char            buffer[512];
static char     mainName[] = "printtiff";

    if (argc != 2 && argc != 3)
	exit(ERROR_INT(" Syntax:  printtiff filein [printer]", mainName, 1));

    filein = argv[1];
    if (argc == 3)
	printer = argv[2];

    sprintf(buffer, "rm -f %s", TEMP_PS);
    system(buffer);

    convertTiffMultipageToPS(filein, TEMP_PS, NULL, FILL_FACTOR);

    if (argc == 3) {
	sprintf(buffer, "lpr -P%s %s &", printer, TEMP_PS);
	system(buffer);
    }

    exit(0);
}

