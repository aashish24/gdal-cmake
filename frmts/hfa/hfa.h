/******************************************************************************
 * $Id$
 *
 * Project:  Erdas Imagine (.img) Translator
 * Purpose:  Public (C callable) interface for the Erdas Imagine reading
 *           code.  This include files, and it's implementing code depends
 *           on CPL, but not GDAL. 
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Intergraph Corporation
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
 * Revision 1.5  2000/10/12 19:30:31  warmerda
 * substantially improved write support
 *
 * Revision 1.4  2000/09/29 21:42:38  warmerda
 * preliminary write support implemented
 *
 * Revision 1.3  1999/01/22 17:39:09  warmerda
 * Added projections structures, and various other calls
 *
 * Revision 1.2  1999/01/04 22:52:47  warmerda
 * field access working
 *
 * Revision 1.1  1999/01/04 05:28:13  warmerda
 * New
 *
 */

#ifndef _HFAOPEN_H_INCLUDED
#define _HFAOPEN_H_INCLUDED

/* -------------------------------------------------------------------- */
/*      Include standard portability stuff.                             */
/* -------------------------------------------------------------------- */
#include "cpl_conv.h"
#include "cpl_string.h"

#ifdef HFA_PRIVATE
typedef HFAInfo_t *HFAHandle;
#else
typedef void *HFAHandle;
#endif

/* -------------------------------------------------------------------- */
/*      Structure definitions from eprj.h, with some type               */
/*      simplifications.                                                */
/* -------------------------------------------------------------------- */
typedef struct {
	double x;			/* coordinate x-value */
	double y;			/* coordinate y-value */
} Eprj_Coordinate;


typedef struct {
	double width;			/* pixelsize width */
	double height;			/* pixelsize height */
} Eprj_Size;


typedef struct {
	char * proName;		/* projection name */
	Eprj_Coordinate upperLeftCenter;	/* map coordinates of center of
						   upper left pixel */
	Eprj_Coordinate lowerRightCenter;	/* map coordinates of center of
						   lower right pixel */
	Eprj_Size pixelSize;			/* pixel size in map units */
	char * units;			/* units of the map */
} Eprj_MapInfo;

typedef enum {
	EPRJ_INTERNAL,		/* Indicates that the projection is built into
				   the eprj package as function calls */
	EPRJ_EXTERNAL		/* Indicates that the projection is accessible
				   as an EXTERNal executable */
} Eprj_ProType;

typedef enum {
	EPRJ_NAD27=1,		/* Use the North America Datum 1927 */
	EPRJ_NAD83=2,		/* Use the North America Datum 1983 */
	EPRJ_HARN		/* Use the North America Datum High Accuracy
				   Reference Network */
} Eprj_NAD;

typedef enum {
	EPRJ_DATUM_PARAMETRIC,		/* The datum info is 7 doubles */
	EPRJ_DATUM_GRID,		/* The datum info is a name */
	EPRJ_DATUM_REGRESSION,
	EPRJ_DATUM_NONE
} Eprj_DatumType;

typedef struct {
	char * datumname;		/* name of the datum */
	Eprj_DatumType type;			/* The datum type */
	double  params[7];		/* The parameters for type
						   EPRJ_DATUM_PARAMETRIC */
	char * gridname;		/* name of the grid file */
} Eprj_Datum;

typedef struct {
	char * sphereName;	/* name of the ellipsoid */
	double a;			/* semi-major axis of ellipsoid */
	double b;			/* semi-minor axis of ellipsoid */
	double eSquared;		/* eccentricity-squared */
	double radius;			/* radius of the sphere */
} Eprj_Spheroid;

typedef struct {
	Eprj_ProType proType;		/* projection type */
	long proNumber;			/* projection number for internal 
					   projections */
	char * proExeName;	/* projection executable name for
					   EXTERNal projections */
	char * proName;	/* projection name */
	long proZone;			/* projection zone (UTM, SP only) */
	double proParams[15];	/* projection parameters array in the
					   GCTP form */
	Eprj_Spheroid proSpheroid;	/* projection spheroid */
} Eprj_ProParameters;

/* -------------------------------------------------------------------- */
/*      Prototypes                                                      */
/* -------------------------------------------------------------------- */

CPL_C_START

HFAHandle HFAOpen( const char * pszFilename, const char * pszMode );
void	HFAClose( HFAHandle );

HFAHandle HFACreateLL( const char *pszFilename );
HFAHandle HFACreate( const char *pszFilename, int nXSize, int nYSize, 
                     int nBands, int nDataType, char ** papszOptions );
CPLErr  HFAFlush( HFAHandle );

const Eprj_MapInfo *HFAGetMapInfo( HFAHandle );
CPLErr HFASetMapInfo( HFAHandle, const Eprj_MapInfo * );
const Eprj_Datum *HFAGetDatum( HFAHandle );
CPLErr HFASetDatum( HFAHandle, const Eprj_Datum * );
const Eprj_ProParameters *HFAGetProParameters( HFAHandle );
CPLErr HFASetProParameters( HFAHandle, const Eprj_ProParameters * );

CPLErr	HFAGetRasterInfo( HFAHandle hHFA, int *pnXSize, int *pnYSize,
                          int *pnBands );
CPLErr  HFAGetBandInfo( HFAHandle hHFA, int nBand, int * pnDataType,
                        int * pnBlockXSize, int * pnBlockYSize );
CPLErr  HFAGetRasterBlock( HFAHandle hHFA, int nBand, int nXBlock, int nYBlock,
                           void * pData );
CPLErr  HFASetRasterBlock( HFAHandle hHFA, int nBand, int nXBlock, int nYBlock,
                           void * pData );
int     HFAGetDataTypeBits( int );
CPLErr	HFAGetPCT( HFAHandle, int, int *, double **, double **, double ** );
CPLErr  HFASetPCT( HFAHandle, int, int, double *, double *, double * );
void    HFADumpTree( HFAHandle, FILE * );
void    HFADumpDictionary( HFAHandle, FILE * );
CPLErr  HFAGetDataRange( HFAHandle, int, double *, double * );

/* -------------------------------------------------------------------- */
/*      data types.                                                     */
/* -------------------------------------------------------------------- */
#define EPT_u1	0
#define EPT_u2	1
#define EPT_u4	2
#define EPT_u8	3
#define EPT_s8	4
#define EPT_u16	5
#define EPT_s16	6
#define EPT_u32	7
#define EPT_s32	8
#define EPT_f32	9
#define EPT_f64	10
#define EPT_c64	11
#define EPT_c128 12

/* -------------------------------------------------------------------- */
/*      Projection codes.                                               */
/* -------------------------------------------------------------------- */
#define EPRJ_LATLONG				0
#define EPRJ_UTM				1
#define EPRJ_STATE_PLANE 			2
#define EPRJ_ALBERS_CONIC_EQUAL_AREA		3
#define EPRJ_LAMBERT_CONFORMAL_CONIC	        4
#define EPRJ_MERCATOR                           5
#define EPRJ_POLAR_STEREOGRAPHIC                6
#define EPRJ_POLYCONIC                          7
#define EPRJ_EQUIDISTANT_CONIC                  8
#define EPRJ_TRANSVERSE_MERCATOR                9
#define EPRJ_STEREOGRAPHIC                      10
#define EPRJ_LAMBERT_AZIMUTHAL_EQUAL_AREA       11
#define EPRJ_AZIMUTHAL_EQUIDISTANT              12
#define EPRJ_GNOMONIC                           13
#define EPRJ_ORTHOGRAPHIC                       14
#define EPRJ_GENERAL_VERTICAL_NEAR_SIDE_PERSPECTIVE 15
#define EPRJ_SINUSOIDAL                         16
#define EPRJ_EQUIRECTANGULAR                    17
#define EPRJ_MILLER_CYLINDRICAL                 18
#define EPRJ_VANDERGRINTEN                      19
#define EPRJ_HOTINE_OBLIQUE_MERCATOR            20
#define EPRJ_SPACE_OBLIQUE_MERCATOR             21
#define EPRJ_MODIFIED_TRANSVERSE_MERCATOR       22

CPL_C_END

#endif /* ndef _HFAOPEN_H_INCLUDED */
