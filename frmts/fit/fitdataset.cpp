/******************************************************************************
 * $Id$
 *
 * Project:  FIT Driver
 * Purpose:  Implement FIT Support - not using the SGI iflFIT library.
 * Author:   Philip Nemec, nemec@keyholecorp.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Keyhole, Inc.
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
 * Revision 1.5  2001/07/10 02:12:40  nemec
 * Fixed writing of files whose dimensions aren't exact multiples of the page size
 *
 * Revision 1.4  2001/07/07 01:03:35  nemec
 * Fixed math on check if 64 bit seek is needed
 *
 * Revision 1.3  2001/07/06 22:02:55  nemec
 * Fixed some bugs related to large files (fseek64 when available, trap otherwise)
 * Better support for different file layouts (upper left, lower left, etc.)
 * Fixed files with dimensinos that aren't exact multiples of the page size
 *
 * Revision 1.2  2001/07/06 18:46:25  nemec
 * Cleanup files - improve Windows build, make proper copyright notice
 *
 *
 */

#include <strings.h> // for bzero

#include "fit.h"
#include "gstEndian.h"
#include "gdal_priv.h"
#include "cpl_string.h"

static GDALDriver	*poFITDriver = NULL;

CPL_C_START
void	GDALRegister_FIT(void);
CPL_C_END

#define FIT_WRITE

#define FIT_PAGE_SIZE 128

/************************************************************************/
/* ==================================================================== */
/*				FITDataset				*/
/* ==================================================================== */
/************************************************************************/

class FITRasterBand;

class FITDataset : public GDALDataset
{
    friend	FITRasterBand;
    
    FILE	*fp;
    FITinfo	*info;
    double      adfGeoTransform[6];
    
  public:
    FITDataset();
    ~FITDataset();
    static GDALDataset *Open( GDALOpenInfo * );
//     virtual CPLErr GetGeoTransform( double * );
};

#ifdef FIT_WRITE
static GDALDataset *FITCreateCopy(const char * pszFilename,
                                  GDALDataset *poSrcDS, 
                                  int bStrict, char ** papszOptions, 
                                  GDALProgressFunc pfnProgress,
                                  void * pProgressData );
#endif // FIT_WRITE

/************************************************************************/
/* ==================================================================== */
/*                            FITRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class FITRasterBand : public GDALRasterBand
{
    friend	FITDataset;
    
    unsigned long recordSize; // number of bytes of a single page/block/record
    unsigned long numXBlocks; // number of pages in the X direction
    unsigned long numYBlocks; // number of pages in the Y direction
    unsigned long bytesPerComponent;
    unsigned long bytesPerPixel;
    char *tmpImage;

public:

    FITRasterBand::FITRasterBand( FITDataset *, int );
    
    // should override RasterIO eventually.
    
    virtual CPLErr IReadBlock( int, int, void * );
//     virtual CPLErr WriteBlock( int, int, void * ); 
    virtual double GetMinimum( int *pbSuccess );
    virtual double GetMaximum( int *pbSuccess );
    virtual GDALColorInterp GetColorInterpretation();
};


/************************************************************************/
/*                           FITRasterBand()                            */
/************************************************************************/

FITRasterBand::FITRasterBand( FITDataset *poDS, int nBand )

{
    this->poDS = poDS;
    this->nBand = nBand;
    
/* -------------------------------------------------------------------- */
/*      Get the GDAL data type.                                         */
/* -------------------------------------------------------------------- */
    eDataType = fitDataType(poDS->info->dtype);

/* -------------------------------------------------------------------- */
/*      Get the page sizes.                                             */
/* -------------------------------------------------------------------- */
    nBlockXSize = poDS->info->xPageSize;
    nBlockYSize = poDS->info->yPageSize;

/* -------------------------------------------------------------------- */
/*      Caculate the values for record offset calculations.             */
/* -------------------------------------------------------------------- */
    bytesPerComponent = (GDALGetDataTypeSize(eDataType) / 8);
    bytesPerPixel = poDS->nBands * bytesPerComponent;
    recordSize = bytesPerPixel * nBlockXSize * nBlockYSize;
    numXBlocks =
        (unsigned long) ceil((double) poDS->info->xSize / nBlockXSize);
    numYBlocks =
        (unsigned long) ceil((double) poDS->info->ySize / nBlockYSize);

    tmpImage = (char *) malloc(recordSize);
    if (! tmpImage)
        CPLError(CE_Fatal, CPLE_NotSupported, 
                 "FITRasterBand couldn't allocate %lu bytes", recordSize);

/* -------------------------------------------------------------------- */
/*      Set the access flag.  For now we set it the same as the         */
/*      whole dataset, but eventually this should take account of       */
/*      locked channels, or read-only secondary data files.             */
/* -------------------------------------------------------------------- */
    /* ... */
}

/************************************************************************/
/*                            IReadBlock()                              */
/************************************************************************/

#define COPY_XFIRST(t) { \
                t *dstp = (t *) pImage; \
                t *srcp = (t *) tmpImage; \
                srcp += nBand-1; \
                long i = 0; \
                for(long y=ystart; y != ystop; y+= yinc) \
                    for(long x=xstart; x != xstop; x+= xinc, i++) { \
                        dstp[i] = srcp[(y * nBlockXSize + x) * \
                                       poFIT_DS->nBands]; \
                    } \
    }

#define COPY_YFIRST(t) { \
                t *dstp = (t *) pImage; \
                t *srcp = (t *) tmpImage; \
                srcp += nBand-1; \
                long i = 0; \
                for(long x=xstart; x != xstop; x+= xinc, i++) \
                    for(long y=ystart; y != ystop; y+= yinc) { \
                        dstp[i] = srcp[(x * nBlockYSize + y) * \
                                       poFIT_DS->nBands]; \
                    } \
    }

CPLErr FITRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )

{
    FITDataset	*poFIT_DS = (FITDataset *) poDS;

    uint64 tilenum = 0;

    switch (poFIT_DS->info->space) {
    case 1:
        // iflUpperLeftOrigin - from upper left corner
        // scan right then down
        tilenum = nBlockYOff * numXBlocks + nBlockXOff;
        break;
    case 2:
        // iflUpperRightOrigin - from upper right corner
        // scan left then down
        tilenum = numYBlocks * numXBlocks + (numXBlocks-1-nBlockXOff);
        break;
    case 3:
        // iflLowerRightOrigin - from lower right corner
        // scan left then up
        tilenum = (numYBlocks-1-nBlockYOff) * numXBlocks +
            (numXBlocks-1-nBlockXOff);
        break;
    case 4:
        // iflLowerLeftOrigin - from lower left corner
        // scan right then up
        tilenum = (numYBlocks-1-nBlockYOff) * numXBlocks + nBlockXOff;
        break;
    case 5:
        // iflLeftUpperOrigin -* from upper left corner
        // scan down then right
        tilenum = nBlockXOff * numYBlocks + nBlockYOff;
        break;
    case 6:
        // iflRightUpperOrigin - from upper right corner
        // scan down then left
        tilenum = (numXBlocks-1-nBlockXOff) * numYBlocks + nBlockYOff;
        break;
    case 7:
        // iflRightLowerOrigin - from lower right corner
        // scan up then left
        tilenum = nBlockXOff * numYBlocks + (numYBlocks-1-nBlockYOff);
        break;
    case 8:
        // iflLeftLowerOrigin -* from lower left corner
        // scan up then right
        tilenum = (numXBlocks-1-nBlockXOff) * numYBlocks +
            (numYBlocks-1-nBlockYOff);
        break;
    default:
        CPLError(CE_Failure, CPLE_NotSupported,
                 "FIT - unrecognized image space %i",
                 poFIT_DS->info->space);
        tilenum = 0;
    } // switch

    uint64 offset = poFIT_DS->info->dataOffset + recordSize * tilenum;
//     CPLDebug("FIT", "%i RasterBand::IReadBlock %i %i (out of %i %i) -- %i",
//              poFIT_DS->info->space,
//              nBlockXOff, nBlockYOff, numXBlocks, numYBlocks, tilenum);

    VSIFSeekL( poFIT_DS->fp, offset, SEEK_SET );

    // XXX - should handle status
    // fast path is single component (ll?) - no copy needed
    char *p;
    int fastpath = FALSE;

    if ((poFIT_DS->nBands == 1) && (poFIT_DS->info->space == 1)) // upper left
        fastpath = TRUE;

    if (! fastpath) {
        fread(tmpImage, recordSize, 1, poFIT_DS->fp);
        // offset to correct component to swap
        p = (char *) tmpImage + nBand-1;
    }
    else {
        fread(pImage, recordSize, 1, poFIT_DS->fp);
        p = (char *) pImage;
    }

#ifdef swapping
    switch(bytesPerComponent) {
    case 1:
        // do nothing
        break;
    case 2:
        for(unsigned long i=0; i < recordSize; i+= bytesPerPixel)
            swap16(p + i);
        break;
    case 4:
        for(unsigned long i=0; i < recordSize; i+= bytesPerPixel)
            swap32(p + i);
        break;
    case 8:
        for(unsigned long i=0; i < recordSize; i+= bytesPerPixel)
            swap64(p + i);
        break;
    default:
        CPLError(CE_Failure, CPLE_NotSupported, 
                 "FITRasterBand::IReadBlock unsupported bytesPerPixel %lu",
                 bytesPerComponent);
    } // switch
#endif // swapping

    if (! fastpath) {
        long xinc, yinc, xstart, ystart, xstop, ystop;
        if (poFIT_DS->info->space <= 4) {
            // scan left/right first

            switch (poFIT_DS->info->space) {
            case 1:
                // iflUpperLeftOrigin - from upper left corner
                // scan right then down
                xinc = 1;
                yinc = 1;
                break;
            case 2:
                // iflUpperRightOrigin - from upper right corner
                // scan left then down
                xinc = -1;
                yinc = 1;
                break;
            case 3:
                // iflLowerRightOrigin - from lower right corner
                // scan left then up
                xinc = -1;
                yinc = -1;
                break;
            case 4:
                // iflLowerLeftOrigin - from lower left corner
                // scan right then up
                xinc = 1;
                yinc = -1;
               break;
            default:
                CPLError(CE_Failure, CPLE_NotSupported,
                         "FIT - unrecognized image space %i",
                         poFIT_DS->info->space);
                xinc = 1;
                yinc = 1;
            } // switch


            if (xinc == 1) {
                xstart = 0;
                xstop = nBlockXSize;
            }
            else {
                xstart = nBlockXSize-1;
                xstop = -1;
            }
            if (yinc == 1) {
                ystart = 0;
                ystop = nBlockYSize;
            }
            else {
                ystart = nBlockYSize-1;
                ystop = -1;
            }

            switch(bytesPerComponent) {
            case 1:
                COPY_XFIRST(char);
                break;
            case 2:
                COPY_XFIRST(uint16);
                break;
            case 4:
                COPY_XFIRST(uint32);
                break;
            case 8:
                COPY_XFIRST(uint64);
                break;
            default:
                CPLError(CE_Failure, CPLE_NotSupported, 
                         "FITRasterBand::IReadBlock unsupported "
                         "bytesPerComponent % %lu", bytesPerComponent);
            } // switch

        } // scan left/right first
        else {
            // scan up/down first

            switch (poFIT_DS->info->space) {
            case 5:
                // iflLeftUpperOrigin -* from upper left corner
                // scan down then right
                xinc = 1;
                yinc = 1;
                break;
            case 6:
                // iflRightUpperOrigin - from upper right corner
                // scan down then left
                xinc = -1;
                yinc = 1;
                break;
            case 7:
                // iflRightLowerOrigin - from lower right corner
                // scan up then left
                xinc = -1;
                yinc = -1;
                break;
            case 8:
                // iflLeftLowerOrigin -* from lower left corner
                // scan up then right
                xinc = 1;
                yinc = -1;
                break;
            default:
                CPLError(CE_Failure, CPLE_NotSupported,
                         "FIT - unrecognized image space %i",
                         poFIT_DS->info->space);
                xinc = 1;
                yinc = 1;
            } // switch

            if (xinc == 1) {
                xstart = 0;
                xstop = nBlockXSize;
            }
            else {
                xstart = nBlockXSize-1;
                xstop = -1;
            }
            if (yinc == 1) {
                ystart = 0;
                ystop = nBlockYSize;
            }
            else {
                ystart = nBlockYSize-1;
                ystop = -1;
            }

            switch(bytesPerComponent) {
            case 1:
                COPY_YFIRST(char);
                break;
            case 2:
                COPY_YFIRST(uint16);
                break;
            case 4:
                COPY_YFIRST(uint32);
                break;
            case 8:
                COPY_YFIRST(uint64);
                break;
            default:
                CPLError(CE_Failure, CPLE_NotSupported, 
                         "FITRasterBand::IReadBlock unsupported "
                         "bytesPerComponent % %lu", bytesPerComponent);
            } // switch

        } // scan up/down first

    } // ! fastpath
    return CE_None;
}

#if 0
/************************************************************************/
/*                             ReadBlock()                              */
/************************************************************************/

CPLErr FITRasterBand::ReadBlock( int nBlockXOff, int nBlockYOff,
                                 void * pImage )

{
    FITDataset	*poFIT_DS = (FITDataset *) poDS;

    

    return CE_None;
}

/************************************************************************/
/*                             WriteBlock()                             */
/************************************************************************/

CPLErr FITRasterBand::WriteBlock( int nBlockXOff, int nBlockYOff,
                                 void * pImage )

{
    FITDataset	*poFIT_DS = (FITDataset *) poDS;

    

    return CE_None;
}
#endif

/************************************************************************/
/*                             GetMinimum()                             */
/************************************************************************/

double FITRasterBand::GetMinimum( int *pbSuccess )
{
    FITDataset *poFIT_DS = (FITDataset *) poDS;

    if ((! poFIT_DS) || (! poFIT_DS->info))
        return GDALRasterBand::GetMinimum( pbSuccess );

    if (pbSuccess)
        *pbSuccess = TRUE;

    if (poFIT_DS->info->version &&
        EQUALN((const char *) &(poFIT_DS->info->version), "02", 2)) {
        return poFIT_DS->info->minValue;
    }
    else {
        return GDALRasterBand::GetMinimum( pbSuccess );
    }
}

/************************************************************************/
/*                             GetMaximum()                             */
/************************************************************************/

double FITRasterBand::GetMaximum( int *pbSuccess )
{
    FITDataset *poFIT_DS = (FITDataset *) poDS;

    if ((! poFIT_DS) || (! poFIT_DS->info))
        return GDALRasterBand::GetMaximum( pbSuccess );

    if (pbSuccess)
        *pbSuccess = TRUE;

    if (EQUALN((const char *) &poFIT_DS->info->version, "02", 2)) {
        return poFIT_DS->info->maxValue;
    }
    else {
        return GDALRasterBand::GetMaximum( pbSuccess );
    }
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

GDALColorInterp FITRasterBand::GetColorInterpretation()
{
    FITDataset	*poFIT_DS = (FITDataset *) poDS;

    if ((! poFIT_DS) || (! poFIT_DS->info))
        return GCI_Undefined;

    switch(poFIT_DS->info->cm) {
    case 1: // iflNegative - inverted luminance (min value is white)
        CPLError( CE_Warning, CPLE_NotSupported, 
                  "FIT - color model Negative not supported - ignoring model",
                  poFIT_DS->info->cm);
            return GCI_Undefined;

    case 2: // iflLuminance - luminance
        if (poFIT_DS->nBands != 1) {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model Luminance mismatch with %i bands",
                      poFIT_DS->nBands);
            return GCI_Undefined;
        }
        switch (nBand) {
        case 1:
            return GCI_GrayIndex;
        default:
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model Luminance unknown band %i", nBand);
            return GCI_Undefined;
        } // switch nBand

    case 3: // iflRGB - full color (Red, Green, Blue triplets)
        if (poFIT_DS->nBands != 3) {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model RGB mismatch with %i bands",
                      poFIT_DS->nBands);
            return GCI_Undefined;
        }
        switch (nBand) {
        case 1:
            return GCI_RedBand;
        case 2:
            return GCI_GreenBand;
        case 3:
            return GCI_BlueBand;
        default:
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model RGB unknown band %i", nBand);
            return GCI_Undefined;
        } // switch nBand

    case 4: // iflRGBPalette - color mapped values
        CPLError( CE_Warning, CPLE_NotSupported, 
                  "FIT - color model RGBPalette not supported - "
                  "ignoring model",
                  poFIT_DS->info->cm);
            return GCI_Undefined;

    case 5: // iflRGBA - full color with transparency (alpha channel)
        if (poFIT_DS->nBands != 4) {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model RGBA mismatch with %i bands",
                      poFIT_DS->nBands);
            return GCI_Undefined;
        }
        switch (nBand) {
        case 1:
            return GCI_RedBand;
        case 2:
            return GCI_GreenBand;
        case 3:
            return GCI_BlueBand;
        case 4:
            return GCI_AlphaBand;
        default:
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model RGBA unknown band %i", nBand);
            return GCI_Undefined;
        } // switch nBand

    case 6: // iflHSV - Hue, Saturation, Value
        if (poFIT_DS->nBands != 3) {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model HSV mismatch with %i bands",
                      poFIT_DS->nBands);
            return GCI_Undefined;
        }
        switch (nBand) {
        case 1:
            return GCI_HueBand;
        case 2:
            return GCI_SaturationBand;
        case 3:
            return GCI_LightnessBand;
        default:
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model HSV unknown band %i", nBand);
            return GCI_Undefined;
        } // switch nBand

    case 7: // iflCMY - Cyan, Magenta, Yellow
        if (poFIT_DS->nBands != 3) {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model CMY mismatch with %i bands",
                      poFIT_DS->nBands);
            return GCI_Undefined;
        }
        switch (nBand) {
        case 1:
            return GCI_CyanBand;
        case 2:
            return GCI_MagentaBand;
        case 3:
            return GCI_YellowBand;
        default:
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model CMY unknown band %i", nBand);
            return GCI_Undefined;
        } // switch nBand

    case 8: // iflCMYK - Cyan, Magenta, Yellow, Black
        if (poFIT_DS->nBands != 4) {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model CMYK mismatch with %i bands",
                      poFIT_DS->nBands);
            return GCI_Undefined;
        }
        switch (nBand) {
        case 1:
            return GCI_CyanBand;
        case 2:
            return GCI_MagentaBand;
        case 3:
            return GCI_YellowBand;
        case 4:
            return GCI_BlackBand;
        default:
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model CMYK unknown band %i", nBand);
            return GCI_Undefined;
        } // switch nBand

    case 9: // iflBGR - full color (ordered Blue, Green, Red)
        if (poFIT_DS->nBands != 3) {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model BGR mismatch with %i bands",
                      poFIT_DS->nBands);
            return GCI_Undefined;
        }
        switch (nBand) {
        case 1:
            return GCI_BlueBand;
        case 2:
            return GCI_GreenBand;
        case 3:
            return GCI_RedBand;
        default:
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model BGR unknown band %i", nBand);
            return GCI_Undefined;
        } // switch nBand

    case 10: // iflABGR - Alpha, Blue, Green, Red (SGI frame buffers)
        if (poFIT_DS->nBands != 4) {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model ABGR mismatch with %i bands",
                      poFIT_DS->nBands);
            return GCI_Undefined;
        }
        switch (nBand) {
        case 1:
            return GCI_AlphaBand;
        case 2:
            return GCI_BlueBand;
        case 3:
            return GCI_GreenBand;
        case 4:
            return GCI_RedBand;
        default:
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model ABGR unknown band %i", nBand);
            return GCI_Undefined;
        } // switch nBand

    case 11: // iflMultiSpectral - multi-spectral data, arbitrary number of
        // chans
        return GCI_Undefined;

    case 12: // iflYCC PhotoCD color model (Luminance, Chrominance)
        CPLError( CE_Warning, CPLE_NotSupported, 
                  "FIT - color model YCC not supported - ignoring model",
                  poFIT_DS->info->cm);
            return GCI_Undefined;

    case 13: // iflLuminanceAlpha - Luminance plus alpha
        if (poFIT_DS->nBands != 2) {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model LuminanceAlpha mismatch with "
                      "%i bands",
                      poFIT_DS->nBands);
            return GCI_Undefined;
        }
        switch (nBand) {
        case 1:
            return GCI_GrayIndex;
        case 2:
            return GCI_AlphaBand;
        default:
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "FIT - color model LuminanceAlpha unknown band %i",
                      nBand);
            return GCI_Undefined;
        } // switch nBand

    default:
        CPLError( CE_Warning, CPLE_NotSupported, 
                  "FIT - unrecognized color model %i - ignoring model",
                  poFIT_DS->info->cm);
        return GCI_Undefined;
    } // switch
}

/************************************************************************/
/*                             FITDataset()                             */
/************************************************************************/

FITDataset::FITDataset()
{
    fp = NULL;
    info = NULL;

    adfGeoTransform[0] = 0.0; // x origin (top left corner)
    adfGeoTransform[1] = 1.0; // x pixel size
    adfGeoTransform[2] = 0.0;

    adfGeoTransform[3] = 0.0; // y origin (top left corner)
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0; // y pixel size
}

/************************************************************************/
/*                             ~FITDataset()                             */
/************************************************************************/

FITDataset::~FITDataset()
{
    if (info)
        delete(info);
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *FITDataset::Open( GDALOpenInfo * poOpenInfo )
{
/* -------------------------------------------------------------------- */
/*	First we check to see if the file has the expected header	*/
/*	bytes.								*/    
/* -------------------------------------------------------------------- */
    if( poOpenInfo->nHeaderBytes < 5 )
        return NULL;

    if( !EQUALN((const char *) poOpenInfo->pabyHeader, "IT01", 4) &&
        !EQUALN((const char *) poOpenInfo->pabyHeader, "IT02", 4) )
        return NULL;

    if( poOpenInfo->eAccess == GA_Update )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "The FIT driver does not support update access to existing"
                  " files.\n" );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    FITDataset 	*poDS;

    poDS = new FITDataset();
    poDS->eAccess = GA_ReadOnly;

    // take over ownership of fp - only works for read only
    poDS->fp = poOpenInfo->fp;
    poOpenInfo->fp = NULL;

    poDS->info = new FITinfo;
    FITinfo *info = poDS->info;

/* -------------------------------------------------------------------- */
/*      Read other header values.                                       */
/* -------------------------------------------------------------------- */
    FIThead02 *head = (FIThead02 *) poOpenInfo->pabyHeader;

    // extract the image attributes from the file header
    if (EQUALN((const char *) &head->version, "02", 2)) {
        // incomplete header
        if( poOpenInfo->nHeaderBytes < (signed) sizeof(FIThead02) )
            return NULL;

        CPLDebug("FIT", "Loading file with header version 02");

        swapb(head->minValue);
	info->minValue = head->minValue;
        swapb(head->maxValue);
	info->maxValue = head->maxValue;
        swapb(head->dataOffset);
	info->dataOffset = head->dataOffset;

        info->userOffset = sizeof(FIThead02);
    }
    else if (EQUALN((const char *) &head->version, "01", 2)) {
        // incomplete header
        if( poOpenInfo->nHeaderBytes < (signed) sizeof(FIThead01) )
            return NULL;

        CPLDebug("FIT", "Loading file with header version 01");

        // map old style header into new header structure
	FIThead01* head01 = (FIThead01*)&head;
        swapb(head->dataOffset);
	info->dataOffset = head01->dataOffset;

        info->userOffset = sizeof(FIThead01);
    }
    else {
        // unrecognized header version
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "FIT - unsupported header version %.2s\n",
                  &head->version);
        return NULL;
    }

    CPLDebug("FIT", "userOffset %i, dataOffset %i",
             info->userOffset, info->dataOffset);

    info->magic = head->magic;
    info->version = head->version;

    swapb(head->xSize);
    info->xSize = head->xSize;
    swapb(head->ySize);
    info->ySize = head->ySize;
    swapb(head->zSize);
    info->zSize = head->zSize;
    swapb(head->cSize);
    info->cSize = head->cSize;
    swapb(head->dtype);
    info->dtype = head->dtype;
    swapb(head->order);
    info->order = head->order;
    swapb(head->space);
    info->space = head->space;
    swapb(head->cm);
    info->cm = head->cm;
    swapb(head->xPageSize);
    info->xPageSize = head->xPageSize;
    swapb(head->yPageSize);
    info->yPageSize = head->yPageSize;
    swapb(head->zPageSize);
    info->zPageSize = head->zPageSize;
    swapb(head->cPageSize);
    info->cPageSize = head->cPageSize;

    /**************************/

    poDS->nRasterXSize = head->xSize;
    poDS->nRasterYSize = head->ySize;
    poDS->nBands = head->cSize;

/* -------------------------------------------------------------------- */
/*      Check if 64 bit seek is needed.                                 */
/* -------------------------------------------------------------------- */
    uint64 maxseek = head->cSize * head->xSize * head->ySize *
        GDALGetDataTypeSize(fitDataType(poDS->info->dtype));
    // 
    // 0xffffffff80000000LL // anything about 31 bits

//     if (maxseek & 0xffffffff00000000LL) // anything about 32 bits
    if (maxseek & 0xffffffff80000000LL) // signed long
#ifdef VSI_LARGE_API_SUPPORTED
        CPLDebug("FIT", "Using 64 bit version of fseek");
#else
        CPLError(CE_Fatal, CPLE_NotSupported, 
                 "FIT - need 64 bit version of fseek");
#endif

/* -------------------------------------------------------------------- */
/*      Verify all "unused" header values.                              */
/* -------------------------------------------------------------------- */

    if( info->zSize != 1 )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "FIT driver - unsupported zSize %i\n", info->zSize);
        return NULL;
    }

    if( info->order != 1 ) // interleaved - RGBRGB
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "FIT driver - unsupported order %i\n", info->order);
        return NULL;
    }

    if( info->zPageSize != 1 )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "FIT driver - unsupported zPageSize %i\n", info->zPageSize);
        return NULL;
    }

    if( info->cPageSize != info->cSize )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "FIT driver - unsupported cPageSize %i (!= %i)\n",
                  info->cPageSize, info->cSize);
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( int i = 0; i < poDS->nBands; i++ )
    {
        poDS->SetBand( i+1,  new FITRasterBand( poDS, i+1 ) ) ;
    }

    return poDS;
}

/************************************************************************/
/*                           FITCreateCopy()                            */
/************************************************************************/

#ifdef FIT_WRITE
static GDALDataset *FITCreateCopy(const char * pszFilename,
                                  GDALDataset *poSrcDS, 
                                  int bStrict, char ** papszOptions, 
                                  GDALProgressFunc pfnProgress,
                                  void * pProgressData )
{
    CPLDebug("FIT", "CreateCopy %s - %i", pszFilename, bStrict);

/* -------------------------------------------------------------------- */
/*      Create the dataset.                                             */
/* -------------------------------------------------------------------- */
    FILE	*fpImage;

    if( !pfnProgress( 0.0, NULL, pProgressData ) )
    {
        CPLError( CE_Failure, CPLE_UserInterrupt, "User terminated" );
        return NULL;
    }

    fpImage = VSIFOpen( pszFilename, "wb" );
    if( fpImage == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "FIT - unable to create file %s.\n", 
                  pszFilename );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Generate header.                                                */
/* -------------------------------------------------------------------- */
    // XXX - shoould FIT_PAGE_SIZE based on file page size ??
    int size = MAX(sizeof(FIThead02), FIT_PAGE_SIZE);
    FIThead02 *head = (FIThead02 *) malloc(size);

    strncpy((char *) &head->magic, "IT", 2);
    strncpy((char *) &head->version, "02", 2);

    head->xSize = poSrcDS->GetRasterXSize();
    swapb(head->xSize);
    head->ySize = poSrcDS->GetRasterYSize();
    swapb(head->ySize);
    head->zSize = 1;
    swapb(head->zSize);
    int nBands = poSrcDS->GetRasterCount();
    head->cSize = nBands;
    swapb(head->cSize);

    GDALRasterBand *firstBand = poSrcDS->GetRasterBand(1);
    if (! firstBand) {
        free(head);
        return NULL;
    }

    head->dtype = fitGetDataType(firstBand->GetRasterDataType());
    if (! head->dtype) {
        free(head);
        return NULL;
    }
    swapb(head->dtype);
    head->order = 1; // interleaved - RGBRGB
    swapb(head->order);
    head->space = 1; // upper left
    swapb(head->space);

    // XXX - need to check all bands
    head->cm = fitGetColorModel(firstBand->GetColorInterpretation(), nBands);
    swapb(head->cm);

    int blockX, blockY;
    firstBand->GetBlockSize(&blockX, &blockY);
    CPLDebug("FIT write", "inherited block size %ix%i", blockX, blockY);

    if( CSLFetchNameValue(papszOptions,"PAGESIZE") != NULL )
    {
        const char *str = CSLFetchNameValue(papszOptions,"PAGESIZE");
        int newBlockX, newBlockY;
        sscanf(str, "%i,%i", &newBlockX, &newBlockY);
        if (newBlockX && newBlockY) {
            blockX = newBlockX;
            blockY = newBlockY;
        }
        else {
            CPLError(CE_Failure, CPLE_OpenFailed, 
                     "FIT - Unable to parse option PAGESIZE values [%s]", str);
        }
    }

    // XXX - need to do lots of checking of block size
    // * provide ability to override block size with options
    // * handle non-square block size (like scanline)
    //   - probably default from non-tiled image - have default block size
    // * handle block size bigger than image size
    // * undesirable block size (non power of 2, others?)
    // * mismatched block sizes for different bands
    // * image that isn't even pages (ie. partially empty pages at edge)
    CPLDebug("FIT write", "using block size %ix%i", blockX, blockY);

    head->xPageSize = blockX;
    swapb(head->xPageSize);
    head->yPageSize = blockY;
    swapb(head->yPageSize);
    head->zPageSize = 1;
    swapb(head->zPageSize);
    head->cPageSize = nBands;
    swapb(head->cPageSize);

    // XXX - need to check all bands
    head->minValue = firstBand->GetMinimum();
    swapb(head->minValue);
    // XXX - need to check all bands
    head->maxValue = firstBand->GetMaximum();
    swapb(head->maxValue);
    head->dataOffset = size;
    swapb(head->dataOffset);

    fwrite(head, size, 1, fpImage);

/* -------------------------------------------------------------------- */
/*      Loop over image, copying image data.                            */
/* -------------------------------------------------------------------- */
    unsigned long bytesPerComponent =
        (GDALGetDataTypeSize(firstBand->GetRasterDataType()) / 8);
    unsigned long bytesPerPixel = nBands * bytesPerComponent;

    unsigned long pageBytes = blockX * blockY * bytesPerPixel;
    char *output = (char *) malloc(pageBytes);
    if (! output)
        CPLError(CE_Fatal, CPLE_NotSupported, 
                 "FITRasterBand couldn't allocate %lu bytes", pageBytes);

    long maxx = (long) ceil(poSrcDS->GetRasterXSize() / (double) blockX);
    long maxy = (long) ceil(poSrcDS->GetRasterYSize() / (double) blockY);
    long maxx_full = (long) floor(poSrcDS->GetRasterXSize() / (double) blockX);
    long maxy_full = (long) floor(poSrcDS->GetRasterYSize() / (double) blockY);

    CPLDebug("FIT", "about to write %ix%i blocks", maxx, maxy);

    for(long y=0; y < maxy; y++)
        for(long x=0; x < maxx; x++) {
            long readX = blockX;
            long readY = blockY;
            int do_clean = FALSE;

            // handle cases where image size isn't an exact multiple
            // of page size
            if (x >= maxx_full) {
                readX = poSrcDS->GetRasterXSize() % blockX;
                do_clean = TRUE;
            }
            if (y >= maxy_full) {
                readY = poSrcDS->GetRasterYSize() % blockY;
                do_clean = TRUE;
            }

            // clean out image sinnce only doing partial reads
            bzero(output, pageBytes);

            for( int iBand = 0; iBand < nBands; iBand++ ) {
                GDALRasterBand * poBand = poSrcDS->GetRasterBand( iBand+1 );
                CPLErr eErr =
                    poBand->RasterIO( GF_Read, // eRWFlag
                                      x * blockX, // nXOff
                                      y * blockY, // nYOff
                                      readX, // nXSize
                                      readY, // nYSize
                                      output + iBand * bytesPerComponent,
                                      // pData
                                      blockX, // nBufXSize
                                      blockY, // nBufYSize
                                      firstBand->GetRasterDataType(),
                                      // eBufType
                                      bytesPerPixel, // nPixelSpace
                                      bytesPerPixel * blockX); // nLineSpace
                if (eErr != CE_None)
                    CPLError(CE_Failure, CPLE_FileIO, 
                             "FIT write - CreateCopy got read error %i", eErr);
            } // for iBand

#ifdef swapping
            char *p = output;
            switch(bytesPerComponent) {
            case 1:
                // do nothing
                break;
            case 2:
                for(unsigned long i=0; i < pageBytes; i+= bytesPerComponent)
                    swap16(p + i);
                break;
            case 4:
                for(unsigned long i=0; i < pageBytes; i+= bytesPerComponent)
                    swap32(p + i);
                break;
            case 8:
                for(unsigned long i=0; i < pageBytes; i+= bytesPerComponent)
                    swap64(p + i);
                break;
            default:
                CPLError(CE_Failure, CPLE_NotSupported, 
                         "FIT write - unsupported bytesPerPixel %lu",
                         bytesPerComponent);
            } // switch
#endif // swapping
            
            fwrite(output, pageBytes, 1, fpImage);

            double perc = ((double) (y * maxx + x)) / (maxx * maxy);
//             printf("progress %f\n", perc);
            if( !pfnProgress( perc, NULL, pProgressData ) )
            {
                CPLError( CE_Failure, CPLE_UserInterrupt, "User terminated" );
                free(output);
                VSIFClose( fpImage );
                VSIUnlink( pszFilename );
                return NULL;
            }
        } // for x

    free(output);

    VSIFClose( fpImage );

    pfnProgress( 1.0, NULL, pProgressData );

    return (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );
}
#endif // FIT_WRITE

/************************************************************************/
/*                           GetGeoTransform()                          */
/************************************************************************/

// CPLErr FITDataset::GetGeoTransform( double * padfTransform )
// {
//     CPLDebug("FIT", "FITDataset::GetGeoTransform");
//     memcpy( padfTransform, adfGeoTransform, sizeof(double) * 6 );
//     return( CE_None );
// }

/************************************************************************/
/*                          GDALRegister_FIT()                          */
/************************************************************************/

void GDALRegister_FIT()

{
    GDALDriver	*poDriver;

    if( poFITDriver == NULL )
    {
        poFITDriver = poDriver = new GDALDriver();
        
        poDriver->pszShortName = "FIT";
        poDriver->pszLongName = "FIT image";
        
        poDriver->pfnOpen = FITDataset::Open;
#ifdef FIT_WRITE
        poDriver->pfnCreateCopy = FITCreateCopy;
#endif // FIT_WRITE

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}
