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
 * listtest.c
 *
 *    This file tests the main functions in the generic
 *    list facility, given in list.c and list.h.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char        *filein;
l_int32      i, n, w, h, samecount, count;
BOX         *box, *boxc;
BOXA        *boxa, *boxan;
DLLIST      *head, *tail, *head2, *tail2, *elem, *nextelem;
PIX         *pixs, *pixd, *pixd1, *pixd2;
static char  mainName[] = "listtest";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  listtest filein", mainName, 1));

    filein = argv[1];

    boxa = boxan = NULL;

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));

	/* start with a boxa */
    boxa = pixConnComp(pixs, NULL, 4);
    n = boxaGetCount(boxa);

  /*-------------------------------------------------------*
   *        Do one of these two ...
   *-------------------------------------------------------*/
#if  1
	/* listAddToTail(): make a list by adding to tail */
    head = NULL;
    tail = NULL;
    for (i = 0; i < n; i++) {
	box = boxaGetBox(boxa, i, L_CLONE);
	listAddToTail(&head, &tail, box);
    }
#else
	/* listAddToHead(): make a list by adding to head */
    head = NULL;
    for (i = 0; i < n; i++) {
	box = boxaGetBox(boxa, i, L_CLONE);
	listAddToHead(&head, box);
    }
#endif

  /*-------------------------------------------------------*
   *        Then do one of these ...
   *-------------------------------------------------------*/
#if 1
	/* list concatenation */
    head2 = NULL;   /* cons up 2nd list from null */
    tail2 = NULL;
    for (i = 0; i < n; i++) {
	box = boxaGetBox(boxa, i, L_CLONE);
	listAddToTail(&head2, &tail2, box); 
    }
    listJoin(&head, &head2);
#endif

    count = listGetCount(head);
    fprintf(stderr, "%d items in list\n", count);
    listReverse(&head);
    count = listGetCount(head);
    fprintf(stderr, "%d items in reversed list\n", count);
    listReverse(&head);
    count = listGetCount(head);
    fprintf(stderr, "%d items in doubly reversed list\n", count);

    boxan = boxaCreate(n);

#if 0 
	/* removal of all elements and data from a list, 
	 * without using BEGIN_LIST_FORWARD macro */
    for (elem = head; elem; elem = nextelem) {
        nextelem = elem->next;
        box = (BOX *)elem->data;
	boxaAddBox(boxan, box, L_INSERT);
	elem->data = NULL;
	listRemoveElement(&head, elem);
    }
#endif

#if 0 
	/* removal of all elements and data from a list, 
	 * using BEGIN_LIST_FORWARD macro */
    BEGIN_LIST_FORWARD(head, elem)
        box = (BOX *)elem->data;
	boxaAddBox(boxan, box, L_INSERT);
	elem->data = NULL;
	listRemoveElement(&head, elem);
    END_LIST
#endif

#if 0 
	/* removal of all elements and data from a list, 
	 * using BEGIN_LIST_REVERSE macro */
    tail = listFindTail(head);
    BEGIN_LIST_REVERSE(tail, elem)
        box = (BOX *)elem->data;
	boxaAddBox(boxan, box, L_INSERT);
	elem->data = NULL;
	listRemoveElement(&head, elem);
    END_LIST
#endif

#if 0
	/* boxa and boxan are same when list made with listAddToHead() */
    tail = listFindTail(head);
    BEGIN_LIST_REVERSE(tail, elem)
        box = (BOX *)elem->data;
	boxaAddBox(boxan, box, L_INSERT);
	elem->data = NULL;
	listRemoveElement(&head, elem);
    END_LIST
    for (i = 0, samecount = 0; i < n; i++) {
	if (boxa->box[i]->w == boxan->box[i]->w  &&
	    boxa->box[i]->h == boxan->box[i]->h)
	    samecount++;
    }
    fprintf(stderr, " num boxes = %d, same count = %d\n",
            boxaGetCount(boxa), samecount);
#endif

#if 0 
	/* boxa and boxan are same when list made with listAddToTail() */
    BEGIN_LIST_FORWARD(head, elem)
        box = (BOX *)elem->data;
	boxaAddBox(boxan, box, L_INSERT);
	elem->data = NULL;
	listRemoveElement(&head, elem);
    END_LIST
    for (i = 0, samecount = 0; i < n; i++) {
	if (boxa->box[i]->w == boxan->box[i]->w  &&
	    boxa->box[i]->h == boxan->box[i]->h)
	    samecount++;
    }
    fprintf(stderr, " num boxes = %d, same count = %d\n",
            boxaGetCount(boxa), samecount);
#endif

#if 0
	/* destroy the boxes and then the list */
    BEGIN_LIST_FORWARD(head, elem)
	box = (BOX *)elem->data;
	boxDestroy(&box);
	elem->data = NULL;
    END_LIST
    listDestroy(&head);
#endif

#if 0 
	/* listInsertBefore(): inserting a copy BEFORE each element */
    BEGIN_LIST_FORWARD(head, elem)
        box = (BOX *)elem->data;
	boxc = boxCopy(box);
	listInsertBefore(&head, elem, boxc);
    END_LIST
    BEGIN_LIST_FORWARD(head, elem)
	box = (BOX *)elem->data;
	boxaAddBox(boxan, box, L_INSERT);
	elem->data = NULL;
    END_LIST
    listDestroy(&head);
#endif

#if 0 
	/* listInsertAfter(): inserting a copy AFTER that element */
    BEGIN_LIST_FORWARD(head, elem)
        box = (BOX *)elem->data;
	boxc = boxCopy(box);
	listInsertAfter(&head, elem, boxc);
    END_LIST
    BEGIN_LIST_FORWARD(head, elem)
	box = (BOX *)elem->data;
	boxaAddBox(boxan, box, L_INSERT);
	elem->data = NULL;
	listRemoveElement(&head, elem);
    END_LIST
/*    listDestroy(&head); */
#endif

#if 0
	/* test listRemoveFromHead(), to successively
	 * remove the head of the list for all elements. */
    count = 0;
    while (head) {
	box = listRemoveFromHead(&head);
	boxDestroy(&box);
	count++;
    }
    fprintf(stderr, "removed %d items\n", count);
#endif

#if 0
	/* another version to test listRemoveFromHead(), using
	 * an iterator macro. */
    count = 0;
    BEGIN_LIST_FORWARD(head, elem)
	box = (BOX *)listRemoveFromHead(&head);
	boxDestroy(&box);
	count++;
    END_LIST
    fprintf(stderr, "removed %d items\n", count);
#endif

#if 0
	/* test listRemoveFromTail(), to successively remove
	 * the tail of the list for all elements. */
    count = 0;
    tail = NULL;   /* will find tail automatically */
    while (head) {
	box = (BOX *)listRemoveFromTail(&head, &tail);
	boxDestroy(&box);
	count++;
    }
    fprintf(stderr, "removed %d items\n", count);
#endif

#if 0
	/* another version to test listRemoveFromTail(), using
	 * an iterator macro. */
    count = 0;
    tail = listFindTail(head);  /* need to initialize tail */
    BEGIN_LIST_REVERSE(tail, elem)
	box = (BOX *)listRemoveFromTail(&head, &tail);
	boxDestroy(&box);
	count++;
    END_LIST
    fprintf(stderr, "removed %d items\n", count);
#endif

#if 0
	/* Iterate backwards over the box array, and use
	 * listFindElement() to find each corresponding data structure
	 * within the list; then remove it.  Should completely
	 * destroy the list.   Note that listFindElement()
	 * returns the cell without removing it from the list! */
    n = boxaGetCount(boxa);
    for (i = 0, count = 0; i < n; i++) {
	box = boxaGetBox(boxa, n - i - 1, L_CLONE);
	if (i % 1709 == 0) boxPrintStream(stderr, box);
	elem = listFindElement(head, box);
	boxDestroy(&box);
	if (elem) {  /* found */
	    box = listRemoveElement(&head, elem);
            if (i % 1709 == 0) boxPrintStream(stderr, box);
	    boxDestroy(&box);
	    count++;
	}
    }
    fprintf(stderr, "removed %d items\n", count);
#endif

    fprintf(stderr, "boxa count = %d; boxan count = %d\n",
                     boxaGetCount(boxa), boxaGetCount(boxan));
    boxaGetExtent(boxa, &w, &h, NULL);
    fprintf(stderr, "boxa extent = (%d, %d)\n", w, h);
    boxaGetExtent(boxan, &w, &h, NULL);
    fprintf(stderr, "boxan extent = (%d, %d)\n", w, h);

    pixDestroy(&pixs);
    boxaDestroy(&boxa);
    boxaDestroy(&boxan);
    exit(0);
}

