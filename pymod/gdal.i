/******************************************************************************
 * $Id$
 *
 * Name:     gdal.i
 * Project:  GDAL Core
 * Purpose:  GDAL Core SWIG Interface declarations.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
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
 * Revision 1.1  2000/03/06 02:24:48  warmerda
 * New
 *
 */

%module _gdal

%{
#include "gdal.h"
%}

/*! Pixel data types */
typedef enum {
    GDT_Unknown = 0,
    /*! Eight bit unsigned integer */ 		GDT_Byte = 1,
    /*! Sixteen bit unsigned integer */         GDT_UInt16 = 2,
    /*! Sixteen bit signed integer */           GDT_Int16 = 3,
    /*! Thirty two bit unsigned integer */      GDT_UInt32 = 4,
    /*! Thirty two bit signed integer */        GDT_Int32 = 5,
    /*! Thirty two bit floating point */        GDT_Float32 = 6,
    /*! Sixty four bit floating point */        GDT_Float64 = 7,
    GDT_TypeCount = 8		/* maximum type # + 1 */
} GDALDataType;

int  GDALGetDataTypeSize( GDALDataType );
const char * GDALGetDataTypeName( GDALDataType );

/*! Flag indicating read/write, or read-only access to data. */
typedef enum {
    /*! Read only (no update) access */ GA_ReadOnly = 0,
    /*! Read/write access. */           GA_Update = 1
} GDALAccess;

typedef enum {
    GF_Read = 0,
    GF_Write = 1
} GDALRWFlag;

/*! Types of color interpretation for raster bands. */
typedef enum
{
    GCI_Undefined=0,
    GCI_GrayIndex=1,
    GCI_PaletteIndex=2,
    GCI_RedBand=3,
    GCI_GreenBand=4,
    GCI_BlueBand=5,
    GCI_AlphaBand=6,
    GCI_HueBand=7,
    GCI_SaturationBand=8,
    GCI_LightnessBand=9,
    GCI_CyanBand=10,
    GCI_MagentaBand=11,
    GCI_YellowBand=12,
    GCI_BlackBand=13
} GDALColorInterp;

/*! Translate a GDALColorInterp into a user displayable string. */
const char *GDALGetColorInterpretationName( GDALColorInterp );

/*! Types of color interpretations for a GDALColorTable. */
typedef enum 
{
    GPI_Gray=0,
    GPI_RGB=1,
    GPI_CMYK=2,
    GPI_HLS=3
} GDALPaletteInterp;

/*! Translate a GDALPaletteInterp into a user displayable string. */
const char *GDALGetPaletteInterpretationName( GDALPaletteInterp );

/* -------------------------------------------------------------------- */
/*      Define handle types related to various internal classes.        */
/* -------------------------------------------------------------------- */

typedef void *GDALMajorObjectH;
typedef void *GDALDatasetH;
typedef void *GDALRasterBandH;
typedef void *GDALDriverH;
typedef void *GDALProjDefH;
typedef void *GDALColorTableH;

/* ==================================================================== */
/*      Registration/driver related.                                    */
/* ==================================================================== */

void GDALAllRegister( void );

GDALDatasetH  GDALCreate( GDALDriverH hDriver,
                                 const char *, int, int, int, GDALDataType,
                                 char ** );
GDALDatasetH  GDALOpen( const char *, GDALAccess );

GDALDriverH  GDALGetDriverByName( const char * );
int          GDALGetDriverCount();
GDALDriverH  GDALGetDriver( int );
int          GDALRegisterDriver( GDALDriverH );
void         GDALDeregisterDriver( GDALDriverH );
CPLErr	     GDALDeleteDataset( GDALDriverH, const char * );

const char  *GDALGetDriverShortName( GDALDriverH );
const char  *GDALGetDriverLongName( GDALDriverH );
const char  *GDALGetDriverHelpTopic( GDALDriverH );

/* ==================================================================== */
/*      GDALDataset class ... normally this represents one file.        */
/* ==================================================================== */

GDALDriverH  GDALGetDatasetDriver( GDALDatasetH );
void    GDALClose( GDALDatasetH );
int 	GDALGetRasterXSize( GDALDatasetH );
int 	GDALGetRasterYSize( GDALDatasetH );
int 	GDALGetRasterCount( GDALDatasetH );
GDALRasterBandH  GDALGetRasterBand( GDALDatasetH, int );
const char  *GDALGetProjectionRef( GDALDatasetH );
CPLErr   GDALSetProjection( GDALDatasetH, const char * );
CPLErr   GDALGetGeoTransform( GDALDatasetH, double * );
CPLErr   GDALSetGeoTransform( GDALDatasetH, double * );
void    *GDALGetInternalHandle( GDALDatasetH, const char * );
int      GDALReferenceDataset( GDALDatasetH );
int      GDALDereferenceDataset( GDALDatasetH );

/* ==================================================================== */
/*      GDALRasterBand ... one band/channel in a dataset.               */
/* ==================================================================== */

GDALDataType  GDALGetRasterDataType( GDALRasterBandH );
void 	GDALGetBlockSize( GDALRasterBandH,
                                  int * pnXSize, int * pnYSize );

CPLErr  GDALRasterIO( GDALRasterBandH hRBand, GDALRWFlag eRWFlag,
                              int nDSXOff, int nDSYOff,
                              int nDSXSize, int nDSYSize,
                              void * pBuffer, int nBXSize, int nBYSize,
                              GDALDataType eBDataType,
                              int nPixelSpace, int nLineSpace );
CPLErr  GDALReadBlock( GDALRasterBandH, int, int, void * );
CPLErr  GDALWriteBlock( GDALRasterBandH, int, int, void * );
int  GDALGetRasterBandXSize( GDALRasterBandH );
int  GDALGetRasterBandYSize( GDALRasterBandH );

GDALColorInterp  GDALGetRasterColorInterpretation( GDALRasterBandH );
GDALColorTableH  GDALGetRasterColorTable( GDALRasterBandH );
int              GDALGetOverviewCount( GDALRasterBandH );
GDALRasterBandH  GDALGetOverview( GDALRasterBandH, int );

/* need to add functions related to block cache */

/* helper functions */
void  GDALSwapWords( void *pData, int nWordSize, int nWordCount,
                            int nWordSkip );
void 
    GDALCopyWords( void * pSrcData, GDALDataType eSrcType, int nSrcPixelOffset,
                   void * pDstData, GDALDataType eDstType, int nDstPixelOffset,
                   int nWordCount );


/* ==================================================================== */
/*      Color tables.                                                   */
/* ==================================================================== */
typedef struct
{
    short      c1;      /* gray, red, cyan or hue */
    short      c2;      /* green, magenta, or lightness */
    short      c3;      /* blue, yellow, or saturation */
    short      c4;      /* alpha or blackband */
} GDALColorEntry;

GDALColorTableH  GDALCreateColorTable( GDALPaletteInterp );
void             GDALDestroyColorTable( GDALColorTableH );
GDALColorTableH  GDALCloneColorTable( GDALColorTableH );
GDALPaletteInterp  GDALGetPaletteInterpretation( GDALColorTableH );
int              GDALGetColorEntryCount( GDALColorTableH );
const GDALColorEntry  *GDALGetColorEntry( GDALColorTableH, int );
int  GDALGetColorEntryAsRGB( GDALColorTableH, int, GDALColorEntry *);
void  GDALSetColorEntry( GDALColorTableH, int, const GDALColorEntry * );

/* ==================================================================== */
/*      Projections                                                     */
/* ==================================================================== */

GDALProjDefH  GDALCreateProjDef( const char * );
CPLErr 	 GDALReprojectToLongLat( GDALProjDefH, double *, double * );
CPLErr 	 GDALReprojectFromLongLat( GDALProjDefH, double *, double * );
void     GDALDestroyProjDef( GDALProjDefH );
const char *GDALDecToDMS( double, const char *, int );

/* ==================================================================== */
/*      Special custom functions.                                       */
/* ==================================================================== */

%{
static PyObject *
py_GDALReadRaster(PyObject *self, PyObject *args) {

    PyObject *result;
    GDALRasterBandH  _arg0;
    char *_argc0 = NULL;
    int  _arg1;
    int  _arg2;
    int  _arg3;
    int  _arg4;
    int  _arg5;
    int  _arg6;
    GDALDataType  _arg7;
    char *result_string;
    int  result_size;

    self = self;
    if(!PyArg_ParseTuple(args,"siiiiiii:GDALReadRaster",
                         &_argc0,&_arg1,&_arg2,&_arg3,&_arg4,
                         &_arg5,&_arg6,&_arg7)) 
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,(char *) 0 )) {
            PyErr_SetString(PyExc_TypeError,"Type error in argument 1 of GDALRasterIO. Expected _GDALRasterBandH.");
        return NULL;
        }
    }
	
    result_size = _arg5 * _arg6 * (GDALGetDataTypeSize(_arg7)/8);
    result_string = (char *) malloc(result_size+1);

    if( GDALRasterIO(_arg0, GF_Read, _arg1, _arg2, _arg3, _arg4, 
		     (void *) result_string, 
		     _arg5, _arg6, _arg7, 0, 0 ) != CE_None )
    {
	free( result_string );
	PyErr_SetString(PyExc_TypeError,CPLGetLastErrorMsg());
	return NULL;
    }

    result = PyString_FromStringAndSize(result_string,result_size);

    free( result_string );

    return result;
}

%}

%native(GDALReadRaster) py_GDALReadRaster;
