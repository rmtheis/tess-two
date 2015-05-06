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
 *  maze.c
 *
 *      This is a game with a pedagogical slant.  A maze is represented
 *      by a binary image.  The ON pixels (fg) are walls.  The goal is
 *      to navigate on OFF pixels (bg), using Manhattan steps
 *      (N, S, E, W), between arbitrary start and end positions.
 *      The problem is thus to find the shortest route between two points
 *      in a binary image that are 4-connected in the bg.  This is done
 *      with a breadth-first search, implemented with a queue.
 *      We also use a queue of pointers to generate the maze (image).
 *
 *          PIX             *generateBinaryMaze()
 *          static MAZEEL   *mazeelCreate()
 *
 *          PIX             *pixSearchBinaryMaze()
 *          static l_int32   localSearchForBackground()
 *
 *      Generalizing a maze to a grayscale image, the search is
 *      now for the "shortest" or least cost path, for some given
 *      cost function.
 *
 *          PIX             *pixSearchGrayMaze()
 *
 *
 *      Elegant method for finding largest white (or black) rectangle
 *      in an image.
 *
 *          l_int32          pixFindLargestRectangle()
 */

#include <string.h>
#ifdef _WIN32
#include <stdlib.h>
#include <time.h>
#endif  /* _WIN32 */
#include "allheaders.h"

static const l_int32  MIN_MAZE_WIDTH = 50;
static const l_int32  MIN_MAZE_HEIGHT = 50;

static const l_float32  DEFAULT_WALL_PROBABILITY = 0.65;
static const l_float32  DEFAULT_ANISOTROPY_RATIO = 0.25;

enum {  /* direction from parent to newly created element */
    START_LOC = 0,
    DIR_NORTH = 1,
    DIR_SOUTH = 2,
    DIR_WEST = 3,
    DIR_EAST = 4
};

struct MazeElement {
    l_float32  distance;
    l_int32    x;
    l_int32    y;
    l_uint32   val;  /* value of maze pixel at this location */
    l_int32    dir;  /* direction from parent to child */
};
typedef struct MazeElement  MAZEEL;


static MAZEEL *mazeelCreate(l_int32  x, l_int32  y, l_int32  dir);
static l_int32 localSearchForBackground(PIX  *pix, l_int32  *px,
                                        l_int32  *py, l_int32  maxrad);

#ifndef  NO_CONSOLE_IO
#define  DEBUG_PATH    0
#define  DEBUG_MAZE    0
#endif  /* ~NO_CONSOLE_IO */


/*---------------------------------------------------------------------*
 *             Binary maze generation as cellular automaton            *
 *---------------------------------------------------------------------*/
/*!
 *  generateBinaryMaze()
 *
 *      Input:  w, h  (size of maze)
 *              xi, yi  (initial location)
 *              wallps (probability that a pixel to the side is ON)
 *              ranis (ratio of prob that pixel in forward direction
 *                     is a wall to the probability that pixel in
 *                     side directions is a wall)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) We have two input probability factors that determine the
 *          density of walls and average length of straight passages.
 *          When ranis < 1.0, you are more likely to generate a wall
 *          to the side than going forward.  Enter 0.0 for either if
 *          you want to use the default values.
 *      (2) This is a type of percolation problem, and exhibits
 *          different phases for different parameters wallps and ranis.
 *          For larger values of these parameters, regions in the maze
 *          are not explored because the maze generator walls them
 *          off and cannot get through.  The boundary between the
 *          two phases in this two-dimensional parameter space goes
 *          near these values:
 *                wallps       ranis
 *                0.35         1.00
 *                0.40         0.85
 *                0.45         0.70
 *                0.50         0.50
 *                0.55         0.40
 *                0.60         0.30
 *                0.65         0.25
 *                0.70         0.19
 *                0.75         0.15
 *                0.80         0.11
 *      (3) Because there is a considerable amount of overhead in calling
 *          pixGetPixel() and pixSetPixel(), this function can be sped
 *          up with little effort using raster line pointers and the
 *          GET_DATA* and SET_DATA* macros.
 */
PIX *
generateBinaryMaze(l_int32  w,
                   l_int32  h,
                   l_int32  xi,
                   l_int32  yi,
                   l_float32  wallps,
                   l_float32  ranis)
{
l_int32    x, y, dir;
l_uint32   val;
l_float32  frand, wallpf, testp;
MAZEEL    *el, *elp;
PIX       *pixd;  /* the destination maze */
PIX       *pixm;  /* for bookkeeping, to indicate pixels already visited */
L_QUEUE   *lq;

    /* On Windows, seeding is apparently necessary to get decent mazes.
     * Windows rand() returns a value up to 2^15 - 1, whereas unix
     * rand() returns a value up to 2^31 - 1.  Therefore the generated
     * mazes will differ on the two platforms. */
#ifdef _WIN32
    srand(28*333);
#endif /* _WIN32 */

    if (w < MIN_MAZE_WIDTH)
        w = MIN_MAZE_WIDTH;
    if (h < MIN_MAZE_HEIGHT)
        h = MIN_MAZE_HEIGHT;
    if (xi <= 0 || xi >= w)
        xi = w / 6;
    if (yi <= 0 || yi >= h)
        yi = h / 5;
    if (wallps < 0.05 || wallps > 0.95)
        wallps = DEFAULT_WALL_PROBABILITY;
    if (ranis < 0.05 || ranis > 1.0)
        ranis = DEFAULT_ANISOTROPY_RATIO;
    wallpf = wallps * ranis;

#if  DEBUG_MAZE
    fprintf(stderr, "(w, h) = (%d, %d), (xi, yi) = (%d, %d)\n", w, h, xi, yi);
    fprintf(stderr, "Using: prob(wall) = %7.4f, anisotropy factor = %7.4f\n",
            wallps, ranis);
#endif  /* DEBUG_MAZE */

        /* These are initialized to OFF */
    pixd = pixCreate(w, h, 1);
    pixm = pixCreate(w, h, 1);

    lq = lqueueCreate(0);

        /* Prime the queue with the first pixel; it is OFF */
    el = mazeelCreate(xi, yi, START_LOC);
    pixSetPixel(pixm, xi, yi, 1);  /* mark visited */
    lqueueAdd(lq, el);

        /* While we're at it ... */
    while (lqueueGetCount(lq) > 0) {
        elp = (MAZEEL *)lqueueRemove(lq);
        x = elp->x;
        y = elp->y;
        dir = elp->dir;
        if (x > 0) {  /* check west */
            pixGetPixel(pixm, x - 1, y, &val);
            if (val == 0) {  /* not yet visited */
                pixSetPixel(pixm, x - 1, y, 1);  /* mark visited */
                frand = (l_float32)rand() / (l_float32)RAND_MAX;
                testp = wallps;
                if (dir == DIR_WEST)
                    testp = wallpf;
                if (frand <= testp) {  /* make it a wall */
                    pixSetPixel(pixd, x - 1, y, 1);
                } else {  /* not a wall */
                    el = mazeelCreate(x - 1, y, DIR_WEST);
                    lqueueAdd(lq, el);
                }
            }
        }
        if (y > 0) {  /* check north */
            pixGetPixel(pixm, x, y - 1, &val);
            if (val == 0) {  /* not yet visited */
                pixSetPixel(pixm, x, y - 1, 1);  /* mark visited */
                frand = (l_float32)rand() / (l_float32)RAND_MAX;
                testp = wallps;
                if (dir == DIR_NORTH)
                    testp = wallpf;
                if (frand <= testp) {  /* make it a wall */
                    pixSetPixel(pixd, x, y - 1, 1);
                } else {  /* not a wall */
                    el = mazeelCreate(x, y - 1, DIR_NORTH);
                    lqueueAdd(lq, el);
                }
            }
        }
        if (x < w - 1) {  /* check east */
            pixGetPixel(pixm, x + 1, y, &val);
            if (val == 0) {  /* not yet visited */
                pixSetPixel(pixm, x + 1, y, 1);  /* mark visited */
                frand = (l_float32)rand() / (l_float32)RAND_MAX;
                testp = wallps;
                if (dir == DIR_EAST)
                    testp = wallpf;
                if (frand <= testp) {  /* make it a wall */
                    pixSetPixel(pixd, x + 1, y, 1);
                } else {  /* not a wall */
                    el = mazeelCreate(x + 1, y, DIR_EAST);
                    lqueueAdd(lq, el);
                }
            }
        }
        if (y < h - 1) {  /* check south */
            pixGetPixel(pixm, x, y + 1, &val);
            if (val == 0) {  /* not yet visited */
                pixSetPixel(pixm, x, y + 1, 1);  /* mark visited */
                frand = (l_float32)rand() / (l_float32)RAND_MAX;
                testp = wallps;
                if (dir == DIR_SOUTH)
                    testp = wallpf;
                if (frand <= testp) {  /* make it a wall */
                    pixSetPixel(pixd, x, y + 1, 1);
                } else {  /* not a wall */
                    el = mazeelCreate(x, y + 1, DIR_SOUTH);
                    lqueueAdd(lq, el);
                }
            }
        }
        FREE(elp);
    }

    lqueueDestroy(&lq, TRUE);
    pixDestroy(&pixm);
    return pixd;
}


static MAZEEL *
mazeelCreate(l_int32  x,
             l_int32  y,
             l_int32  dir)
{
MAZEEL *el;

    el = (MAZEEL *)CALLOC(1, sizeof(MAZEEL));
    el->x = x;
    el->y = y;
    el->dir = dir;
    return el;
}


/*---------------------------------------------------------------------*
 *                           Binary maze search                        *
 *---------------------------------------------------------------------*/
/*!
 *  pixSearchBinaryMaze()
 *
 *      Input:  pixs (1 bpp, maze)
 *              xi, yi  (beginning point; use same initial point
 *                       that was used to generate the maze)
 *              xf, yf  (end point, or close to it)
 *              &ppixd (<optional return> maze with path illustrated, or
 *                     if no path possible, the part of the maze
 *                     that was searched)
 *      Return: pta (shortest path), or null if either no path
 *              exists or on error
 *
 *  Notes:
 *      (1) Because of the overhead in calling pixGetPixel() and
 *          pixSetPixel(), we have used raster line pointers and the
 *          GET_DATA* and SET_DATA* macros for many of the pix accesses.
 *      (2) Commentary:
 *            The goal is to find the shortest path between beginning and
 *          end points, without going through walls, and there are many
 *          ways to solve this problem.
 *            We use a queue to implement a breadth-first search.  Two auxiliary
 *          "image" data structures can be used: one to mark the visited
 *          pixels and one to give the direction to the parent for each
 *          visited pixel.  The first structure is used to avoid putting
 *          pixels on the queue more than once, and the second is used
 *          for retracing back to the origin, like the breadcrumbs in
 *          Hansel and Gretel.  Each pixel taken off the queue is destroyed
 *          after it is used to locate the allowed neighbors.  In fact,
 *          only one distance image is required, if you initialize it
 *          to some value that signifies "not yet visited."  (We use
 *          a binary image for marking visited pixels because it is clearer.)
 *          This method for a simple search of a binary maze is implemented in
 *          pixSearchBinaryMaze().
 *            An alternative method would store the (manhattan) distance
 *          from the start point with each pixel on the queue.  The children
 *          of each pixel get a distance one larger than the parent.  These
 *          values can be stored in an auxiliary distance map image
 *          that is constructed simultaneously with the search.  Once the
 *          end point is reached, the distance map is used to backtrack
 *          along a minimum path.  There may be several equal length
 *          minimum paths, any one of which can be chosen this way.
 */
PTA *
pixSearchBinaryMaze(PIX     *pixs,
                    l_int32  xi,
                    l_int32  yi,
                    l_int32  xf,
                    l_int32  yf,
                    PIX    **ppixd)
{
l_int32    i, j, x, y, w, h, d, found;
l_uint32   val, rpixel, gpixel, bpixel;
void     **lines1, **linem1, **linep8, **lined32;
MAZEEL    *el, *elp;
PIX       *pixd;  /* the shortest path written on the maze image */
PIX       *pixm;  /* for bookkeeping, to indicate pixels already visited */
PIX       *pixp;  /* for bookkeeping, to indicate direction to parent */
L_QUEUE   *lq;
PTA       *pta;

    PROCNAME("pixSearchBinaryMaze");

    if (ppixd) *ppixd = NULL;
    if (!pixs)
        return (PTA *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1)
        return (PTA *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (xi <= 0 || xi >= w)
        return (PTA *)ERROR_PTR("xi not valid", procName, NULL);
    if (yi <= 0 || yi >= h)
        return (PTA *)ERROR_PTR("yi not valid", procName, NULL);
    pixGetPixel(pixs, xi, yi, &val);
    if (val != 0)
        return (PTA *)ERROR_PTR("(xi,yi) not bg pixel", procName, NULL);
    pixd = NULL;
    pta = NULL;

        /* Find a bg pixel near input point (xf, yf) */
    localSearchForBackground(pixs, &xf, &yf, 5);

#if  DEBUG_MAZE
    fprintf(stderr, "(xi, yi) = (%d, %d), (xf, yf) = (%d, %d)\n",
            xi, yi, xf, yf);
#endif  /* DEBUG_MAZE */

    pixm = pixCreate(w, h, 1);  /* initialized to OFF */
    pixp = pixCreate(w, h, 8);  /* direction to parent stored as enum val */
    lines1 = pixGetLinePtrs(pixs, NULL);
    linem1 = pixGetLinePtrs(pixm, NULL);
    linep8 = pixGetLinePtrs(pixp, NULL);

    lq = lqueueCreate(0);

        /* Prime the queue with the first pixel; it is OFF */
    el = mazeelCreate(xi, yi, 0);  /* don't need direction here */
    pixSetPixel(pixm, xi, yi, 1);  /* mark visited */
    lqueueAdd(lq, el);

        /* Fill up the pix storing directions to parents,
         * stopping when we hit the point (xf, yf)  */
    found = FALSE;
    while (lqueueGetCount(lq) > 0) {
        elp = (MAZEEL *)lqueueRemove(lq);
        x = elp->x;
        y = elp->y;
        if (x == xf && y == yf) {
            found = TRUE;
            FREE(elp);
            break;
        }

        if (x > 0) {  /* check to west */
            val = GET_DATA_BIT(linem1[y], x - 1);
            if (val == 0) {  /* not yet visited */
                SET_DATA_BIT(linem1[y], x - 1);  /* mark visited */
                val = GET_DATA_BIT(lines1[y], x - 1);
                if (val == 0) {  /* bg, not a wall */
                    SET_DATA_BYTE(linep8[y], x - 1, DIR_EAST);  /* parent E */
                    el = mazeelCreate(x - 1, y, 0);
                    lqueueAdd(lq, el);
                }
            }
        }
        if (y > 0) {  /* check north */
            val = GET_DATA_BIT(linem1[y - 1], x);
            if (val == 0) {  /* not yet visited */
                SET_DATA_BIT(linem1[y - 1], x);  /* mark visited */
                val = GET_DATA_BIT(lines1[y - 1], x);
                if (val == 0) {  /* bg, not a wall */
                    SET_DATA_BYTE(linep8[y - 1], x, DIR_SOUTH);  /* parent S */
                    el = mazeelCreate(x, y - 1, 0);
                    lqueueAdd(lq, el);
                }
            }
        }
        if (x < w - 1) {  /* check east */
            val = GET_DATA_BIT(linem1[y], x + 1);
            if (val == 0) {  /* not yet visited */
                SET_DATA_BIT(linem1[y], x + 1);  /* mark visited */
                val = GET_DATA_BIT(lines1[y], x + 1);
                if (val == 0) {  /* bg, not a wall */
                    SET_DATA_BYTE(linep8[y], x + 1, DIR_WEST);  /* parent W */
                    el = mazeelCreate(x + 1, y, 0);
                    lqueueAdd(lq, el);
                }
            }
        }
        if (y < h - 1) {  /* check south */
            val = GET_DATA_BIT(linem1[y + 1], x);
            if (val == 0) {  /* not yet visited */
                SET_DATA_BIT(linem1[y + 1], x);  /* mark visited */
                val = GET_DATA_BIT(lines1[y + 1], x);
                if (val == 0) {  /* bg, not a wall */
                    SET_DATA_BYTE(linep8[y + 1], x, DIR_NORTH);  /* parent N */
                    el = mazeelCreate(x, y + 1, 0);
                    lqueueAdd(lq, el);
                }
            }
        }
        FREE(elp);
    }

    lqueueDestroy(&lq, TRUE);
    pixDestroy(&pixm);
    FREE(linem1);

    if (ppixd) {
        pixd = pixUnpackBinary(pixs, 32, 1);
        *ppixd = pixd;
    }
    composeRGBPixel(255, 0, 0, &rpixel);  /* start point */
    composeRGBPixel(0, 255, 0, &gpixel);
    composeRGBPixel(0, 0, 255, &bpixel);  /* end point */

    if (found) {
        L_INFO(" Path found\n", procName);
        pta = ptaCreate(0);
        x = xf;
        y = yf;
        while (1) {
            ptaAddPt(pta, x, y);
            if (x == xi && y == yi)
                break;
            if (pixd)  /* write 'gpixel' onto the path */
                pixSetPixel(pixd, x, y, gpixel);
            pixGetPixel(pixp, x, y, &val);
            if (val == DIR_NORTH)
                y--;
            else if (val == DIR_SOUTH)
                y++;
            else if (val == DIR_EAST)
                x++;
            else if (val == DIR_WEST)
                x--;
        }
    } else {
        L_INFO(" No path found\n", procName);
        if (pixd) {  /* paint all visited locations */
            lined32 = pixGetLinePtrs(pixd, NULL);
            for (i = 0; i < h; i++) {
                for (j = 0; j < w; j++) {
                    val = GET_DATA_BYTE(linep8[i], j);
                    if (val != 0 && pixd)
                        SET_DATA_FOUR_BYTES(lined32[i], j, gpixel);
                }
            }
            FREE(lined32);
        }
    }
    if (pixd) {
        pixSetPixel(pixd, xi, yi, rpixel);
        pixSetPixel(pixd, xf, yf, bpixel);
    }

    pixDestroy(&pixp);
    FREE(lines1);
    FREE(linep8);
    return pta;
}


/*!
 *  localSearchForBackground()
 *
 *      Input:  &x, &y (starting position for search; return found position)
 *              maxrad (max distance to search from starting location)
 *      Return: 0 if bg pixel found; 1 if not found
 */
static l_int32
localSearchForBackground(PIX  *pix,
                         l_int32  *px,
                         l_int32  *py,
                         l_int32  maxrad)
{
l_int32   x, y, w, h, r, i, j;
l_uint32  val;

    x = *px;
    y = *py;
    pixGetPixel(pix, x, y, &val);
    if (val == 0) return 0;

        /* For each value of r, restrict the search to the boundary
         * pixels in a square centered on (x,y), clipping to the
         * image boundaries if necessary.  */
    pixGetDimensions(pix, &w, &h, NULL);
    for (r = 1; r < maxrad; r++) {
        for (i = -r; i <= r; i++) {
            if (y + i < 0 || y + i >= h)
                continue;
            for (j = -r; j <= r; j++) {
                if (x + j < 0 || x + j >= w)
                    continue;
                if (L_ABS(i) != r && L_ABS(j) != r)  /* not on "r ring" */
                    continue;
                pixGetPixel(pix, x + j, y + i, &val);
                if (val == 0) {
                    *px = x + j;
                    *py = y + i;
                    return 0;
                }
            }
        }
    }
    return 1;
}



/*---------------------------------------------------------------------*
 *                            Gray maze search                         *
 *---------------------------------------------------------------------*/
/*!
 *  pixSearchGrayMaze()
 *
 *      Input:  pixs (1 bpp, maze)
 *              xi, yi  (beginning point; use same initial point
 *                       that was used to generate the maze)
 *              xf, yf  (end point, or close to it)
 *              &ppixd (<optional return> maze with path illustrated, or
 *                     if no path possible, the part of the maze
 *                     that was searched)
 *      Return: pta (shortest path), or null if either no path
 *              exists or on error
 *
 *  Commentary:
 *      Consider first a slight generalization of the binary maze
 *      search problem.  Suppose that you can go through walls,
 *      but the cost is higher (say, an increment of 3 to go into
 *      a wall pixel rather than 1)?  You're still trying to find
 *      the shortest path.  One way to do this is with an ordered
 *      queue, and a simple way to visualize an ordered queue is as
 *      a set of stacks, each stack being marked with the distance
 *      of each pixel in the stack from the start.  We place the
 *      start pixel in stack 0, pop it, and process its 4 children.
 *      Each pixel is given a distance that is incremented from that
 *      of its parent (0 in this case), depending on if it is a wall
 *      pixel or not.  That value may be recorded on a distance map,
 *      according to the algorithm below.  For children of the first
 *      pixel, those not on a wall go in stack 1, and wall
 *      children go in stack 3.  Stack 0 being emptied, the process
 *      then continues with pixels being popped from stack 1.
 *      Here is the algorithm for each child pixel.  The pixel's
 *      distance value, were it to be placed on a stack, is compared
 *      with the value for it that is on the distance map.  There
 *      are three possible cases:
 *         (1) If the pixel has not yet been registered, it is pushed
 *             on its stack and the distance is written to the map.
 *         (2) If it has previously been registered with a higher distance,
 *             the distance on the map is relaxed to that of the
 *             current pixel, which is then placed on its stack.
 *         (3) If it has previously been registered with an equal
 *             or lower value, the pixel is discarded.
 *      The pixels are popped and processed successively from
 *      stack 1, and when stack 1 is empty, popping starts on stack 2.
 *      This continues until the destination pixel is popped off
 *      a stack.   The minimum path is then derived from the distance map,
 *      going back from the end point as before.  This is just Dijkstra's
 *      algorithm for a directed graph; here, the underlying graph
 *      (consisting of the pixels and four edges connecting each pixel
 *      to its 4-neighbor) is a special case of a directed graph, where
 *      each edge is bi-directional.  The implementation of this generalized
 *      maze search is left as an exercise to the reader.
 *
 *      Let's generalize a bit further.  Suppose the "maze" is just
 *      a grayscale image -- think of it as an elevation map.  The cost
 *      of moving on this surface depends on the height, or the gradient,
 *      or whatever you want.  All that is required is that the cost
 *      is specified and non-negative on each link between adjacent
 *      pixels.  Now the problem becomes: find the least cost path
 *      moving on this surface between two specified end points.
 *      For example, if the cost across an edge between two pixels
 *      depends on the "gradient", you can use:
 *           cost = 1 + L_ABS(deltaV)
 *      where deltaV is the difference in value between two adjacent
 *      pixels.  If the costs are all integers, we can still use an array
 *      of stacks to avoid ordering the queue (e.g., by using a heap sort.)
 *      This is a neat problem, because you don't even have to build a
 *      maze -- you can can use it on any grayscale image!
 *
 *      Rather than using an array of stacks, a more practical
 *      approach is to implement with a priority queue, which is
 *      a queue that is sorted so that the elements with the largest
 *      (or smallest) key values always come off first.  The
 *      priority queue is efficiently implemented as a heap, and
 *      this is how we do it.  Suppose you run the algorithm
 *      using a priority queue, doing the bookkeeping with an
 *      auxiliary image data structure that saves the distance of
 *      each pixel put on the queue as before, according to the method
 *      described above.  We implement it as a 2-way choice by
 *      initializing the distance array to a large value and putting
 *      a pixel on the queue if its distance is less than the value
 *      found on the array.  When you finally pop the end pixel from
 *      the queue, you're done, and you can trace the path backward,
 *      either always going downhill or using an auxiliary image to
 *      give you the direction to go at each step.  This is implemented
 *      here in searchGrayMaze().
 *
 *      Do we really have to use a sorted queue?  Can we solve this
 *      generalized maze with an unsorted queue of pixels?  (Or even
 *      an unsorted stack, doing a depth-first search (DFS)?)
 *      Consider a different algorithm for this generalized maze, where
 *      we travel again breadth first, but this time use a single,
 *      unsorted queue.  An auxiliary image is used as before to
 *      store the distances and to determine if pixels get pushed
 *      on the stack or dropped.  As before, we must allow pixels
 *      to be revisited, with relaxation of the distance if a shorter
 *      path arrives later.  As a result, we will in general have
 *      multiple instances of the same pixel on the stack with different
 *      distances.  However, because the queue is not ordered, some of
 *      these pixels will be popped when another instance with a lower
 *      distance is still on the stack.  Here, we're just popping them
 *      in the order they go on, rather than setting up a priority
 *      based on minimum distance.  Thus, unlike the priority queue,
 *      when a pixel is popped we have to check the distance map to
 *      see if a pixel with a lower distance has been put on the queue,
 *      and, if so, we discard the pixel we just popped.  So the
 *      "while" loop looks like this:
 *        - pop a pixel from the queue
 *        - check its distance against the distance stored in the
 *          distance map; if larger, discard
 *        - otherwise, for each of its neighbors:
 *            - compute its distance from the start pixel
 *            - compare this distance with that on the distance map:
 *                - if the distance map value higher, relax the distance
 *                  and push the pixel on the queue
 *                - if the distance map value is lower, discard the pixel
 *
 *      How does this loop terminate?  Before, with an ordered queue,
 *      it terminates when you pop the end pixel.  But with an unordered
 *      queue (or stack), the first time you hit the end pixel, the
 *      distance is not guaranteed to be correct, because the pixels
 *      along the shortest path may not have yet been visited and relaxed.
 *      Because the shortest path can theoretically go anywhere,
 *      we must keep going.  How do we know when to stop?   Dijkstra
 *      uses an ordered queue to systematically remove nodes from
 *      further consideration.  (Each time a pixel is popped, we're
 *      done with it; it's "finalized" in the Dijkstra sense because
 *      we know the shortest path to it.)  However, with an unordered
 *      queue, the brute force answer is: stop when the queue
 *      (or stack) is empty, because then every pixel in the image
 *      has been assigned its minimum "distance" from the start pixel.
 *
 *      This is similar to the situation when you use a stack for the
 *      simpler uniform-step problem: with breadth-first search (BFS)
 *      the pixels on the queue are automatically ordered, so you are
 *      done when you locate the end pixel as a neighbor of a popped pixel;
 *      whereas depth-first search (DFS), using a stack, requires,
 *      in general, a search of every accessible pixel.  Further, if
 *      a pixel is revisited with a smaller distance, that distance is
 *      recorded and the pixel is put on the stack again.
 *
 *      But surely, you ask, can't we stop sooner?  What if the
 *      start and end pixels are very close to each other?
 *      OK, suppose they are, and you have very high walls and a
 *      long snaking level path that is actually the minimum cost.
 *      That long path can wind back and forth across the entire
 *      maze many times before ending up at the end point, which
 *      could be just over a wall from the start.  With the unordered
 *      queue, you very quickly get a high distance for the end
 *      pixel, which will be relaxed to the minimum distance only
 *      after all the pixels of the path have been visited and placed
 *      on the queue, multiple times for many of them.  So that's the
 *      price for not ordering the queue!
 */
PTA *
pixSearchGrayMaze(PIX     *pixs,
                  l_int32  xi,
                  l_int32  yi,
                  l_int32  xf,
                  l_int32  yf,
                  PIX    **ppixd)
{
l_int32   x, y, w, h, d;
l_uint32  val, valr, vals, rpixel, gpixel, bpixel;
void    **lines8, **liner32, **linep8;
l_int32   cost, dist, distparent, sival, sivals;
MAZEEL   *el, *elp;
PIX      *pixd;  /* optionally plot the path on this RGB version of pixs */
PIX      *pixr;  /* for bookkeeping, to indicate the minimum distance */
                 /* to pixels already visited */
PIX      *pixp;  /* for bookkeeping, to indicate direction to parent */
L_HEAP   *lh;
PTA      *pta;

    PROCNAME("pixSearchGrayMaze");

    if (ppixd) *ppixd = NULL;
    if (!pixs)
        return (PTA *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return (PTA *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (xi <= 0 || xi >= w)
        return (PTA *)ERROR_PTR("xi not valid", procName, NULL);
    if (yi <= 0 || yi >= h)
        return (PTA *)ERROR_PTR("yi not valid", procName, NULL);
    pixd = NULL;
    pta = NULL;

    pixr = pixCreate(w, h, 32);
    pixSetAll(pixr);  /* initialize to max value */
    pixp = pixCreate(w, h, 8);  /* direction to parent stored as enum val */
    lines8 = pixGetLinePtrs(pixs, NULL);
    linep8 = pixGetLinePtrs(pixp, NULL);
    liner32 = pixGetLinePtrs(pixr, NULL);

    lh = lheapCreate(0, L_SORT_INCREASING);  /* always remove closest pixels */

        /* Prime the heap with the first pixel */
    pixGetPixel(pixs, xi, yi, &val);
    el = mazeelCreate(xi, yi, 0);  /* don't need direction here */
    el->distance = 0;
    pixGetPixel(pixs, xi, yi, &val);
    el->val = val;
    pixSetPixel(pixr, xi, yi, 0);  /* distance is 0 */
    lheapAdd(lh, el);

        /* Breadth-first search with priority queue (implemented by
           a heap), labeling direction to parents in pixp and minimum
           distance to visited pixels in pixr.  Stop when we pull
           the destination point (xf, yf) off the queue. */
    while (lheapGetCount(lh) > 0) {
        elp = (MAZEEL *)lheapRemove(lh);
        if (!elp)
            return (PTA *)ERROR_PTR("heap broken!!", procName, NULL);
        x = elp->x;
        y = elp->y;
        if (x == xf && y == yf) {  /* exit condition */
            FREE(elp);
            break;
        }
        distparent = (l_int32)elp->distance;
        val = elp->val;
        sival = val;

        if (x > 0) {  /* check to west */
            vals = GET_DATA_BYTE(lines8[y], x - 1);
            valr = GET_DATA_FOUR_BYTES(liner32[y], x - 1);
            sivals = (l_int32)vals;
            cost = 1 + L_ABS(sivals - sival);  /* cost to move to this pixel */
            dist = distparent + cost;
            if (dist < valr) {  /* shortest path so far to this pixel */
                SET_DATA_FOUR_BYTES(liner32[y], x - 1, dist);  /* new dist */
                SET_DATA_BYTE(linep8[y], x - 1, DIR_EAST);  /* parent to E */
                el = mazeelCreate(x - 1, y, 0);
                el->val = vals;
                el->distance = dist;
                lheapAdd(lh, el);
            }
        }
        if (y > 0) {  /* check north */
            vals = GET_DATA_BYTE(lines8[y - 1], x);
            valr = GET_DATA_FOUR_BYTES(liner32[y - 1], x);
            sivals = (l_int32)vals;
            cost = 1 + L_ABS(sivals - sival);  /* cost to move to this pixel */
            dist = distparent + cost;
            if (dist < valr) {  /* shortest path so far to this pixel */
                SET_DATA_FOUR_BYTES(liner32[y - 1], x, dist);  /* new dist */
                SET_DATA_BYTE(linep8[y - 1], x, DIR_SOUTH);  /* parent to S */
                el = mazeelCreate(x, y - 1, 0);
                el->val = vals;
                el->distance = dist;
                lheapAdd(lh, el);
            }
        }
        if (x < w - 1) {  /* check east */
            vals = GET_DATA_BYTE(lines8[y], x + 1);
            valr = GET_DATA_FOUR_BYTES(liner32[y], x + 1);
            sivals = (l_int32)vals;
            cost = 1 + L_ABS(sivals - sival);  /* cost to move to this pixel */
            dist = distparent + cost;
            if (dist < valr) {  /* shortest path so far to this pixel */
                SET_DATA_FOUR_BYTES(liner32[y], x + 1, dist);  /* new dist */
                SET_DATA_BYTE(linep8[y], x + 1, DIR_WEST);  /* parent to W */
                el = mazeelCreate(x + 1, y, 0);
                el->val = vals;
                el->distance = dist;
                lheapAdd(lh, el);
            }
        }
        if (y < h - 1) {  /* check south */
            vals = GET_DATA_BYTE(lines8[y + 1], x);
            valr = GET_DATA_FOUR_BYTES(liner32[y + 1], x);
            sivals = (l_int32)vals;
            cost = 1 + L_ABS(sivals - sival);  /* cost to move to this pixel */
            dist = distparent + cost;
            if (dist < valr) {  /* shortest path so far to this pixel */
                SET_DATA_FOUR_BYTES(liner32[y + 1], x, dist);  /* new dist */
                SET_DATA_BYTE(linep8[y + 1], x, DIR_NORTH);  /* parent to N */
                el = mazeelCreate(x, y + 1, 0);
                el->val = vals;
                el->distance = dist;
                lheapAdd(lh, el);
            }
        }
        FREE(elp);
    }

    lheapDestroy(&lh, TRUE);

    if (ppixd) {
        pixd = pixConvert8To32(pixs);
        *ppixd = pixd;
    }
    composeRGBPixel(255, 0, 0, &rpixel);  /* start point */
    composeRGBPixel(0, 255, 0, &gpixel);
    composeRGBPixel(0, 0, 255, &bpixel);  /* end point */

    x = xf;
    y = yf;
    pta = ptaCreate(0);
    while (1) {  /* write path onto pixd */
        ptaAddPt(pta, x, y);
        if (x == xi && y == yi)
            break;
        if (pixd)
            pixSetPixel(pixd, x, y, gpixel);
        pixGetPixel(pixp, x, y, &val);
        if (val == DIR_NORTH)
            y--;
        else if (val == DIR_SOUTH)
            y++;
        else if (val == DIR_EAST)
            x++;
        else if (val == DIR_WEST)
            x--;
        pixGetPixel(pixr, x, y, &val);

#if  DEBUG_PATH
        fprintf(stderr, "(x,y) = (%d, %d); dist = %d\n", x, y, val);
#endif  /* DEBUG_PATH */

    }
    if (pixd) {
        pixSetPixel(pixd, xi, yi, rpixel);
        pixSetPixel(pixd, xf, yf, bpixel);
    }

    pixDestroy(&pixp);
    pixDestroy(&pixr);
    FREE(lines8);
    FREE(linep8);
    FREE(liner32);
    return pta;
}


/*---------------------------------------------------------------------*
 *                      Largest rectangle in an image                  *
 *---------------------------------------------------------------------*/
/*!
 *  pixFindLargestRectangle()
 *
 *      Input:  pixs  (1 bpp)
 *              polarity (0 within background, 1 within foreground)
 *              &box (<return> largest rectangle, either by area or
 *                    by perimeter)
 *              debugflag (1 to output image with rectangle drawn on it)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Why is this here?  This is a simple and elegant solution to
 *          a problem in computational geometry that at first appears
 *          quite difficult: what is the largest rectangle that can
 *          be placed in the image, covering only pixels of one polarity
 *          (bg or fg)?  The solution is O(n), where n is the number
 *          of pixels in the image, and it requires nothing more than
 *          using a simple recursion relation in a single sweep of the image.
 *      (2) In a sweep from UL to LR with left-to-right being the fast
 *          direction, calculate the largest white rectangle at (x, y),
 *          using previously calculated values at pixels #1 and #2:
 *             #1:    (x, y - 1)
 *             #2:    (x - 1, y)
 *          We also need the most recent "black" pixels that were seen
 *          in the current row and column.
 *          Consider the largest area.  There are only two possibilities:
 *             (a)  Min(w(1), horizdist) * (h(1) + 1)
 *             (b)  Min(h(2), vertdist) * (w(2) + 1)
 *          where
 *             horizdist: the distance from the rightmost "black" pixel seen
 *                        in the current row across to the current pixel
 *             vertdist: the distance from the lowest "black" pixel seen
 *                       in the current column down to the current pixel
 *          and we choose the Max of (a) and (b).
 *      (3) To convince yourself that these recursion relations are correct,
 *          it helps to draw the maximum rectangles at #1 and #2.
 *          Then for #1, you try to extend the rectangle down one line,
 *          so that the height is h(1) + 1.  Do you get the full
 *          width of #1, w(1)?  It depends on where the black pixels are
 *          in the current row.  You know the final width is bounded by w(1)
 *          and w(2) + 1, but the actual value depends on the distribution
 *          of black pixels in the current row that are at a distance
 *          from the current pixel that is between these limits.
 *          We call that value "horizdist", and the area is then given
 *          by the expression (a) above.  Using similar reasoning for #2,
 *          where you attempt to extend the rectangle to the right
 *          by 1 pixel, you arrive at (b).  The largest rectangle is
 *          then found by taking the Max.
 */
l_int32
pixFindLargestRectangle(PIX         *pixs,
                        l_int32      polarity,
                        BOX        **pbox,
                        const char  *debugfile)
{
l_int32    i, j, w, h, d, wpls, val;
l_int32    wp, hp, w1, w2, h1, h2, wmin, hmin, area1, area2;
l_int32    xmax, ymax;  /* LR corner of the largest rectangle */
l_int32    maxarea, wmax, hmax, vertdist, horizdist, prevfg;
l_int32   *lowestfg;
l_uint32  *datas, *lines;
l_uint32 **linew, **lineh;
BOX       *box;
PIX       *pixw, *pixh;  /* keeps the width and height for the largest */
                         /* rectangles whose LR corner is located there. */

    PROCNAME("pixFindLargestRectangle");

    if (!pbox)
        return ERROR_INT("&box not defined", procName, 1);
    *pbox = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1)
        return ERROR_INT("pixs not 1 bpp", procName, 1);
    if (polarity != 0 && polarity != 1)
        return ERROR_INT("invalid polarity", procName, 1);

        /* Initialize lowest "fg" seen so far for each column */
    lowestfg = (l_int32 *)CALLOC(w, sizeof(l_int32));
    for (i = 0; i < w; i++)
        lowestfg[i] = -1;

        /* The combination (val ^ polarity) is the color for which we
         * are searching for the maximum rectangle.  For polarity == 0,
         * we search in the bg (white). */
    pixw = pixCreate(w, h, 32);  /* stores width */
    pixh = pixCreate(w, h, 32);  /* stores height */
    linew = (l_uint32 **)pixGetLinePtrs(pixw, NULL);
    lineh = (l_uint32 **)pixGetLinePtrs(pixh, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    maxarea = xmax = ymax = wmax = hmax = 0;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        prevfg = -1;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BIT(lines, j);
            if ((val ^ polarity) == 0) {  /* bg (0) if polarity == 0, etc. */
                if (i == 0 && j == 0) {
                    wp = hp = 1;
                } else if (i == 0) {
                    wp = linew[i][j - 1] + 1;
                    hp = 1;
                } else if (j == 0) {
                    wp = 1;
                    hp = lineh[i - 1][j] + 1;
                } else {
                        /* Expand #1 prev rectangle down */
                    w1 = linew[i - 1][j];
                    h1 = lineh[i - 1][j];
                    horizdist = j - prevfg;
                    wmin = L_MIN(w1, horizdist);  /* width of new rectangle */
                    area1 = wmin * (h1 + 1);

                        /* Expand #2 prev rectangle to right */
                    w2 = linew[i][j - 1];
                    h2 = lineh[i][j - 1];
                    vertdist = i - lowestfg[j];
                    hmin = L_MIN(h2, vertdist);  /* height of new rectangle */
                    area2 = hmin * (w2 + 1);

                    if (area1 > area2) {
                         wp = wmin;
                         hp = h1 + 1;
                    } else {
                         wp = w2 + 1;
                         hp = hmin;
                    }
                }
            } else {  /* fg (1) if polarity == 0; bg (0) if polarity == 1 */
                prevfg = j;
                lowestfg[j] = i;
                wp = hp = 0;
            }
            linew[i][j] = wp;
            lineh[i][j] = hp;
            if (wp * hp > maxarea) {
                maxarea = wp * hp;
                xmax = j;
                ymax = i;
                wmax = wp;
                hmax = hp;
            }
        }
    }

        /* Translate from LR corner to Box coords (UL corner, w, h) */
    box = boxCreate(xmax - wmax + 1, ymax - hmax + 1, wmax, hmax);
    *pbox = box;

    if (debugfile) {
        PIX  *pixdb;
        pixdb = pixConvertTo8(pixs, TRUE);
        pixRenderHashBoxArb(pixdb, box, 6, 2, L_NEG_SLOPE_LINE, 1, 255, 0, 0);
        pixWrite(debugfile, pixdb, IFF_PNG);
        pixDestroy(&pixdb);
    }

    FREE(linew);
    FREE(lineh);
    FREE(lowestfg);
    pixDestroy(&pixw);
    pixDestroy(&pixh);
    return 0;
}
