/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Base class for format specific band class implementation.  This
 *           base class provides default implementation for many methods.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
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
 * $Log$
 * Revision 1.36  2003/02/20 18:34:12  warmerda
 * added GDALGetRasterAccess()
 *
 * Revision 1.35  2002/12/18 15:19:26  warmerda
 * added errors in some unimplemented methods
 *
 * Revision 1.34  2002/11/11 16:02:06  dron
 * More error messages added.
 *
 * Revision 1.33  2002/10/17 17:55:31  warmerda
 * Minor improvement to RasterIO() docs.
 *
 * Revision 1.32  2002/09/15 15:32:50  dron
 * Added InitBlockInfo() call to ReadBlock() and WriteBlock() functions.
 *
 * Revision 1.31  2002/08/14 12:33:34  warmerda
 * Validate eRWFlag.
 *
 * Revision 1.30  2002/07/09 20:33:12  warmerda
 * expand tabs
 *
 * Revision 1.29  2002/05/29 15:58:26  warmerda
 * removed GetDescription(), added SetColorInterpretation()
 *
 * Revision 1.28  2002/05/28 18:55:08  warmerda
 * added GetDataset()
 *
 * Revision 1.27  2002/03/01 14:29:09  warmerda
 * added GetBand() method on GDALRasterBand
 *
 * Revision 1.26  2001/12/07 15:29:45  warmerda
 * added InitBlockInfo() in GetHistogram()
 *
 * Revision 1.25  2001/10/18 14:35:22  warmerda
 * avoid conflicts between parameters and member data
 *
 * Revision 1.24  2001/10/17 16:20:45  warmerda
 * make histograming work with complex data (r(treat as magnitude)
 *
 * Revision 1.23  2001/07/18 04:04:30  warmerda
 * added CPL_CVSID
 *
 * Revision 1.22  2001/07/05 13:13:40  warmerda
 * added UnitType from C support
 *
 * Revision 1.21  2000/10/06 15:25:48  warmerda
 * added setnodata, and some other methods
 *
 * Revision 1.20  2000/08/25 14:26:51  warmerda
 * added GDALHasArbitraryOverviews
 *
 * Revision 1.19  2000/08/16 15:50:52  warmerda
 * fixed some bugs with floating (datasetless) bands
 *
 * Revision 1.18  2000/07/12 00:19:29  warmerda
 * Removed extra line feed.
 *
 * Revision 1.17  2000/06/05 17:24:05  warmerda
 * added real complex support
 *
 * Revision 1.16  2000/04/21 21:56:59  warmerda
 * moved metadata to GDALMajorObject
 *
 * Revision 1.15  2000/03/31 13:42:27  warmerda
 * added metadata support
 *
 * Revision 1.14  2000/03/24 00:09:05  warmerda
 * rewrote cache management
 *
 * Revision 1.13  2000/03/10 13:54:37  warmerda
 * fixed use of overviews in gethistogram
 *
 * Revision 1.12  2000/03/09 23:22:03  warmerda
 * added GetHistogram
 *
 * Revision 1.11  2000/03/08 19:59:16  warmerda
 * added GDALFlushRasterCache
 *
 * Revision 1.10  2000/03/06 21:50:37  warmerda
 * added min/max support
 *
 * Revision 1.9  2000/03/06 02:22:01  warmerda
 * added overviews, colour tables, and many other methods
 *
 * Revision 1.8  2000/02/28 16:34:28  warmerda
 * added arg window check in RasterIO()
 *
 * Revision 1.7  1999/11/17 16:18:10  warmerda
 * fixed example code
 *
 * Revision 1.6  1999/10/21 13:24:37  warmerda
 * Fixed some build breaking variable name differences.
 *
 * Revision 1.5  1999/10/01 14:44:02  warmerda
 * added documentation
 *
 * Revision 1.4  1998/12/31 18:54:25  warmerda
 * Implement initial GDALRasterBlock support, and block cache
 *
 * Revision 1.3  1998/12/06 22:17:09  warmerda
 * Fill out rasterio support.
 *
 * Revision 1.2  1998/12/06 02:52:08  warmerda
 * Added new methods, and C cover functions.
 *
 * Revision 1.1  1998/12/03 18:32:01  warmerda
 * New
 */

#include "gdal_priv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           GDALRasterBand()                           */
/************************************************************************/

/*! Constructor. Applications should never create GDALRasterBands directly. */

GDALRasterBand::GDALRasterBand()

{
    poDS = NULL;
    nBand = 0;

    eAccess = GA_ReadOnly;
    nBlockXSize = nBlockYSize = -1;
    eDataType = GDT_Byte;

    nBlocksPerRow = 0;
    nBlocksPerColumn = 0;

    papoBlocks = NULL;
}

/************************************************************************/
/*                          ~GDALRasterBand()                           */
/************************************************************************/

/*! Destructor. Applications should never destroy GDALRasterBands directly,
    instead destroy the GDALDataset. */

GDALRasterBand::~GDALRasterBand()

{
    FlushCache();
    
    CPLFree( papoBlocks );
}

/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

/**
 * Read/write a region of image data for this band.
 *
 * This method allows reading a region of a GDALRasterBand into a buffer,
 * or writing data from a buffer into a region of a GDALRasterBand.  It
 * automatically takes care of data type translation if the data type
 * (eBufType) of the buffer is different than that of the GDALRasterBand.
 * The method also takes care of image decimation / replication if the
 * buffer size (nBufXSize x nBufYSize) is different than the size of the
 * region being accessed (nXSize x nYSize).
 *
 * The nPixelSpace and nLineSpace parameters allow reading into or
 * writing from unusually organized buffers.  This is primarily used
 * for buffers containing more than one bands raster data in interleaved
 * format. 
 *
 * Some formats may efficiently implement decimation into a buffer by
 * reading from lower resolution overview images.
 *
 * For highest performance full resolution data access, read and write
 * on "block boundaries" as returned by GetBlockSize(), or use the
 * ReadBlock() and WriteBlock() methods.
 *
 * This method is the same as the C GDALRasterIO() function.
 *
 * @param eRWFlag Either GF_Read to read a region of data, or GT_Write to
 * write a region of data.
 *
 * @param nXOff The pixel offset to the top left corner of the region
 * of the band to be accessed.  This would be zero to start from the left side.
 *
 * @param nYOff The line offset to the top left corner of the region
 * of the band to be accessed.  This would be zero to start from the top.
 *
 * @param nXSize The width of the region of the band to be accessed in pixels.
 *
 * @param nYSize The height of the region of the band to be accessed in lines.
 *
 * @param pData The buffer into which the data should be read, or from which
 * it should be written.  This buffer must contain at least nBufXSize *
 * nBufYSize words of type eBufType.  It is organized in left to right,
 * top to bottom pixel order.  Spacing is controlled by the nPixelSpace,
 * and nLineSpace parameters.
 *
 * @param nBufXSize the width of the buffer image into which the desired region is
 * to be read, or from which it is to be written.
 *
 * @param nBufYSize the height of the buffer image into which the desired region is
 * to be read, or from which it is to be written.
 *
 * @param eBufType the type of the pixel values in the pData data buffer.  The
 * pixel values will automatically be translated to/from the GDALRasterBand
 * data type as needed.
 *
 * @param nPixelSpace The byte offset from the start of one pixel value in
 * pData to the start of the next pixel value within a scanline.  If defaulted
 * (0) the size of the datatype eBufType is used.
 *
 * @param nLineSpace The byte offset from the start of one scanline in
 * pData to the start of the next.  If defaulted the size of the datatype
 * eBufType * nBufXSize is used.
 *
 * @return CE_Failure if the access fails, otherwise CE_None.
 */

CPLErr GDALRasterBand::RasterIO( GDALRWFlag eRWFlag,
                                 int nXOff, int nYOff, int nXSize, int nYSize,
                                 void * pData, int nBufXSize, int nBufYSize,
                                 GDALDataType eBufType,
                                 int nPixelSpace,
                                 int nLineSpace )

{
/* -------------------------------------------------------------------- */
/*      If pixel and line spaceing are defaulted assign reasonable      */
/*      value assuming a packed buffer.                                 */
/* -------------------------------------------------------------------- */
    if( nPixelSpace == 0 )
        nPixelSpace = GDALGetDataTypeSize( eBufType ) / 8;
    
    if( nLineSpace == 0 )
        nLineSpace = nPixelSpace * nBufXSize;
    
/* -------------------------------------------------------------------- */
/*      Do some validation of parameters.                               */
/* -------------------------------------------------------------------- */
    if( nXOff < 0 || nXOff + nXSize > nRasterXSize
        || nYOff < 0 || nYOff + nYSize > nRasterYSize )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "Access window out of range in RasterIO().  Requested\n"
                  "(%d,%d) of size %dx%d on raster of %dx%d.",
                  nXOff, nYOff, nXSize, nYSize, nRasterXSize, nRasterYSize );
        return CE_Failure;
    }

    if( eRWFlag != GF_Read && eRWFlag != GF_Write )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "eRWFlag = %d, only GF_Read (0) and GF_Write (1) are legal.",
                  eRWFlag );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Some size values are "noop".  Lets just return to avoid         */
/*      stressing lower level functions.                                */
/* -------------------------------------------------------------------- */
    if( nXSize < 1 || nYSize < 1 || nBufXSize < 1 || nBufYSize < 1 )
    {
        CPLDebug( "GDAL", 
                  "RasterIO() skipped for odd window or buffer size.\n"
                  "  Window = (%d,%d)x%dx%d\n"
                  "  Buffer = %dx%d\n",
                  nXOff, nYOff, nXSize, nYSize, 
                  nBufXSize, nBufYSize );

        return CE_None;
    }
    
/* -------------------------------------------------------------------- */
/*      Call the format specific function.                              */
/* -------------------------------------------------------------------- */
    return( IRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize,
                       pData, nBufXSize, nBufYSize, eBufType,
                       nPixelSpace, nLineSpace ) );
}

/************************************************************************/
/*                            GDALRasterIO()                            */
/************************************************************************/

CPLErr GDALRasterIO( GDALRasterBandH hBand, GDALRWFlag eRWFlag,
                     int nXOff, int nYOff,
                     int nXSize, int nYSize,
                     void * pData,
                     int nBufXSize, int nBufYSize,
                     GDALDataType eBufType,
                     int nPixelSpace,
                     int nLineSpace )

{
    GDALRasterBand      *poBand = (GDALRasterBand *) hBand;

    return( poBand->RasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize,
                              pData, nBufXSize, nBufYSize, eBufType,
                              nPixelSpace, nLineSpace ) );
}
                     
/************************************************************************/
/*                             ReadBlock()                              */
/************************************************************************/

/**
 * Read a block of image data efficiently.
 *
 * This method accesses a "natural" block from the raster band without
 * resampling, or data type conversion.  For a more generalized, but
 * potentially less efficient access use RasterIO().
 *
 * This method is the same as the C GDALReadBlock() function.
 *
 * See the GetBlockRef() method for a way of accessing internally cached
 * block oriented data without an extra copy into an application buffer.
 *
 * @param nXBlockOff the horizontal block offset, with zero indicating
 * the left most block, 1 the next block and so forth. 
 *
 * @param nYBlockOff the vertical block offset, with zero indicating
 * the left most block, 1 the next block and so forth.
 *
 * @param pImage the buffer into which the data will be read.  The buffer
 * must be large enough to hold GetBlockXSize()*GetBlockYSize() words
 * of type GetRasterDataType().
 *
 * @return CE_None on success or CE_Failure on an error.
 *
 * The following code would efficiently compute a histogram of eight bit
 * raster data.  Note that the final block may be partial ... data beyond
 * the edge of the underlying raster band in these edge blocks is of an
 * undermined value.
 *
<pre>
 CPLErr GetHistogram( GDALRasterBand *poBand, int *panHistogram )

 {
     int        nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
     int        iXBlock, iYBlock;

     memset( panHistogram, 0, sizeof(int) * 256 );

     CPLAssert( poBand->GetRasterDataType() == GDT_Byte );

     poBand->GetBlockSize( &nXBlockSize, &nYBlockSize );
     nXBlocks = (poBand->GetXSize() + nXBlockSize - 1) / nXBlockSize;
     nYBlocks = (poBand->GetYSize() + nYBlockSize - 1) / nYBlockSize;

     pabyData = (GByte *) CPLMalloc(nXBlockSize * nYBlockSize);

     for( iYBlock = 0; iYBlock < nYBlocks; iYBlock++ )
     {
         for( iXBlock = 0; iXBlock < nXBlocks; iXBlock++ )
         {
             int        nXValid, nYValid;
             
             poBand->ReadBlock( iXBlock, iYBlock, pabyData );

             // Compute the portion of the block that is valid
             // for partial edge blocks.
             if( iXBlock * nXBlockSize > poBand->GetXSize() )
                 nXValid = poBand->GetXSize() - iXBlock * nXBlockSize;
             else
                 nXValid = nXBlockSize;

             if( iYBlock * nYBlockSize > poBand->GetYSize() )
                 nYValid = poBand->GetXSize() - iYBlock * nYBlockSize;
             else
                 nYValid = nYBlockSize;

             // Collect the histogram counts.
             for( int iY = 0; iY < nXValid; iY++ )
             {
                 for( int iX = 0; iX < nXValid; iX++ )
                 {
                     pabyHistogram[pabyData[iX + iY * nXBlockSize]] += 1;
                 }
             }
         }
     }
 }
 
</pre>
 */


CPLErr GDALRasterBand::ReadBlock( int nXBlockOff, int nYBlockOff,
                                   void * pImage )

{
/* -------------------------------------------------------------------- */
/*      Validate arguments.                                             */
/* -------------------------------------------------------------------- */
    CPLAssert( pImage != NULL );
    
    if( nXBlockOff < 0
        || nXBlockOff*nBlockXSize >= nRasterXSize )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "Illegal nXBlockOff value (%d) in "
                        "GDALRasterBand::ReadBlock()\n",
                  nXBlockOff );

        return( CE_Failure );
    }

    if( nYBlockOff < 0
        || nYBlockOff*nBlockYSize >= nRasterYSize )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "Illegal nYBlockOff value (%d) in "
                        "GDALRasterBand::ReadBlock()\n",
                  nYBlockOff );

        return( CE_Failure );
    }
    
    InitBlockInfo();

/* -------------------------------------------------------------------- */
/*      Invoke underlying implementation method.                        */
/* -------------------------------------------------------------------- */
    return( IReadBlock( nXBlockOff, nYBlockOff, pImage ) );
}

/************************************************************************/
/*                           GDALReadBlock()                            */
/************************************************************************/

CPLErr GDALReadBlock( GDALRasterBandH hBand, int nXOff, int nYOff,
                      void * pData )

{
    GDALRasterBand      *poBand = (GDALRasterBand *) hBand;

    return( poBand->ReadBlock( nXOff, nYOff, pData ) );
}

/************************************************************************/
/*                            IWriteBlock()                             */
/*                                                                      */
/*      Default internal implementation ... to be overriden by          */
/*      subclasses that support writing.                                */
/************************************************************************/

CPLErr GDALRasterBand::IWriteBlock( int, int, void * )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "WriteBlock() not supported for this dataset." );
    
    return( CE_Failure );
}

/************************************************************************/
/*                             WriteBlock()                             */
/************************************************************************/

/**
 * Write a block of image data efficiently.
 *
 * This method accesses a "natural" block from the raster band without
 * resampling, or data type conversion.  For a more generalized, but
 * potentially less efficient access use RasterIO().
 *
 * This method is the same as the C GDALWriteBlock() function.
 *
 * See ReadBlock() for an example of block oriented data access.
 *
 * @param nXBlockOff the horizontal block offset, with zero indicating
 * the left most block, 1 the next block and so forth. 
 *
 * @param nYBlockOff the vertical block offset, with zero indicating
 * the left most block, 1 the next block and so forth.
 *
 * @param pImage the buffer from which the data will be written.  The buffer
 * must be large enough to hold GetBlockXSize()*GetBlockYSize() words
 * of type GetRasterDataType().
 *
 * @return CE_None on success or CE_Failure on an error.
 *
 * The following code would efficiently compute a histogram of eight bit
 * raster data.  Note that the final block may be partial ... data beyond
 * the edge of the underlying raster band in these edge blocks is of an
 * undermined value.
 *
 */

CPLErr GDALRasterBand::WriteBlock( int nXBlockOff, int nYBlockOff,
                                   void * pImage )

{
/* -------------------------------------------------------------------- */
/*      Validate arguments.                                             */
/* -------------------------------------------------------------------- */
    CPLAssert( pImage != NULL );
    
    if( nXBlockOff < 0
        || nXBlockOff*nBlockXSize >= GetXSize() )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "Illegal nXBlockOff value (%d) in "
                        "GDALRasterBand::WriteBlock()\n",
                  nXBlockOff );

        return( CE_Failure );
    }

    if( nYBlockOff < 0
        || nYBlockOff*nBlockYSize >= GetYSize() )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "Illegal nYBlockOff value (%d) in "
                        "GDALRasterBand::WriteBlock()\n",
                  nYBlockOff );

        return( CE_Failure );
    }

    if( eAccess == GA_ReadOnly )
    {
        CPLError( CE_Failure, CPLE_NoWriteAccess,
                  "Attempt to write to read only dataset in"
                  "GDALRasterBand::WriteBlock().\n" );

        return( CE_Failure );
    }

    InitBlockInfo();
    
/* -------------------------------------------------------------------- */
/*      Invoke underlying implementation method.                        */
/* -------------------------------------------------------------------- */
    return( IWriteBlock( nXBlockOff, nYBlockOff, pImage ) );
}

/************************************************************************/
/*                           GDALWriteBlock()                           */
/************************************************************************/

CPLErr GDALWriteBlock( GDALRasterBandH hBand, int nXOff, int nYOff,
                       void * pData )

{
    GDALRasterBand      *poBand = (GDALRasterBand *) hBand;

    return( poBand->WriteBlock( nXOff, nYOff, pData ) );
}


/************************************************************************/
/*                         GetRasterDataType()                          */
/************************************************************************/

/**
 * Fetch the pixel data type for this band.
 *
 * @return the data type of pixels for this band.
 */
  

GDALDataType GDALRasterBand::GetRasterDataType()

{
    return eDataType;
}

/************************************************************************/
/*                       GDALGetRasterDataType()                        */
/************************************************************************/

GDALDataType GDALGetRasterDataType( GDALRasterBandH hBand )

{
    return( ((GDALRasterBand *) hBand)->GetRasterDataType() );
}

/************************************************************************/
/*                            GetBlockSize()                            */
/************************************************************************/

/**
 * Fetch the "natural" block size of this band.
 *
 * GDAL contains a concept of the natural block size of rasters so that
 * applications can organized data access efficiently for some file formats.
 * The natural block size is the block size that is most efficient for
 * accessing the format.  For many formats this is simple a whole scanline
 * in which case *pnXSize is set to GetXSize(), and *pnYSize is set to 1.
 *
 * However, for tiled images this will typically be the tile size.
 *
 * Note that the X and Y block sizes don't have to divide the image size
 * evenly, meaning that right and bottom edge blocks may be incomplete.
 * See ReadBlock() for an example of code dealing with these issues.
 *
 * @param pnXSize integer to put the X block size into or NULL.
 *
 * @param pnYSize integer to put the Y block size into or NULL.
 */

void GDALRasterBand::GetBlockSize( int * pnXSize, int *pnYSize )

{
    CPLAssert( nBlockXSize > 0 && nBlockYSize > 0 );
    
    if( pnXSize != NULL )
        *pnXSize = nBlockXSize;
    if( pnYSize != NULL )
        *pnYSize = nBlockYSize;
}

/************************************************************************/
/*                          GDALGetBlockSize()                          */
/************************************************************************/

void GDALGetBlockSize( GDALRasterBandH hBand, int * pnXSize, int * pnYSize )

{
    GDALRasterBand      *poBand = (GDALRasterBand *) hBand;

    poBand->GetBlockSize( pnXSize, pnYSize );
}

/************************************************************************/
/*                           InitBlockInfo()                            */
/************************************************************************/

void GDALRasterBand::InitBlockInfo()

{
    if( papoBlocks != NULL )
        return;

    CPLAssert( nBlockXSize > 0 && nBlockYSize > 0 );
    
    nBlocksPerRow = (nRasterXSize+nBlockXSize-1) / nBlockXSize;
    nBlocksPerColumn = (nRasterYSize+nBlockYSize-1) / nBlockYSize;
    
    papoBlocks = (GDALRasterBlock **)
        CPLCalloc( sizeof(void*), nBlocksPerRow * nBlocksPerColumn );
}


/************************************************************************/
/*                             AdoptBlock()                             */
/*                                                                      */
/*      Add a block to the raster band's block matrix.  If this         */
/*      exceeds our maximum blocks for this layer, flush the oldest     */
/*      block out.                                                      */
/*                                                                      */
/*      This method is protected.                                       */
/************************************************************************/

CPLErr GDALRasterBand::AdoptBlock( int nBlockXOff, int nBlockYOff,
                                   GDALRasterBlock * poBlock )

{
    int         nBlockIndex;
    
    InitBlockInfo();
    
    CPLAssert( nBlockXOff >= 0 && nBlockXOff < nBlocksPerRow );
    CPLAssert( nBlockYOff >= 0 && nBlockYOff < nBlocksPerColumn );

    nBlockIndex = nBlockXOff + nBlockYOff * nBlocksPerRow;
    if( papoBlocks[nBlockIndex] == poBlock )
        return( CE_None );

    if( papoBlocks[nBlockIndex] != NULL )
        FlushBlock( nBlockXOff, nBlockYOff );

    papoBlocks[nBlockIndex] = poBlock;
    poBlock->Touch();

    return( CE_None );
}

/************************************************************************/
/*                             FlushCache()                             */
/************************************************************************/

/**
 * Flush raster data cache.
 *
 * This call will recover memory used to cache data blocks for this raster
 * band, and ensure that new requests are referred to the underlying driver.
 *
 * This method is the same as the C function GDALFlushRasterCache().
 *
 * @return CE_None on success.
 */

CPLErr GDALRasterBand::FlushCache()

{
    for( int iY = 0; iY < nBlocksPerColumn; iY++ )
    {
        for( int iX = 0; iX < nBlocksPerRow; iX++ )
        {
            if( papoBlocks[iX + iY*nBlocksPerRow] != NULL )
            {
                CPLErr    eErr;

                eErr = FlushBlock( iX, iY );

                if( eErr != CE_None )
                    return eErr;
            }
        }
    }

    return( CE_None );
}

/************************************************************************/
/*                        GDALFlushRasterCache()                        */
/************************************************************************/

CPLErr GDALFlushRasterCache( GDALRasterBandH hBand )

{
    return ((GDALRasterBand *) hBand)->FlushCache();
}

/************************************************************************/
/*                             FlushBlock()                             */
/*                                                                      */
/*      Flush a block out of the block cache.  If it has been           */
/*      modified write it to disk.  If no specific tile is              */
/*      indicated, write the oldest tile.                               */
/*                                                                      */
/*      Protected method.                                               */
/************************************************************************/

CPLErr GDALRasterBand::FlushBlock( int nBlockXOff, int nBlockYOff )

{
    int         nBlockIndex;
    GDALRasterBlock *poBlock;
    CPLErr      eErr = CE_None;
        
    InitBlockInfo();
    
/* -------------------------------------------------------------------- */
/*      Validate                                                        */
/* -------------------------------------------------------------------- */
    CPLAssert( nBlockXOff >= 0 && nBlockXOff < nBlocksPerRow );
    CPLAssert( nBlockYOff >= 0 && nBlockYOff < nBlocksPerColumn );

    nBlockIndex = nBlockXOff + nBlockYOff * nBlocksPerRow;
    poBlock = papoBlocks[nBlockIndex];
    if( poBlock == NULL )
        return( CE_None );

/* -------------------------------------------------------------------- */
/*      Remove, and update count.                                       */
/* -------------------------------------------------------------------- */
    papoBlocks[nBlockIndex] = NULL;

/* -------------------------------------------------------------------- */
/*      Is the target block dirty?  If so we need to write it.          */
/* -------------------------------------------------------------------- */
    if( poBlock->GetDirty() )
        poBlock->Write();

/* -------------------------------------------------------------------- */
/*      Deallocate the block;                                           */
/* -------------------------------------------------------------------- */
    delete poBlock;

    return( eErr );
}


/************************************************************************/
/*                            GetBlockRef()                             */
/************************************************************************/

/**
 * Fetch a pointer to an internally cached raster block.
 *
 * Note that calling GetBlockRef() on a previously uncached band will
 * enable caching.
 * 
 * @param nXBlockOff the horizontal block offset, with zero indicating
 * the left most block, 1 the next block and so forth. 
 *
 * @param nYBlockOff the vertical block offset, with zero indicating
 * the left most block, 1 the next block and so forth.
 *
 * @return pointer to the block object, or NULL on failure.
 */

GDALRasterBlock * GDALRasterBand::GetBlockRef( int nXBlockOff,
                                               int nYBlockOff )

{
    int         nBlockIndex;

    InitBlockInfo();
    
/* -------------------------------------------------------------------- */
/*      Validate the request                                            */
/* -------------------------------------------------------------------- */
    if( nXBlockOff < 0 || nXBlockOff >= nBlocksPerRow )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "Illegal nBlockXOff value (%d) in "
                        "GDALRasterBand::GetBlockRef()\n",
                  nXBlockOff );

        return( NULL );
    }

    if( nYBlockOff < 0 || nYBlockOff >= nBlocksPerColumn )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "Illegal nBlockYOff value (%d) in "
                        "GDALRasterBand::GetBlockRef()\n",
                  nYBlockOff );

        return( NULL );
    }

/* -------------------------------------------------------------------- */
/*      If the block isn't already in the cache, we will need to        */
/*      create it, read into it, and adopt it.  Adopting it may         */
/*      flush an old tile from the cache.                               */
/* -------------------------------------------------------------------- */
    nBlockIndex = nXBlockOff + nYBlockOff * nBlocksPerRow;
    
    if( papoBlocks[nBlockIndex] == NULL )
    {
        GDALRasterBlock *poBlock;
        
        poBlock = new GDALRasterBlock( this, nXBlockOff, nYBlockOff );

        /* allocate data space */
        if( poBlock->Internalize() != CE_None )
        {
            delete poBlock;
	    CPLError( CE_Failure, CPLE_AppDefined, "Internalize failed",
		      nXBlockOff, nYBlockOff);
            return( NULL );
        }

        if( IReadBlock(nXBlockOff,nYBlockOff,poBlock->GetDataRef()) != CE_None)
        {
            delete poBlock;
            CPLError( CE_Failure, CPLE_AppDefined,
		      "IReadBlock failed at X offset %d, Y offset %d",
		      nXBlockOff, nYBlockOff );
	    return( NULL );
        }

        AdoptBlock( nXBlockOff, nYBlockOff, poBlock );
    }

/* -------------------------------------------------------------------- */
/*      Every read access updates the last touched time.                */
/* -------------------------------------------------------------------- */
    if( papoBlocks[nBlockIndex] != NULL )
        papoBlocks[nBlockIndex]->Touch();

    return( papoBlocks[nBlockIndex] );
}

/************************************************************************/
/*                             GetAccess()                              */
/************************************************************************/

/**
 * Find out if we have update permission for this band.
 *
 * This method is the same as the C function GDALGetRasterAccess().
 *
 * @return Either GA_Update or GA_ReadOnly.
 */

GDALAccess GDALRasterBand::GetAccess()

{
    return eAccess;
}

/************************************************************************/
/*                        GDALGetRasterAccess()                         */
/************************************************************************/

GDALAccess GDALGetRasterAccess( GDALRasterBandH hBand )

{
    return ((GDALRasterBand *) hBand)->GetAccess();
}

/************************************************************************/
/*                          GetCategoryNames()                          */
/************************************************************************/

/**
 * Fetch the list of category names for this raster.
 *
 * The return list is a "StringList" in the sense of the CPL functions.
 * That is a NULL terminated array of strings.  Raster values without 
 * associated names will have an empty string in the returned list.  The
 * first entry in the list is for raster values of zero, and so on. 
 *
 * The returned stringlist should not be altered or freed by the application.
 * It may change on the next GDAL call, so please copy it if it is needed
 * for any period of time. 
 * 
 * @return list of names, or NULL if none.
 */

char **GDALRasterBand::GetCategoryNames()

{
    return NULL;
}

/************************************************************************/
/*                     GDALGetRasterCategoryNames()                     */
/************************************************************************/

char **GDALGetRasterCategoryNames( GDALRasterBandH hBand )

{
    return ((GDALRasterBand *) hBand)->GetCategoryNames();
}

/************************************************************************/
/*                          SetCategoryNames()                          */
/************************************************************************/

/**
 * Set the category names for this band.
 *
 * See the GetCategoryNames() method for more on the interpretation of
 * category names. 
 *
 * This method is the same as the C function GDALSetRasterCategoryNames().
 *
 * @param papszNames the NULL terminated StringList of category names.  May
 * be NULL to just clear the existing list. 
 *
 * @return CE_None on success of CE_Failure on failure.  If unsupported
 * by the driver CE_Failure is returned, but no error message is reported.
 */

CPLErr GDALRasterBand::SetCategoryNames( char ** )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "SetCategoryNames() not supported for this dataset." );
    
    return CE_Failure;
}

/************************************************************************/
/*                        GDALSetCategoryNames()                        */
/************************************************************************/

CPLErr GDALSetRasterCategoryNames( GDALRasterBandH hBand, char ** papszNames )

{
    return ((GDALRasterBand *) hBand)->SetCategoryNames( papszNames );
}

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/

/**
 * Fetch the no data value for this band.
 * 
 * If there is no out of data value, an out of range value will generally
 * be returned.  The no data value for a band is generally a special marker
 * value used to mark pixels that are not valid data.  Such pixels should
 * generally not be displayed, nor contribute to analysis operations.
 *
 * This method is the same as the C function GDALGetRasterNoDataValue().
 *
 * @param pbSuccess pointer to a boolean to use to indicate if a value
 * is actually associated with this layer.  May be NULL (default).
 *
 * @return the nodata value for this band.
 */

double GDALRasterBand::GetNoDataValue( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = FALSE;
    
    return -1e10;
}

/************************************************************************/
/*                      GDALGetRasterNoDataValue()                      */
/************************************************************************/

double GDALGetRasterNoDataValue( GDALRasterBandH hBand, int *pbSuccess )

{
    return ((GDALRasterBand *) hBand)->GetNoDataValue( pbSuccess );
}

/************************************************************************/
/*                           SetNoDataValue()                           */
/************************************************************************/

/**
 * Set the no data value for this band. 
 *
 * To clear the nodata value, just set it with an "out of range" value.
 * Complex band no data values must have an imagery component of zero.
 *
 * This method is the same as the C function GDALSetRasterNoDataValue().
 *
 * @param dfNoData the value to set.
 *
 * @return CE_None on success, or CE_Failure on failure.  If unsupported
 * by the driver, CE_Failure is returned by no error message will have
 * been emitted.
 */

CPLErr GDALRasterBand::SetNoDataValue( double )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "SetNoDataValue() not supported for this dataset." );
    return CE_Failure;
}

/************************************************************************/
/*                         GDALSetRasterNoDataValue()                   */
/************************************************************************/

CPLErr GDALSetRasterNoDataValue( GDALRasterBandH hBand, double dfValue )

{
    return ((GDALRasterBand *) hBand)->SetNoDataValue( dfValue );
}

/************************************************************************/
/*                             GetMaximum()                             */
/************************************************************************/

/**
 * Fetch the maximum value for this band.
 * 
 * For file formats that don't know this intrinsically, the maximum supported
 * value for the data type will generally be returned.  
 *
 * This method is the same as the C function GDALGetRasterMaximum().
 *
 * @param pbSuccess pointer to a boolean to use to indicate if the
 * returned value is a tight maximum or not.  May be NULL (default).
 *
 * @return the maximum raster value (excluding no data pixels)
 */

double GDALRasterBand::GetMaximum( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = FALSE;

    switch( eDataType )
    {
      case GDT_Byte:
        return 255;

      case GDT_UInt16:
        return 65535;

      case GDT_Int16:
      case GDT_CInt16:
        return 32767;

      case GDT_Int32:
      case GDT_CInt32:
        return 2147483647.0;

      case GDT_UInt32:
        return 4294967295.0;

      case GDT_Float32:
      case GDT_CFloat32:
        return 4294967295.0; /* not actually accurate */

      case GDT_Float64:
      case GDT_CFloat64:
        return 4294967295.0; /* not actually accurate */

      default:
        return 4294967295.0; /* not actually accurate */
    }
}

/************************************************************************/
/*                        GDALGetRasterMaximum()                        */
/************************************************************************/

double GDALGetRasterMaximum( GDALRasterBandH hBand, int *pbSuccess )

{
    return ((GDALRasterBand *) hBand)->GetMaximum( pbSuccess );
}

/************************************************************************/
/*                             GetMinimum()                             */
/************************************************************************/

/**
 * Fetch the minimum value for this band.
 * 
 * For file formats that don't know this intrinsically, the minimum supported
 * value for the data type will generally be returned.  
 *
 * This method is the same as the C function GDALGetRasterMinimum().
 *
 * @param pbSuccess pointer to a boolean to use to indicate if the
 * returned value is a tight minimum or not.  May be NULL (default).
 *
 * @return the minimum raster value (excluding no data pixels)
 */

double GDALRasterBand::GetMinimum( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = FALSE;

    switch( eDataType )
    {
      case GDT_Byte:
        return 0;

      case GDT_UInt16:
        return 0;

      case GDT_Int16:
        return -32768;

      case GDT_Int32:
        return -2147483648.0;

      case GDT_UInt32:
        return 0;

      case GDT_Float32:
        return -4294967295.0; /* not actually accurate */

      case GDT_Float64:
        return -4294967295.0; /* not actually accurate */

      default:
        return -4294967295.0; /* not actually accurate */
    }
}

/************************************************************************/
/*                        GDALGetRasterMinimum()                        */
/************************************************************************/

double GDALGetRasterMinimum( GDALRasterBandH hBand, int *pbSuccess )

{
    return ((GDALRasterBand *) hBand)->GetMinimum( pbSuccess );
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

/**
 * How should this band be interpreted as color?
 *
 * CV_Undefined is returned when the format doesn't know anything
 * about the color interpretation. 
 *
 * This method is the same as the C function 
 * GDALGetRasterColorInterpretation().
 *
 * @return color interpretation value for band.
 */

GDALColorInterp GDALRasterBand::GetColorInterpretation()

{
    return GCI_Undefined;
}

/************************************************************************/
/*                  GDALGetRasterColorInterpretation()                  */
/************************************************************************/

GDALColorInterp GDALGetRasterColorInterpretation( GDALRasterBandH hBand )

{
    return ((GDALRasterBand *) hBand)->GetColorInterpretation();
}

/************************************************************************/
/*                       SetColorInterpretation()                       */
/************************************************************************/

/**
 * Set color interpretation of a band.
 *
 * @param eColorInterp the new color interpretation to apply to this band.
 * 
 * @return CE_None on success or CE_Failure if method is unsupported by format.
 */

CPLErr GDALRasterBand::SetColorInterpretation( GDALColorInterp eColorInterp )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "SetColorInterpretation() not supported for this dataset." );
    return CE_Failure;
}

/************************************************************************/
/*                  GDALSetRasterColorInterpretation()                  */
/************************************************************************/

CPLErr GDALSetRasterColorInterpretation( GDALRasterBandH hBand,
                                         GDALColorInterp eColorInterp )

{
    return ((GDALRasterBand *) hBand)->SetColorInterpretation(eColorInterp);
}

/************************************************************************/
/*                           GetColorTable()                            */
/************************************************************************/

/**
 * Fetch the color table associated with band.
 *
 * If there is no associated color table, the return result is NULL.  The
 * returned color table remains owned by the GDALRasterBand, and can't
 * be depended on for long, nor should it ever be modified by the caller.
 *
 * This method is the same as the C function GDALGetRasterColorTable().
 *
 * @return internal color table, or NULL.
 */

GDALColorTable *GDALRasterBand::GetColorTable()

{
    return NULL;
}

/************************************************************************/
/*                      GDALGetRasterColorTable()                       */
/************************************************************************/

GDALColorTableH GDALGetRasterColorTable( GDALRasterBandH hBand )

{
    return (GDALColorTableH) ((GDALRasterBand *) hBand)->GetColorTable();
}

/************************************************************************/
/*                           SetColorTable()                            */
/************************************************************************/

/**
 * Set the raster color table. 
 * 
 * The driver will make a copy of all desired data in the colortable.  It
 * remains owned by the caller after the call.
 *
 * This method is the same as the C function GDALSetRasterColorTable().
 *
 * @param poCT the color table to apply.
 *
 * @return CE_None on success, or CE_Failure on failure.  If the action is
 * unsupported by the driver, a value of CE_Failure is returned, but no
 * error is issued.
 */

CPLErr GDALRasterBand::SetColorTable( GDALColorTable * poCT )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "SetColorTable() not supported for this dataset." );
    return CE_Failure;
}

/************************************************************************/
/*                      GDALSetRasterColorTable()                       */
/************************************************************************/

CPLErr GDALSetRasterColorTable( GDALRasterBandH hBand, GDALColorTableH hCT )

{
    return ((GDALRasterBand *) hBand)->SetColorTable( (GDALColorTable *) hCT );
}

/************************************************************************/
/*                       HasArbitraryOverviews()                        */
/************************************************************************/

/**
 * Check for arbitrary overviews.
 *
 * This returns TRUE if the underlying datastore can compute arbitrary 
 * overviews efficiently, such as is the case with OGDI over a network. 
 * Datastores with arbitrary overviews don't generally have any fixed
 * overviews, but the RasterIO() method can be used in downsampling mode
 * to get overview data efficiently.
 *
 * This method is the same as the C function GDALHasArbitraryOverviews(),
 *
 * @return TRUE if arbitrary overviews available (efficiently), otherwise
 * FALSE. 
 */

int GDALRasterBand::HasArbitraryOverviews()

{
    return FALSE;
}

/************************************************************************/
/*                     GDALHasArbitraryOverviews()                      */
/************************************************************************/

int GDALHasArbitraryOverviews( GDALRasterBandH hBand )

{
    return ((GDALRasterBand *) hBand)->HasArbitraryOverviews();
}

/************************************************************************/
/*                          GetOverviewCount()                          */
/************************************************************************/

/**
 * Return the number of overview layers available.
 *
 * This method is the same as the C function GDALGetOverviewCount();
 *
 * @return overview count, zero if none.
 */

int GDALRasterBand::GetOverviewCount()

{
    if( poDS != NULL && poDS->oOvManager.IsInitialized() )
        return poDS->oOvManager.GetOverviewCount( nBand );
    else
        return 0;
}

/************************************************************************/
/*                        GDALGetOverviewCount()                        */
/************************************************************************/

int GDALGetOverviewCount( GDALRasterBandH hBand )

{
    return ((GDALRasterBand *) hBand)->GetOverviewCount();
}


/************************************************************************/
/*                            GetOverview()                             */
/************************************************************************/

/**
 * Fetch overview raster band object.
 *
 * This method is the same as the C function GDALGetOverview().
 * 
 * @param i overview index between 0 and GetOverviewCount()-1.
 * 
 * @return overview GDALRasterBand.
 */

GDALRasterBand * GDALRasterBand::GetOverview( int i )

{
    if( poDS != NULL && poDS->oOvManager.IsInitialized() )
        return poDS->oOvManager.GetOverview( nBand, i );
    else
        return NULL;
}

/************************************************************************/
/*                          GDALGetOverview()                           */
/************************************************************************/

GDALRasterBandH GDALGetOverview( GDALRasterBandH hBand, int i )

{
    return (GDALRasterBandH) ((GDALRasterBand *) hBand)->GetOverview(i);
}

/************************************************************************/
/*                           BuildOverviews()                           */
/************************************************************************/

/**
 * Build raster overview(s)
 *
 * If the operation is unsupported for the indicated dataset, then 
 * CE_Failure is returned, and CPLGetLastError() will return CPLE_NonSupported.
 *
 * @param pszResampling one of "NEAREST", "AVERAGE" or "MODE" controlling
 * the downsampling method applied.
 * @param nOverviews number of overviews to build. 
 * @param panOverviewList the list of overview decimation factors to build. 
 * @param pfnProgress a function to call to report progress, or NULL.
 * @param pProgressData application data to pass to the progress function.
 *
 * @return CE_None on success or CE_Failure if the operation doesn't work. 
 */

CPLErr GDALRasterBand::BuildOverviews( const char *pszResampling, 
                                       int nOverviews, int *panOverviewList, 
                                       GDALProgressFunc pfnProgress, 
                                       void * pProgressData )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "BuildOverviews() not supported for this dataset." );
    
    return( CE_Failure );
}

/************************************************************************/
/*                             GetOffset()                              */
/************************************************************************/

/**
 * Fetch the raster value offset.
 *
 * This value (in combination with the GetScale() value) is used to
 * transform raw pixel values into the units returned by GetUnits().  
 * For example this might be used to store elevations in GUInt16 bands
 * with a precision of 0.1, and starting from -100. 
 * 
 * Units value = (raw value * scale) + offset
 * 
 * For file formats that don't know this intrinsically a value of zero
 * is returned. 
 *
 * @param pbSuccess pointer to a boolean to use to indicate if the
 * returned value is meaningful or not.  May be NULL (default).
 *
 * @return the raster offset.
 */

double GDALRasterBand::GetOffset( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = FALSE;

    return 0.0;
}

/************************************************************************/
/*                              GetScale()                              */
/************************************************************************/

/**
 * Fetch the raster value scale.
 *
 * This value (in combination with the GetOffset() value) is used to
 * transform raw pixel values into the units returned by GetUnits().  
 * For example this might be used to store elevations in GUInt16 bands
 * with a precision of 0.1, and starting from -100. 
 * 
 * Units value = (raw value * scale) + offset
 * 
 * For file formats that don't know this intrinsically a value of one
 * is returned. 
 *
 * @param pbSuccess pointer to a boolean to use to indicate if the
 * returned value is meaningful or not.  May be NULL (default).
 *
 * @return the raster scale.
 */

double GDALRasterBand::GetScale( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = FALSE;

    return 1.0;
}

/************************************************************************/
/*                            GetUnitType()                             */
/************************************************************************/

/**
 * Return raster unit type.
 *
 * Return a name for the units of this raster's values.  For instance, it
 * might be "m" for an elevation model in meters, or "ft" for feet.  If no 
 * units are available, a value of "" will be returned.  The returned string 
 * should not be modified, nor freed by the calling application.
 *
 * This method is the same as the C function GDALGetRasterUnitType(). 
 *
 * @return unit name string.
 */

const char *GDALRasterBand::GetUnitType()

{
    return "";
}

/************************************************************************/
/*                       GDALGetRasterUnitType()                        */
/************************************************************************/

const char *GDALGetRasterUnitType( GDALRasterBandH hBand )

{
    return ((GDALRasterBand *) hBand)->GetUnitType();
}

/************************************************************************/
/*                              GetXSize()                              */
/************************************************************************/

/**
 * Fetch XSize of raster. 
 *
 * This method is the same as the C function GDALGetRasterBandXSize(). 
 *
 * @return the width in pixels of this band.
 */

int GDALRasterBand::GetXSize()

{
    return nRasterXSize;
}

/************************************************************************/
/*                       GDALGetRasterBandXSize()                       */
/************************************************************************/

int GDALGetRasterBandXSize( GDALRasterBandH hBand )

{
    return ((GDALRasterBand *) hBand)->GetXSize();
}

/************************************************************************/
/*                              GetYSize()                              */
/************************************************************************/

/**
 * Fetch YSize of raster. 
 *
 * This method is the same as the C function GDALGetRasterBandYSize(). 
 *
 * @return the height in pixels of this band.
 */

int GDALRasterBand::GetYSize()

{
    return nRasterYSize;
}

/************************************************************************/
/*                       GDALGetRasterBandYSize()                       */
/************************************************************************/

int GDALGetRasterBandYSize( GDALRasterBandH hBand )

{
    return ((GDALRasterBand *) hBand)->GetYSize();
}

/************************************************************************/
/*                              GetBand()                               */
/************************************************************************/

/**
 * Fetch the band number.
 *
 * This method returns the band that this GDALRasterBand objects represents
 * within it's dataset.  This method may return a value of 0 to indicate
 * GDALRasterBand objects without an apparently relationship to a dataset,
 * such as GDALRasterBands serving as overviews.
 *
 * There is currently no C analog to this method.
 *
 * @return band number (1+) or 0 if the band number isn't known.
 */

int GDALRasterBand::GetBand()

{
    return nBand;
}

/************************************************************************/
/*                             GetDataset()                             */
/************************************************************************/

/**
 * Fetch the owning dataset handle.
 *
 * Note that some GDALRasterBands are not considered to be a part of a dataset,
 * such as overviews or other "freestanding" bands. 
 *
 * There is currently no C analog to this method.
 *
 * @return the pointer to the GDALDataset to which this band belongs, or
 * NULL if this cannot be determined.
 */

GDALDataset *GDALRasterBand::GetDataset()

{
    return poDS;
}

/************************************************************************/
/*                            GetHistogram()                            */
/************************************************************************/

/**
 * Compute raster histogram. 
 *
 * Note that the bucket size is (dfMax-dfMin) / nBuckets.  
 *
 * For example to compute a simple 256 entry histogram of eight bit data, 
 * the following would be suitable.  The unusual bounds are to ensure that
 * bucket boundaries don't fall right on integer values causing possible errors
 * due to rounding after scaling. 
<pre>
    int anHistogram[256];

    poBand->GetHistogram( -0.5, 255.5, 256, anHistogram, FALSE, FALSE, 
                          GDALDummyProgress, NULL );
</pre>
 *
 * Note that setting bApproxOK will generally result in a subsampling of the
 * file, and will utilize overviews if available.  It should generally 
 * produce a representative histogram for the data that is suitable for use
 * in generating histogram based luts for instance.  Generally bApproxOK is
 * much faster than an exactly computed histogram.
 *
 * @param dfMin the lower bound of the histogram.
 * @param dfMax the upper bound of the histogram.
 * @param nBuckets the number of buckets in panHistogram.
 * @param panHistogram array into which the histogram totals are placed.
 * @param bIncludeOutOfRange if TRUE values below the histogram range will
 * mapped into panHistogram[0], and values above will be mapped into 
 * panHistogram[nBuckets-1] otherwise out of range values are discarded.
 * @param bApproxOK TRUE if an approximate, or incomplete histogram OK.
 * @param pfnProgress function to report progress to completion. 
 * @param pProgressData application data to pass to pfnProgress. 
 *
 * @return CE_None on success, or CE_Failure if something goes wrong. 
 */

CPLErr GDALRasterBand::GetHistogram( double dfMin, double dfMax, 
                                     int nBuckets, int *panHistogram, 
                                     int bIncludeOutOfRange, int bApproxOK,
                                     GDALProgressFunc pfnProgress, 
                                     void *pProgressData )

{
    CPLAssert( pfnProgress != NULL );

/* -------------------------------------------------------------------- */
/*      If we have overviews, use them for the histogram.               */
/* -------------------------------------------------------------------- */
    if( bApproxOK && GetOverviewCount() > 0 )
    {
        double dfBestPixels = GetXSize() * GetYSize();
        GDALRasterBand *poBestOverview = NULL;
        
        for( int i = 0; i < GetOverviewCount(); i++ )
        {
            GDALRasterBand *poOverview = GetOverview(i);
            double         dfPixels;

            dfPixels = poOverview->GetXSize() * poOverview->GetYSize();
            if( dfPixels < dfBestPixels )
            {
                dfBestPixels = dfPixels;
                poBestOverview = poOverview;
            }
            
            if( poBestOverview != NULL )
                return poBestOverview->
                    GetHistogram( dfMin, dfMax, nBuckets, panHistogram, 
                                  bIncludeOutOfRange, bApproxOK, 
                                  pfnProgress, pProgressData );
        }
    }

/* -------------------------------------------------------------------- */
/*      Figure out the ratio of blocks we will read to get an           */
/*      approximate value.                                              */
/* -------------------------------------------------------------------- */
    int         nSampleRate;
    double      dfScale;

    InitBlockInfo();
    
    if( bApproxOK )
        nSampleRate = 
            (int) MAX(1,sqrt((double) nBlocksPerRow * nBlocksPerColumn));
    else
        nSampleRate = 1;
    
    dfScale = nBuckets / (dfMax - dfMin);

/* -------------------------------------------------------------------- */
/*      Read the blocks, and add to histogram.                          */
/* -------------------------------------------------------------------- */
    memset( panHistogram, 0, sizeof(int) * nBuckets );
    for( int iSampleBlock = 0; 
         iSampleBlock < nBlocksPerRow * nBlocksPerColumn;
         iSampleBlock += nSampleRate )
    {
        double dfValue = 0.0, dfReal, dfImag;
        int  iXBlock, iYBlock, nXCheck, nYCheck;
        GDALRasterBlock *poBlock;

        if( !pfnProgress( iSampleBlock/(double)nBlocksPerRow*nBlocksPerColumn,
                          NULL, pProgressData ) )
            return CE_Failure;

        iYBlock = iSampleBlock / nBlocksPerRow;
        iXBlock = iSampleBlock - nBlocksPerRow * iYBlock;
        
        poBlock = GetBlockRef( iXBlock, iYBlock );
        if( poBlock == NULL )
            return CE_Failure;
        
        if( (iXBlock+1) * nBlockXSize > GetXSize() )
            nXCheck = GetXSize() - iXBlock * nBlockXSize;
        else
            nXCheck = nBlockXSize;

        if( (iYBlock+1) * nBlockYSize > GetYSize() )
            nYCheck = GetYSize() - iYBlock * nBlockYSize;
        else
            nYCheck = nBlockYSize;

        /* this is a special case for a common situation */
        if( poBlock->GetDataType() == GDT_Byte
            && dfScale == 1.0 && (dfMin >= -0.5 && dfMin <= 0.5)
            && nYCheck == nBlockYSize && nXCheck == nBlockXSize
            && nBuckets == 256 )
        {
            int    nPixels = nXCheck * nYCheck;
            GByte  *pabyData = (GByte *) poBlock->GetDataRef();
            
            for( int i = 0; i < nPixels; i++ )
                panHistogram[pabyData[i]]++;
            
            continue; /* to next sample block */
        }

        /* this isn't the fastest way to do this, but is easier for now */
        for( int iY = 0; iY < nYCheck; iY++ )
        {
            for( int iX = 0; iX < nXCheck; iX++ )
            {
                int    iOffset = iX + iY * nBlockXSize;
                int    nIndex;

                switch( poBlock->GetDataType() )
                {
                  case GDT_Byte:
                    dfValue = ((GByte *) poBlock->GetDataRef())[iOffset];
                    break;

                  case GDT_UInt16:
                    dfValue = ((GUInt16 *) poBlock->GetDataRef())[iOffset];
                    break;
                  case GDT_Int16:
                    dfValue = ((GInt16 *) poBlock->GetDataRef())[iOffset];
                    break;
                  case GDT_UInt32:
                    dfValue = ((GUInt32 *) poBlock->GetDataRef())[iOffset];
                    break;
                  case GDT_Int32:
                    dfValue = ((GInt32 *) poBlock->GetDataRef())[iOffset];
                    break;
                  case GDT_Float32:
                    dfValue = ((float *) poBlock->GetDataRef())[iOffset];
                    break;
                  case GDT_Float64:
                    dfValue = ((double *) poBlock->GetDataRef())[iOffset];
                    break;
                  case GDT_CInt16:
                    dfReal = ((GInt16 *) poBlock->GetDataRef())[iOffset*2];
                    dfImag = ((GInt16 *) poBlock->GetDataRef())[iOffset*2+1];
                    dfValue = sqrt( dfReal * dfReal + dfImag * dfImag );
                    break;
                  case GDT_CInt32:
                    dfReal = ((GInt32 *) poBlock->GetDataRef())[iOffset*2];
                    dfImag = ((GInt32 *) poBlock->GetDataRef())[iOffset*2+1];
                    dfValue = sqrt( dfReal * dfReal + dfImag * dfImag );
                    break;
                  case GDT_CFloat32:
                    dfReal = ((float *) poBlock->GetDataRef())[iOffset*2];
                    dfImag = ((float *) poBlock->GetDataRef())[iOffset*2+1];
                    dfValue = sqrt( dfReal * dfReal + dfImag * dfImag );
                    break;
                  case GDT_CFloat64:
                    dfReal = ((double *) poBlock->GetDataRef())[iOffset*2];
                    dfImag = ((double *) poBlock->GetDataRef())[iOffset*2+1];
                    dfValue = sqrt( dfReal * dfReal + dfImag * dfImag );
                    break;
                  default:
                    CPLAssert( FALSE );
                    return CE_Failure;
                }
                
                nIndex = (int) floor((dfValue - dfMin) * dfScale);

                if( nIndex < 0 )
                {
                    if( bIncludeOutOfRange )
                        panHistogram[0]++;
                }
                else if( nIndex >= nBuckets )
                {
                    if( bIncludeOutOfRange )
                        panHistogram[nBuckets-1]++;
                }
                else
                {
                    panHistogram[nIndex]++;
                }
            }
        }
    }

    pfnProgress( 1.0, NULL, pProgressData );

    return CE_None;
}

/************************************************************************/
/*                       GDALGetRasterHistogram()                       */
/************************************************************************/

CPLErr GDALGetRasterHistogram( GDALRasterBandH hBand, 
                               double dfMin, double dfMax, 
                               int nBuckets, int *panHistogram, 
                               int bIncludeOutOfRange, int bApproxOK,
                               GDALProgressFunc pfnProgress, 
                               void *pProgressData )

{
    return ((GDALRasterBand *) hBand)->
        GetHistogram( dfMin, dfMax, nBuckets, panHistogram, 
                      bIncludeOutOfRange, bApproxOK,
                      pfnProgress, pProgressData );
}
