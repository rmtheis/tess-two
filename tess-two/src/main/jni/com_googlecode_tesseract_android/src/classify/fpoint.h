/******************************************************************************
 **	Filename:    fpoint.h
 **	Purpose:     Abstract data type for 2D points (floating point coords)
 **	Author:      Dan Johnson
 **	History:     Thu Apr 12 10:50:01 1990, DSJ, Created.
 **
 **	(c) Copyright Hewlett-Packard Company, 1988.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 ******************************************************************************/
#ifndef   FPOINT_H
#define   FPOINT_H

/**----------------------------------------------------------------------------
          Include Files and Type Defines
----------------------------------------------------------------------------**/
#include "host.h"
#include <stdio.h>
#include <math.h>

/* define data structure to hold 2D points or vectors using floating point */
typedef struct
{
  FLOAT32 x, y;
} FPOINT;
typedef FPOINT FVECTOR;

/**----------------------------------------------------------------------------
            Macros
----------------------------------------------------------------------------**/
/* macros for computing miscellaneous functions of 2 points */
#define XDelta(A,B)   ( (B).x - (A).x )
#define YDelta(A,B)   ( (B).y - (A).y )
#define SlopeFrom(A,B)    ( YDelta(A,B) / XDelta(A,B) )
#define AngleFrom(A,B)		( atan2((double) YDelta(A,B),		\
					(double) XDelta(A,B) ) )

#define XIntersectionOf(A,B,X)  ( SlopeFrom(A,B) * ((X) - A.x) + A.y)

/*-------------------------------------------------------------------------
        Public Function Prototypes
---------------------------------------------------------------------------*/

FLOAT32 DistanceBetween(FPOINT A, FPOINT B);

FLOAT32 NormalizedAngleFrom(FPOINT *Point1, FPOINT *Point2, FLOAT32 FullScale);
#endif
