/******************************************************************************
 * $Id$
 *
 * Name:     gdal.i
 * Project:  GDAL Python Interface
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
 * Revision 1.13  2000/07/09 20:56:38  warmerda
 * added exportToPrettyWkt
 *
 * Revision 1.12  2000/06/27 16:48:57  warmerda
 * added progress func support
 *
 * Revision 1.11  2000/06/26 19:54:23  warmerda
 * added options support in GDALCreateCopy
 *
 * Revision 1.10  2000/06/26 18:46:31  warmerda
 * Added Dataset.BuildOverviews
 *
 * Revision 1.9  2000/06/26 17:58:15  warmerda
 * added driver, createcopy support
 *
 * Revision 1.8  2000/06/13 18:14:19  warmerda
 * added control of the gdal raster cache
 *
 * Revision 1.7  2000/04/21 22:05:56  warmerda
 * updated metadata support
 *
 * Revision 1.6  2000/03/31 14:25:28  warmerda
 * added metadata and gcp support
 *
 * Revision 1.5  2000/03/22 01:10:42  warmerda
 * added OSR and OCT wrappers for coordinate systems
 *
 * Revision 1.4  2000/03/10 13:55:33  warmerda
 * removed constants, added gethistogram
 *
 * Revision 1.3  2000/03/08 20:01:04  warmerda
 * added geotransforms
 *
 * Revision 1.2  2000/03/06 03:30:51  warmerda
 * Added geotransform stuff
 *
 * Revision 1.1  2000/03/06 02:24:48  warmerda
 * New
 *
 */

%module _gdal

%{
#include "gdal.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_srs_api.h"
%}

typedef int GDALDataType;
typedef int GDALAccess;
typedef int GDALRWFlag;
typedef int GDALColorInterp;
typedef int GDALPaletteInterp;

int  GDALGetDataTypeSize( GDALDataType );
const char * GDALGetDataTypeName( GDALDataType );

/*! Translate a GDALColorInterp into a user displayable string. */
const char *GDALGetColorInterpretationName( GDALColorInterp );

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
int      GDALReferenceDataset( GDALDatasetH );
int      GDALDereferenceDataset( GDALDatasetH );
int      GDALGetGCPCount( GDALDatasetH );
const char *GDALGetGCPProjection( GDALDatasetH );

/* ==================================================================== */
/*      GDALRasterBand ... one band/channel in a dataset.               */
/* ==================================================================== */

GDALDataType  GDALGetRasterDataType( GDALRasterBandH );
void 	GDALGetBlockSize( GDALRasterBandH,
	                  int * pnXSize, int * pnYSize );

int  GDALGetRasterBandXSize( GDALRasterBandH );
int  GDALGetRasterBandYSize( GDALRasterBandH );

GDALColorInterp  GDALGetRasterColorInterpretation( GDALRasterBandH );
GDALColorTableH  GDALGetRasterColorTable( GDALRasterBandH );
int              GDALGetOverviewCount( GDALRasterBandH );
GDALRasterBandH  GDALGetOverview( GDALRasterBandH, int );
CPLErr           GDALFlushRasterCache( GDALRasterBandH );

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
/*      GDAL Cache Management                                           */
/* ==================================================================== */

void  GDALSetCacheMax( int nBytes );
int   GDALGetCacheMax();
int   GDALGetCacheUsed();
int   GDALFlushCacheBlock();

/* ==================================================================== */
/*	Support function for progress callbacks to python.              */
/* ==================================================================== */

%{

typedef struct {
    PyObject *psPyCallback;
    PyObject *psPyCallbackData;
    int nLastReported;
} PyProgressData;

/************************************************************************/
/*                          PyProgressProxy()                           */
/************************************************************************/

int PyProgressProxy( double dfComplete, const char *pszMessage, void *pData )

{
    PyProgressData *psInfo = (PyProgressData *) pData;
    PyObject *psArgs, *psResult;
    int      bContinue = TRUE;

    if( psInfo->nLastReported == (int) (100.0 * dfComplete) )
        return TRUE;

    if( psInfo->psPyCallback == NULL || psInfo->psPyCallback == Py_None )
        return TRUE;

    psInfo->nLastReported = (int) 100.0 * dfComplete;
    
    if( pszMessage == NULL )
        pszMessage = "";

    if( psInfo->psPyCallbackData == NULL )
        psArgs = Py_BuildValue("(dsO)", dfComplete, pszMessage, Py_None );
    else
        psArgs = Py_BuildValue("(dsO)", dfComplete, pszMessage, 
	                       psInfo->psPyCallbackData );

    psResult = PyEval_CallObject( psInfo->psPyCallback, psArgs);
    Py_XDECREF(psArgs);

    if( psResult == NULL )
    {
        return TRUE;
    }

    if( psResult == Py_None )
    {
	Py_XDECREF(Py_None);
        return TRUE;
    }

    if( !PyArg_Parse( psResult, "i", &bContinue ) )
    {
        PyErr_SetString(PyExc_ValueError, "bad progress return value");
	return FALSE;
    }

    Py_XDECREF(psResult);

    return bContinue;    
}

%}

/* ==================================================================== */
/*      Special custom functions.                                       */
/* ==================================================================== */

%{
/************************************************************************/
/*                           GDALBuildOverviews()                       */
/************************************************************************/
static PyObject *
py_GDALBuildOverviews(PyObject *self, PyObject *args) {

    char *pszSwigDS = NULL;
    GDALDatasetH hDS = NULL;   
    char *pszResampling = "NEAREST";
    PyObject *psPyOverviewList = NULL, *psPyBandList = NULL;
    int   nOverviews, *panOverviewList, i;
    CPLErr eErr;
    PyProgressData sProgressInfo;

    self = self;
    sProgressInfo.psPyCallback = NULL;
    sProgressInfo.psPyCallbackData = NULL;
    if(!PyArg_ParseTuple(args,"ssO!O!|OO:GDALBuildOverviews",	
			 &pszSwigDS, &pszResampling, 
		         &PyList_Type, &psPyOverviewList, 
			 &PyList_Type, &psPyBandList,
                         &(sProgressInfo.psPyCallback), 
		         &(sProgressInfo.psPyCallbackData) ) )
        return NULL;

    if (SWIG_GetPtr(pszSwigDS,(void **) &hDS, "_GDALDatasetH" )) {
        PyErr_SetString(PyExc_TypeError,
	   	        "Type error in argument 1 of GDALBuildOverviews."
			" Expected _GDALDatasetH.");
        return NULL;
    }

    nOverviews = PyList_Size(psPyOverviewList);
    panOverviewList = (int *) CPLCalloc(sizeof(int),nOverviews);
    for( i = 0; i < nOverviews; i++ )
    {
	if( !PyArg_Parse( PyList_GET_ITEM(psPyOverviewList,i), "i", 
			  panOverviewList+i) )
        {
	    PyErr_SetString(PyExc_ValueError, "bad overview value");
	    return NULL;
        }
    }

    eErr = GDALBuildOverviews( hDS, pszResampling, nOverviews, panOverviewList,
			       0, NULL, PyProgressProxy, &sProgressInfo );

    CPLFree( panOverviewList );

    return Py_BuildValue( "i", eErr );
}

%}

%native(GDALBuildOverviews) py_GDALBuildOverviews;

%{
/************************************************************************/
/*                           GDALCreateCopy()                           */
/************************************************************************/
static PyObject *
py_GDALCreateCopy(PyObject *self, PyObject *args) {

    PyObject *poPyOptions=NULL;
    char *pszSwigDriver=NULL, *pszFilename=NULL, *pszSwigSourceDS=NULL;
    int  bStrict = FALSE;
    GDALDriverH hDriver = NULL;
    GDALDatasetH hSourceDS = NULL, hTargetDS = NULL;   
    char **papszOptions = NULL;
    PyProgressData sProgressInfo;

    self = self;
    sProgressInfo.psPyCallback = NULL;
    sProgressInfo.psPyCallbackData = NULL;
    if(!PyArg_ParseTuple(args,"sss|iO!OO:GDALCreateCopy",	
			 &pszSwigDriver, &pszFilename, &pszSwigSourceDS, 
			 &bStrict, &PyList_Type, &poPyOptions,
			 &(sProgressInfo.psPyCallback), 
		         &(sProgressInfo.psPyCallbackData)) )
        return NULL;

    if (SWIG_GetPtr(pszSwigDriver,(void **) &hDriver, "_GDALDriverH" )) {
        PyErr_SetString(PyExc_TypeError,
	   	        "Type error in argument 1 of GDALCreateCopy."
			" Expected _GDALDriverH.");
        return NULL;
    }
	
    if (SWIG_GetPtr(pszSwigSourceDS,(void **) &hSourceDS, "_GDALDatasetH" )) {
        PyErr_SetString(PyExc_TypeError,
	   	        "Type error in argument 3 of GDALCreateCopy."
			" Expected _GDALDatasetH.");
        return NULL;
    }

    if( poPyOptions != NULL )
    {
        int i;

	for( i = 0; i < PyList_Size(poPyOptions); i++ )
        {
            char *pszItem = NULL;

	    if( !PyArg_Parse(PyList_GET_ITEM(poPyOptions,i), "s", 
			     &pszItem) )
            {
	        PyErr_SetString(PyExc_ValueError, "bad option list item");
	        return NULL;
            }
            papszOptions = CSLAddString( papszOptions, pszItem );
        }
    }

    hTargetDS = GDALCreateCopy( hDriver, pszFilename, hSourceDS, bStrict, 
			        papszOptions, PyProgressProxy, &sProgressInfo);
	
    CSLDestroy( papszOptions );

    if( hTargetDS == NULL )
    {
        Py_INCREF(Py_None);
	return Py_None;
    }
    else
    {
        char  szSwigTarget[48];

	SWIG_MakePtr( szSwigTarget, hTargetDS, "_GDALDatasetH" );	
	return Py_BuildValue( "s", szSwigTarget );
    }
}

%}

%native(GDALCreateCopy) py_GDALCreateCopy;

%{
/************************************************************************/
/*                         GDALReadRaster()                             */
/************************************************************************/
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
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,"_GDALRasterBandH" )) {
            PyErr_SetString(PyExc_TypeError,
			    "Type error in argument 1 of GDALReadRaster."
			    " Expected _GDALRasterBandH.");
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

%{
/************************************************************************/
/*                          GDALWriteRaster()                           */
/************************************************************************/
static PyObject *
py_GDALWriteRaster(PyObject *self, PyObject *args) {

    GDALRasterBandH  _arg0;
    char *_argc0 = NULL;
    int  _arg1;
    int  _arg2;
    int  _arg3;
    int  _arg4;
    char *strbuffer_arg = NULL;
    int  strbuffer_size;
    int  _arg5;
    int  _arg6;
    GDALDataType  _arg7;

    self = self;
    if(!PyArg_ParseTuple(args,"siiiis#iii:GDALWriteRaster",
                         &_argc0,&_arg1,&_arg2,&_arg3,&_arg4,
                         &strbuffer_arg,&strbuffer_size,&_arg5,&_arg6,&_arg7)) 
        return NULL;



    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,"_GDALRasterBandH" )) {
            PyErr_SetString(PyExc_TypeError,
			    "Type error in argument 1 of GDALWriteRaster."
			    " Expected _GDALRasterBandH.");
            return NULL;
        }
    }
	
    if( GDALRasterIO(_arg0, GF_Write, _arg1, _arg2, _arg3, _arg4, 
		     (void *) strbuffer_arg,
		     _arg5, _arg6, _arg7, 0, 0 ) != CE_None )
    {
	PyErr_SetString(PyExc_TypeError,CPLGetLastErrorMsg());
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

%}

%native(GDALWriteRaster) py_GDALWriteRaster;


%{
/************************************************************************/
/*                            GDALGetGCPs()                             */
/************************************************************************/
static PyObject *
py_GDALGetGCPs(PyObject *self, PyObject *args) {

    GDALDatasetH  _arg0;
    char *_argc0 = NULL;
    const GDAL_GCP * pasGCPList;
    PyObject *psList;
    int iGCP;

    self = self;
    if(!PyArg_ParseTuple(args,"s:GDALGetGCPs",&_argc0))
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,"_GDALDatasetH" )) {
            PyErr_SetString(PyExc_TypeError,
                            "Type error in argument 1 of GDALGetGCPs."
                            "  Expected _GDALDatasetH.");
            return NULL;
        }
    }

    pasGCPList = GDALGetGCPs( _arg0 );	

    psList = PyList_New(GDALGetGCPCount(_arg0));
    for( iGCP = 0; pasGCPList != NULL && iGCP < GDALGetGCPCount(_arg0); iGCP++)
    {
	PyList_SetItem(psList, iGCP, 
                       Py_BuildValue("(ssfffff)", 
                                     pasGCPList[iGCP].pszId,
                                     pasGCPList[iGCP].pszInfo,
                                     pasGCPList[iGCP].dfGCPPixel,
                                     pasGCPList[iGCP].dfGCPLine,
                                     pasGCPList[iGCP].dfGCPX,
                                     pasGCPList[iGCP].dfGCPY,
                                     pasGCPList[iGCP].dfGCPZ ) );
    }

    return psList;
}
%}

%native(GDALGetGCPs) py_GDALGetGCPs;

%{
/************************************************************************/
/*                        GDALGetGeoTransform()                         */
/************************************************************************/
static PyObject *
py_GDALGetGeoTransform(PyObject *self, PyObject *args) {

    GDALDatasetH  _arg0;
    char *_argc0 = NULL;
    double geotransform[6];

    self = self;
    if(!PyArg_ParseTuple(args,"s:GDALGetGeoTransform",&_argc0))
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,"_GDALDatasetH" )) {
            PyErr_SetString(PyExc_TypeError,
                            "Type error in argument 1 of GDALGetGeoTransform."
                            "  Expected _GDALDatasetH.");
            return NULL;
        }
    }
	
    GDALGetGeoTransform(_arg0,geotransform);

    return Py_BuildValue("dddddd",
	                 geotransform[0],
	                 geotransform[1],
	                 geotransform[2],
	                 geotransform[3],
	                 geotransform[4],
	                 geotransform[5] );
}
%}

%native(GDALGetGeoTransform) py_GDALGetGeoTransform;

%{
/************************************************************************/
/*                        GDALSetGeoTransform()                         */
/************************************************************************/
static PyObject *
py_GDALSetGeoTransform(PyObject *self, PyObject *args) {

    GDALDatasetH  _arg0;
    char *_argc0 = NULL;
    double geotransform[6];
    CPLErr err;

    self = self;
    if(!PyArg_ParseTuple(args,"s(dddddd):GDALSetGeoTransform",&_argc0,
	geotransform+0, geotransform+1, geotransform+2, 
	geotransform+3, geotransform+4, geotransform+5) )
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,"_GDALDatasetH" )) {
            PyErr_SetString(PyExc_TypeError,
                            "Type error in argument 1 of GDALSetGeoTransform."
                            "  Expected _GDALDatasetH.");
            return NULL;
        }
    }
	
    err = GDALSetGeoTransform(_arg0,geotransform);

    if( err != CE_None )
    {
	PyErr_SetString(PyExc_TypeError,CPLGetLastErrorMsg());
	return NULL;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}
%}

%native(GDALSetGeoTransform) py_GDALSetGeoTransform;

%{
/************************************************************************/
/*                       GDALGetRasterHistogram()                       */
/************************************************************************/
static PyObject *
py_GDALGetRasterHistogram(PyObject *self, PyObject *args) {

    GDALRasterBandH  hBand;
    char *_argc0 = NULL;
    double dfMin = -0.5, dfMax = 255.5;
    int nBuckets = 256, bIncludeOutOfRange = FALSE, bApproxOK = FALSE;
    int *panHistogram, i;
    PyObject *psList;

    self = self;
    if(!PyArg_ParseTuple(args,"s|ddiii:GDALGetRasterHistogram",&_argc0,
			 &dfMin, &dfMax, &nBuckets, &bIncludeOutOfRange, 
	 	         &bApproxOK))
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &hBand,"_GDALRasterBandH" )) {
            PyErr_SetString(PyExc_TypeError,
                          "Type error in argument 1 of GDALGetRasterHistogram."
                          "  Expected _GDALRasterBandH.");
            return NULL;
        }
    }
	
    panHistogram = (int *) CPLCalloc(sizeof(int),nBuckets);
    GDALGetRasterHistogram(hBand, dfMin, dfMax, nBuckets, panHistogram, 
			   bIncludeOutOfRange, bApproxOK, 
		           GDALDummyProgress, NULL);

    psList = PyList_New(nBuckets);
    for( i = 0; i < nBuckets; i++ )
	PyList_SetItem(psList, i, Py_BuildValue("i", panHistogram[i] ));

    CPLFree( panHistogram );

    return psList;
}
%}

%native(GDALGetRasterHistogram) py_GDALGetRasterHistogram;

%{
/************************************************************************/
/*                       GDALGetMetadata()                              */
/************************************************************************/
static PyObject *
py_GDALGetMetadata(PyObject *self, PyObject *args) {

    GDALMajorObjectH  hObject;
    char *_argc0 = NULL;
    PyObject *psDict;
    int i;
    char **papszMetadata;
    char *pszDomain = NULL;

    self = self;
    if(!PyArg_ParseTuple(args,"s|s:GDALGetMetadata",&_argc0, &pszDomain))
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &hObject,NULL )) {
            PyErr_SetString(PyExc_TypeError,
                          "Type error in argument 1 of GDALGetMetadata."
                          "  Expected _GDALMajorObjectH.");
            return NULL;
        }
    }

    psDict = PyDict_New();
    
    papszMetadata = GDALGetMetadata( hObject, pszDomain );

    for( i = 0; i < CSLCount(papszMetadata); i++ )
    {
	char	*pszKey;
	const char *pszValue;

	pszValue = CPLParseNameValue( papszMetadata[i], &pszKey );
	if( pszValue != NULL )
	    PyDict_SetItem( psDict, 
                            Py_BuildValue("s", pszKey ), 
                            Py_BuildValue("s", pszValue ) );
	CPLFree( pszKey );
    }

    return psDict;
}
%}

%native(GDALGetMetadata) py_GDALGetMetadata;

/* -------------------------------------------------------------------- */
/*      OGRSpatialReference stuff.                                      */
/* -------------------------------------------------------------------- */
typedef void *OGRSpatialReferenceH;                               
typedef void *OGRCoordinateTransformationH;

OGRSpatialReferenceH OSRNewSpatialReference( const char * /* = NULL */);
void    OSRDestroySpatialReference( OGRSpatialReferenceH );

int     OSRReference( OGRSpatialReferenceH );
int     OSRDereference( OGRSpatialReferenceH );

OGRErr  OSRImportFromEPSG( OGRSpatialReferenceH, int );

OGRErr  OSRSetAttrValue( OGRSpatialReferenceH hSRS, const char * pszNodePath,
                         const char * pszNewNodeValue );
const char *OSRGetAttrValue( OGRSpatialReferenceH hSRS,
                             const char * pszName, int iChild /* = 0 */ );

OGRErr  OSRSetLinearUnits( OGRSpatialReferenceH, const char *, double );
double  OSRGetLinearUnits( OGRSpatialReferenceH, char ** );

int     OSRIsGeographic( OGRSpatialReferenceH );
int     OSRIsProjected( OGRSpatialReferenceH );
int     OSRIsSameGeogCS( OGRSpatialReferenceH, OGRSpatialReferenceH );
int     OSRIsSame( OGRSpatialReferenceH, OGRSpatialReferenceH );

OGRErr  OSRSetProjCS( OGRSpatialReferenceH, const char * );
OGRErr  OSRSetWellKnownGeogCS( OGRSpatialReferenceH, const char * );

OGRErr  OSRSetGeogCS( OGRSpatialReferenceH hSRS,
                      const char * pszGeogName,
                      const char * pszDatumName,
                      const char * pszEllipsoidName,
                      double dfSemiMajor, double dfInvFlattening,
                      const char * pszPMName /* = NULL */,
                      double dfPMOffset /* = 0.0 */,
                      const char * pszUnits /* = NULL */,
                      double dfConvertToRadians /* = 0.0 */ );

double  OSRGetSemiMajor( OGRSpatialReferenceH, OGRErr * /* = NULL */ );
double  OSRGetSemiMinor( OGRSpatialReferenceH, OGRErr * /* = NULL */ );
double  OSRGetInvFlattening( OGRSpatialReferenceH, OGRErr * /* = NULL */ );

OGRErr  OSRSetAuthority( OGRSpatialReferenceH hSRS,
                         const char * pszTargetKey,
                         const char * pszAuthority,
                         int nCode );
OGRErr  OSRSetProjParm( OGRSpatialReferenceH, const char *, double );
double  OSRGetProjParm( OGRSpatialReferenceH hSRS,
                        const char * pszParmName, 
                        double dfDefault /* = 0.0 */,
                        OGRErr * /* = NULL */ );

OGRErr  OSRSetUTM( OGRSpatialReferenceH hSRS, int nZone, int bNorth );
int     OSRGetUTMZone( OGRSpatialReferenceH hSRS, int *pbNorth );

%{
/************************************************************************/
/*                          OSRImportFromWkt()                          */
/************************************************************************/
static PyObject *
py_OSRImportFromWkt(PyObject *self, PyObject *args) {

    OGRSpatialReferenceH _arg0;
    char *_argc0 = NULL;
    char *wkt;
    OGRErr err;

    self = self;
    if(!PyArg_ParseTuple(args,"ss:OSRImportFromWkt",&_argc0,&wkt) )
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,"_OGRSpatialReferenceH" )) {
            PyErr_SetString(PyExc_TypeError,
                            "Type error in argument 1 of OSRImportFromWkt."
                            "  Expected _OGRSpatialReferenceH.");
            return NULL;
        }
    }
	
    err = OSRImportFromWkt( _arg0, &wkt );

    return Py_BuildValue( "i", err );
}
%}

%native(OSRImportFromWkt) py_OSRImportFromWkt;

%{
/************************************************************************/
/*                           OSRExportToWkt()                           */
/************************************************************************/
static PyObject *
py_OSRExportToWkt(PyObject *self, PyObject *args) {

    OGRSpatialReferenceH _arg0;
    char *_argc0 = NULL;
    char *wkt = NULL;
    OGRErr err;
    PyObject *ret;

    self = self;
    if(!PyArg_ParseTuple(args,"s:OSRExportToWkt",&_argc0) )
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,"_OGRSpatialReferenceH" )) {
            PyErr_SetString(PyExc_TypeError,
                            "Type error in argument 1 of OSRExportToWkt."
                            "  Expected _OGRSpatialReferenceH.");
            return NULL;
        }
    }
	
    err = OSRExportToWkt( _arg0, &wkt );
    if( wkt == NULL )
	wkt = "";

    ret = Py_BuildValue( "s", wkt );
    OGRFree( wkt );
    return ret;
}
%}

%native(OSRExportToWkt) py_OSRExportToWkt;

%{
/************************************************************************/
/*                        OSRExportToPrettyWkt()                        */
/************************************************************************/
static PyObject *
py_OSRExportToPrettyWkt(PyObject *self, PyObject *args) {

    OGRSpatialReferenceH _arg0;
    char *_argc0 = NULL;
    char *wkt = NULL;
    int  bSimplify = FALSE;
    OGRErr err;
    PyObject *ret;

    self = self;
    if(!PyArg_ParseTuple(args,"s|i:OSRExportToPrettyWkt",&_argc0, &bSimplify) )
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,"_OGRSpatialReferenceH" )) {
            PyErr_SetString(PyExc_TypeError,
                            "Type error in argument 1 of OSRExportToWkt."
                            "  Expected _OGRSpatialReferenceH.");
            return NULL;
        }
    }
	
    err = OSRExportToPrettyWkt( _arg0, &wkt, bSimplify );
    if( wkt == NULL )
	wkt = "";

    ret = Py_BuildValue( "s", wkt );
    OGRFree( wkt );
    return ret;
}
%}

%native(OSRExportToPrettyWkt) py_OSRExportToPrettyWkt;

/* -------------------------------------------------------------------- */
/*      OGRCoordinateTransform C API.                                   */
/* -------------------------------------------------------------------- */
OGRCoordinateTransformationH
OCTNewCoordinateTransformation( OGRSpatialReferenceH hSourceSRS,
                                OGRSpatialReferenceH hTargetSRS );
void OCTDestroyCoordinateTransformation( OGRCoordinateTransformationH );

%{
/************************************************************************/
/*                             OCTTransform                             */
/************************************************************************/
static PyObject *
py_OCTTransform(PyObject *self, PyObject *args) {

    OGRCoordinateTransformationH _arg0;
    PyObject *pnts;
    char *_argc0 = NULL;
    double *x, *y, *z;
    int    pnt_count, i;

    self = self;
    if(!PyArg_ParseTuple(args,"sO!:OCTTransform",&_argc0, &PyList_Type, &pnts))
        return NULL;

    if (_argc0) {
        if (SWIG_GetPtr(_argc0,(void **) &_arg0,
		        "_OGRCoordinateTransformationH" )) {
            PyErr_SetString(PyExc_TypeError,
                            "Type error in argument 1 of OCTTransform."
                            "  Expected _OGRCoordinateTransformationH.");
            return NULL;
        }
    }

    pnt_count = PyList_Size(pnts);
    x = (double *) CPLCalloc(sizeof(double),pnt_count);
    y = (double *) CPLCalloc(sizeof(double),pnt_count);
    z = (double *) CPLCalloc(sizeof(double),pnt_count);
    for( i = 0; i < pnt_count; i++ )
    {
	if( !PyArg_ParseTuple(PyList_GET_ITEM(pnts,i), "dd|d", x+i, y+i, z+i) )
        {
	    CPLFree( x );
	    CPLFree( y );
	    CPLFree( z );
	    return NULL;
        }
    }  	

    /* perform the transformation */
    if( !OCTTransform( _arg0, pnt_count, x, y, z ) )
    {
        CPLFree( x );
        CPLFree( y );
        CPLFree( z );

        PyErr_SetString(PyExc_TypeError,"OCTTransform failed.");
	return NULL;
    }

    /* make a new points list */
    pnts = PyList_New(pnt_count);
    for( i = 0; i < pnt_count; i++ )
    {
	PyList_SetItem(pnts, i, Py_BuildValue("(ddd)", x[i], y[i], z[i] ) );
    }

    CPLFree( x );
    CPLFree( y );
    CPLFree( z );

    return pnts;
}
%}

%native(OCTTransform) py_OCTTransform;
