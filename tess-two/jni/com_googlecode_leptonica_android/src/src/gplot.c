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
 *  gplot.c
 *
 *     Basic plotting functions
 *          GPLOT      *gplotCreate()
 *          void        gplotDestroy()
 *          l_int32     gplotAddPlot()
 *          l_int32     gplotSetScaling()
 *          l_int32     gplotMakeOutput()
 *          l_int32     gplotGenCommandFile()
 *          l_int32     gplotGenDataFiles()
 *
 *     Quick and dirty plots
 *          l_int32     gplotSimple1()
 *          l_int32     gplotSimple2()
 *          l_int32     gplotSimpleN()
 *
 *     Serialize for I/O
 *          GPLOT      *gplotRead()
 *          l_int32     gplotWrite()
 *
 *
 *     Utility for programmatic plotting using gnuplot 7.3.2 or later
 *     Enabled:
 *         - output to png (color), ps (mono), x11 (color), latex (mono)
 *         - optional title for graph
 *         - optional x and y axis labels
 *         - multiple plots on one frame
 *         - optional title for each plot on the frame
 *         - optional log scaling on either or both axes
 *         - choice of 5 plot styles for each plot
 *         - choice of 2 plot modes, either using one input array
 *           (Y vs index) or two input arrays (Y vs X).  This
 *           choice is made implicitly depending on the number of
 *           input arrays.
 *
 *     Usage:
 *         gplotCreate() initializes for plotting
 *         gplotAddPlot() for each plot on the frame
 *         gplotMakeOutput() to generate all output files and run gnuplot
 *         gplotDestroy() to clean up
 *
 *     Example of use:
 *         gplot = gplotCreate("tempskew", GPLOT_PNG, "Skew score vs angle",
 *                    "angle (deg)", "score");
 *         gplotAddPlot(gplot, natheta, nascore1, GPLOT_LINES, "plot 1");
 *         gplotAddPlot(gplot, natheta, nascore2, GPLOT_POINTS, "plot 2");
 *         gplotSetScaling(gplot, GPLOT_LOG_SCALE_Y);
 *         gplotMakeOutput(gplot);
 *         gplotDestroy(&gplot);
 *
 *     Note for output to GPLOT_LATEX:
 *         This creates latex output of the plot, named <rootname>.tex.
 *         It needs to be placed in a latex file <latexname>.tex
 *         that precedes the plot output with, at a minimum:
 *           \documentclass{article}
 *           \begin{document}
 *         and ends with
 *           \end{document}
 *         You can then generate a dvi file <latexname>.dvi using
 *           latex <latexname>.tex
 *         and a PostScript file <psname>.ps from that using
 *           dvips -o <psname>.ps <latexname>.dvi
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  L_BUF_SIZE = 512;

const char  *gplotstylenames[] = {"with lines",
                                  "with points",
                                  "with impulses",
                                  "with linespoints",
                                  "with dots"};
const char  *gplotfilestyles[] = {"LINES",
                                  "POINTS",
                                  "IMPULSES",
                                  "LINESPOINTS",
                                  "DOTS"};
const char  *gplotfileoutputs[] = {"",
                                   "PNG",
                                   "PS",
                                   "EPS",
                                   "X11",
                                   "LATEX"};


/*-----------------------------------------------------------------*
 *                       Basic Plotting Functions                  *
 *-----------------------------------------------------------------*/
/*!
 *  gplotCreate()
 *
 *      Input:  rootname (root for all output files)
 *              outformat (GPLOT_PNG, GPLOT_PS, GPLOT_EPS, GPLOT_X11,
 *                         GPLOT_LATEX)
 *              title  (<optional> overall title)
 *              xlabel (<optional> x axis label)
 *              ylabel (<optional> y axis label)
 *      Return: gplot, or null on error
 *
 *  Notes:
 *      (1) This initializes the plot.
 *      (2) The 'title', 'xlabel' and 'ylabel' strings can have spaces,
 *          double quotes and backquotes, but not single quotes.
 */
GPLOT  *
gplotCreate(const char  *rootname,
            l_int32      outformat,
            const char  *title,
            const char  *xlabel,
            const char  *ylabel)
{
char   *newroot;
char    buf[L_BUF_SIZE];
GPLOT  *gplot;

    PROCNAME("gplotCreate");

    if (!rootname)
        return (GPLOT *)ERROR_PTR("rootname not defined", procName, NULL);
    if (outformat != GPLOT_PNG && outformat != GPLOT_PS &&
        outformat != GPLOT_EPS && outformat != GPLOT_X11 &&
        outformat != GPLOT_LATEX)
        return (GPLOT *)ERROR_PTR("outformat invalid", procName, NULL);

    if ((gplot = (GPLOT *)CALLOC(1, sizeof(GPLOT))) == NULL)
        return (GPLOT *)ERROR_PTR("gplot not made", procName, NULL);
    gplot->cmddata = sarrayCreate(0);
    gplot->datanames = sarrayCreate(0);
    gplot->plotdata = sarrayCreate(0);
    gplot->plottitles = sarrayCreate(0);
    gplot->plotstyles = numaCreate(0);

        /* Save title, labels, rootname, outformat, cmdname, outname */
    newroot = genPathname(rootname, NULL);  /* remove '/tmp' on windows */
    gplot->rootname = newroot;
    gplot->outformat = outformat;
    snprintf(buf, L_BUF_SIZE, "%s.cmd", newroot);
    gplot->cmdname = stringNew(buf);
    if (outformat == GPLOT_PNG)
        snprintf(buf, L_BUF_SIZE, "%s.png", newroot);
    else if (outformat == GPLOT_PS)
        snprintf(buf, L_BUF_SIZE, "%s.ps", newroot);
    else if (outformat == GPLOT_EPS)
        snprintf(buf, L_BUF_SIZE, "%s.eps", newroot);
    else if (outformat == GPLOT_LATEX)
        snprintf(buf, L_BUF_SIZE, "%s.tex", newroot);
    else  /* outformat == GPLOT_X11 */
        buf[0] = '\0';
    gplot->outname = stringNew(buf);
    if (title) gplot->title = stringNew(title);
    if (xlabel) gplot->xlabel = stringNew(xlabel);
    if (ylabel) gplot->ylabel = stringNew(ylabel);

    return gplot;
}


/*!
 *   gplotDestroy()
 *
 *        Input: &gplot (<to be nulled>)
 *        Return: void
 */
void
gplotDestroy(GPLOT  **pgplot)
{
GPLOT  *gplot;

    PROCNAME("gplotDestroy");

    if (pgplot == NULL) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }

    if ((gplot = *pgplot) == NULL)
        return;

    FREE(gplot->rootname);
    FREE(gplot->cmdname);
    sarrayDestroy(&gplot->cmddata);
    sarrayDestroy(&gplot->datanames);
    sarrayDestroy(&gplot->plotdata);
    sarrayDestroy(&gplot->plottitles);
    numaDestroy(&gplot->plotstyles);
    FREE(gplot->outname);
    if (gplot->title)
        FREE(gplot->title);
    if (gplot->xlabel)
        FREE(gplot->xlabel);
    if (gplot->ylabel)
        FREE(gplot->ylabel);

    FREE(gplot);
    *pgplot = NULL;
    return;
}


/*!
 *  gplotAddPlot()
 *
 *      Input:  gplot
 *              nax (<optional> numa: set to null for Y_VS_I;
 *                   required for Y_VS_X)
 *              nay (numa: required for both Y_VS_I and Y_VS_X)
 *              plotstyle (GPLOT_LINES, GPLOT_POINTS, GPLOT_IMPULSES,
 *                         GPLOT_LINESPOINTS, GPLOT_DOTS)
 *              plottitle  (<optional> title for individual plot)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) There are 2 options for (x,y) values:
 *            o  To plot an array vs the index, set nax = NULL.
 *            o  To plot one array vs another, use both nax and nay.
 *      (2) If nax is defined, it must be the same size as nay.
 *      (3) The 'plottitle' string can have spaces, double
 *          quotes and backquotes, but not single quotes.
 */
l_int32
gplotAddPlot(GPLOT       *gplot,
             NUMA        *nax,
             NUMA        *nay,
             l_int32      plotstyle,
             const char  *plottitle)
{
char       buf[L_BUF_SIZE];
char       emptystring[] = "";
char      *datastr, *title;
l_int32    n, i;
l_float32  valx, valy, startx, delx;
SARRAY    *sa;

    PROCNAME("gplotAddPlot");

    if (!gplot)
        return ERROR_INT("gplot not defined", procName, 1);
    if (!nay)
        return ERROR_INT("nay not defined", procName, 1);
    if (plotstyle != GPLOT_LINES && plotstyle != GPLOT_POINTS &&
        plotstyle != GPLOT_IMPULSES && plotstyle != GPLOT_LINESPOINTS &&
        plotstyle != GPLOT_DOTS)
        return ERROR_INT("invalid plotstyle", procName, 1);

    n = numaGetCount(nay);
    numaGetParameters(nay, &startx, &delx);
    if (nax) {
        if (n != numaGetCount(nax))
            return ERROR_INT("nax and nay sizes differ", procName, 1);
    }

        /* Save plotstyle and plottitle */
    numaAddNumber(gplot->plotstyles, plotstyle);
    if (plottitle) {
        title = stringNew(plottitle);
        sarrayAddString(gplot->plottitles, title, L_INSERT);
    } else {
        sarrayAddString(gplot->plottitles, emptystring, L_COPY);
    }

        /* Generate and save data filename */
    gplot->nplots++;
    snprintf(buf, L_BUF_SIZE, "%s.data.%d", gplot->rootname, gplot->nplots);
    sarrayAddString(gplot->datanames, buf, L_COPY);

        /* Generate data and save as a string */
    sa = sarrayCreate(n);
    for (i = 0; i < n; i++) {
        if (nax)
            numaGetFValue(nax, i, &valx);
        else
            valx = startx + i * delx;
        numaGetFValue(nay, i, &valy);
        snprintf(buf, L_BUF_SIZE, "%f %f\n", valx, valy);
        sarrayAddString(sa, buf, L_COPY);
    }
    datastr = sarrayToString(sa, 0);
    sarrayAddString(gplot->plotdata, datastr, L_INSERT);
    sarrayDestroy(&sa);

    return 0;
}


/*!
 *  gplotSetScaling()
 *
 *      Input:  gplot
 *              scaling (GPLOT_LINEAR_SCALE, GPLOT_LOG_SCALE_X,
 *                       GPLOT_LOG_SCALE_Y, GPLOT_LOG_SCALE_X_Y)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) By default, the x and y axis scaling is linear.
 *      (2) Call this function to set semi-log or log-log scaling.
 */
l_int32
gplotSetScaling(GPLOT   *gplot,
                l_int32  scaling)
{
    PROCNAME("gplotSetScaling");

    if (!gplot)
        return ERROR_INT("gplot not defined", procName, 1);
    if (scaling != GPLOT_LINEAR_SCALE &&
        scaling != GPLOT_LOG_SCALE_X &&
        scaling != GPLOT_LOG_SCALE_Y &&
        scaling != GPLOT_LOG_SCALE_X_Y)
        return ERROR_INT("invalid gplot scaling", procName, 1);
    gplot->scaling = scaling;
    return 0;
}


/*!
 *  gplotMakeOutput()
 *
 *      Input:  gplot
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This uses gplot and the new arrays to add a plot
 *          to the output, by writing a new data file and appending
 *          the appropriate plot commands to the command file.
 *      (2) The gnuplot program for windows is wgnuplot.exe.  The
 *          standard gp426win32 distribution does not have a X11 terminal.
 */
l_int32
gplotMakeOutput(GPLOT  *gplot)
{
char     buf[L_BUF_SIZE];
l_int32  ignore;

    PROCNAME("gplotMakeOutput");

    if (!gplot)
        return ERROR_INT("gplot not defined", procName, 1);

    gplotGenCommandFile(gplot);
    gplotGenDataFiles(gplot);

#ifndef _WIN32
    if (gplot->outformat != GPLOT_X11)
        snprintf(buf, L_BUF_SIZE, "gnuplot %s &", gplot->cmdname);
    else
        snprintf(buf, L_BUF_SIZE,
                 "gnuplot -persist -geometry +10+10 %s &", gplot->cmdname);
#else
   if (gplot->outformat != GPLOT_X11)
       snprintf(buf, L_BUF_SIZE, "wgnuplot %s", gplot->cmdname);
   else
       snprintf(buf, L_BUF_SIZE,
               "wgnuplot -persist %s", gplot->cmdname);
#endif  /* _WIN32 */
    ignore = system(buf);
    return 0;
}


/*!
 *  gplotGenCommandFile()
 *
 *      Input:  gplot
 *      Return: 0 if OK, 1 on error
 */
l_int32
gplotGenCommandFile(GPLOT  *gplot)
{
char     buf[L_BUF_SIZE];
char    *cmdstr, *plottitle, *dataname;
l_int32  i, plotstyle, nplots;
FILE    *fp;

    PROCNAME("gplotGenCommandFile");

    if (!gplot)
        return ERROR_INT("gplot not defined", procName, 1);

        /* Remove any previous command data */
    sarrayClear(gplot->cmddata);

        /* Generate command data instructions */
    if (gplot->title) {   /* set title */
        snprintf(buf, L_BUF_SIZE, "set title '%s'", gplot->title);
        sarrayAddString(gplot->cmddata, buf, L_COPY);
    }
    if (gplot->xlabel) {   /* set xlabel */
        snprintf(buf, L_BUF_SIZE, "set xlabel '%s'", gplot->xlabel);
        sarrayAddString(gplot->cmddata, buf, L_COPY);
    }
    if (gplot->ylabel) {   /* set ylabel */
        snprintf(buf, L_BUF_SIZE, "set ylabel '%s'", gplot->ylabel);
        sarrayAddString(gplot->cmddata, buf, L_COPY);
    }

    if (gplot->outformat == GPLOT_PNG)    /* set terminal type and output */
        snprintf(buf, L_BUF_SIZE, "set terminal png; set output '%s'",
                 gplot->outname);
    else if (gplot->outformat == GPLOT_PS)
        snprintf(buf, L_BUF_SIZE, "set terminal postscript; set output '%s'",
                 gplot->outname);
    else if (gplot->outformat == GPLOT_EPS)
        snprintf(buf, L_BUF_SIZE,
                "set terminal postscript eps; set output '%s'",
                gplot->outname);
    else if (gplot->outformat == GPLOT_LATEX)
        snprintf(buf, L_BUF_SIZE, "set terminal latex; set output '%s'",
                 gplot->outname);
    else  /* gplot->outformat == GPLOT_X11 */
#ifndef _WIN32
        snprintf(buf, L_BUF_SIZE, "set terminal x11");
#else
        snprintf(buf, L_BUF_SIZE, "set terminal windows");
#endif  /* _WIN32 */
    sarrayAddString(gplot->cmddata, buf, L_COPY);

    if (gplot->scaling == GPLOT_LOG_SCALE_X ||
        gplot->scaling == GPLOT_LOG_SCALE_X_Y) {
        snprintf(buf, L_BUF_SIZE, "set logscale x");
        sarrayAddString(gplot->cmddata, buf, L_COPY);
    }
    if (gplot->scaling == GPLOT_LOG_SCALE_Y ||
        gplot->scaling == GPLOT_LOG_SCALE_X_Y) {
        snprintf(buf, L_BUF_SIZE, "set logscale y");
        sarrayAddString(gplot->cmddata, buf, L_COPY);
    }

    nplots = sarrayGetCount(gplot->datanames);
    for (i = 0; i < nplots; i++) {
        plottitle = sarrayGetString(gplot->plottitles, i, L_NOCOPY);
        dataname = sarrayGetString(gplot->datanames, i, L_NOCOPY);
        numaGetIValue(gplot->plotstyles, i, &plotstyle);
        if (nplots == 1) {
            snprintf(buf, L_BUF_SIZE, "plot '%s' title '%s' %s",
                     dataname, plottitle, gplotstylenames[plotstyle]);
        } else {
            if (i == 0)
                snprintf(buf, L_BUF_SIZE, "plot '%s' title '%s' %s, \\",
                     dataname, plottitle, gplotstylenames[plotstyle]);
            else if (i < nplots - 1)
                snprintf(buf, L_BUF_SIZE, " '%s' title '%s' %s, \\",
                     dataname, plottitle, gplotstylenames[plotstyle]);
            else
                snprintf(buf, L_BUF_SIZE, " '%s' title '%s' %s",
                     dataname, plottitle, gplotstylenames[plotstyle]);
        }
        sarrayAddString(gplot->cmddata, buf, L_COPY);
    }

        /* Write command data to file */
    cmdstr = sarrayToString(gplot->cmddata, 1);
    if ((fp = fopenWriteStream(gplot->cmdname, "w")) == NULL)
        return ERROR_INT("cmd stream not opened", procName, 1);
    fwrite(cmdstr, 1, strlen(cmdstr), fp);
    fclose(fp);
    FREE(cmdstr);
    return 0;
}


/*!
 *  gplotGenDataFiles()
 *
 *      Input:  gplot
 *      Return: 0 if OK, 1 on error
 */
l_int32
gplotGenDataFiles(GPLOT  *gplot)
{
char    *plotdata, *dataname;
l_int32  i, nplots;
FILE    *fp;

    PROCNAME("gplotGenDataFiles");

    if (!gplot)
        return ERROR_INT("gplot not defined", procName, 1);

    nplots = sarrayGetCount(gplot->datanames);
    for (i = 0; i < nplots; i++) {
        plotdata = sarrayGetString(gplot->plotdata, i, L_NOCOPY);
        dataname = sarrayGetString(gplot->datanames, i, L_NOCOPY);
        if ((fp = fopenWriteStream(dataname, "w")) == NULL)
            return ERROR_INT("datafile stream not opened", procName, 1);
        fwrite(plotdata, 1, strlen(plotdata), fp);
        fclose(fp);
    }

    return 0;
}


/*-----------------------------------------------------------------*
 *                       Quick and Dirty Plots                     *
 *-----------------------------------------------------------------*/
/*!
 *  gplotSimple1()
 *
 *      Input:  na (numa; plot Y_VS_I)
 *              outformat (GPLOT_PNG, GPLOT_PS, GPLOT_EPS, GPLOT_X11,
 *                         GPLOT_LATEX)
 *              outroot (root of output files)
 *              title  (<optional>, can be NULL)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This gives a line plot of a numa, where the array value
 *          is plotted vs the array index.  The plot is generated
 *          in the specified output format; the title  is optional.
 *      (2) When calling this function more than once, be sure the
 *          outroot strings are different; otherwise, you will
 *          overwrite the output files.
 */
l_int32
gplotSimple1(NUMA        *na,
             l_int32      outformat,
             const char  *outroot,
             const char  *title)
{
GPLOT  *gplot;

    PROCNAME("gplotSimple1");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if (outformat != GPLOT_PNG && outformat != GPLOT_PS &&
        outformat != GPLOT_EPS && outformat != GPLOT_X11 &&
        outformat != GPLOT_LATEX)
        return ERROR_INT("invalid outformat", procName, 1);
    if (!outroot)
        return ERROR_INT("outroot not specified", procName, 1);

    if ((gplot = gplotCreate(outroot, outformat, title, NULL, NULL)) == 0)
        return ERROR_INT("gplot not made", procName, 1);
    gplotAddPlot(gplot, NULL, na, GPLOT_LINES, NULL);
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    return 0;
}


/*!
 *  gplotSimple2()
 *
 *      Input:  na1 (numa; we plot Y_VS_I)
 *              na2 (ditto)
 *              outformat (GPLOT_PNG, GPLOT_PS, GPLOT_EPS, GPLOT_X11,
 *                         GPLOT_LATEX)
 *              outroot (root of output files)
 *              title  (<optional>)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This gives a line plot of two numa, where the array values
 *          are each plotted vs the array index.  The plot is generated
 *          in the specified output format; the title  is optional.
 *      (2) When calling this function more than once, be sure the
 *          outroot strings are different; otherwise, you will
 *          overwrite the output files.
 */
l_int32
gplotSimple2(NUMA        *na1,
             NUMA        *na2,
             l_int32      outformat,
             const char  *outroot,
             const char  *title)
{
GPLOT  *gplot;

    PROCNAME("gplotSimple2");

    if (!na1 || !na2)
        return ERROR_INT("na1 and na2 not both defined", procName, 1);
    if (outformat != GPLOT_PNG && outformat != GPLOT_PS &&
        outformat != GPLOT_EPS && outformat != GPLOT_X11 &&
        outformat != GPLOT_LATEX)
        return ERROR_INT("invalid outformat", procName, 1);
    if (!outroot)
        return ERROR_INT("outroot not specified", procName, 1);

    if ((gplot = gplotCreate(outroot, outformat, title, NULL, NULL)) == 0)
        return ERROR_INT("gplot not made", procName, 1);
    gplotAddPlot(gplot, NULL, na1, GPLOT_LINES, NULL);
    gplotAddPlot(gplot, NULL, na2, GPLOT_LINES, NULL);
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    return 0;
}


/*!
 *  gplotSimpleN()
 *
 *      Input:  naa (numaa; we plot Y_VS_I for each numa)
 *              outformat (GPLOT_PNG, GPLOT_PS, GPLOT_EPS, GPLOT_X11,
 *                         GPLOT_LATEX)
 *              outroot (root of output files)
 *              title (<optional>)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This gives a line plot of all numas in a numaa (array of numa),
 *          where the array values are each plotted vs the array index.
 *          The plot is generated in the specified output format;
 *          the title  is optional.
 *      (2) When calling this function more than once, be sure the
 *          outroot strings are different; otherwise, you will
 *          overwrite the output files.
 */
l_int32
gplotSimpleN(NUMAA       *naa,
             l_int32      outformat,
             const char  *outroot,
             const char  *title)
{
l_int32  i, n;
GPLOT   *gplot;
NUMA    *na;

    PROCNAME("gplotSimpleN");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);
    if ((n = numaaGetCount(naa)) == 0)
        return ERROR_INT("no numa in array", procName, 1);
    if (outformat != GPLOT_PNG && outformat != GPLOT_PS &&
        outformat != GPLOT_EPS && outformat != GPLOT_X11 &&
        outformat != GPLOT_LATEX)
        return ERROR_INT("invalid outformat", procName, 1);
    if (!outroot)
        return ERROR_INT("outroot not specified", procName, 1);

    if ((gplot = gplotCreate(outroot, outformat, title, NULL, NULL)) == 0)
        return ERROR_INT("gplot not made", procName, 1);
    for (i = 0; i < n; i++) {
        na = numaaGetNuma(naa, i, L_CLONE);
        gplotAddPlot(gplot, NULL, na, GPLOT_LINES, NULL);
        numaDestroy(&na);
    }
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    return 0;
}


/*-----------------------------------------------------------------*
 *                           Serialize for I/O                     *
 *-----------------------------------------------------------------*/
/*!
 *  gplotRead()
 *
 *      Input:  filename
 *      Return: gplot, or NULL on error
 */
GPLOT *
gplotRead(const char  *filename)
{
char     buf[L_BUF_SIZE];
char    *rootname, *title, *xlabel, *ylabel, *ignores;
l_int32  outformat, ret, version, ignore;
FILE    *fp;
GPLOT   *gplot;

    PROCNAME("gplotRead");

    if (!filename)
        return (GPLOT *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (GPLOT *)ERROR_PTR("stream not opened", procName, NULL);

    ret = fscanf(fp, "Gplot Version %d\n", &version);
    if (ret != 1) {
        fclose(fp);
        return (GPLOT *)ERROR_PTR("not a gplot file", procName, NULL);
    }
    if (version != GPLOT_VERSION_NUMBER) {
        fclose(fp);
        return (GPLOT *)ERROR_PTR("invalid gplot version", procName, NULL);
    }

    ignore = fscanf(fp, "Rootname: %s\n", buf);
    rootname = stringNew(buf);
    ignore = fscanf(fp, "Output format: %d\n", &outformat);
    ignores = fgets(buf, L_BUF_SIZE, fp);   /* Title: ... */
    title = stringNew(buf + 7);
    title[strlen(title) - 1] = '\0';
    ignores = fgets(buf, L_BUF_SIZE, fp);   /* X axis label: ... */
    xlabel = stringNew(buf + 14);
    xlabel[strlen(xlabel) - 1] = '\0';
    ignores = fgets(buf, L_BUF_SIZE, fp);   /* Y axis label: ... */
    ylabel = stringNew(buf + 14);
    ylabel[strlen(ylabel) - 1] = '\0';

    if (!(gplot = gplotCreate(rootname, outformat, title, xlabel, ylabel))) {
        fclose(fp);
        return (GPLOT *)ERROR_PTR("gplot not made", procName, NULL);
    }
    FREE(rootname);
    FREE(title);
    FREE(xlabel);
    FREE(ylabel);
    sarrayDestroy(&gplot->cmddata);
    sarrayDestroy(&gplot->datanames);
    sarrayDestroy(&gplot->plotdata);
    sarrayDestroy(&gplot->plottitles);
    numaDestroy(&gplot->plotstyles);

    ignore = fscanf(fp, "Commandfile name: %s\n", buf);
    stringReplace(&gplot->cmdname, buf);
    ignore = fscanf(fp, "\nCommandfile data:");
    gplot->cmddata = sarrayReadStream(fp);
    ignore = fscanf(fp, "\nDatafile names:");
    gplot->datanames = sarrayReadStream(fp);
    ignore = fscanf(fp, "\nPlot data:");
    gplot->plotdata = sarrayReadStream(fp);
    ignore = fscanf(fp, "\nPlot titles:");
    gplot->plottitles = sarrayReadStream(fp);
    ignore = fscanf(fp, "\nPlot styles:");
    gplot->plotstyles = numaReadStream(fp);

    ignore = fscanf(fp, "Number of plots: %d\n", &gplot->nplots);
    ignore = fscanf(fp, "Output file name: %s\n", buf);
    stringReplace(&gplot->outname, buf);
    ignore = fscanf(fp, "Axis scaling: %d\n", &gplot->scaling);

    fclose(fp);
    return gplot;
}


/*!
 *  gplotWrite()
 *
 *      Input:  filename
 *              gplot
 *      Return: 0 if OK; 1 on error
 */
l_int32
gplotWrite(const char  *filename,
           GPLOT       *gplot)
{
FILE  *fp;

    PROCNAME("gplotWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!gplot)
        return ERROR_INT("gplot not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);

    fprintf(fp, "Gplot Version %d\n", GPLOT_VERSION_NUMBER);
    fprintf(fp, "Rootname: %s\n", gplot->rootname);
    fprintf(fp, "Output format: %d\n", gplot->outformat);
    fprintf(fp, "Title: %s\n", gplot->title);
    fprintf(fp, "X axis label: %s\n", gplot->xlabel);
    fprintf(fp, "Y axis label: %s\n", gplot->ylabel);

    fprintf(fp, "Commandfile name: %s\n", gplot->cmdname);
    fprintf(fp, "\nCommandfile data:");
    sarrayWriteStream(fp, gplot->cmddata);
    fprintf(fp, "\nDatafile names:");
    sarrayWriteStream(fp, gplot->datanames);
    fprintf(fp, "\nPlot data:");
    sarrayWriteStream(fp, gplot->plotdata);
    fprintf(fp, "\nPlot titles:");
    sarrayWriteStream(fp, gplot->plottitles);
    fprintf(fp, "\nPlot styles:");
    numaWriteStream(fp, gplot->plotstyles);

    fprintf(fp, "Number of plots: %d\n", gplot->nplots);
    fprintf(fp, "Output file name: %s\n", gplot->outname);
    fprintf(fp, "Axis scaling: %d\n", gplot->scaling);

    fclose(fp);
    return 0;
}
