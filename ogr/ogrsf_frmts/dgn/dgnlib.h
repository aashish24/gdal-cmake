/******************************************************************************
 * $Id$
 *
 * Project:  Microstation DGN Access Library
 * Purpose:  Definitions of public structures and API of DGN Library.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam (warmerdam@pobox.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.4  2001/01/10 16:11:33  warmerda
 * added lots of documentation, and extents api
 *
 * Revision 1.3  2000/12/28 21:28:43  warmerda
 * added element index support
 *
 * Revision 1.2  2000/12/14 17:10:57  warmerda
 * implemented TCB, Ellipse, TEXT
 *
 * Revision 1.1  2000/11/28 19:03:47  warmerda
 * New
 *
 */

#ifndef _DGNLIB_H_INCLUDED
#define _DGNLIB_H_INCLUDED

#include "cpl_conv.h"

CPL_C_START

#define DGN_DEBUG

/**
 * \file dgnlib.h
 *
 * Definitions of public structures and API of DGN Library.
 */

/**
 * DGN Point structure.
 *
 * Note that the DGNReadElement() function transforms points into "master"
 * coordinate system space when they are in the file in UOR (units of
 * resolution) coordinates. 
 */

typedef struct {
    double x;	/*!< X (normally eastwards) coordinate. */
    double y;   /*!< y (normally northwards) coordinate. */
    double z;   /*!< z, up coordinate.  Zero for 2D objects. */
} DGNPoint;

/**
 * Element summary information.
 *
 * Minimal information kept about each element if an element summary
 * index is built for a file by DGNGetElementIndex().
 */
typedef struct {
    unsigned char	level;   /*!< Element Level: 0-63 */
    unsigned char       type;    /*!< Element type (DGNT_*) */
    unsigned char       stype;   /*!< Structure type (DGNST_*) */
    unsigned char       flags;   /*!< Other flags */
    long                offset;  /*!< Offset within file (private) */
} DGNElementInfo;  

/**
 * Core element structure. 
 *
 * Core information kept about each element that can be read from a DGN
 * file.  This structure is the first component of each specific element 
 * structure (like DGNElemMultiPoint).  Normally the DGNElemCore.stype
 * field would be used to decide what specific structure type to case the
 * DGNElemCore pointer to. 
 *
 */

typedef struct {
#ifdef DGN_DEBUG
    GUInt32     offset;
    GUInt32     size;
#endif

    int         element_id;     /*!< Element number (zero based) */
    int         stype;          /*!< Structure type: (DGNST_*) */
    int		level;		/*!< Element Level: 0-63 */
    int		type;		/*!< Element type (DGNT_) */
    int		complex;	/*!< Is element complex? */

    int		graphic_group;  /*!< Graphic group number */
    int		properties;     /*!< Properties: ORing of DGNP_ flags */
    int         color;          /*!< Color index (0-255) */
    int         weight;         /*!< Line Weight (0-31) */
    int         style;          /*!< Line Style: One of DGNS_* values */

    int		attr_bytes;	/*!< Bytes of attribute data, usually zero. */
    unsigned char *attr_data;   /*!< Raw attribute data */
} DGNElemCore;

/** 
 * Multipoint element 
 *
 * The core.stype code is DGNST_MULTIPOINT.
 *
 * Used for: DGNT_LINE(3), DGNT_LINE_STRING(4), DGNT_SHAPE(6), DGNT_CURVE(11),
 * DGNT_BSPLINE(21)
 */

typedef struct {
  DGNElemCore 	core;

  int		num_vertices;  /*!< Number of vertices in "vertices" */
  DGNPoint      vertices[2];   /*!< Array of two or more vertices */

} DGNElemMultiPoint;    

/** 
 * Ellipse element 
 *
 * The core.stype code is DGNST_ELLIPSE.
 *
 * Used for: DGNT_ELLIPSE(15).
 */

typedef struct {
  DGNElemCore 	core;

  DGNPoint	origin;		/*!< Origin of ellipse */

  double	primary_axis;	/*!< Primary axis length */
  double        secondary_axis; /*!< Secondary axis length */

  double	rotation;       /*!< Counterclockwise rotation in degrees */
  long          quat[4];

} DGNElemEllipse;

/** 
 * Text element 
 *
 * The core.stype code is DGNST_TEXT.
 *
 * NOTE: Currently we are not capturing the "editable fields" information.
 *
 * Used for: DGNT_TEXT(17).
 */

typedef struct {
    DGNElemCore core;
    
    int		font_id;
    int		justification;
    long        length_mult;
    long        height_mult;
    double	rotation;
    DGNPoint	origin;
    char	string[1];
} DGNElemText;

/** 
 * Color table.
 *
 * The core.stype code is DGNST_COLORTABLE.
 *
 * Returned for DGNT_GROUP_DATA(5) elements, with a level number of 
 * DGN_GDL_COLOR_TABLE(1).
 */

typedef struct {
  DGNElemCore 	core;

  int           screen_flag;
  GByte         color_info[256][3]; /*!< Color table, 256 colors by red (0), green(1) and blue(2) component. */
} DGNElemColorTable;

/** 
 * Terminal Control Block (header).
 *
 * The core.stype code is DGNST_TCB.
 *
 * Returned for DGNT_TCB(9).
 *
 * The first TCB in the file is used to determine the dimension (2D vs. 3D),
 * and transformation from UOR (units of resolution) to subunits, and subunits
 * to master units.  This is handled transparently within DGNReadElement(), so
 * it is not normally necessary to handle this element type at the application
 * level, though it can be useful to get the sub_units, and master_units names.
 */

typedef struct {
    DGNElemCore core;

    int		dimension;         /*!< Dimension (2 or 3) */

    double	origin_x;       /*!< X origin of UOR space in master units(?)*/
    double      origin_y;       /*!< Y origin of UOR space in master units(?)*/
    double      origin_z;       /*!< Z origin of UOR space in master units(?)*/
    
    long	uor_per_subunit;   /*!< UOR per subunit. */
    char	sub_units[3];      /*!< User name for subunits (2 chars)*/
    long        subunits_per_master; /*!< Subunits per master unit. */
    char        master_units[3];   /*!< User name for master units (2 chars)*/

} DGNElemTCB;

/* -------------------------------------------------------------------- */
/*      Structure types                                                 */
/* -------------------------------------------------------------------- */

/** DGNElemCore style: Element uses DGNElemCore structure */
#define DGNST_CORE		   1 

/** DGNElemCore style: Element uses DGNElemMultiPoint structure */
#define DGNST_MULTIPOINT	   2 

/** DGNElemCore style: Element uses DGNElemColorTable structure */
#define DGNST_COLORTABLE           3 

/** DGNElemCore style: Element uses DGNElemTCB structure */
#define DGNST_TCB                  4 

/** DGNElemCore style: Element uses DGNElemEllipse structure */
#define DGNST_ELLIPSE              5 

/** DGNElemCore style: Element uses DGNElemText structure */
#define DGNST_TEXT                 6 

/* -------------------------------------------------------------------- */
/*      Element types                                                   */
/* -------------------------------------------------------------------- */
#define DGNT_CELL_LIBRARY	   1
#define DGNT_CELL_HEADER	   2
#define DGNT_LINE		   3
#define DGNT_LINE_STRING	   4
#define DGNT_GROUP_DATA            5
#define DGNT_SHAPE		   6
#define DGNT_TEXT_NODE             7
#define DGNT_DIGITIZER_SETUP       8
#define DGNT_TCB                   9
#define DGNT_LEVEL_SYMBOLOGY      10
#define DGNT_CURVE		  11
#define DGNT_COMPLEX_CHAIN_HEADER 12
#define DGNT_COMPLEX_SHAPE_HEADER 14
#define DGNT_ELLIPSE              15
#define DGNT_ARC                  16
#define DGNT_TEXT                 17
#define DGNT_BSPLINE              21
#define DGNT_APPLICATION_ELEM     66

/* -------------------------------------------------------------------- */
/*      Line Styles                                                     */
/* -------------------------------------------------------------------- */
#define DGNS_SOLID		0
#define DGNS_DOTTED		1
#define DGNS_MEDIUM_DASH	2
#define DGNS_LONG_DASH		3
#define DGNS_DOT_DASH		4
#define DGNS_SHORT_DASH		5
#define DGNS_DASH_DOUBLE_DOT    6
#define DGNS_LONG_DASH_SHORT_DASH 7

/* -------------------------------------------------------------------- */
/*      Group Data level numbers.                                       */
/*                                                                      */
/*      These are symbolic values for the typ 5 (DGNT_GROUP_DATA)       */
/*      level values that have special meanings.                        */
/* -------------------------------------------------------------------- */
#define DGN_GDL_COLOR_TABLE     1
#define DGN_GDL_NAMED_VIEW      3
#define DGN_GDL_REF_FILE        9

/* -------------------------------------------------------------------- */
/*      Word 17 property flags.                                         */
/* -------------------------------------------------------------------- */
#define DGNPF_HOLE	   0x8000
#define DGNPF_SNAPPABLE    0x4000
#define DGNPF_PLANAR       0x2000
#define DGNPF_ORIENTATION  0x1000
#define DGNPF_ATTRIBUTES   0x0800
#define DGNPF_MODIFIED     0x0400
#define DGNPF_NEW          0x0200
#define DGNPF_LOCKED       0x0100

/* -------------------------------------------------------------------- */
/*      API                                                             */
/* -------------------------------------------------------------------- */
/** Opaque handle representing DGN file, used with DGN API. */
typedef void *DGNHandle;

DGNHandle CPL_DLL    DGNOpen( const char * );
const DGNElementInfo CPL_DLL *DGNGetElementIndex( DGNHandle, int * );
int CPL_DLL          DGNGetExtents( DGNHandle, double * );
DGNElemCore CPL_DLL *DGNReadElement( DGNHandle );
void CPL_DLL         DGNFreeElement( DGNHandle, DGNElemCore * );
void CPL_DLL         DGNRewind( DGNHandle );
int  CPL_DLL         DGNGotoElement( DGNHandle, int );
void CPL_DLL         DGNClose( DGNHandle );

void CPL_DLL         DGNDumpElement( DGNHandle, DGNElemCore *, FILE * );
const char CPL_DLL  *DGNTypeToName( int );


CPL_C_END

#endif /* ndef _DGNLIB_H_INCLUDED */



