/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Free standing functions for GDAL.
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
 * Revision 1.8  2000/03/23 16:53:21  warmerda
 * use overviews for approximate min/max
 *
 * Revision 1.7  2000/03/09 23:21:44  warmerda
 * added GDALDummyProgress
 *
 * Revision 1.6  2000/03/06 21:59:44  warmerda
 * added min/max calculate
 *
 * Revision 1.5  2000/03/06 02:20:15  warmerda
 * added getname functions for colour interpretations
 *
 * Revision 1.4  1999/07/23 19:35:47  warmerda
 * added GDALGetDataTypeName
 *
 * Revision 1.3  1999/05/17 02:00:45  vgough
 * made pure_virtual C linkage
 *
 * Revision 1.2  1999/05/16 19:32:13  warmerda
 * Added __pure_virtual.
 *
 * Revision 1.1  1998/12/06 02:50:16  warmerda
 * New
 *
 */

#include "gdal_priv.h"

/************************************************************************/
/*                           __pure_virtual()                           */
/*                                                                      */
/*      The following is a gross hack to remove the last remaining      */
/*      dependency on the GNU C++ standard library.                     */
/************************************************************************/

#ifdef __GNUC__

extern "C"
void __pure_virtual()

{
}

#endif

/************************************************************************/
/*                        GDALGetDataTypeSize()                         */
/************************************************************************/
int GDALGetDataTypeSize( GDALDataType eDataType )

{
    switch( eDataType )
    {
      case GDT_Byte:
        return 8;

      case GDT_UInt16:
      case GDT_Int16:
        return 16;

      case GDT_UInt32:
      case GDT_Int32:
      case GDT_Float32:
        return 32;

      case GDT_Float64:
        return 64;

      default:
        CPLAssert( FALSE );
        return 0;
    }
}

/************************************************************************/
/*                        GDALGetDataTypeName()                         */
/************************************************************************/

const char *GDALGetDataTypeName( GDALDataType eDataType )

{
    switch( eDataType )
    {
      case GDT_Byte:
        return "Byte";

      case GDT_UInt16:
        return "UInt16";

      case GDT_Int16:
        return "Int16";

      case GDT_UInt32:
        return "UInt32";
        
      case GDT_Int32:
        return "Int32";
        
      case GDT_Float32:
        return "Float32";

      case GDT_Float64:
        return "Float64";

      default:
        return NULL;
    }
}

/************************************************************************/
/*                  GDALGetPaletteInterpretationName()                  */
/************************************************************************/

const char *GDALGetPaletteInterpretationName( GDALPaletteInterp eInterp )

{
    switch( eInterp )
    {
      case GPI_Gray:
        return "Gray";

      case GPI_RGB:
        return "RGB";
        
      case GPI_CMYK:
        return "CMYK";

      case GPI_HLS:
        return "HLS";
        
      default:
        return "Unknown";
    }
}

/************************************************************************/
/*                   GDALGetColorInterpretationName()                   */
/************************************************************************/

const char *GDALGetColorInterpretationName( GDALColorInterp eInterp )

{
    switch( eInterp )
    {
      case GCI_Undefined:
        return "Undefined";

      case GCI_GrayIndex:
        return "Gray";

      case GCI_PaletteIndex:
        return "Palette";

      case GCI_RedBand:
        return "Red";

      case GCI_GreenBand:
        return "Green";

      case GCI_BlueBand:
        return "Blue";

      case GCI_AlphaBand:
        return "Alpha";

      case GCI_HueBand:
        return "Hue";

      case GCI_SaturationBand:
        return "Saturation";

      case GCI_LightnessBand:
        return "Lightness";

      case GCI_CyanBand:
        return "Cyan";

      case GCI_MagentaBand:
        return "Magenta";

      case GCI_YellowBand:
        return "Yellow";

      case GCI_BlackBand:
        return "Black";
        
      default:
        return "Unknown";
    }
}

/************************************************************************/
/*                      GDALComputeRasterMinMax()                       */
/************************************************************************/

/**
 * Compute the min/max values for a band.
 * 
 * If approximate is OK, then the band's GetMinimum()/GetMaximum() will
 * be trusted.  If it doesn't work, a subsample of blocks will be read to
 * get an approximate min/max.  If the band has a nodata value it will
 * be excluded from the minimum and maximum.
 *
 * If bApprox is FALSE, then all pixels will be read and used to compute
 * an exact range.
 * 
 * @param hBand the band to copmute the range for.
 * @param bApproxOK TRUE if an approximate (faster) answer is OK, otherwise
 * FALSE.
 * @param adfMinMax the array in which the minimum (adfMinMax[0]) and the
 * maximum (adfMinMax[1]) are returned.
 */

void GDALComputeRasterMinMax( GDALRasterBandH hBand, int bApproxOK, 
                              double adfMinMax[2] )

{
    double       dfMin=0.0, dfMax=0.0;
    GDALRasterBand *poBand = (GDALRasterBand *) hBand;

/* -------------------------------------------------------------------- */
/*      Does the driver already know the min/max?                       */
/* -------------------------------------------------------------------- */
    if( bApproxOK )
    {
        int          bSuccessMin, bSuccessMax;

        dfMin = GDALGetRasterMinimum( hBand, &bSuccessMin );
        dfMax = GDALGetRasterMaximum( hBand, &bSuccessMax );
        if( bSuccessMin && bSuccessMax )
        {
            adfMinMax[0] = dfMin;
            adfMinMax[1] = dfMax;
            return;
        }
    }
    
/* -------------------------------------------------------------------- */
/*      If we have overview bands, use them for min/max.                */
/* -------------------------------------------------------------------- */
    if( bApproxOK && GDALGetOverviewCount(hBand) > 0 )
    {
        int     nBestSize = poBand->GetXSize() * poBand->GetYSize();
        GDALRasterBand *poBestBand = poBand;

        for( int iOverview = 0; 
             iOverview < poBand->GetOverviewCount(); 
             iOverview++ )
        {
            GDALRasterBand *poTestBand = poBand->GetOverview(iOverview);

            if( poTestBand->GetXSize() * poTestBand->GetYSize() < nBestSize )
            {
                nBestSize = poTestBand->GetXSize() * poTestBand->GetYSize();
                poBestBand = poTestBand;
            }
        }

        poBand = poBestBand;
    }
    
/* -------------------------------------------------------------------- */
/*      Figure out the ratio of blocks we will read to get an           */
/*      approximate value.                                              */
/* -------------------------------------------------------------------- */
    int         nBlockXSize, nBlockYSize;
    int         nBlocksPerRow, nBlocksPerColumn;
    int         nSampleRate;
    int         bGotNoDataValue, bFirstValue = TRUE;
    double      dfNoDataValue;

    dfNoDataValue = poBand->GetNoDataValue( &bGotNoDataValue );

    poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
    nBlocksPerRow = (poBand->GetXSize() + nBlockXSize - 1) / nBlockXSize;
    nBlocksPerColumn = (poBand->GetYSize() + nBlockYSize - 1) / nBlockYSize;

    if( bApproxOK )
        nSampleRate = 
            (int) MAX(1,sqrt((double) nBlocksPerRow * nBlocksPerColumn));
    else
        nSampleRate = 1;
    
    for( int iSampleBlock = 0; 
         iSampleBlock < nBlocksPerRow * nBlocksPerColumn;
         iSampleBlock += nSampleRate )
    {
        double dfValue = 0.0;
        int  iXBlock, iYBlock, nXCheck, nYCheck;
        GDALRasterBlock *poBlock;

        iYBlock = iSampleBlock / nBlocksPerRow;
        iXBlock = iSampleBlock - nBlocksPerRow * iYBlock;
        
        poBlock = poBand->GetBlockRef( iXBlock, iYBlock );
        
        if( (iXBlock+1) * nBlockXSize > poBand->GetXSize() )
            nXCheck = poBand->GetXSize() - iXBlock * nBlockXSize;
        else
            nXCheck = nBlockXSize;

        if( (iYBlock+1) * nBlockYSize > poBand->GetYSize() )
            nYCheck = poBand->GetYSize() - iYBlock * nBlockYSize;
        else
            nYCheck = nBlockYSize;

        /* this isn't the fastest way to do this, but is easier for now */
        for( int iY = 0; iY < nYCheck; iY++ )
        {
            for( int iX = 0; iX < nXCheck; iX++ )
            {
                int    iOffset = iX + iY * nBlockXSize;

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
                  default:
                    CPLAssert( FALSE );
                }
                
                if( bGotNoDataValue && dfValue == dfNoDataValue )
                    continue;

                if( bFirstValue )
                {
                    dfMin = dfMax = dfValue;
                    bFirstValue = FALSE;
                }
                else
                {
                    dfMin = MIN(dfMin,dfValue);
                    dfMax = MAX(dfMax,dfValue);
                }
            }
        }
    }

    adfMinMax[0] = dfMin;
    adfMinMax[1] = dfMax;
}

/************************************************************************/
/*                         GDALDummyProgress()                          */
/************************************************************************/

int GDALDummyProgress( double, const char *, void * )

{
    return TRUE;
}
