/******************************************************************************
 * $Id$
 *
 * Project:  Virtual GDAL Datasets
 * Purpose:  Declaration of virtual gdal dataset classes.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.9  2003/07/17 20:30:24  warmerda
 * Added custom VRTDriver and moved all the sources class declarations in here.
 *
 * Revision 1.8  2003/06/10 19:59:33  warmerda
 * added new Func based source type for passthrough to a callback
 *
 * Revision 1.7  2003/03/13 20:38:30  dron
 * bNoDataValueSet added to VRTRasterBand class.
 *
 * Revision 1.6  2002/11/30 16:55:49  warmerda
 * added OpenXML method
 *
 * Revision 1.5  2002/11/24 04:29:02  warmerda
 * Substantially rewrote VRTSimpleSource.  Now VRTSource is base class, and
 * sources do their own SerializeToXML(), and XMLInit().  New VRTComplexSource
 * supports scaling and nodata values.
 *
 * Revision 1.4  2002/05/29 18:13:44  warmerda
 * added nodata handling for averager
 *
 * Revision 1.3  2002/05/29 16:06:05  warmerda
 * complete detailed band metadata
 *
 * Revision 1.2  2001/11/18 15:46:45  warmerda
 * added SRS and GeoTransform
 *
 * Revision 1.1  2001/11/16 21:14:31  warmerda
 * New
 *
 */

#ifndef VIRTUALDATASET_H_INCLUDED
#define VIRTUALDATASET_H_INCLUDED

#include "gdal_priv.h"
#include "cpl_minixml.h"

CPL_C_START
void	GDALRegister_VRT(void);
typedef CPLErr
(*VRTImageReadFunc)( void *hCBData,
                     int nXOff, int nYOff, int nXSize, int nYSize,
                     void *pData );
CPL_C_END

int VRTApplyMetadata( CPLXMLNode *, GDALMajorObject * );
CPLXMLNode *VRTSerializeMetadata( GDALMajorObject * );

/************************************************************************/
/*                              VRTSource                               */
/************************************************************************/

class VRTSource 
{
public:
    virtual ~VRTSource();

    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace ) = 0;

    virtual CPLErr  XMLInit( CPLXMLNode *psTree ) = 0;
    virtual CPLXMLNode *SerializeToXML() = 0;
};

typedef VRTSource *(*VRTSourceParser)(CPLXMLNode *);

VRTSource *VRTParseCoreSources( CPLXMLNode *psTree );
VRTSource *VRTParseFilterSources( CPLXMLNode *psTree );

/************************************************************************/
/*                              VRTDataset                              */
/************************************************************************/

class CPL_DLL VRTDataset : public GDALDataset
{
    char           *pszProjection;

    int            bGeoTransformSet;
    double         adfGeoTransform[6];

    int           nGCPCount;
    GDAL_GCP      *pasGCPList;
    char          *pszGCPProjection;

    int            bNeedsFlush;

  public:
                 VRTDataset(int nXSize, int nYSize);
                ~VRTDataset();

    void          SetNeedsFlush() { bNeedsFlush = TRUE; }
    virtual void  FlushCache();

    virtual const char *GetProjectionRef(void);
    virtual CPLErr SetProjection( const char * );
    virtual CPLErr GetGeoTransform( double * );
    virtual CPLErr SetGeoTransform( double * );

    virtual int    GetGCPCount();
    virtual const char *GetGCPProjection();
    virtual const GDAL_GCP *GetGCPs();
    virtual CPLErr SetGCPs( int nGCPCount, const GDAL_GCP *pasGCPList,
                            const char *pszGCPProjection );

    virtual CPLErr AddBand( GDALDataType eType, 
                            char **papszOptions=NULL );

    CPLXMLNode *   SerializeToXML(void);
 
    static GDALDataset *Open( GDALOpenInfo * );
    static GDALDataset *OpenXML( const char * );
    static GDALDataset *Create( const char * pszName,
                                int nXSize, int nYSize, int nBands,
                                GDALDataType eType, char ** papszOptions );
};

/************************************************************************/
/*                            VRTRasterBand                             */
/************************************************************************/

class CPL_DLL VRTRasterBand : public GDALRasterBand
{
    int		   nSources;
    VRTSource    **papoSources;

    int            bEqualAreas;

    int            bNoDataValueSet;
    double         dfNoDataValue;

    GDALColorTable *poColorTable;

    GDALColorInterp eColorInterp;

    void           Initialize( int nXSize, int nYSize );

    virtual CPLErr IRasterIO( GDALRWFlag, int, int, int, int,
                              void *, int, int, GDALDataType,
                              int, int );
  public:

    		   VRTRasterBand( GDALDataset *poDS, int nBand );
                   VRTRasterBand( GDALDataType eType, 
                                  int nXSize, int nYSize );
                   VRTRasterBand( GDALDataset *poDS, int nBand, 
                                  GDALDataType eType, 
                                  int nXSize, int nYSize );
    virtual        ~VRTRasterBand();

    CPLErr         XMLInit( CPLXMLNode * );
    CPLXMLNode *   SerializeToXML(void);

#define VRT_NODATA_UNSET -1234.56

    CPLErr         AddSource( VRTSource * );
    CPLErr         AddSimpleSource( GDALRasterBand *poSrcBand, 
                                    int nSrcXOff=-1, int nSrcYOff=-1, 
                                    int nSrcXSize=-1, int nSrcYSize=-1, 
                                    int nDstXOff=-1, int nDstYOff=-1, 
                                    int nDstXSize=-1, int nDstYSize=-1,
                                    const char *pszResampling = "near",
                                    double dfNoDataValue = VRT_NODATA_UNSET);
    CPLErr         AddComplexSource( GDALRasterBand *poSrcBand, 
                                     int nSrcXOff=-1, int nSrcYOff=-1, 
                                     int nSrcXSize=-1, int nSrcYSize=-1, 
                                     int nDstXOff=-1, int nDstYOff=-1, 
                                     int nDstXSize=-1, int nDstYSize=-1,
                                     double dfScaleOff=0.0, 
                                     double dfScaleRatio=1.0,
                                     double dfNoDataValue = VRT_NODATA_UNSET);

    CPLErr         AddFuncSource( VRTImageReadFunc pfnReadFunc, void *hCBData,
                                  double dfNoDataValue = VRT_NODATA_UNSET );


    virtual CPLErr IReadBlock( int, int, void * );

    virtual char      **GetMetadata( const char * pszDomain = "" );
    virtual CPLErr      SetMetadata( char ** papszMetadata,
                                     const char * pszDomain = "" );

    virtual CPLErr SetNoDataValue( double );
    virtual double GetNoDataValue( int *pbSuccess = NULL );

    virtual CPLErr SetColorTable( GDALColorTable * ); 
    virtual GDALColorTable *GetColorTable();

    virtual CPLErr SetColorInterpretation( GDALColorInterp );
    virtual GDALColorInterp GetColorInterpretation();
};

/************************************************************************/
/*                              VRTDriver                               */
/************************************************************************/

class VRTDriver : public GDALDriver
{
  public:
                 VRTDriver();
                 ~VRTDriver();

    char         **papszSourceParsers;

    virtual char      **GetMetadata( const char * pszDomain = "" );
    virtual CPLErr      SetMetadata( char ** papszMetadata,
                                     const char * pszDomain = "" );

    VRTSource   *ParseSource( CPLXMLNode *psSrc );
    void         AddSourceParser( const char *pszElementName, 
                                  VRTSourceParser pfnParser );
};

/************************************************************************/
/*                           VRTSimpleSource                            */
/************************************************************************/

class VRTSimpleSource : public VRTSource
{
protected:
    GDALRasterBand      *poRasterBand;

    int                 nSrcXOff;
    int                 nSrcYOff;
    int                 nSrcXSize;
    int                 nSrcYSize;

    int                 nDstXOff;
    int                 nDstYOff;
    int                 nDstXSize;
    int                 nDstYSize;

    int                 bNoDataSet;
    double              dfNoDataValue;

public:
            VRTSimpleSource();
    virtual ~VRTSimpleSource();

    virtual CPLErr  XMLInit( CPLXMLNode *psTree );
    virtual CPLXMLNode *SerializeToXML();

    void           SetSrcBand( GDALRasterBand * );
    void           SetSrcWindow( int, int, int, int );
    void           SetDstWindow( int, int, int, int );
    void           SetNoDataValue( double dfNoDataValue );

    int            GetSrcDstWindow( int, int, int, int, int, int, 
                                    int *, int *, int *, int *,
                                    int *, int *, int *, int * );

    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace );

    void            DstToSrc( double dfX, double dfY,
                              double &dfXOut, double &dfYOut );
    void            SrcToDst( double dfX, double dfY,
                              double &dfXOut, double &dfYOut );

};

/************************************************************************/
/*                          VRTAveragedSource                           */
/************************************************************************/

class VRTAveragedSource : public VRTSimpleSource
{
public:
    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace );
    virtual CPLXMLNode *SerializeToXML();
};

/************************************************************************/
/*                           VRTComplexSource                           */
/************************************************************************/

class VRTComplexSource : public VRTSimpleSource
{
public:
                   VRTComplexSource();

    virtual CPLErr RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                             void *pData, int nBufXSize, int nBufYSize, 
                             GDALDataType eBufType, 
                             int nPixelSpace, int nLineSpace );
    virtual CPLXMLNode *SerializeToXML();
    virtual CPLErr XMLInit( CPLXMLNode * );

    int		   bDoScaling;
    double	   dfScaleOff;
    double         dfScaleRatio;

};

/************************************************************************/
/*                           VRTFilteredSource                          */
/************************************************************************/

class VRTFilteredSource : public VRTComplexSource
{
private:
    int          IsTypeSupported( GDALDataType eType );

protected:
    int          nSupportedTypesCount;
    GDALDataType aeSupportedTypes[20];

    int          nExtraEdgePixels;

public:
            VRTFilteredSource();
    virtual ~VRTFilteredSource();

    void    SetExtraEdgePixels( int );
    void    SetFilteringDataTypesSupported( int, GDALDataType * );

    virtual CPLErr  FilterData( int nXSize, int nYSize, GDALDataType eType, 
                                GByte *pabySrcData, GByte *pabyDstData ) = 0;

    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace );
};

/************************************************************************/
/*                       VRTKernelFilteredSource                        */
/************************************************************************/

class VRTKernelFilteredSource : public VRTFilteredSource
{
protected:
    int     nKernelSize;

    double  *padfKernelCoefs;

public:
            VRTKernelFilteredSource();
    virtual ~VRTKernelFilteredSource();

    virtual CPLErr  XMLInit( CPLXMLNode *psTree );
    virtual CPLXMLNode *SerializeToXML();

    virtual CPLErr  FilterData( int nXSize, int nYSize, GDALDataType eType, 
                                GByte *pabySrcData, GByte *pabyDstData );

    CPLErr          SetKernel( int nKernelSize, double *padfCoefs );
};

/************************************************************************/
/*                       VRTAverageFilteredSource                       */
/************************************************************************/

class VRTAverageFilteredSource : public VRTKernelFilteredSource
{
public:
            VRTAverageFilteredSource( int nKernelSize );
    virtual ~VRTAverageFilteredSource();

    virtual CPLErr  XMLInit( CPLXMLNode *psTree );
    virtual CPLXMLNode *SerializeToXML();
};

/************************************************************************/
/*                            VRTFuncSource                             */
/************************************************************************/
class VRTFuncSource : public VRTSource
{
public:
            VRTFuncSource();
    virtual ~VRTFuncSource();

    virtual CPLErr  XMLInit( CPLXMLNode * ) { return CE_Failure; }
    virtual CPLXMLNode *SerializeToXML();

    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace );

    VRTImageReadFunc    pfnReadFunc;
    void               *pCBData;
    GDALDataType        eType;
    
    float               fNoDataValue;
};

#endif /* ndef VIRTUALDATASET_H_INCLUDED */
