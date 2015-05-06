/* -*-C-*-
 ********************************************************************************
 *
 * File:        chopper.h  (Formerly chopper.h)
 * Description:
 * Author:       Mark Seaman, SW Productivity
 * Created:      Fri Oct 16 14:37:00 1987
 * Modified:     Wed May 15 14:24:26 1991 (Mark Seaman) marks@hpgrlt
 * Language:     C
 * Package:      N/A
 * Status:       Reusable Software Component
 *
 * (c) Copyright 1987, Hewlett-Packard Company.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 *********************************************************************************/

#ifndef CHOPPER_H
#define CHOPPER_H

#include "cutil.h"
#include "matrix.h"
#include "seam.h"
#include "stopper.h"


void preserve_outline(EDGEPT *start);

void preserve_outline_tree(TESSLINE *srcline);

EDGEPT *restore_outline(EDGEPT *start);

void restore_outline_tree(TESSLINE *srcline);

int any_shared_split_points(const GenericVector<SEAM*>& seams, SEAM *seam);

int check_blob(TBLOB *blob);

inT16 check_seam_order(TBLOB *blob, SEAM *seam);

inT16 total_containment(TBLOB *blob1, TBLOB *blob2);
#endif
