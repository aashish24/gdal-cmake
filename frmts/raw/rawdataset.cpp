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
 * Revision 1.2  1999/08/13 02:36:57  warmerda
 * added write support
 *
 * Revision 1.1  1999/07/23 19:34:34  warmerda
 * New
 *
 */

#include "rawdataset.h"

/************************************************************************/
/*                           RawRasterBand()                            */
/************************************************************************/

RawRasterBand::RawRasterBand( RawDataset *poDS, int nBand,
                              FILE * fpRaw, unsigned int nImgOffset,
                              int nPixelOffset, int nLineOffset,
                              GDALDataType eDataType, int bNativeOrder )

{
    this->poDS = poDS;
    this->nBand = nBand;
    this->eDataType = eDataType;

    this->fpRaw = fpRaw;
    this->nImgOffset = nImgOffset;
    this->nPixelOffset = nPixelOffset;
    this->nLineOffset = nLineOffset;
    this->bNativeOrder = bNativeOrder;

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
/*                           ~RawRasterBand()                           */
/************************************************************************/

RawRasterBand::~RawRasterBand()

{
    FlushCache();
    
    CPLFree( pLineBuffer );
}

/************************************************************************/
/*                             AccessLine()                             */
/************************************************************************/

CPLErr RawRasterBand::AccessLine( int iLine )

{
    if( nLoadedScanline == iLine )
        return CE_None;

    if( VSIFSeek( fpRaw, nImgOffset + iLine * nLineOffset,
                  SEEK_SET ) == -1
        || VSIFRead( pLineBuffer, nPixelOffset, nBlockXSize, fpRaw ) < 1 )
    {
        // for now I just set to zero under the assumption we might
        // be trying to read from a file past the data that has
        // actually been written out.  Eventually we should differentiate
        // between newly created datasets, and existing datasets. Existing
        // datasets should generate an error in this case.
        memset( pLineBuffer, 0, nPixelOffset * nBlockXSize );
    }

/* -------------------------------------------------------------------- */
/*      Byte swap the interesting data, if required.                    */
/* -------------------------------------------------------------------- */
    if( !bNativeOrder  && eDataType != GDT_Byte )
    {
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
/*      Copy data from disk buffer to user block buffer.                */
/* -------------------------------------------------------------------- */
    GDALCopyWords( pImage, eDataType, GDALGetDataTypeSize(eDataType)/8,
                   pLineBuffer, eDataType, nPixelOffset,
                   nBlockXSize );

/* -------------------------------------------------------------------- */
/*      Byte swap (if necessary) back into disk order before writing.   */
/* -------------------------------------------------------------------- */
    if( !bNativeOrder && eDataType != GDT_Byte )
    {
        GDALSwapWords( pLineBuffer, GDALGetDataTypeSize(eDataType)/8,
                       nBlockXSize, nPixelOffset );
    }

/* -------------------------------------------------------------------- */
/*      Write to disk.                                                  */
/* -------------------------------------------------------------------- */
    if( VSIFSeek( fpRaw, nImgOffset + nBlockYOff * nLineOffset,
                  SEEK_SET ) == -1 )
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Failed to seek to scanline %d to file.\n",
                  nBlockYOff );
        
        eErr = CE_Failure;
    }
    else if( VSIFWrite( pLineBuffer, nPixelOffset, nBlockXSize, fpRaw ) < 1 )
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

