/******************************************************************************
 * $Id$
 *
 * Project:  Virtual GDAL Datasets
 * Purpose:  Implementation of VRTRasterBand
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.9  2002/12/04 03:12:57  warmerda
 * Fixed some bugs in last change.
 *
 * Revision 1.8  2002/12/03 15:21:19  warmerda
 * finished up window computations for odd cases
 *
 * Revision 1.7  2002/11/24 04:29:02  warmerda
 * Substantially rewrote VRTSimpleSource.  Now VRTSource is base class, and
 * sources do their own SerializeToXML(), and XMLInit().  New VRTComplexSource
 * supports scaling and nodata values.
 *
 * Revision 1.6  2002/07/15 18:51:46  warmerda
 * ensure even unshared datasets are dereferenced properly
 *
 * Revision 1.5  2002/05/29 18:13:44  warmerda
 * added nodata handling for averager
 *
 * Revision 1.4  2002/05/29 16:06:05  warmerda
 * complete detailed band metadata
 *
 * Revision 1.3  2001/12/06 19:53:58  warmerda
 * added write check to rasterio
 *
 * Revision 1.2  2001/11/16 21:38:07  warmerda
 * try to work even if bModified
 *
 * Revision 1.1  2001/11/16 21:14:31  warmerda
 * New
 *
 */

#include "vrtdataset.h"
#include "cpl_minixml.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*                             VRTSource                                */
/* ==================================================================== */
/************************************************************************/

VRTSource::~VRTSource()
{
}

/************************************************************************/
/* ==================================================================== */
/*                          VRTSimpleSource                             */
/* ==================================================================== */
/************************************************************************/

class VRTSimpleSource : public VRTSource
{
public:
            VRTSimpleSource();
    virtual ~VRTSimpleSource();

    virtual CPLErr  XMLInit( CPLXMLNode *psTree );
    virtual CPLXMLNode *SerializeToXML();

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

    GDALRasterBand      *poRasterBand;

    int                 nSrcXOff;
    int                 nSrcYOff;
    int                 nSrcXSize;
    int                 nSrcYSize;

    int                 nDstXOff;
    int                 nDstYOff;
    int                 nDstXSize;
    int                 nDstYSize;

    float               fNoDataValue;
};

/************************************************************************/
/*                          VRTSimpleSource()                           */
/************************************************************************/

VRTSimpleSource::VRTSimpleSource()

{
    poRasterBand = NULL;
}

/************************************************************************/
/*                          ~VRTSimpleSource()                          */
/************************************************************************/

VRTSimpleSource::~VRTSimpleSource()

{
    if( poRasterBand != NULL && poRasterBand->GetDataset() != NULL )
    {
        if( poRasterBand->GetDataset()->GetShared() )
            GDALClose( (GDALDatasetH) poRasterBand->GetDataset() );
        else
            poRasterBand->GetDataset()->Dereference();
    }
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTSimpleSource::SerializeToXML()

{
    CPLXMLNode      *psSrc;
    GDALDataset     *poDS = poRasterBand->GetDataset();

    if( poDS == NULL || poRasterBand->GetBand() < 1 )
        return NULL;

    psSrc = CPLCreateXMLNode( NULL, CXT_Element, "SimpleSource" );

    CPLSetXMLValue( psSrc, "SourceFilename", poDS->GetDescription() );

    CPLSetXMLValue( psSrc, "SourceBand", 
                    CPLSPrintf("%d",poRasterBand->GetBand()) );

    if( nSrcXOff != -1 
        || nSrcYOff != -1 
        || nSrcXSize != -1 
        || nSrcYSize != -1 )
    {
        CPLSetXMLValue( psSrc, "SrcRect.#xOff", 
                        CPLSPrintf( "%d", nSrcXOff ) );
        CPLSetXMLValue( psSrc, "SrcRect.#yOff", 
                        CPLSPrintf( "%d", nSrcYOff ) );
        CPLSetXMLValue( psSrc, "SrcRect.#xSize", 
                        CPLSPrintf( "%d", nSrcXSize ) );
        CPLSetXMLValue( psSrc, "SrcRect.#ySize", 
                        CPLSPrintf( "%d", nSrcYSize ) );
    }

    if( nDstXOff != -1 
        || nDstYOff != -1 
        || nDstXSize != -1 
        || nDstYSize != -1 )
    {
        CPLSetXMLValue( psSrc, "DstRect.#xOff", CPLSPrintf( "%d", nDstXOff ) );
        CPLSetXMLValue( psSrc, "DstRect.#yOff", CPLSPrintf( "%d", nDstYOff ) );
        CPLSetXMLValue( psSrc, "DstRect.#xSize",CPLSPrintf( "%d", nDstXSize ));
        CPLSetXMLValue( psSrc, "DstRect.#ySize",CPLSPrintf( "%d", nDstYSize ));
    }

    return psSrc;
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTSimpleSource::XMLInit( CPLXMLNode *psSrc )

{
/* -------------------------------------------------------------------- */
/*      Get the raster band.                                            */
/* -------------------------------------------------------------------- */
    const char *pszFilename = 
        CPLGetXMLValue(psSrc, "SourceFilename", NULL);
    
    if( pszFilename == NULL )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "Missing <SourceFilename> element in VRTRasterBand." );
        return CE_Failure;
    }
    
    int nSrcBand = atoi(CPLGetXMLValue(psSrc,"SourceBand","1"));
    
    GDALDataset *poSrcDS = (GDALDataset *) 
        GDALOpenShared( pszFilename, GA_ReadOnly );
    
    if( poSrcDS == NULL || poSrcDS->GetRasterBand(nSrcBand) == NULL )
        return CE_Failure;

    poRasterBand = poSrcDS->GetRasterBand(nSrcBand);
    
/* -------------------------------------------------------------------- */
/*      Set characteristics.                                            */
/* -------------------------------------------------------------------- */
    nSrcXOff = atoi(CPLGetXMLValue(psSrc,"SrcRect.xOff","-1"));
    nSrcYOff = atoi(CPLGetXMLValue(psSrc,"SrcRect.yOff","-1"));
    nSrcXSize = atoi(CPLGetXMLValue(psSrc,"SrcRect.xSize","-1"));
    nSrcYSize = atoi(CPLGetXMLValue(psSrc,"SrcRect.ySize","-1"));
    nDstXOff = atoi(CPLGetXMLValue(psSrc,"DstRect.xOff","-1"));
    nDstYOff = atoi(CPLGetXMLValue(psSrc,"DstRect.yOff","-1"));
    nDstXSize = atoi(CPLGetXMLValue(psSrc,"DstRect.xSize","-1"));
    nDstYSize = atoi(CPLGetXMLValue(psSrc,"DstRect.ySize","-1"));

    return CE_None;
}

/************************************************************************/
/*                              SrcToDst()                              */
/************************************************************************/

void VRTSimpleSource::SrcToDst( double dfX, double dfY,
                                double &dfXOut, double &dfYOut )

{
    dfXOut = ((dfX - nSrcXOff) / nSrcXSize) * nDstXSize + nDstXOff;
    dfYOut = ((dfY - nSrcYOff) / nSrcYSize) * nDstYSize + nDstYOff;
}

/************************************************************************/
/*                              DstToSrc()                              */
/************************************************************************/

void VRTSimpleSource::DstToSrc( double dfX, double dfY,
                                double &dfXOut, double &dfYOut )

{
    dfXOut = ((dfX - nDstXOff) / nDstXSize) * nSrcXSize + nSrcXOff;
    dfYOut = ((dfY - nDstYOff) / nDstYSize) * nSrcYSize + nSrcYOff;
}

/************************************************************************/
/*                          GetSrcDstWindow()                           */
/************************************************************************/

int 
VRTSimpleSource::GetSrcDstWindow( int nXOff, int nYOff, int nXSize, int nYSize,
                                  int nBufXSize, int nBufYSize,
                                  int *pnReqXOff, int *pnReqYOff,
                                  int *pnReqXSize, int *pnReqYSize,
                                  int *pnOutXOff, int *pnOutYOff, 
                                  int *pnOutXSize, int *pnOutYSize )

{
/* -------------------------------------------------------------------- */
/*      Translate requested region in virtual file into the source      */
/*      band coordinates.                                               */
/* -------------------------------------------------------------------- */
    double      dfScaleX = nSrcXSize / (double) nDstXSize;
    double      dfScaleY = nSrcYSize / (double) nDstYSize;

    *pnReqXOff = (int) floor((nXOff - nDstXOff) * dfScaleX + nSrcXOff);
    *pnReqYOff = (int) floor((nYOff - nDstYOff) * dfScaleY + nSrcYOff);

    *pnReqXSize = (int) floor(nXSize * dfScaleX + 0.5);
    *pnReqYSize = (int) floor(nYSize * dfScaleY + 0.5);

/* -------------------------------------------------------------------- */
/*      This request window corresponds to the whole output buffer.     */
/* -------------------------------------------------------------------- */
    *pnOutXOff = 0;
    *pnOutYOff = 0;
    *pnOutXSize = nBufXSize;
    *pnOutYSize = nBufYSize;

/* -------------------------------------------------------------------- */
/*      Clamp within the bounds of the available source data.           */
/* -------------------------------------------------------------------- */
    int bModified = FALSE;

    if( *pnReqXOff < 0 )
    {
        *pnReqXSize += *pnReqXOff;
        *pnReqXOff = 0;

        bModified = TRUE;
    }
    
    if( *pnReqYOff < 0 )
    {
        *pnReqYSize += *pnReqYOff;
        *pnReqYOff = 0;
        bModified = TRUE;
    }

    if( *pnReqXSize == 0 )
        *pnReqXSize = 1;
    if( *pnReqYSize == 0 )
        *pnReqYSize = 1;

    if( *pnReqXOff + *pnReqXSize > poRasterBand->GetXSize() )
    {
        *pnReqXSize = poRasterBand->GetXSize() - *pnReqXOff;
        bModified = TRUE;
    }

    if( *pnReqYOff + *pnReqYSize > poRasterBand->GetYSize() )
    {
        *pnReqYSize = poRasterBand->GetYSize() - *pnReqYOff;
        bModified = TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Don't do anything if the requesting region is completely off    */
/*      the source image.                                               */
/* -------------------------------------------------------------------- */
    if( *pnReqXOff >= poRasterBand->GetXSize()
        || *pnReqYOff >= poRasterBand->GetYSize()
        || *pnReqXSize <= 0 || *pnReqYSize <= 0 )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      If we haven't had to modify the source rectangle, then the      */
/*      destination rectangle must be the whole region.                 */
/* -------------------------------------------------------------------- */
    if( !bModified )
        return TRUE;

/* -------------------------------------------------------------------- */
/*      Now transform this possibly reduced request back into the       */
/*      destination buffer coordinates in case the output region is     */
/*      less than the whole buffer.                                     */
/* -------------------------------------------------------------------- */
    double dfDstULX, dfDstULY, dfDstLRX, dfDstLRY;
    double dfScaleWinToBufX, dfScaleWinToBufY;

    SrcToDst( (double) *pnReqXOff, (double) *pnReqYOff, dfDstULX, dfDstULY );
    SrcToDst( *pnReqXOff + *pnReqXSize, *pnReqYOff + *pnReqYSize, 
              dfDstLRX, dfDstLRY );

    dfScaleWinToBufX = nBufXSize / (double) nXSize;
    dfScaleWinToBufY = nBufYSize / (double) nYSize;

    *pnOutXOff = (int) ((dfDstULX - nXOff) * dfScaleWinToBufX);
    *pnOutYOff = (int) ((dfDstULY - nYOff) * dfScaleWinToBufY);
    *pnOutXSize = (int) ((dfDstLRX - nXOff) * dfScaleWinToBufX) - *pnOutXOff;
    *pnOutYSize = (int) ((dfDstLRY - nYOff) * dfScaleWinToBufY) - *pnOutYOff;

    *pnOutXOff = MAX(0,*pnOutXOff);
    *pnOutYOff = MAX(0,*pnOutYOff);
    if( *pnOutXOff + *pnOutXSize > nBufXSize )
        *pnOutXSize = nBufXSize - *pnOutXOff;
    if( *pnOutYOff + *pnOutYSize > nBufYSize )
        *pnOutYSize = nBufYSize - *pnOutYOff;

    if( *pnOutXSize < 1 || *pnOutYSize < 1 )
        return FALSE;
    else
        return TRUE;
}

/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

CPLErr 
VRTSimpleSource::RasterIO( int nXOff, int nYOff, int nXSize, int nYSize,
                           void *pData, int nBufXSize, int nBufYSize, 
                           GDALDataType eBufType, 
                           int nPixelSpace, int nLineSpace )

{
    // The window we will actually request from the source raster band.
    int nReqXOff, nReqYOff, nReqXSize, nReqYSize;

    // The window we will actual set _within_ the pData buffer.
    int nOutXOff, nOutYOff, nOutXSize, nOutYSize;

    if( !GetSrcDstWindow( nXOff, nYOff, nXSize, nYSize,
                          nBufXSize, nBufYSize, 
                          &nReqXOff, &nReqYOff, &nReqXSize, &nReqYSize,
                          &nOutXOff, &nOutYOff, &nOutXSize, &nOutYSize ) )
        return CE_None;

/* -------------------------------------------------------------------- */
/*      Actually perform the IO request.                                */
/* -------------------------------------------------------------------- */
    return 
        poRasterBand->RasterIO( GF_Read, 
                                nReqXOff, nReqYOff, nReqXSize, nReqYSize,
                                ((unsigned char *) pData) 
                                + nOutXOff * nPixelSpace
                                + nOutYOff * nLineSpace, 
                                nOutXSize, nOutYSize, 
                                eBufType, nPixelSpace, nLineSpace );
}

/************************************************************************/
/* ==================================================================== */
/*                         VRTAveragedSource                            */
/* ==================================================================== */
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
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTAveragedSource::SerializeToXML()

{
    CPLXMLNode *psSrc = VRTSimpleSource::SerializeToXML();

    if( psSrc == NULL )
        return NULL;

    CPLFree( psSrc->pszValue );
    psSrc->pszValue = CPLStrdup( "AveragedSource" );

    return psSrc;
}

/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

CPLErr 
VRTAveragedSource::RasterIO( int nXOff, int nYOff, int nXSize, int nYSize,
                           void *pData, int nBufXSize, int nBufYSize, 
                           GDALDataType eBufType, 
                           int nPixelSpace, int nLineSpace )

{
    // The window we will actually request from the source raster band.
    int nReqXOff, nReqYOff, nReqXSize, nReqYSize;

    // The window we will actual set _within_ the pData buffer.
    int nOutXOff, nOutYOff, nOutXSize, nOutYSize;

    if( !GetSrcDstWindow( nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize, 
                          &nReqXOff, &nReqYOff, &nReqXSize, &nReqYSize,
                          &nOutXOff, &nOutYOff, &nOutXSize, &nOutYSize ) )
        return CE_None;

/* -------------------------------------------------------------------- */
/*      Allocate a temporary buffer to whole the full resolution        */
/*      data from the area of interest.                                 */
/* -------------------------------------------------------------------- */
    float *pafSrc;

    pafSrc = (float *) VSIMalloc(sizeof(float) * nReqXSize * nReqYSize);
    if( pafSrc == NULL )
    {
        CPLError( CE_Failure, CPLE_OutOfMemory, 
                  "Out of memory allocating working buffer in VRTAveragedSource::RasterIO()." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Load it.                                                        */
/* -------------------------------------------------------------------- */
    CPLErr eErr;
    
    eErr = poRasterBand->RasterIO( GF_Read, 
                                   nReqXOff, nReqYOff, nReqXSize, nReqYSize,
                                   pafSrc, nReqXSize, nReqYSize, GDT_Float32, 
                                   0, 0 );

    if( eErr != CE_None )
    {
        VSIFree( pafSrc );
        return eErr;
    }

/* -------------------------------------------------------------------- */
/*      Do the averaging.                                               */
/* -------------------------------------------------------------------- */
    for( int iBufLine = nOutYOff; iBufLine < nOutYOff + nOutYSize; iBufLine++ )
    {
        double  dfYDst;

        dfYDst = (iBufLine / (double) nBufYSize) * nYSize + nYOff;
        
        for( int iBufPixel = nOutXOff; 
             iBufPixel < nOutXOff + nOutXSize; 
             iBufPixel++ )
        {
            double dfXDst;
            double dfXSrcStart, dfXSrcEnd, dfYSrcStart, dfYSrcEnd;
            int    iXSrcStart, iYSrcStart, iXSrcEnd, iYSrcEnd;

            dfXDst = (iBufPixel / (double) nBufXSize) * nXSize + nXOff;

            // Compute the source image rectangle needed for this pixel.
            DstToSrc( dfXDst, dfYDst, dfXSrcStart, dfYSrcStart );
            DstToSrc( dfXDst+1.0, dfYDst+1.0, dfXSrcEnd, dfYSrcEnd );

            // Convert to integers, assuming that the center of the source
            // pixel must be in our rect to get included.
            iXSrcStart = (int) floor(dfXSrcStart+0.5);
            iYSrcStart = (int) floor(dfYSrcStart+0.5);
            iXSrcEnd = (int) floor(dfXSrcEnd+0.5);
            iYSrcEnd = (int) floor(dfYSrcEnd+0.5);

            // Transform into the coordinate system of the source *buffer*
            iXSrcStart -= nReqXOff;
            iYSrcStart -= nReqYOff;
            iXSrcEnd -= nReqXOff;
            iYSrcEnd -= nReqYOff;

            double dfSum = 0.0;
            int    nPixelCount = 0;
            
            for( int iY = iYSrcStart; iY < iYSrcEnd; iY++ )
            {
                if( iY < 0 || iY >= nReqYSize )
                    continue;

                for( int iX = iXSrcStart; iX < iXSrcEnd; iX++ )
                {
                    if( iX < 0 || iX >= nReqXSize )
                        continue;

                    float fSampledValue = pafSrc[iX + iY * nReqXSize];

                    if( ABS(fSampledValue-fNoDataValue) < 0.0001 )
                        continue;

                    nPixelCount++;
                    dfSum += pafSrc[iX + iY * nReqXSize];
                }
            }

            if( nPixelCount == 0 )
                continue;

            // Compute output value.
            float dfOutputValue = dfSum / nPixelCount;

            // Put it in the output buffer.
            GByte *pDstLocation;

            pDstLocation = ((GByte *)pData) 
                + nPixelSpace * iBufPixel
                + nLineSpace * iBufLine;

            if( eBufType == GDT_Byte )
                *pDstLocation = (int) MIN(255,MAX(0,dfOutputValue));
            else
                GDALCopyWords( &dfOutputValue, GDT_Float32, 4, 
                               pDstLocation, eBufType, 8, 1 );
        }
    }

    VSIFree( pafSrc );

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                          VRTComplexSource                            */
/* ==================================================================== */
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

    int            bNoDataSet;
    double         dfNoDataValue;
};

/************************************************************************/
/*                          VRTComplexSource()                          */
/************************************************************************/

VRTComplexSource::VRTComplexSource()

{
    bDoScaling = FALSE;
    dfScaleOff = 0.0;
    dfScaleRatio = 1.0;
    
    bNoDataSet = FALSE;
    dfNoDataValue = 0.0;
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTComplexSource::SerializeToXML()

{
    CPLXMLNode *psSrc = VRTSimpleSource::SerializeToXML();

    if( psSrc == NULL )
        return NULL;

    CPLFree( psSrc->pszValue );
    psSrc->pszValue = CPLStrdup( "ComplexSource" );

    if( bNoDataSet )
    {
        CPLSetXMLValue( psSrc, "NODATA", 
                        CPLSPrintf("%g", dfNoDataValue) );
    }
        
    if( bDoScaling )
    {
        CPLSetXMLValue( psSrc, "ScaleOffset", 
                        CPLSPrintf("%g", dfScaleOff) );
        CPLSetXMLValue( psSrc, "ScaleRatio", 
                        CPLSPrintf("%g", dfScaleRatio) );
    }

    return psSrc;
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTComplexSource::XMLInit( CPLXMLNode *psSrc )

{
    CPLErr eErr;

/* -------------------------------------------------------------------- */
/*      Do base initialization.                                         */
/* -------------------------------------------------------------------- */
    eErr = VRTSimpleSource::XMLInit( psSrc );
    if( eErr != CE_None )
        return eErr;

/* -------------------------------------------------------------------- */
/*      Complex parameters.                                             */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLValue(psSrc, "ScaleOffset", NULL) != NULL 
        || CPLGetXMLValue(psSrc, "ScaleRatio", NULL) != NULL )
    {
        bDoScaling = TRUE;
        dfScaleOff = atof(CPLGetXMLValue(psSrc, "ScaleOffset", "0") );
        dfScaleRatio = atof(CPLGetXMLValue(psSrc, "ScaleRatio", "1") );
    }

    if( CPLGetXMLValue(psSrc, "NODATA", NULL) != NULL )
    {
        bNoDataSet = TRUE;
        dfNoDataValue = atof(CPLGetXMLValue(psSrc, "NODATA", "0"));
    }

    return CE_None;
}

/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

CPLErr 
VRTComplexSource::RasterIO( int nXOff, int nYOff, int nXSize, int nYSize,
                            void *pData, int nBufXSize, int nBufYSize, 
                            GDALDataType eBufType, 
                            int nPixelSpace, int nLineSpace )
    
{
    // The window we will actually request from the source raster band.
    int nReqXOff, nReqYOff, nReqXSize, nReqYSize;

    // The window we will actual set _within_ the pData buffer.
    int nOutXOff, nOutYOff, nOutXSize, nOutYSize;

    if( !GetSrcDstWindow( nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize, 
                          &nReqXOff, &nReqYOff, &nReqXSize, &nReqYSize,
                          &nOutXOff, &nOutYOff, &nOutXSize, &nOutYSize ) )
        return CE_None;

/* -------------------------------------------------------------------- */
/*      Read into a temporary buffer.                                   */
/* -------------------------------------------------------------------- */
    float *pafData;
    CPLErr eErr;

    pafData = (float *) CPLMalloc(nOutXSize*nOutYSize*sizeof(float));
    eErr = poRasterBand->RasterIO( GF_Read, 
                                   nReqXOff, nReqYOff, nReqXSize, nReqYSize,
                                   pafData, nOutXSize, nOutYSize, GDT_Float32,
                                   sizeof(float), sizeof(float) * nOutXSize );
    if( eErr != CE_None )
    {
        CPLFree( pafData );
        return eErr;
    }

/* -------------------------------------------------------------------- */
/*      Selectively copy into output buffer with nodata masking,        */
/*      and/or scaling.                                                 */
/* -------------------------------------------------------------------- */
    int iX, iY;

    for( iY = 0; iY < nOutYSize; iY++ )
    {
        for( iX = 0; iX < nOutXSize; iX++ )
        {
            float fResult;

            fResult = pafData[iX + iY * nOutXSize];
            if( bNoDataSet && fResult == dfNoDataValue )
                continue;

            if( bDoScaling )
                fResult = fResult * dfScaleRatio + dfScaleOff;

            GByte *pDstLocation;

            pDstLocation = ((GByte *)pData) 
                + nPixelSpace * (iX + nOutXOff)
                + nLineSpace * (iY + nOutYOff);

            if( eBufType == GDT_Byte )
                *pDstLocation = (int) MIN(255,MAX(0,fResult));
            else
                GDALCopyWords( &fResult, GDT_Float32, 4, 
                               pDstLocation, eBufType, 8, 1 );
                
        }
    }

    CPLFree( pafData );

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                          VRTRasterBand                               */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           VRTRasterBand()                            */
/************************************************************************/

VRTRasterBand::VRTRasterBand( GDALDataset *poDS, int nBand )

{
    Initialize( poDS->GetRasterXSize(), poDS->GetRasterYSize() );

    this->poDS = poDS;
    this->nBand = nBand;
}

/************************************************************************/
/*                           VRTRasterBand()                            */
/************************************************************************/

VRTRasterBand::VRTRasterBand( GDALDataType eType, 
                              int nXSize, int nYSize )

{
    Initialize( nXSize, nYSize );

    eDataType = eType;
}

/************************************************************************/
/*                           VRTRasterBand()                            */
/************************************************************************/

VRTRasterBand::VRTRasterBand( GDALDataset *poDS, int nBand,
                              GDALDataType eType, 
                              int nXSize, int nYSize )

{
    Initialize( nXSize, nYSize );

    this->poDS = poDS;
    this->nBand = nBand;

    eDataType = eType;
}

/************************************************************************/
/*                             Initialize()                             */
/************************************************************************/

void VRTRasterBand::Initialize( int nXSize, int nYSize )

{
    poDS = NULL;
    nBand = 0;
    eAccess = GA_ReadOnly;
    eDataType = GDT_Byte;

    nRasterXSize = nXSize;
    nRasterYSize = nYSize;
    
    nSources = 0;
    papoSources = NULL;

    nBlockXSize = MIN(128,nXSize);
    nBlockYSize = MIN(128,nYSize);

    bNoDataValueSet = FALSE;
    dfNoDataValue = -10000.0;
    poColorTable = NULL;
    eColorInterp = GCI_Undefined;
}


/************************************************************************/
/*                           ~VRTRasterBand()                           */
/************************************************************************/

VRTRasterBand::~VRTRasterBand()

{
    for( int i = 0; i < nSources; i++ )
        delete papoSources[i];

    CPLFree( papoSources );
    nSources = 0;

    if( poColorTable != NULL )
        delete poColorTable;
}

/************************************************************************/
/*                             IRasterIO()                              */
/************************************************************************/

CPLErr VRTRasterBand::IRasterIO( GDALRWFlag eRWFlag,
                                 int nXOff, int nYOff, int nXSize, int nYSize,
                                 void * pData, int nBufXSize, int nBufYSize,
                                 GDALDataType eBufType,
                                 int nPixelSpace, int nLineSpace )

{
    int         iSource;
    CPLErr      eErr = CE_Failure;

    if( eRWFlag == GF_Write )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Writing through VRTRasterBand is not supported." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Initialize the buffer to some background value. Use the         */
/*      nodata value if available.                                      */
/* -------------------------------------------------------------------- */
    double dfWriteValue = 0.0;
    int    iLine;

    if( bNoDataValueSet )
        dfWriteValue = dfNoDataValue;
    
    for( iLine = 0; iLine < nBufYSize; iLine++ )
    {
        GDALCopyWords( &dfWriteValue, GDT_Float64, 0, 
                       ((GByte *)pData) + nLineSpace * iLine, 
                       eBufType, nPixelSpace, nBufXSize );
    }
    
/* -------------------------------------------------------------------- */
/*      Overlay each source in turn over top this.                      */
/* -------------------------------------------------------------------- */
    for( iSource = 0; iSource < nSources; iSource++ )
    {
        eErr = 
            papoSources[iSource]->RasterIO( nXOff, nYOff, nXSize, nYSize, 
                                            pData, nBufXSize, nBufYSize, 
                                            eBufType, nPixelSpace, nLineSpace);
    }
    
    return eErr;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr VRTRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                   void * pImage )

{
    return IRasterIO( GF_Read, 0, nBlockYOff, nBlockXSize, 1, 
                      pImage, nBlockXSize, 1, eDataType, 
                      GDALGetDataTypeSize(eDataType)/8, 0 );
}

/************************************************************************/
/*                             AddSource()                              */
/************************************************************************/

CPLErr VRTRasterBand::AddSource( VRTSource *poNewSource )

{
    nSources++;

    papoSources = (VRTSource **) 
        CPLRealloc(papoSources, sizeof(void*) * nSources);
    papoSources[nSources-1] = poNewSource;

    return CE_None;
}

/************************************************************************/
/*                          AddSimpleSource()                           */
/************************************************************************/

CPLErr VRTRasterBand::AddSimpleSource( GDALRasterBand *poSrcBand, 
                                       int nSrcXOff, int nSrcYOff, 
                                       int nSrcXSize, int nSrcYSize, 
                                       int nDstXOff, int nDstYOff, 
                                       int nDstXSize, int nDstYSize,
                                       const char *pszResampling, 
                                       double dfNoDataValue )

{
/* -------------------------------------------------------------------- */
/*      Default source and dest rectangles.                             */
/* -------------------------------------------------------------------- */
    if( nSrcYSize == -1 )
    {
        nSrcXOff = 0;
        nSrcYOff = 0;
        nSrcXSize = poSrcBand->GetXSize();
        nSrcYSize = poSrcBand->GetYSize();
    }

    if( nDstYSize == -1 )
    {
        nDstXOff = 0;
        nDstYOff = 0;
        nDstXSize = nRasterXSize;
        nDstYSize = nRasterYSize;
    }

/* -------------------------------------------------------------------- */
/*      Create source.                                                  */
/* -------------------------------------------------------------------- */
    VRTSimpleSource *poSimpleSource;

    if( pszResampling != NULL && EQUALN(pszResampling,"aver",4) )
        poSimpleSource = new VRTAveragedSource();
    else
    {
        poSimpleSource = new VRTSimpleSource();
        if( dfNoDataValue != VRT_NODATA_UNSET )
            CPLError( 
                CE_Warning, CPLE_AppDefined, 
                "NODATA setting not currently supported for nearest\n"
                "neighbour sampled simple sources on Virtual Datasources." );
    }

    poSimpleSource->poRasterBand = poSrcBand;

    poSimpleSource->nSrcXOff  = nSrcXOff;
    poSimpleSource->nSrcYOff  = nSrcYOff;
    poSimpleSource->nSrcXSize = nSrcXSize;
    poSimpleSource->nSrcYSize = nSrcYSize;

    poSimpleSource->nDstXOff  = nDstXOff;
    poSimpleSource->nDstYOff  = nDstYOff;
    poSimpleSource->nDstXSize = nDstXSize;
    poSimpleSource->nDstYSize = nDstYSize;
    
    poSimpleSource->fNoDataValue = (float) dfNoDataValue;

/* -------------------------------------------------------------------- */
/*      If we can get the associated GDALDataset, add a reference to it.*/
/* -------------------------------------------------------------------- */
    if( poSrcBand->GetDataset() != NULL )
        poSrcBand->GetDataset()->Reference();

/* -------------------------------------------------------------------- */
/*      add to list.                                                    */
/* -------------------------------------------------------------------- */
    return AddSource( poSimpleSource );
}

/************************************************************************/
/*                          AddComplexSource()                          */
/************************************************************************/

CPLErr VRTRasterBand::AddComplexSource( GDALRasterBand *poSrcBand, 
                                        int nSrcXOff, int nSrcYOff, 
                                        int nSrcXSize, int nSrcYSize, 
                                        int nDstXOff, int nDstYOff, 
                                        int nDstXSize, int nDstYSize,
                                        double dfScaleOff, 
                                        double dfScaleRatio,
                                        double dfNoDataValue )

{
/* -------------------------------------------------------------------- */
/*      Default source and dest rectangles.                             */
/* -------------------------------------------------------------------- */
    if( nSrcYSize == -1 )
    {
        nSrcXOff = 0;
        nSrcYOff = 0;
        nSrcXSize = poSrcBand->GetXSize();
        nSrcYSize = poSrcBand->GetYSize();
    }

    if( nDstYSize == -1 )
    {
        nDstXOff = 0;
        nDstYOff = 0;
        nDstXSize = nRasterXSize;
        nDstYSize = nRasterYSize;
    }

/* -------------------------------------------------------------------- */
/*      Create source.                                                  */
/* -------------------------------------------------------------------- */
    VRTComplexSource *poSource;

    poSource = new VRTComplexSource();

    poSource->poRasterBand = poSrcBand;

    poSource->nSrcXOff  = nSrcXOff;
    poSource->nSrcYOff  = nSrcYOff;
    poSource->nSrcXSize = nSrcXSize;
    poSource->nSrcYSize = nSrcYSize;

    poSource->nDstXOff  = nDstXOff;
    poSource->nDstYOff  = nDstYOff;
    poSource->nDstXSize = nDstXSize;
    poSource->nDstYSize = nDstYSize;
    
/* -------------------------------------------------------------------- */
/*      Set complex parameters.                                         */
/* -------------------------------------------------------------------- */
    if( dfNoDataValue != VRT_NODATA_UNSET )
    {
        poSource->bNoDataSet = TRUE;
        poSource->dfNoDataValue = dfNoDataValue;
    }

    if( dfScaleOff != 0.0 || dfScaleRatio != 1.0 )
    {
        poSource->bDoScaling = TRUE;
        poSource->dfScaleOff = dfScaleOff;
        poSource->dfScaleRatio = dfScaleRatio;
          
    }
/* -------------------------------------------------------------------- */
/*      If we can get the associated GDALDataset, add a reference to it.*/
/* -------------------------------------------------------------------- */
    if( poSrcBand->GetDataset() != NULL )
        poSrcBand->GetDataset()->Reference();

/* -------------------------------------------------------------------- */
/*      add to list.                                                    */
/* -------------------------------------------------------------------- */
    return AddSource( poSource );
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTRasterBand::XMLInit( CPLXMLNode * psTree )

{
/* -------------------------------------------------------------------- */
/*      Validate a bit.                                                 */
/* -------------------------------------------------------------------- */
    if( psTree == NULL || psTree->eType != CXT_Element
        || !EQUAL(psTree->pszValue,"VRTRasterBand") )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Invalid node passed to VRTRasterBand::XMLInit()." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Set the band if provided as an attribute.                       */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLValue( psTree, "band", NULL) != NULL )
        nBand = atoi(CPLGetXMLValue( psTree, "band", "0"));

/* -------------------------------------------------------------------- */
/*      Set the band if provided as an attribute.                       */
/* -------------------------------------------------------------------- */
    const char *pszDataType = CPLGetXMLValue( psTree, "dataType", NULL);
    if( pszDataType != NULL )
    {
        for( int iType = 0; iType < GDT_TypeCount; iType++ )
        {
            const char *pszThisName = GDALGetDataTypeName((GDALDataType)iType);

            if( pszThisName != NULL && EQUAL(pszDataType,pszThisName) )
            {
                eDataType = (GDALDataType) iType;
                break;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Apply any band level metadata.                                  */
/* -------------------------------------------------------------------- */
    VRTApplyMetadata( psTree, this );

/* -------------------------------------------------------------------- */
/*      Collect various other items of metadata.                        */
/* -------------------------------------------------------------------- */
    SetDescription( CPLGetXMLValue( psTree, "Description", "" ) );
    
    if( CPLGetXMLValue( psTree, "NoDataValue", NULL ) != NULL )
        SetNoDataValue( atof(CPLGetXMLValue( psTree, "NoDataValue", "0" )) );

    if( CPLGetXMLValue( psTree, "ColorInterp", NULL ) != NULL )
    {
        const char *pszInterp = CPLGetXMLValue( psTree, "ColorInterp", NULL );
        int        iInterp;
        
        for( iInterp = 0; iInterp < 13; iInterp++ )
        {
            const char *pszCandidate 
                = GDALGetColorInterpretationName( (GDALColorInterp) iInterp );

            if( pszCandidate != NULL && EQUAL(pszCandidate,pszInterp) )
            {
                SetColorInterpretation( (GDALColorInterp) iInterp );
                break;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Collect a color table.                                          */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLNode( psTree, "ColorTable" ) != NULL )
    {
        CPLXMLNode *psEntry;
        GDALColorTable oTable;
        int        iEntry = 0;

        for( psEntry = CPLGetXMLNode( psTree, "ColorTable" )->psChild;
             psEntry != NULL; psEntry = psEntry->psNext )
        {
            GDALColorEntry sCEntry;

            sCEntry.c1 = atoi(CPLGetXMLValue( psEntry, "c1", "0" ));
            sCEntry.c2 = atoi(CPLGetXMLValue( psEntry, "c2", "0" ));
            sCEntry.c3 = atoi(CPLGetXMLValue( psEntry, "c3", "0" ));
            sCEntry.c4 = atoi(CPLGetXMLValue( psEntry, "c4", "255" ));

            oTable.SetColorEntry( iEntry++, &sCEntry );
        }
        
        SetColorTable( &oTable );
    }

/* -------------------------------------------------------------------- */
/*      Process SimpleSources.                                          */
/* -------------------------------------------------------------------- */
    CPLXMLNode  *psChild;

    for( psChild = psTree->psChild; psChild != NULL; psChild = psChild->psNext)
    {
        if( EQUAL(psChild->pszValue,"AveragedSource") 
            || (EQUAL(psChild->pszValue,"SimpleSource")
                && EQUALN(CPLGetXMLValue(psChild, "Resampling", "Nearest"),
                          "Aver",4)) )
        {
            VRTSource * poSource = new VRTAveragedSource();
            poSource->XMLInit( psChild );
            AddSource( poSource );
        }
        else if( EQUAL(psChild->pszValue,"SimpleSource") )
        {
            VRTSource * poSource = new VRTSimpleSource();
            poSource->XMLInit( psChild );
            AddSource( poSource );
        }
        else if( EQUAL(psChild->pszValue,"ComplexSource") )
        {
            VRTSource * poSource = new VRTComplexSource();
            poSource->XMLInit( psChild );
            AddSource( poSource );
        }
    }

/* -------------------------------------------------------------------- */
/*      Done.                                                           */
/* -------------------------------------------------------------------- */
    if( nSources > 0 )
        return CE_None;
    else
        return CE_Failure;
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTRasterBand::SerializeToXML()

{
    CPLXMLNode *psTree;

    psTree = CPLCreateXMLNode( NULL, CXT_Element, "VRTRasterBand" );

/* -------------------------------------------------------------------- */
/*      Various kinds of metadata.                                      */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psMD;

    CPLSetXMLValue( psTree, "#dataType", 
                    GDALGetDataTypeName( GetRasterDataType() ) );

    if( nBand > 0 )
        CPLSetXMLValue( psTree, "#band", CPLSPrintf( "%d", GetBand() ) );

    psMD = VRTSerializeMetadata( this );
    if( psMD != NULL )
        CPLAddXMLChild( psTree, psMD );

    if( strlen(GetDescription()) > 0 )
        CPLSetXMLValue( psTree, "Description", GetDescription() );

    if( bNoDataValueSet )
        CPLSetXMLValue( psTree, "NoDataValue", 
                        CPLSPrintf( "%.14E", dfNoDataValue ) );

    if( eColorInterp != GCI_Undefined )
        CPLSetXMLValue( psTree, "ColorInterp", 
                        GDALGetColorInterpretationName( eColorInterp ) );

/* -------------------------------------------------------------------- */
/*      Color Table.                                                    */
/* -------------------------------------------------------------------- */
    if( poColorTable != NULL )
    {
        CPLXMLNode *psCT_XML = CPLCreateXMLNode( psTree, CXT_Element, 
                                                 "ColorTable" );

        for( int iEntry=0; iEntry < poColorTable->GetColorEntryCount(); 
             iEntry++ )
        {
            GDALColorEntry sEntry;
            CPLXMLNode *psEntry_XML = CPLCreateXMLNode( psCT_XML, CXT_Element, 
                                                        "Entry" );

            poColorTable->GetColorEntryAsRGB( iEntry, &sEntry );
            
            CPLSetXMLValue( psEntry_XML, "#c1", CPLSPrintf("%d",sEntry.c1) );
            CPLSetXMLValue( psEntry_XML, "#c2", CPLSPrintf("%d",sEntry.c2) );
            CPLSetXMLValue( psEntry_XML, "#c3", CPLSPrintf("%d",sEntry.c3) );
            CPLSetXMLValue( psEntry_XML, "#c4", CPLSPrintf("%d",sEntry.c4) );
        }
    }

/* -------------------------------------------------------------------- */
/*      Process SimpleSources.                                          */
/* -------------------------------------------------------------------- */
    for( int iSource = 0; iSource < nSources; iSource++ )
    {
        CPLXMLNode      *psXMLSrc;

        psXMLSrc = papoSources[iSource]->SerializeToXML();
        
        if( psXMLSrc != NULL )
            CPLAddXMLChild( psTree, psXMLSrc );
    }

    return psTree;
}

/************************************************************************/
/*                           SetNoDataValue()                           */
/************************************************************************/

CPLErr VRTRasterBand::SetNoDataValue( double dfNewValue )

{
    bNoDataValueSet = TRUE;
    dfNoDataValue = dfNewValue;
    
    ((VRTDataset *)poDS)->SetNeedsFlush();

    return CE_None;
}

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/

double VRTRasterBand::GetNoDataValue( int *pbSuccess )

{
    if( pbSuccess )
        *pbSuccess = bNoDataValueSet;

    return dfNoDataValue;
}

/************************************************************************/
/*                           SetColorTable()                            */
/************************************************************************/

CPLErr VRTRasterBand::SetColorTable( GDALColorTable *poTableIn )

{
    if( poColorTable != NULL )
    {
        delete poColorTable;
        poColorTable = NULL;
    }

    if( poTableIn )
    {
        poColorTable = poTableIn->Clone();
        eColorInterp = GCI_PaletteIndex;
    }

    ((VRTDataset *)poDS)->SetNeedsFlush();

    return CE_None;
}

/************************************************************************/
/*                           GetColorTable()                            */
/************************************************************************/

GDALColorTable *VRTRasterBand::GetColorTable()

{
    return poColorTable;
}

/************************************************************************/
/*                       SetColorInterpretation()                       */
/************************************************************************/

CPLErr VRTRasterBand::SetColorInterpretation( GDALColorInterp eInterpIn )

{
    ((VRTDataset *)poDS)->SetNeedsFlush();

    eColorInterp = eInterpIn;

    return CE_None;
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

GDALColorInterp VRTRasterBand::GetColorInterpretation()

{
    return eColorInterp;
}
