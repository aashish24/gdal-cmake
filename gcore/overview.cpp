/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Helper code to implement overview support in different drivers.
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
 * Revision 1.3  2000/07/17 17:08:45  warmerda
 * added support for complex data
 *
 * Revision 1.2  2000/06/26 22:17:58  warmerda
 * added scaled progress support
 *
 * Revision 1.1  2000/04/21 21:54:05  warmerda
 * New
 *
 */

#include "gdal_priv.h"

/************************************************************************/
/*                       GDALDownsampleChunk32R()                       */
/************************************************************************/

static CPLErr
GDALDownsampleChunk32R( int nSrcWidth, int nSrcHeight, 
                        float * pafChunk, int nChunkYOff, int nChunkYSize,
                        GDALRasterBand * poOverview,
                        const char * pszResampling )

{
    int      nDstYOff, nDstYOff2, nOXSize, nOYSize;
    float    *pafDstScanline;

    nOXSize = poOverview->GetXSize();
    nOYSize = poOverview->GetYSize();

    pafDstScanline = (float *) CPLMalloc(nOXSize * sizeof(float));

/* -------------------------------------------------------------------- */
/*      Figure out the line to start writing to, and the first line     */
/*      to not write to.  In theory this approach should ensure that    */
/*      every output line will be written if all input chunks are       */
/*      processed.                                                      */
/* -------------------------------------------------------------------- */
    nDstYOff = (int) (0.5 + (nChunkYOff/(double)nSrcHeight) * nOYSize);
    nDstYOff2 = (int) 
        (0.5 + ((nChunkYOff+nChunkYSize)/(double)nSrcHeight) * nOYSize);

    if( nChunkYOff + nChunkYSize == nSrcHeight )
        nDstYOff2 = nOYSize;
    
/* ==================================================================== */
/*      Loop over destination scanlines.                                */
/* ==================================================================== */
    for( int iDstLine = nDstYOff; iDstLine < nDstYOff2; iDstLine++ )
    {
        float *pafSrcScanline;
        int   nSrcYOff, nSrcYOff2, iDstPixel;

        nSrcYOff = (int) (0.5 + (iDstLine/(double)nOYSize) * nSrcHeight);
        if( nSrcYOff < nChunkYOff )
            nSrcYOff = nChunkYOff;
        
        nSrcYOff2 = (int) (0.5 + ((iDstLine+1)/(double)nOYSize) * nSrcHeight);
        if( nSrcYOff2 > nSrcHeight || iDstLine == nOYSize-1 )
            nSrcYOff2 = nSrcHeight;
        if( nSrcYOff2 > nChunkYOff + nChunkYSize )
            nSrcYOff2 = nChunkYOff + nChunkYSize;

        pafSrcScanline = pafChunk + ((nSrcYOff-nChunkYOff) * nSrcWidth);

/* -------------------------------------------------------------------- */
/*      Loop over destination pixels                                    */
/* -------------------------------------------------------------------- */
        for( iDstPixel = 0; iDstPixel < nOXSize; iDstPixel++ )
        {
            int   nSrcXOff, nSrcXOff2;

            nSrcXOff = (int) (0.5 + (iDstPixel/(double)nOXSize) * nSrcWidth);
            nSrcXOff2 = (int) 
                (0.5 + ((iDstPixel+1)/(double)nOXSize) * nSrcWidth);
            if( nSrcXOff2 > nSrcWidth )
                nSrcXOff2 = nSrcWidth;
            
            if( EQUALN(pszResampling,"NEAR",4) )
            {
                pafDstScanline[iDstPixel] = pafSrcScanline[nSrcXOff];
            }
            else if( EQUALN(pszResampling,"AVER",4) )
            {
                double dfTotal = 0.0;
                int    nCount = 0, iX, iY;

                for( iY = nSrcYOff; iY < nSrcYOff2; iY++ )
                {
                    for( iX = nSrcXOff; iX < nSrcXOff2; iX++ )
                    {
                        dfTotal += pafSrcScanline[iX+(iY-nSrcYOff)*nSrcWidth];
                        nCount++;
                    }
                }
                
                CPLAssert( nCount > 0 );
                if( nCount == 0 )
                {
                    pafDstScanline[iDstPixel] = 0.0;
                }
                else
                    pafDstScanline[iDstPixel] = dfTotal / nCount;
            }
        }

        poOverview->RasterIO( GF_Write, 0, iDstLine, nOXSize, 1, 
                              pafDstScanline, nOXSize, 1, GDT_Float32, 
                              0, 0 );
    }

    CPLFree( pafDstScanline );

    return CE_None;
}

/************************************************************************/
/*                       GDALDownsampleChunkC32R()                      */
/************************************************************************/

static CPLErr
GDALDownsampleChunkC32R( int nSrcWidth, int nSrcHeight, 
                         float * pafChunk, int nChunkYOff, int nChunkYSize,
                         GDALRasterBand * poOverview,
                         const char * pszResampling )
    
{
    int      nDstYOff, nDstYOff2, nOXSize, nOYSize;
    float    *pafDstScanline;

    nOXSize = poOverview->GetXSize();
    nOYSize = poOverview->GetYSize();

    pafDstScanline = (float *) CPLMalloc(nOXSize * sizeof(float) * 2);

/* -------------------------------------------------------------------- */
/*      Figure out the line to start writing to, and the first line     */
/*      to not write to.  In theory this approach should ensure that    */
/*      every output line will be written if all input chunks are       */
/*      processed.                                                      */
/* -------------------------------------------------------------------- */
    nDstYOff = (int) (0.5 + (nChunkYOff/(double)nSrcHeight) * nOYSize);
    nDstYOff2 = (int) 
        (0.5 + ((nChunkYOff+nChunkYSize)/(double)nSrcHeight) * nOYSize);

    if( nChunkYOff + nChunkYSize == nSrcHeight )
        nDstYOff2 = nOYSize;
    
/* ==================================================================== */
/*      Loop over destination scanlines.                                */
/* ==================================================================== */
    for( int iDstLine = nDstYOff; iDstLine < nDstYOff2; iDstLine++ )
    {
        float *pafSrcScanline;
        int   nSrcYOff, nSrcYOff2, iDstPixel;

        nSrcYOff = (int) (0.5 + (iDstLine/(double)nOYSize) * nSrcHeight);
        if( nSrcYOff < nChunkYOff )
            nSrcYOff = nChunkYOff;
        
        nSrcYOff2 = (int) (0.5 + ((iDstLine+1)/(double)nOYSize) * nSrcHeight);
        if( nSrcYOff2 > nSrcHeight || iDstLine == nOYSize-1 )
            nSrcYOff2 = nSrcHeight;
        if( nSrcYOff2 > nChunkYOff + nChunkYSize )
            nSrcYOff2 = nChunkYOff + nChunkYSize;

        pafSrcScanline = pafChunk + ((nSrcYOff-nChunkYOff) * nSrcWidth);

/* -------------------------------------------------------------------- */
/*      Loop over destination pixels                                    */
/* -------------------------------------------------------------------- */
        for( iDstPixel = 0; iDstPixel < nOXSize; iDstPixel++ )
        {
            int   nSrcXOff, nSrcXOff2;

            nSrcXOff = (int) (0.5 + (iDstPixel/(double)nOXSize) * nSrcWidth);
            nSrcXOff2 = (int) 
                (0.5 + ((iDstPixel+1)/(double)nOXSize) * nSrcWidth);
            if( nSrcXOff2 > nSrcWidth )
                nSrcXOff2 = nSrcWidth;
            
            if( EQUALN(pszResampling,"NEAR",4) )
            {
                pafDstScanline[iDstPixel*2] = pafSrcScanline[nSrcXOff*2];
                pafDstScanline[iDstPixel*2+1] = pafSrcScanline[nSrcXOff*2+1];
            }
            else if( EQUALN(pszResampling,"AVER",4) )
            {
                double dfTotalR = 0.0, dfTotalI = 0.0;
                int    nCount = 0, iX, iY;

                for( iY = nSrcYOff; iY < nSrcYOff2; iY++ )
                {
                    for( iX = nSrcXOff; iX < nSrcXOff2; iX++ )
                    {
                        dfTotalR += pafSrcScanline[iX*2+(iY-nSrcYOff)*nSrcWidth*2];
                        dfTotalI += pafSrcScanline[iX*2+(iY-nSrcYOff)*nSrcWidth*2+1];
                        nCount++;
                    }
                }
                
                CPLAssert( nCount > 0 );
                if( nCount == 0 )
                {
                    pafDstScanline[iDstPixel*2] = 0.0;
                    pafDstScanline[iDstPixel*2+1] = 0.0;
                }
                else
                {
                    pafDstScanline[iDstPixel*2  ] = dfTotalR / nCount;
                    pafDstScanline[iDstPixel*2+1] = dfTotalI / nCount;
                }
            }
        }

        poOverview->RasterIO( GF_Write, 0, iDstLine, nOXSize, 1, 
                              pafDstScanline, nOXSize, 1, GDT_CFloat32, 
                              0, 0 );
    }

    CPLFree( pafDstScanline );

    return CE_None;
}


/************************************************************************/
/*                      GDALRegenerateOverviews()                       */
/************************************************************************/
CPLErr 
GDALRegenerateOverviews( GDALRasterBand *poSrcBand,
                         int nOverviews, GDALRasterBand **papoOvrBands, 
                         const char * pszResampling, 
                         GDALProgressFunc pfnProgress, void * pProgressData )

{
    int    nFullResYChunk, nWidth;
    int    nFRXBlockSize, nFRYBlockSize;
    GDALDataType eType;

/* -------------------------------------------------------------------- */
/*      Setup one horizontal swath to read from the raw buffer.         */
/* -------------------------------------------------------------------- */
    float *pafChunk;

    poSrcBand->GetBlockSize( &nFRXBlockSize, &nFRYBlockSize );
    
    if( nFRYBlockSize < 4 || nFRYBlockSize > 256 )
        nFullResYChunk = 32;
    else
        nFullResYChunk = nFRYBlockSize;

    if( GDALDataTypeIsComplex( poSrcBand->GetRasterDataType() ) )
        eType = GDT_CFloat32;
    else
        eType = GDT_Float32;

    nWidth = poSrcBand->GetXSize();
    pafChunk = (float *) 
        VSIMalloc((GDALGetDataTypeSize(eType)/8) * nFullResYChunk * nWidth );

    if( pafChunk == NULL )
    {
        CPLError( CE_Failure, CPLE_OutOfMemory, 
                  "Out of memory in GDALRegenerateOverviews()." );

        return CE_Failure;
    }
    
/* -------------------------------------------------------------------- */
/*      Loop over image operating on chunks.                            */
/* -------------------------------------------------------------------- */
    int  nChunkYOff = 0;

    for( nChunkYOff = 0; 
         nChunkYOff < poSrcBand->GetYSize(); 
         nChunkYOff += nFullResYChunk )
    {
        if( !pfnProgress( nChunkYOff / (double) poSrcBand->GetYSize(), 
                          NULL, pProgressData ) )
        {
            CPLError( CE_Failure, CPLE_UserInterrupt, "User terminated" );
            return CE_Failure;
        }

        if( nFullResYChunk + nChunkYOff > poSrcBand->GetYSize() )
            nFullResYChunk = poSrcBand->GetYSize() - nChunkYOff;
        
        /* read chunk */
        poSrcBand->RasterIO( GF_Read, 0, nChunkYOff, nWidth, nFullResYChunk, 
                             pafChunk, nWidth, nFullResYChunk, eType,
                             0, 0 );
        
        for( int iOverview = 0; iOverview < nOverviews; iOverview++ )
        {
            if( eType == GDT_Float32 )
                GDALDownsampleChunk32R(nWidth, poSrcBand->GetYSize(), 
                                       pafChunk, nChunkYOff, nFullResYChunk,
                                       papoOvrBands[iOverview], pszResampling);
            else
                GDALDownsampleChunkC32R(nWidth, poSrcBand->GetYSize(), 
                                       pafChunk, nChunkYOff, nFullResYChunk,
                                       papoOvrBands[iOverview], pszResampling);
        }
    }

    VSIFree( pafChunk );
    
/* -------------------------------------------------------------------- */
/*      It can be important to flush out data to overviews.             */
/* -------------------------------------------------------------------- */
    for( int iOverview = 0; iOverview < nOverviews; iOverview++ )
        papoOvrBands[iOverview]->FlushCache();

    pfnProgress( 1.0, NULL, pProgressData );

    return CE_None;
}
