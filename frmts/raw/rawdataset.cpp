/******************************************************************************
 * $Id$
 *
 * Project:  Generic Raw Binary Driver
 * Purpose:  Implementation of RawDataset and RawRasterBand classes.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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
 * Revision 1.16  2003/03/18 05:59:41  warmerda
 * Added FlushCache() implementation that uses fflush() to force
 * everything out.
 *
 * Revision 1.15  2002/11/23 18:54:47  warmerda
 * added setnodatavalue
 *
 * Revision 1.14  2002/02/07 15:14:59  warmerda
 * ensure that no more bytes are read or written than necessary
 *
 * Revision 1.13  2001/12/14 19:18:38  ldjohn
 * Typecast offset parameter to Seek for large file support.
 *
 * Revision 1.12  2001/12/12 18:41:33  warmerda
 * don't pass vsi_l_offset values in debug calls
 *
 * Revision 1.11  2001/12/12 18:15:46  warmerda
 * preliminary update for large raw file support
 *
 * Revision 1.10  2001/07/18 04:51:57  warmerda
 * added CPL_CVSID
 *
 * Revision 1.9  2001/03/26 18:31:55  warmerda
 * Fixed nodata handling in first constructor.
 *
 * Revision 1.8  2001/03/23 03:25:32  warmerda
 * Added nodata support
 *
 * Revision 1.7  2001/01/03 18:54:25  warmerda
 * improved seek error message
 *
 * Revision 1.6  2000/08/16 15:51:17  warmerda
 * allow floating (datasetless) raw bands
 *
 * Revision 1.5  2000/08/09 16:26:27  warmerda
 * improved error checking
 *
 * Revision 1.4  2000/06/05 17:24:06  warmerda
 * added real complex support
 *
 * Revision 1.3  2000/03/31 13:36:40  warmerda
 * RawRasterBand no longer depends on RawDataset
 *
 * Revision 1.2  1999/08/13 02:36:57  warmerda
 * added write support
 *
 * Revision 1.1  1999/07/23 19:34:34  warmerda
 * New
 *
 */

#include "rawdataset.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           RawRasterBand()                            */
/************************************************************************/

RawRasterBand::RawRasterBand( GDALDataset *poDS, int nBand,
                              FILE * fpRaw, vsi_l_offset nImgOffset,
                              int nPixelOffset, int nLineOffset,
                              GDALDataType eDataType, int bNativeOrder,
                              int bIsVSIL )

{
    this->poDS = poDS;
    this->nBand = nBand;
    this->eDataType = eDataType;
    this->bIsVSIL = bIsVSIL;

    this->fpRaw = fpRaw;
    this->nImgOffset = nImgOffset;
    this->nPixelOffset = nPixelOffset;
    this->nLineOffset = nLineOffset;
    this->bNativeOrder = bNativeOrder;

    CPLDebug( "GDALRaw", 
              "RawRasterBand(%p,%d,%p,\n"
              "              Off=%d,PixOff=%d,LineOff=%d,%s,%d)\n",
              poDS, nBand, fpRaw, 
              (unsigned int) nImgOffset, nPixelOffset, nLineOffset, 
              GDALGetDataTypeName(eDataType), bNativeOrder );

    dfNoDataValue = 0.0;
    bNoDataSet = FALSE;

/* -------------------------------------------------------------------- */
/*      Treat one scanline as the block size.                           */
/* -------------------------------------------------------------------- */
    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;

/* -------------------------------------------------------------------- */
/*      Allocate working scanline.                                      */
/* -------------------------------------------------------------------- */
    nLoadedScanline = -1;
    pLineBuffer = CPLMalloc( nPixelOffset * nBlockXSize );
}

/************************************************************************/
/*                           RawRasterBand()                            */
/************************************************************************/

RawRasterBand::RawRasterBand( FILE * fpRaw, vsi_l_offset nImgOffset,
                              int nPixelOffset, int nLineOffset,
                              GDALDataType eDataType, int bNativeOrder,
                              int nXSize, int nYSize, int bIsVSIL )

{
    this->poDS = NULL;
    this->nBand = 1;
    this->eDataType = eDataType;
    this->bIsVSIL = bIsVSIL;

    this->fpRaw = fpRaw;
    this->nImgOffset = nImgOffset;
    this->nPixelOffset = nPixelOffset;
    this->nLineOffset = nLineOffset;
    this->bNativeOrder = bNativeOrder;
    
    CPLDebug( "GDALRaw", 
              "RawRasterBand(floating,Off=%d,PixOff=%d,LineOff=%d,%s,%d)\n",
              (unsigned int) nImgOffset, nPixelOffset, nLineOffset, 
              GDALGetDataTypeName(eDataType), bNativeOrder );

    dfNoDataValue = 0.0;
    bNoDataSet = FALSE;

/* -------------------------------------------------------------------- */
/*      Treat one scanline as the block size.                           */
/* -------------------------------------------------------------------- */
    nBlockXSize = nXSize;
    nBlockYSize = 1;
    nRasterXSize = nXSize;
    nRasterYSize = nYSize;

/* -------------------------------------------------------------------- */
/*      Allocate working scanline.                                      */
/* -------------------------------------------------------------------- */
    nLoadedScanline = -1;
    pLineBuffer = CPLMalloc( nPixelOffset * nBlockXSize );
}

/************************************************************************/
/*                           ~RawRasterBand()                           */
/************************************************************************/

RawRasterBand::~RawRasterBand()

{
    FlushCache();
    
    CPLFree( pLineBuffer );
}

/************************************************************************/
/*                             FlushCache()                             */
/*                                                                      */
/*      We override this so we have the opportunity to call             */
/*      fflush().  We don't want to do this all the time in the         */
/*      write block function as it is kind of expensive.                */
/************************************************************************/

CPLErr RawRasterBand::FlushCache()

{
    CPLErr eErr;

    eErr = GDALRasterBand::FlushCache();
    if( eErr != CE_None )
        return eErr;

    if( bIsVSIL )
        VSIFFlushL( fpRaw );
    else
        VSIFFlush( fpRaw );

    return CE_None;
}

/************************************************************************/
/*                             AccessLine()                             */
/************************************************************************/

CPLErr RawRasterBand::AccessLine( int iLine )

{
    if( nLoadedScanline == iLine )
        return CE_None;

/* -------------------------------------------------------------------- */
/*      Seek to the right line.                                         */
/* -------------------------------------------------------------------- */
    if( Seek(nImgOffset + (vsi_l_offset)iLine * nLineOffset, SEEK_SET) == -1 )
    {
        // for now I just set to zero under the assumption we might
        // be trying to read from a file past the data that has
        // actually been written out.  Eventually we should differentiate
        // between newly created datasets, and existing datasets. Existing
        // datasets should generate an error in this case.
        memset( pLineBuffer, 0, nPixelOffset * nBlockXSize );
        nLoadedScanline = iLine;
        return CE_None;
    }

/* -------------------------------------------------------------------- */
/*      Read the line.  Take care not to request any more bytes than    */
/*      are needed, and not to lose a partially successful scanline     */
/*      read.                                                           */
/* -------------------------------------------------------------------- */
    int	nBytesToRead, nBytesActuallyRead;

    nBytesToRead = nPixelOffset * (nBlockXSize - 1) 
        + GDALGetDataTypeSize(GetRasterDataType()) / 8;

    nBytesActuallyRead = Read( pLineBuffer, 1, nBytesToRead );
    if( nBytesActuallyRead < nBlockXSize )
    {
        // for now I just set to zero under the assumption we might
        // be trying to read from a file past the data that has
        // actually been written out.  Eventually we should differentiate
        // between newly created datasets, and existing datasets. Existing
        // datasets should generate an error in this case.
        memset( ((GByte *) pLineBuffer) + nBytesActuallyRead, 
                0, nBytesToRead - nBytesActuallyRead );
    }

/* -------------------------------------------------------------------- */
/*      Byte swap the interesting data, if required.                    */
/* -------------------------------------------------------------------- */
    if( !bNativeOrder  && eDataType != GDT_Byte )
    {
        if( GDALDataTypeIsComplex( eDataType ) )
        {
            int nWordSize;

            nWordSize = GDALGetDataTypeSize(eDataType)/16;
            GDALSwapWords( pLineBuffer, nWordSize, nBlockXSize, nPixelOffset );
            GDALSwapWords( ((GByte *) pLineBuffer)+nWordSize, 
                           nWordSize, nBlockXSize, nPixelOffset );
        }
        else
            GDALSwapWords( pLineBuffer, GDALGetDataTypeSize(eDataType)/8,
                           nBlockXSize, nPixelOffset );
    }

    nLoadedScanline = iLine;

    return CE_None;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr RawRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )

{
    CPLErr		eErr = CE_None;

    CPLAssert( nBlockXOff == 0 );

    AccessLine( nBlockYOff );
    
/* -------------------------------------------------------------------- */
/*      Copy data from disk buffer to user block buffer.                */
/* -------------------------------------------------------------------- */
    GDALCopyWords( pLineBuffer, eDataType, nPixelOffset,
                   pImage, eDataType, GDALGetDataTypeSize(eDataType)/8,
                   nBlockXSize );

    return eErr;
}

/************************************************************************/
/*                            IWriteBlock()                             */
/************************************************************************/

CPLErr RawRasterBand::IWriteBlock( int nBlockXOff, int nBlockYOff,
                                   void * pImage )

{
    CPLErr		eErr = CE_None;

    CPLAssert( nBlockXOff == 0 );

/* -------------------------------------------------------------------- */
/*      If the data for this band is completely contiguous we don't     */
/*      have to worry about pre-reading from disk.                      */
/* -------------------------------------------------------------------- */
    if( nPixelOffset > GDALGetDataTypeSize(eDataType) / 8 )
        eErr = AccessLine( nBlockYOff );

/* -------------------------------------------------------------------- */
/*	Copy data from user buffer into disk buffer.                    */
/* -------------------------------------------------------------------- */
    GDALCopyWords( pImage, eDataType, GDALGetDataTypeSize(eDataType)/8,
                   pLineBuffer, eDataType, nPixelOffset,
                   nBlockXSize );

/* -------------------------------------------------------------------- */
/*      Byte swap (if necessary) back into disk order before writing.   */
/* -------------------------------------------------------------------- */
    if( !bNativeOrder && eDataType != GDT_Byte )
    {
        if( GDALDataTypeIsComplex( eDataType ) )
        {
            int nWordSize;

            nWordSize = GDALGetDataTypeSize(eDataType)/16;
            GDALSwapWords( pLineBuffer, nWordSize, nBlockXSize, nPixelOffset );
            GDALSwapWords( ((GByte *) pLineBuffer)+nWordSize, 
                           nWordSize, nBlockXSize, nPixelOffset );
        }
        else
            GDALSwapWords( pLineBuffer, GDALGetDataTypeSize(eDataType)/8,
                           nBlockXSize, nPixelOffset );
    }

/* -------------------------------------------------------------------- */
/*      Seek to correct location.                                       */
/* -------------------------------------------------------------------- */
    if( Seek( nImgOffset + (vsi_l_offset) nBlockYOff * nLineOffset,
              SEEK_SET ) == -1 ) 
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Failed to seek to scanline %d @ %d to write to file.\n",
                  nBlockYOff, (int) (nImgOffset + nBlockYOff * nLineOffset) );
        
        eErr = CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Write data buffer.                                              */
/* -------------------------------------------------------------------- */
    int	nBytesToWrite;

    nBytesToWrite = nPixelOffset * (nBlockXSize - 1) 
        + GDALGetDataTypeSize(GetRasterDataType()) / 8;

    if( eErr == CE_None 
        && Write( pLineBuffer, 1, nBytesToWrite ) < (size_t) nBytesToWrite )
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Failed to write scanline %d to file.\n",
                  nBlockYOff );
        
        eErr = CE_Failure;
    }
    
/* -------------------------------------------------------------------- */
/*      Byte swap (if necessary) back into machine order so the         */
/*      buffer is still usable for reading purposes.                    */
/* -------------------------------------------------------------------- */
    if( !bNativeOrder && eDataType != GDT_Byte )
    {
        GDALSwapWords( pLineBuffer, GDALGetDataTypeSize(eDataType)/8,
                       nBlockXSize, nPixelOffset );
    }

    return eErr;
}

/************************************************************************/
/*                                Seek()                                */
/************************************************************************/

int RawRasterBand::Seek( vsi_l_offset nOffset, int nSeekMode )

{
    if( bIsVSIL )
        return VSIFSeekL( fpRaw, nOffset, nSeekMode );
    else
        return VSIFSeek( fpRaw, nOffset, nSeekMode );
}

/************************************************************************/
/*                                Read()                                */
/************************************************************************/

size_t RawRasterBand::Read( void *pBuffer, size_t nSize, size_t nCount )

{
    if( bIsVSIL )
        return VSIFReadL( pBuffer, nSize, nCount, fpRaw );
    else
        return VSIFRead( pBuffer, nSize, nCount, fpRaw );
}

/************************************************************************/
/*                               Write()                                */
/************************************************************************/

size_t RawRasterBand::Write( void *pBuffer, size_t nSize, size_t nCount )

{
    if( bIsVSIL )
        return VSIFWriteL( pBuffer, nSize, nCount, fpRaw );
    else
        return VSIFWrite( pBuffer, nSize, nCount, fpRaw );
}

/************************************************************************/
/*                          StoreNoDataValue()                          */
/*                                                                      */
/*      This is a helper function for datasets to associate a no        */
/*      data value with this band, it isn't intended to be called by    */
/*      applications.                                                   */
/************************************************************************/

void RawRasterBand::StoreNoDataValue( double dfValue )

{
    bNoDataSet = TRUE;
    dfNoDataValue = dfValue;
}

/************************************************************************/
/*                           SetNoDataValue()                           */
/************************************************************************/

CPLErr RawRasterBand::SetNoDataValue( double dfValue )

{
    bNoDataSet = TRUE;
    dfNoDataValue = dfValue;
    return CE_None;
}

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/

double RawRasterBand::GetNoDataValue( int * pbSuccess )

{
    if( pbSuccess )
        *pbSuccess = bNoDataSet;

    return dfNoDataValue;
}

/************************************************************************/
/* ==================================================================== */
/*      RawDataset                                                      */
/* ==================================================================== */
/************************************************************************/


/************************************************************************/
/*                            RawDataset()                              */
/************************************************************************/

RawDataset::RawDataset()

{
}

/************************************************************************/
/*                           ~RawDataset()                              */
/************************************************************************/

RawDataset::~RawDataset()

{
}

