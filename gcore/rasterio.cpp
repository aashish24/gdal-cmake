/******************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
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
 * rasterio.cpp: This files exists to contain the default implementation
 *               of GDALRasterBand::IRasterIO() which is rather complex.
 *
 * $Log$
 * Revision 1.4  1999/01/11 15:38:38  warmerda
 * Added optimized case for simple 1:1 copies.
 *
 * Revision 1.3  1999/01/02 21:14:01  warmerda
 * Added write support
 *
 * Revision 1.2  1998/12/31 18:54:25  warmerda
 * Implement initial GDALRasterBlock support, and block cache
 *
 * Revision 1.1  1998/12/06 22:15:42  warmerda
 * New
 *
 */

#include "gdal_priv.h"

/************************************************************************/
/*                             IRasterIO()                              */
/*                                                                      */
/*      Default internal implementation of RasterIO() ... utilizes      */
/*      the Block access methods to satisfy the request.  This would    */
/*      normally only be overridden by formats with overviews.          */
/************************************************************************/

CPLErr GDALRasterBand::IRasterIO( GDALRWFlag eRWFlag,
                                  int nXOff, int nYOff, int nXSize, int nYSize,
                                  void * pData, int nBufXSize, int nBufYSize,
                                  GDALDataType eBufType,
                                  int nPixelSpace, int nLineSpace )

{
    int		nBandDataSize = GDALGetDataTypeSize( eDataType ) / 8;
    GByte	*pabySrcBlock = NULL;
    GDALRasterBlock *poBlock;
    int		nLBlockX=-1, nLBlockY=-1, iBufYOff, iBufXOff, iSrcY;

/* ==================================================================== */
/*      A common case is the data requested with it's inherent data     */
/*      type, the destination is packed, and the block width is the     */
/*      raster width.                                                   */
/* ==================================================================== */
    if( eBufType == eDataType
        && nPixelSpace == GDALGetDataTypeSize(eBufType)/8
        && nLineSpace == nPixelSpace * nXSize
        && nBlockXSize == poDS->GetRasterXSize()
        && nBufXSize == nXSize )
    {
        for( iBufYOff = 0; iBufYOff < nBufYSize; iBufYOff++ )
        {
            int		nSrcByteOffset;
            
            iSrcY = iBufYOff + nYOff;
            
            if( iSrcY < nLBlockY * nBlockYSize
                || iSrcY >= (nLBlockY+1) * nBlockYSize )
            {
                nLBlockY = iSrcY / nBlockYSize;

                poBlock = GetBlockRef( 0, nLBlockY );
                if( poBlock == NULL )
                {
                    return( CE_Failure );
                }

                if( eRWFlag == GF_Write )
                    poBlock->MarkDirty();
                
                pabySrcBlock = (GByte *) poBlock->GetDataRef();
            }

            nSrcByteOffset = ((iSrcY-nLBlockY*nBlockYSize)*nBlockXSize + nXOff)
                * nPixelSpace;
            
            if( eRWFlag == GF_Write )
                memcpy( pabySrcBlock + nSrcByteOffset, 
                        ((GByte *) pData) + iBufYOff * nLineSpace,
                        nLineSpace );
            else
                memcpy( ((GByte *) pData) + iBufYOff * nLineSpace,
                        pabySrcBlock + nSrcByteOffset, 
                        nLineSpace );
        }

        return CE_None;
    }
    
    
/* ==================================================================== */
/*      Loop reading required source blocks to satisfy output           */
/*      request.  This is the most general implementation.              */
/* ==================================================================== */

/* -------------------------------------------------------------------- */
/*      Compute stepping increment.                                     */
/* -------------------------------------------------------------------- */
    double	dfSrcX, dfSrcY, dfSrcXInc, dfSrcYInc;
    int		iSrcX;
    
    dfSrcXInc = nXSize / (double) nBufXSize;
    dfSrcYInc = nYSize / (double) nBufYSize;

/* -------------------------------------------------------------------- */
/*      Loop over buffer computing source locations.                    */
/* -------------------------------------------------------------------- */
    for( iBufYOff = 0; iBufYOff < nBufYSize; iBufYOff++ )
    {
        int	iBufOffset, iSrcOffset;
        
        dfSrcY = (iBufYOff+0.5) * dfSrcYInc + nYOff;
        iSrcY = (int) dfSrcY;

        iBufOffset = iBufYOff * nLineSpace;
        
        for( iBufXOff = 0; iBufXOff < nBufXSize; iBufXOff++ )
        {
            dfSrcX = (iBufXOff+0.5) * dfSrcXInc + nXOff;
            
            iSrcX = (int) dfSrcX;

/* -------------------------------------------------------------------- */
/*      Ensure we have the appropriate block loaded.                    */
/* -------------------------------------------------------------------- */
            if( iSrcX < nLBlockX * nBlockXSize
                || iSrcX >= (nLBlockX+1) * nBlockXSize
                || iSrcY < nLBlockY * nBlockYSize
                || iSrcY >= (nLBlockY+1) * nBlockYSize )
            {
                nLBlockX = iSrcX / nBlockXSize;
                nLBlockY = iSrcY / nBlockYSize;

                poBlock = GetBlockRef( nLBlockX, nLBlockY );
                if( poBlock == NULL )
                {
                    return( CE_Failure );
                }

                if( eRWFlag == GF_Write )
                    poBlock->MarkDirty();
                
                pabySrcBlock = (GByte *) poBlock->GetDataRef();
            }

/* -------------------------------------------------------------------- */
/*      Copy over this pixel of data.                                   */
/* -------------------------------------------------------------------- */
            iSrcOffset = (iSrcX - nLBlockX*nBlockXSize
                + (iSrcY - nLBlockY*nBlockYSize) * nBlockXSize)*nBandDataSize;

            if( eDataType == eBufType )
            {
                if( eRWFlag == GF_Read )
                    memcpy( ((GByte *) pData) + iBufOffset,
                            pabySrcBlock + iSrcOffset, nBandDataSize );
                else
                    memcpy( pabySrcBlock + iSrcOffset, 
                            ((GByte *) pData) + iBufOffset, nBandDataSize );
            }
            else
            {
                /* type to type translation */
                CPLAssert( FALSE );
            }

            iBufOffset += nPixelSpace;
        }
    }

    return( CE_None );
}

