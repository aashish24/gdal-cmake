/******************************************************************************
 * $Id$
 *
 * Project:  GDAL 
 * Purpose:  ECW (ERMapper Wavelet Compression Format) Driver
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam
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
 *****************************************************************************
 *
 * $Log$
 * Revision 1.7  2002/07/23 14:05:35  warmerda
 * Avoid warnings.
 *
 * Revision 1.6  2002/06/12 21:12:25  warmerda
 * update to metadata based driver info
 *
 * Revision 1.5  2002/01/22 14:48:19  warmerda
 * Fixed pabyWorkBuffer memory leak in IRasterIO().
 *
 * Revision 1.4  2001/11/11 23:50:59  warmerda
 * added required class keyword to friend declarations
 *
 * Revision 1.3  2001/07/18 04:51:56  warmerda
 * added CPL_CVSID
 *
 * Revision 1.2  2001/04/20 03:06:07  warmerda
 * addec compression support
 *
 * Revision 1.1  2001/04/02 17:06:46  warmerda
 * New
 *
 */

#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_string.h"
#include <NCSECWClient.h>
#include <NCSECWCompressClient.h>
#include <NCSErrors.h>

CPL_CVSID("$Id$");

static GDALDriver	*poECWDriver = NULL;

#ifdef FRMT_ecw

typedef struct {
    int              bCancelled;
    GDALProgressFunc pfnProgress;
    void             *pProgressData;
    GDALDataset      *poSrc;
} ECWCompressInfo;

/************************************************************************/
/* ==================================================================== */
/*				ECWDataset				*/
/* ==================================================================== */
/************************************************************************/

class ECWRasterBand;

class CPL_DLL ECWDataset : public GDALDataset
{
    friend class ECWRasterBand;

    NCSFileView *hFileView;
    NCSFileViewFileInfo *psFileInfo;

  public:
    		ECWDataset();
    		~ECWDataset();
                
    static GDALDataset *Open( GDALOpenInfo * );

    virtual CPLErr GetGeoTransform( double * );
};

/************************************************************************/
/* ==================================================================== */
/*                            ECWRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class ECWRasterBand : public GDALRasterBand
{
    friend class ECWDataset;
    
    ECWDataset     *poGDS;

    virtual CPLErr IRasterIO( GDALRWFlag, int, int, int, int,
                              void *, int, int, GDALDataType,
                              int, int );

  public:

                   ECWRasterBand( ECWDataset *, int );
                   ~ECWRasterBand();

    virtual CPLErr IReadBlock( int, int, void * );
    virtual int    HasArbitraryOverviews() { return TRUE; }
};

/************************************************************************/
/*                           ECWRasterBand()                            */
/************************************************************************/

ECWRasterBand::ECWRasterBand( ECWDataset *poDS, int nBand )

{
    this->poDS = poDS;
    this->nBand = nBand;
    eDataType = GDT_Byte;
    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;
}

/************************************************************************/
/*                          ~ECWRasterBand()                           */
/************************************************************************/

ECWRasterBand::~ECWRasterBand()

{
    FlushCache();
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr ECWRasterBand::IReadBlock( int, int nBlockYOff, void * pImage )

{
    return IRasterIO( GF_Read, 0, nBlockYOff, nBlockXSize, 1, 
                      pImage, nBlockXSize, 1, GDT_Byte, 0, 0 );
}

/************************************************************************/
/*                             IRasterIO()                              */
/************************************************************************/

CPLErr ECWRasterBand::IRasterIO( GDALRWFlag eRWFlag,
                                 int nXOff, int nYOff, int nXSize, int nYSize,
                                 void * pData, int nBufXSize, int nBufYSize,
                                 GDALDataType eBufType,
                                 int nPixelSpace, int nLineSpace )
    
{
    ECWDataset	*poODS = (ECWDataset *) poDS;
    NCSError     eNCSErr;
    int          iBand, bDirect;
    GByte        *pabyWorkBuffer = NULL;

    CPLDebug( "ECWRasterBand", 
              "RasterIO(%d,%d,%d,%d -> %dx%d)\n", 
              nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize );

    if( nLineSpace == 0 )
        nLineSpace = nBufXSize;

    if( nPixelSpace == 0 )
        nPixelSpace = 1;

/* -------------------------------------------------------------------- */
/*      Can we perform direct loads, or must we load into a working     */
/*      buffer, and transform?                                          */
/* -------------------------------------------------------------------- */
    bDirect = nPixelSpace == 1 && eBufType == GDT_Byte;
    if( !bDirect )
        pabyWorkBuffer = (GByte *) CPLMalloc(nBufXSize);

/* -------------------------------------------------------------------- */
/*      Establish access at the desired resolution.                     */
/* -------------------------------------------------------------------- */
    iBand = nBand-1;
    eNCSErr = NCScbmSetFileView( poODS->hFileView, 
                                 1, (unsigned int *) (&iBand),
                                 nXOff, nYOff, 
                                 nXOff + nXSize - 1, 
                                 nYOff + nYSize - 1,
                                 nBufXSize, nBufYSize );
    if( eNCSErr != NCS_SUCCESS )
    {
        CPLFree( pabyWorkBuffer );
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s", NCSGetErrorText(eNCSErr) );
        
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Read back one scanline at a time, till request is satisfied.    */
/* -------------------------------------------------------------------- */
    int      iScanline;

    for( iScanline = 0; iScanline < nBufYSize; iScanline++ )
    {
        NCSEcwReadStatus  eRStatus;
        unsigned char *pabySrcBuf;

        if( bDirect )
            pabySrcBuf = ((GByte *)pData)+iScanline*nLineSpace;
        else
            pabySrcBuf = pabyWorkBuffer;

        eRStatus = NCScbmReadViewLineBIL( poODS->hFileView, &pabySrcBuf );

        if( eRStatus != NCSECW_READ_OK )
        {
            CPLFree( pabyWorkBuffer );
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "NCScbmReadViewLineBIL failed." );
            return CE_Failure;
        }

        if( !bDirect )
        {
            GDALCopyWords( pabyWorkBuffer, GDT_Byte, 1, 
                           ((GByte *)pData)+iScanline*nLineSpace, 
                           eBufType, nPixelSpace, nBufXSize );
        }
    }

    CPLFree( pabyWorkBuffer );

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                            ECWDataset                               */
/* ==================================================================== */
/************************************************************************/


/************************************************************************/
/*                            ECWDataset()                            */
/************************************************************************/

ECWDataset::ECWDataset()

{
}

/************************************************************************/
/*                           ~ECWDataset()                            */
/************************************************************************/

ECWDataset::~ECWDataset()

{
}


/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *ECWDataset::Open( GDALOpenInfo * poOpenInfo )

{
    NCSFileView      *hFileView = NULL;
    NCSError         eErr;
    int              i;

    if( !EQUAL(CPLGetExtension(poOpenInfo->pszFilename),"ecw") )
        return( NULL );

/* -------------------------------------------------------------------- */
/*      Open the client interface.                                      */
/* -------------------------------------------------------------------- */
    eErr = NCScbmOpenFileView( poOpenInfo->pszFilename, &hFileView, NULL );
    if( eErr != NCS_SUCCESS )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s", NCSGetErrorText(eErr) );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    ECWDataset 	*poDS;

    poDS = new ECWDataset();

    poDS->poDriver = poECWDriver;
    poDS->hFileView = hFileView;
    
/* -------------------------------------------------------------------- */
/*      Fetch general file information.                                 */
/* -------------------------------------------------------------------- */
    eErr = NCScbmGetViewFileInfo( hFileView, &(poDS->psFileInfo) );
    if( eErr != NCS_SUCCESS )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s", NCSGetErrorText(eErr) );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Establish raster info.                                          */
/* -------------------------------------------------------------------- */
    poDS->nRasterXSize = poDS->psFileInfo->nSizeX; 
    poDS->nRasterYSize = poDS->psFileInfo->nSizeY;

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( i=0; i < poDS->psFileInfo->nBands; i++ )
    {
        poDS->SetBand( i+1, new ECWRasterBand( poDS, i+1 ) );
    }

    return( poDS );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr ECWDataset::GetGeoTransform( double * padfTransform )

{
    padfTransform[0] = psFileInfo->fOriginX;
    padfTransform[1] = psFileInfo->fCellIncrementX;
    padfTransform[2] = 0.0;

    padfTransform[3] = psFileInfo->fOriginY;
    padfTransform[4] = 0.0;
    padfTransform[5] = psFileInfo->fCellIncrementY;

    return( CE_None );
}

/************************************************************************/
/*                         ECWCompressReadCB()                          */
/************************************************************************/

static BOOLEAN ECWCompressReadCB( struct NCSEcwCompressClient *psClient, 
                                  UINT32 nNextLine, IEEE4 **ppInputArray )

{
    ECWCompressInfo      *psCompressInfo;
    int                  iBand;

    psCompressInfo = (ECWCompressInfo *) psClient->pClientData;

    for( iBand = 0; iBand < (int) psClient->nInputBands; iBand++ )
    {
        GDALRasterBand      *poBand;


        poBand = psCompressInfo->poSrc->GetRasterBand( iBand+1 );

        if( poBand->RasterIO( GF_Read, 0, nNextLine, poBand->GetXSize(), 1, 
                              ppInputArray[iBand], poBand->GetXSize(), 1, 
                              GDT_Float32, 0, 0 ) != CE_None )
            return FALSE;
    }

    return TRUE;
}

/************************************************************************/
/*                        ECWCompressStatusCB()                         */
/************************************************************************/

static void ECWCompressStatusCB( NCSEcwCompressClient *psClient, 
                                 UINT32 nCurrentLine )

{
    ECWCompressInfo      *psCompressInfo;

    psCompressInfo = (ECWCompressInfo *) psClient->pClientData;
    
    psCompressInfo->bCancelled = 
        !psCompressInfo->pfnProgress( nCurrentLine 
                                      / (float) psClient->nInOutSizeY,
                                      NULL, psCompressInfo->pProgressData );
}

/************************************************************************/
/*                        ECWCompressCancelCB()                         */
/************************************************************************/

static BOOLEAN ECWCompressCancelCB( NCSEcwCompressClient *psClient )

{
    ECWCompressInfo      *psCompressInfo;

    psCompressInfo = (ECWCompressInfo *) psClient->pClientData;

    return psCompressInfo->bCancelled;
}

/************************************************************************/
/*                           ECWCreateCopy()                            */
/************************************************************************/

static GDALDataset *
ECWCreateCopy( const char * pszFilename, GDALDataset *poSrcDS, 
                 int bStrict, char ** papszOptions, 
                 GDALProgressFunc pfnProgress, void * pProgressData )

{
    int  nBands = poSrcDS->GetRasterCount();
    int  nXSize = poSrcDS->GetRasterXSize();
    int  nYSize = poSrcDS->GetRasterYSize();

    if( !pfnProgress( 0.0, NULL, pProgressData ) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Do some rudimentary checking in input.                          */
/* -------------------------------------------------------------------- */
    if( nBands == 0 )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "ECW driver requires at least one band as input." );
        return NULL;
    }

    if( nXSize < 128 || nYSize < 128 )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "ECW driver requires image to be at least 128x128,\n"
                  "the source image is %dx%d.\n", 
                  nXSize, nYSize );
        return NULL;
    }

    if( poSrcDS->GetRasterBand(1)->GetRasterDataType() != GDT_Byte 
        && bStrict )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "CW driver doesn't support data type %s. "
                  "Only eight bit bands supported.\n", 
                  GDALGetDataTypeName( 
                      poSrcDS->GetRasterBand(1)->GetRasterDataType()) );

        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Parse out some known options.                                   */
/* -------------------------------------------------------------------- */
    float      dfTargetCompression = 75.0;

    if( CSLFetchNameValue(papszOptions, "TARGET") != NULL )
    {
        dfTargetCompression = atof(CSLFetchNameValue(papszOptions, "TARGET"));
        
        if( dfTargetCompression < 1.1 || dfTargetCompression > 100.0 )
        {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "TARGET compression of %.3f invalid, should be a\n"
                      "value between 1 and 100 percent.\n", 
                      dfTargetCompression );
            return NULL;
        }
    }
        
/* -------------------------------------------------------------------- */
/*      Create and initialize compressor.                               */
/* -------------------------------------------------------------------- */
    NCSEcwCompressClient      *psClient;
    ECWCompressInfo           sCompressInfo;

    psClient = NCSEcwCompressAllocClient();
    if( psClient == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "NCSEcwCompressAllocClient() failed.\n" );
        return NULL;
    }

    psClient->nInputBands = nBands;
    psClient->nInOutSizeX = nXSize;
    psClient->nInOutSizeY = nYSize;
    psClient->fTargetCompression = dfTargetCompression;

    if( nBands == 1 )
        psClient->eCompressFormat = COMPRESS_UINT8;
    else if( nBands == 3 )
        psClient->eCompressFormat = COMPRESS_RGB;
    else
        psClient->eCompressFormat = COMPRESS_MULTI;

    strcpy( psClient->szOutputFilename, pszFilename );

    sCompressInfo.bCancelled = FALSE;
    sCompressInfo.pfnProgress = pfnProgress;
    sCompressInfo.pProgressData = pProgressData;
    sCompressInfo.poSrc = poSrcDS;

    psClient->pReadCallback = ECWCompressReadCB;
    psClient->pStatusCallback = ECWCompressStatusCB;
    psClient->pCancelCallback = ECWCompressCancelCB;
    psClient->pClientData = (void *) &sCompressInfo;

/* -------------------------------------------------------------------- */
/*      Set block size if desired.                                      */
/* -------------------------------------------------------------------- */
    psClient->nBlockSizeX = 256;
    psClient->nBlockSizeY = 256;
    if( CSLFetchNameValue(papszOptions, "BLOCKSIZE") != NULL )
    {
        psClient->nBlockSizeX = atoi(CSLFetchNameValue(papszOptions, 
                                                       "BLOCKSIZE"));
        psClient->nBlockSizeY = atoi(CSLFetchNameValue(papszOptions, 
                                                       "BLOCKSIZE"));
    }
        
    if( CSLFetchNameValue(papszOptions, "BLOCKXSIZE") != NULL )
        psClient->nBlockSizeX = atoi(CSLFetchNameValue(papszOptions, 
                                                       "BLOCKXSIZE"));
        
    if( CSLFetchNameValue(papszOptions, "BLOCKYSIZE") != NULL )
        psClient->nBlockSizeX = atoi(CSLFetchNameValue(papszOptions, 
                                                       "BLOCKYSIZE"));
        
/* -------------------------------------------------------------------- */
/*      Georeferencing.                                                 */
/* -------------------------------------------------------------------- */
    double      adfGeoTransform[6];

    if( poSrcDS->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
        if( adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0 )
            CPLError( CE_Warning, CPLE_NotSupported, 
                      "Rotational coefficients ignored, georeferencing of\n"
                      "output ECW file will be incorrect.\n" );
        else
        {
            psClient->fOriginX = adfGeoTransform[0];
            psClient->fOriginY = adfGeoTransform[3];
            psClient->fCellIncrementX = adfGeoTransform[1];
            psClient->fCellIncrementY = adfGeoTransform[5];
        }
    }

/* -------------------------------------------------------------------- */
/*      Start the compression.                                          */
/* -------------------------------------------------------------------- */
    NCSError      eError;

    eError = NCSEcwCompressOpen( psClient, FALSE );
    if( eError == NCS_SUCCESS )
    {
        eError = NCSEcwCompress( psClient );
        NCSEcwCompressClose( psClient );
    }

    if( eError != NCS_SUCCESS )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "ECW compression failed.\n%s", 
                  NCSGetErrorText( eError ) );

        NCSEcwCompressFreeClient( psClient );
        return NULL;
    }

    if( !pfnProgress( 1.0, NULL, pProgressData ) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Cleanup, and return read-only handle.                           */
/* -------------------------------------------------------------------- */
    NCSEcwCompressFreeClient( psClient );
    
    return (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );
}
#endif /* def FRMT_ecw */

/************************************************************************/
/*                          GDALRegister_ECW()                        */
/************************************************************************/

void GDALRegister_ECW()

{
#ifdef FRMT_ecw 
    GDALDriver	*poDriver;

    if( poECWDriver == NULL )
    {
        poECWDriver = poDriver = new GDALDriver();
        
        poDriver->SetDescription( "ECW" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "ERMapper Compressed Wavelets" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_ecw.html" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "ecw" );
        
        poDriver->pfnOpen = ECWDataset::Open;
        poDriver->pfnCreateCopy = ECWCreateCopy;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
#endif /* def FRMT_ecw */
}

