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
 * Revision 1.29  2004/12/20 22:15:41  fwarmerdam
 * avoid ECWRasterBand::AdviseRead() now calls on poGDS, limit debug msgs
 *
 * Revision 1.28  2004/12/20 06:40:32  fwarmerdam
 * fixed up for reading non-byte data
 *
 * Revision 1.27  2004/12/20 05:44:46  fwarmerdam
 * stripped out old full res mode - use TryWin and AdviseRead
 *
 * Revision 1.26  2004/12/20 05:02:43  fwarmerdam
 * moved list reader into ECWGetCSList()
 *
 * Revision 1.25  2004/12/17 14:50:30  fwarmerdam
 * implement AdviseRead API
 *
 * Revision 1.24  2004/12/10 19:18:05  fwarmerdam
 * Overhauled to use C++ ECW SDK classes.
 * Moved creation code into ecwcreatecopy.cpp
 * Support J2K_SUBFILE for reading "virtually".
 *
 * Revision 1.23  2004/06/24 04:53:40  warmerda
 * remove assert in IRasterIO(), seems to be unneeded
 *
 * Revision 1.22  2004/04/15 13:45:03  warmerda
 * Don't require the file to be open in ::Open() for .ecw files.  Large
 * files (>2GB) will generally not be open.
 *
 * Revision 1.21  2004/04/05 21:29:40  warmerda
 * improved logic to recognise jp2/jpc
 *
 * Revision 1.20  2004/04/05 20:49:34  warmerda
 * allow jpeg2000 files to be handled
 *
 * Revision 1.19  2004/03/30 17:33:51  warmerda
 * fixed last fix for ecwp:
 *
 * Revision 1.18  2004/03/30 17:00:07  warmerda
 * Allow "ecwp:" urls to be opened.
 *
 * Revision 1.17  2003/12/16 20:24:01  dron
 * Implemented support for supersampling in IRasterIO().
 *
 * Revision 1.16  2003/10/27 15:04:36  warmerda
 * fix small new memory leak
 *
 * Revision 1.15  2003/10/27 15:02:44  warmerda
 * Added ECWDataset::IRasterIO special case
 *
 * Revision 1.14  2003/09/23 13:47:47  warmerda
 * never return a NULL projection
 *
 * Revision 1.13  2003/06/19 18:50:20  warmerda
 * added projection reading support
 *
 * Revision 1.12  2003/06/18 17:26:35  warmerda
 * dont try ecwopen function if path isnt to a valid file
 *
 * Revision 1.11  2002/11/23 18:54:17  warmerda
 * added CREATIONDATATYPES metadata for drivers
 *
 * Revision 1.10  2002/10/01 19:34:27  warmerda
 * fixed problems with supersampling by forcing through fullres blocks
 *
 * Revision 1.9  2002/09/04 06:50:37  warmerda
 * avoid static driver pointers
 *
 * Revision 1.8  2002/07/23 14:40:41  warmerda
 * disable compress support on Unix
 *
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
#include "cpl_conv.h"
#include "vsiiostream.h"

CPL_CVSID("$Id$");

#ifdef FRMT_ecw

static unsigned char jpc_header[] = {0xff,0x4f};
static unsigned char jp2_header[] = 
{0x00,0x00,0x00,0x0c,0x6a,0x50,0x20,0x20,0x0d,0x0a,0x87,0x0a};

static int    gnTriedCSFile = FALSE;
static char **gpapszCSLookup = NULL;

/************************************************************************/
/* ==================================================================== */
/*				ECWDataset				*/
/* ==================================================================== */
/************************************************************************/

class ECWRasterBand;

class CPL_DLL ECWDataset : public GDALDataset
{
    friend class ECWRasterBand;

    FILE        *fpVSIL;
    
    CNCSJP2FileView *poFileView;
    NCSFileViewFileInfoEx *psFileInfo;

    GDALDataType eRasterDataType;
    NCSEcwCellType eNCSRequestDataType;

    // Current view window. 
    int         bWinActive;
    int         nWinXOff, nWinYOff, nWinXSize, nWinYSize;
    int         nWinBufXSize, nWinBufYSize;
    int         nWinBandCount;
    int         *panWinBandList;
    int         nWinBufLoaded;
    void        **papCurLineBuf;

    char        *pszProjection;

    void        ECW2WKTProjection();

    void        CleanupWindow();
    int         TryWinRasterIO( GDALRWFlag, int, int, int, int,
                                GByte *, int, int, GDALDataType,
                                int, int *, int, int, int );
    CPLErr      LoadNextLine();

  public:
    		ECWDataset();
    		~ECWDataset();
                
    static GDALDataset *Open( GDALOpenInfo * );
    static GDALDataset *OpenJPEG2000( GDALOpenInfo * );
    static GDALDataset *OpenECW( GDALOpenInfo * );

    virtual CPLErr IRasterIO( GDALRWFlag, int, int, int, int,
                              void *, int, int, GDALDataType,
                              int, int *, int, int, int );

    virtual CPLErr GetGeoTransform( double * );
    virtual const char *GetProjectionRef();

    virtual CPLErr AdviseRead( int nXOff, int nYOff, int nXSize, int nYSize,
                               int nBufXSize, int nBufYSize, 
                               GDALDataType eDT, 
                               int nBandCount, int *panBandList,
                               char **papszOptions );
};

/************************************************************************/
/* ==================================================================== */
/*                            ECWRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class ECWRasterBand : public GDALRasterBand
{
    friend class ECWDataset;
    
    // NOTE: poDS may be altered for NITF/JPEG2000 files!
    ECWDataset     *poGDS;

    virtual CPLErr IRasterIO( GDALRWFlag, int, int, int, int,
                              void *, int, int, GDALDataType,
                              int, int );

  public:

                   ECWRasterBand( ECWDataset *, int );
                   ~ECWRasterBand();

    virtual CPLErr IReadBlock( int, int, void * );
    virtual int    HasArbitraryOverviews() { return TRUE; }

    virtual CPLErr AdviseRead( int nXOff, int nYOff, int nXSize, int nYSize,
                               int nBufXSize, int nBufYSize, 
                               GDALDataType eDT, char **papszOptions );
};

/************************************************************************/
/*                           ECWRasterBand()                            */
/************************************************************************/

ECWRasterBand::ECWRasterBand( ECWDataset *poDS, int nBand )

{
    this->poDS = poDS;
    poGDS = poDS;

    this->nBand = nBand;
    eDataType = poDS->eRasterDataType;
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
/*                             AdviseRead()                             */
/************************************************************************/

CPLErr ECWRasterBand::AdviseRead( int nXOff, int nYOff, int nXSize, int nYSize,
                                  int nBufXSize, int nBufYSize, 
                                  GDALDataType eDT, 
                                  char **papszOptions )
{
    return poGDS->AdviseRead( nXOff, nYOff, nXSize, nYSize, 
                              nBufXSize, nBufYSize, eDT, 
                              1, &nBand, papszOptions );
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
    NCSError     eNCSErr;
    int          iBand, bDirect;
    int          nNewXSize = nBufXSize, nNewYSize = nBufYSize;
    GByte        *pabyWorkBuffer = NULL;

/* -------------------------------------------------------------------- */
/*      Try to do it based on existing "advised" access.                */
/* -------------------------------------------------------------------- */
    if( poGDS->TryWinRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize, 
                               (GByte *) pData, nBufXSize, nBufYSize, 
                               eBufType, 1, &nBand, 
                               nPixelSpace, nLineSpace, 0 ) )
        return CE_None;

/* -------------------------------------------------------------------- */
/*      We will drop down to the block oriented API if only a single    */
/*      scanline was requested. This is based on the assumption that    */
/*      doing lots of single scanline windows is expensive.             */
/* -------------------------------------------------------------------- */
    if( nYSize == 1 )
    {
#ifdef notdef
        CPLDebug( "ECWRasterBand", 
                  "RasterIO(%d,%d,%d,%d -> %dx%d) - redirected.", 
                  nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize );
#endif
        return GDALRasterBand::IRasterIO(eRWFlag, nXOff, nYOff, nXSize, nYSize,
                                         pData, nBufXSize, nBufYSize, 
                                         eBufType, nPixelSpace, nLineSpace );
    }

    CPLDebug( "ECWRasterBand", 
              "RasterIO(nXOff=%d,nYOff=%d,nXSize=%d,nYSize=%d -> %dx%d)", 
              nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize );


    if ( nXSize < nBufXSize )
            nNewXSize = nXSize;

    if ( nYSize < nBufYSize )
            nNewYSize = nYSize;

/* -------------------------------------------------------------------- */
/*      Default line and pixel spacing if needed.                       */
/* -------------------------------------------------------------------- */
    if( nLineSpace == 0 )
        nLineSpace = nBufXSize;

    if( nPixelSpace == 0 )
        nPixelSpace = 1;

/* -------------------------------------------------------------------- */
/*      Can we perform direct loads, or must we load into a working     */
/*      buffer, and transform?                                          */
/* -------------------------------------------------------------------- */
    bDirect = nPixelSpace == 1 && eBufType == GDT_Byte
	    && nNewXSize == nBufXSize && nNewYSize == nBufYSize;
    if( !bDirect )
        pabyWorkBuffer = (GByte *) CPLMalloc(nNewXSize);

/* -------------------------------------------------------------------- */
/*      Establish access at the desired resolution.                     */
/* -------------------------------------------------------------------- */
    CNCSError oErr;

    poGDS->CleanupWindow();

    iBand = nBand-1;
    oErr = poGDS->poFileView->SetView( 1, (unsigned int *) (&iBand),
                                       nXOff, nYOff, 
                                       nXOff + nXSize - 1, 
                                       nYOff + nYSize - 1,
                                       nNewXSize, nNewYSize );
    if( oErr.GetErrorNumber() != NCS_SUCCESS )
    {
        CPLFree( pabyWorkBuffer );
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s", oErr.GetErrorMessage() );
        
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Read back one scanline at a time, till request is satisfied.    */
/*      Supersampling is not supported by the ECW API, so we will do    */
/*      it ourselves.                                                   */
/* -------------------------------------------------------------------- */
    double  	dfSrcYInc = (double)nNewYSize / nBufYSize;
    double  	dfSrcXInc = (double)nNewXSize / nBufXSize;
    int      	nBufDataSize = GDALGetDataTypeSize( eBufType ) / 8;
    int      	iSrcLine, iDstLine;

    for( iSrcLine = 0, iDstLine = 0; iDstLine < nBufYSize; iDstLine++ )
    {
        NCSEcwReadStatus eRStatus;
        int     	iDstLineOff = iDstLine * nLineSpace;
        unsigned char 	*pabySrcBuf;

        if( bDirect )
            pabySrcBuf = ((GByte *)pData) + iDstLineOff;
        else
            pabySrcBuf = pabyWorkBuffer;

	if ( nNewYSize == nBufYSize || iSrcLine == (int)(iDstLine * dfSrcYInc) )
	{
            eRStatus = poGDS->poFileView->ReadLineBIL( &pabySrcBuf );

	    if( eRStatus != NCSECW_READ_OK )
	    {
	        CPLFree( pabyWorkBuffer );
	        CPLError( CE_Failure, CPLE_AppDefined,
			  "NCScbmReadViewLineBIL failed." );
		return CE_Failure;
	    }

            if( !bDirect )
            {
                if ( nNewXSize == nBufXSize )
                {
                    GDALCopyWords( pabyWorkBuffer, GDT_Byte, 1, 
                                   ((GByte *)pData) + iDstLine * nLineSpace, 
                                   eBufType, nPixelSpace, nBufXSize );
                }
		else
		{
	            int 	iPixel;

                    for ( iPixel = 0; iPixel < nBufXSize; iPixel++ )
                    {
                        GDALCopyWords( pabyWorkBuffer + (int)(iPixel*dfSrcXInc),
                                       GDT_Byte, 1,
                                       (GByte *)pData + iDstLineOff
				       + iPixel * nBufDataSize,
                                       eBufType, nPixelSpace, 1 );
                    }
		}
            }

            iSrcLine++;
	}
	else
	{
	    // Just copy the previous line in this case
	    memcpy( (GByte *)pData + iDstLineOff,
		    (GByte *)pData + (iDstLineOff - nLineSpace), nLineSpace );
	}
    }

    CPLFree( pabyWorkBuffer );

    return CE_None;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr ECWRasterBand::IReadBlock( int, int nBlockYOff, void * pImage )

{
    CPLErr eErr = CE_None;

    if( poGDS->TryWinRasterIO( GF_Read, 0, nBlockYOff, nBlockXSize, 1, 
                               (GByte *) pImage, nBlockXSize, 1, 
                               eDataType, 1, &nBand, 0, 0, 0 ) )
        return CE_None;

    eErr = AdviseRead( 0, nBlockYOff, nRasterXSize, nRasterYSize - nBlockYOff,
                       nRasterXSize, nRasterYSize - nBlockYOff, 
                       eDataType, NULL );
    if( eErr != CE_None )
        return eErr;

    if( poGDS->TryWinRasterIO( GF_Read, 0, nBlockYOff, nBlockXSize, 1, 
                               (GByte *) pImage, nBlockXSize, 1, 
                               eDataType, 1, &nBand, 0, 0, 0 ) )
        return CE_None;

    CPLError( CE_Failure, CPLE_AppDefined, 
              "TryWinRasterIO() failed for blocked scanline." );
    return CE_Failure;
}

/************************************************************************/
/* ==================================================================== */
/*                            ECWDataset                               */
/* ==================================================================== */
/************************************************************************/


/************************************************************************/
/*                            ECWDataset()                              */
/************************************************************************/

ECWDataset::ECWDataset()

{
    pszProjection = NULL;
    poFileView = NULL;
    fpVSIL = NULL;
    bWinActive = FALSE;
    panWinBandList = NULL;
    eRasterDataType = GDT_Byte;
}

/************************************************************************/
/*                           ~ECWDataset()                              */
/************************************************************************/

ECWDataset::~ECWDataset()

{
    CleanupWindow();
    CPLFree( pszProjection );
    if( poFileView != NULL )
    {
        //delete poFileView;
    }

    if( fpVSIL != NULL )
        delete fpVSIL;

}

/************************************************************************/
/*                             AdviseRead()                             */
/************************************************************************/

CPLErr ECWDataset::AdviseRead( int nXOff, int nYOff, int nXSize, int nYSize,
                               int nBufXSize, int nBufYSize, 
                               GDALDataType eDT, 
                               int nBandCount, int *panBandList,
                               char **papszOptions )

{
    int *panAdjustedBandList = NULL;

    CPLDebug( "ECW",
              "ECWDataset::AdviseRead(%d,%d,%d,%d->%d,%d)\n",
              nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize );

    if( nBufXSize > nXSize || nBufYSize > nYSize )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "Supersampling not directly supported by ECW toolkit,\n"
                  "ignoring AdviseRead() request." );
        return CE_Warning; 
    }

/* -------------------------------------------------------------------- */
/*      Adjust band numbers to be zero based.                           */
/* -------------------------------------------------------------------- */
    panAdjustedBandList = (int *) 
        CPLMalloc(sizeof(int) * nBandCount );
    for( int ii= 0; ii < nBandCount; ii++ )
        panAdjustedBandList[ii] = panBandList[ii] - 1;

/* -------------------------------------------------------------------- */
/*      Cleanup old window cache information.                           */
/* -------------------------------------------------------------------- */
    CleanupWindow();

/* -------------------------------------------------------------------- */
/*      Set the new requested window.                                   */
/* -------------------------------------------------------------------- */
    CNCSError oErr;
    
    oErr = poFileView->SetView( nBandCount, (UINT32 *) panAdjustedBandList, 
                                nXOff, nYOff, 
                                nXOff + nXSize-1, nYOff + nYSize-1,
                                nBufXSize, nBufYSize );

    CPLFree( panAdjustedBandList );
    if( oErr.GetErrorNumber() != NCS_SUCCESS )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s", oErr.GetErrorMessage() );
        bWinActive = FALSE;
        return CE_Failure;
    }

    bWinActive = TRUE;

/* -------------------------------------------------------------------- */
/*      Record selected window.                                         */
/* -------------------------------------------------------------------- */
    nWinXOff = nXOff;
    nWinYOff = nYOff;
    nWinXSize = nXSize;
    nWinYSize = nYSize;
    nWinBufXSize = nBufXSize;
    nWinBufYSize = nBufYSize;

    panWinBandList = (int *) CPLMalloc(sizeof(int)*nBandCount);
    memcpy( panWinBandList, panBandList, sizeof(int)* nBandCount);
    nWinBandCount = nBandCount;

    nWinBufLoaded = -1;

/* -------------------------------------------------------------------- */
/*      Allocate current scanline buffer.                               */
/* -------------------------------------------------------------------- */
    papCurLineBuf = (void **) CPLMalloc(sizeof(void*) * nWinBandCount );
    for( int iBand = 0; iBand < nWinBandCount; iBand++ )
        papCurLineBuf[iBand] = 
            CPLMalloc(nBufXSize * (GDALGetDataTypeSize(eRasterDataType)/8) );
        
    return CE_None;
}

/************************************************************************/
/*                           TryWinRasterIO()                           */
/*                                                                      */
/*      Try to satisfy the given request based on the currently         */
/*      defined window.  Return TRUE on success or FALSE on             */
/*      failure.  On failure, the caller should satisfy the request     */
/*      another way (not report an error).                              */
/************************************************************************/

int ECWDataset::TryWinRasterIO( GDALRWFlag eFlag, 
                                int nXOff, int nYOff, int nXSize, int nYSize,
                                GByte *pabyData, int nBufXSize, int nBufYSize, 
                                GDALDataType eDT,
                                int nBandCount, int *panBandList, 
                                int nPixelSpace, int nLineSpace, 
                                int nBandSpace )

{
    int iBand, i;

/* -------------------------------------------------------------------- */
/*      Provide default buffer organization.                            */
/* -------------------------------------------------------------------- */
    if( nPixelSpace == 0 )
        nPixelSpace = GDALGetDataTypeSize( eDT ) / 8;
    if( nLineSpace != NULL )
        nLineSpace = nPixelSpace * nBufXSize;
    if( nBandSpace == 0 )
        nBandSpace = nLineSpace * nBufYSize;

/* -------------------------------------------------------------------- */
/*      Do some simple tests to see if the current window can           */
/*      satisfy our requirement.                                        */
/* -------------------------------------------------------------------- */
    if( !bWinActive )
        return FALSE;
    
    if( nXOff != nWinXOff || nXSize != nWinXSize )
        return FALSE;

    if( nBufXSize != nWinBufXSize )
        return FALSE;

    for( iBand = 0; iBand < nBandCount; iBand++ )
    {
        for( i = 0; i < nWinBandCount; i++ )
        {
            if( panWinBandList[iBand] == panBandList[iBand] )
                break;
        }

        if( i == nWinBandCount )
            return FALSE;
    }

    if( nYOff < nWinYOff || nYOff + nYSize > nWinYOff + nWinYSize )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Now we try more subtle tests.                                   */
/* -------------------------------------------------------------------- */
    {
        static int nDebugCount = 0;

        if( nDebugCount < 30 )
            CPLDebug( "ECWDataset", 
                      "TryWinRasterIO(%d,%d,%d,%d -> %dx%d) - doing advised read.", 
                      nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize );

        if( nDebugCount == 29 )
            CPLDebug( "ECWDataset", "No more TryWinRasterIO messages will be reported" );
        
        nDebugCount++;
    }

/* -------------------------------------------------------------------- */
/*      Actually load data one buffer line at a time.                   */
/* -------------------------------------------------------------------- */
    int iBufLine;

    for( iBufLine = 0; iBufLine < nBufYSize; iBufLine++ )
    {
        float fFileLine = ((iBufLine+0.5) / nBufYSize) * nYSize + nYOff;
        int iWinLine = 
            (int) (((fFileLine - nWinYOff) / nWinYSize) * nWinBufYSize);
        
        if( iWinLine == nWinBufLoaded + 1 )
            LoadNextLine();

        if( iWinLine != nWinBufLoaded )
            return FALSE;

/* -------------------------------------------------------------------- */
/*      Copy out all our target bands.                                  */
/* -------------------------------------------------------------------- */
        int iWinBand;
        for( iBand = 0; iBand < nBandCount; iBand++ )
        {
            for( iWinBand = 0; iWinBand < nWinBandCount; iWinBand++ )
            {
                if( panWinBandList[iWinBand] == panBandList[iBand] )
                    break;
            }

            GDALCopyWords( papCurLineBuf[iWinBand], eRasterDataType,
                           GDALGetDataTypeSize( eRasterDataType ) / 8, 
                           pabyData + nBandSpace * iBand 
                           + iBufLine * nLineSpace, eDT, nPixelSpace,
                           nBufXSize );
        }
    }

    return TRUE;
}

/************************************************************************/
/*                            LoadNextLine()                            */
/************************************************************************/

CPLErr ECWDataset::LoadNextLine()

{
    if( !bWinActive )
        return CE_Failure;

    if( nWinBufLoaded == nWinBufYSize-1 )
    {
        CleanupWindow();
        return CE_Failure;
    }

    NCSEcwReadStatus  eRStatus;
    eRStatus = poFileView->ReadLineBIL( eNCSRequestDataType, nWinBandCount,
                                        papCurLineBuf );
//    eRStatus = poFileView->ReadLineBIL( (UINT8 **) papCurLineBuf );
//
    if( eRStatus != NCSECW_READ_OK )
        return CE_Failure;

    nWinBufLoaded++;

    return CE_None;
}

/************************************************************************/
/*                           CleanupWindow()                            */
/************************************************************************/

void ECWDataset::CleanupWindow()

{
    if( !bWinActive )
        return;

    bWinActive = FALSE;
    CPLFree( panWinBandList );
    panWinBandList = NULL;

    for( int iBand = 0; iBand < nWinBandCount; iBand++ )
        CPLFree( papCurLineBuf[iBand] );
    CPLFree( papCurLineBuf );
    papCurLineBuf = NULL;
}

/************************************************************************/
/*                             IRasterIO()                              */
/************************************************************************/

CPLErr ECWDataset::IRasterIO( GDALRWFlag eRWFlag,
                              int nXOff, int nYOff, int nXSize, int nYSize,
                              void * pData, int nBufXSize, int nBufYSize,
                              GDALDataType eBufType, 
                              int nBandCount, int *panBandMap,
                              int nPixelSpace, int nLineSpace, int nBandSpace)
    
{
/* -------------------------------------------------------------------- */
/*      Try to do it based on existing "advised" access.                */
/* -------------------------------------------------------------------- */
    if( TryWinRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize, 
                        (GByte *) pData, nBufXSize, nBufYSize, 
                        eBufType, nBandCount, panBandMap,
                        nPixelSpace, nLineSpace, nBandSpace ) )
        return CE_None;

/* -------------------------------------------------------------------- */
/*      If we are supersampling we need to fall into the general        */
/*      purpose logic.  We also use the general logic if we are in      */
/*      some cases unlikely to benefit from interleaved access.         */
/*                                                                      */
/*      The one case we would like to handle better here is the         */
/*      nBufYSize == 1 case (requesting a scanline at a time).  We      */
/*      should eventually have some logic similiar to the band by       */
/*      band case where we post a big window for the view, and allow    */
/*      sequential reads.                                               */
/* -------------------------------------------------------------------- */
    if( nXSize < nBufXSize || nYSize < nBufYSize || nYSize == 1 
        || nBandCount > 100 || nBandCount == 1 || nBufYSize == 1 )
    {
        return 
            GDALDataset::IRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize,
                                    pData, nBufXSize, nBufYSize,
                                    eBufType, 
                                    nBandCount, panBandMap,
                                    nPixelSpace, nLineSpace, nBandSpace);
    }

    CPLDebug( "ECWDataset", 
              "RasterIO(%d,%d,%d,%d -> %dx%d) - doing interleaved read.", 
              nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize );

/* -------------------------------------------------------------------- */
/*      Setup view.                                                     */
/* -------------------------------------------------------------------- */
    UINT32 anBandIndices[100];
    int    i;
    NCSError     eNCSErr;
    CNCSError    oErr;
    
    for( i = 0; i < nBandCount; i++ )
        anBandIndices[i] = panBandMap[i] - 1;

    CleanupWindow();

    oErr = poFileView->SetView( nBandCount, anBandIndices,
                                nXOff, nYOff, 
                                nXOff + nXSize - 1, 
                                nYOff + nYSize - 1,
                                nBufXSize, nBufYSize );
    eNCSErr = oErr.GetErrorNumber();
    
    if( eNCSErr != NCS_SUCCESS )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s", NCSGetErrorText(eNCSErr) );
        
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Setup working scanline, and the pointers into it.               */
/* -------------------------------------------------------------------- */
    GByte *pabyBILScanline = (GByte *) CPLMalloc(nBufXSize * nBandCount);
    GByte **papabyBIL = (GByte **) CPLMalloc(nBandCount * sizeof(void*));

    for( i = 0; i < nBandCount; i++ )
        papabyBIL[i] = pabyBILScanline + i * nBufXSize;

/* -------------------------------------------------------------------- */
/*      Read back all the data for the requested view.                  */
/* -------------------------------------------------------------------- */
    for( int iScanline = 0; iScanline < nBufYSize; iScanline++ )
    {
        NCSEcwReadStatus  eRStatus;
        int  iX;

        eRStatus = poFileView->ReadLineBIL( papabyBIL );
        if( eRStatus != NCSECW_READ_OK )
        {
            CPLFree( pabyBILScanline );
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "NCScbmReadViewLineBIL failed." );
            return CE_Failure;
        }


        for( i = 0; i < nBandCount; i++ )
        {
            GByte *pabyThisLine = ((GByte *)pData) + nLineSpace * iScanline + nBandSpace * i;
            
            for( iX = 0; iX < nBufXSize; iX++ )
                pabyThisLine[iX * nPixelSpace] = 
                    pabyBILScanline[i * nBufXSize + iX];
        }
    }

    CPLFree( papabyBIL );
    CPLFree( pabyBILScanline );

    return CE_None;
}

/************************************************************************/
/*                            OpenJPEG2000()                            */
/************************************************************************/

GDALDataset *ECWDataset::OpenJPEG2000( GDALOpenInfo * poOpenInfo )

{
    if( EQUALN(poOpenInfo->pszFilename,"J2K_SUBFILE:",12) )
        return Open( poOpenInfo );

    else if( poOpenInfo->nHeaderBytes >= 16 
        && (memcmp( poOpenInfo->pabyHeader, jpc_header, 
                    sizeof(jpc_header) ) == 0
            || memcmp( poOpenInfo->pabyHeader, jp2_header, 
                    sizeof(jp2_header) ) == 0) )
        return Open( poOpenInfo );
    
    else
        return NULL;
}
    
/************************************************************************/
/*                              OpenECW()                               */
/*                                                                      */
/*      Open method that only supports ECW files.                       */
/************************************************************************/

GDALDataset *ECWDataset::OpenECW( GDALOpenInfo * poOpenInfo )

{
/* -------------------------------------------------------------------- */
/*      This has to either be a file on disk ending in .ecw or a        */
/*      ecwp: protocol url.                                             */
/* -------------------------------------------------------------------- */
    if( (!EQUAL(CPLGetExtension(poOpenInfo->pszFilename),"ecw")
         || poOpenInfo->nHeaderBytes == 0)
        && !EQUALN(poOpenInfo->pszFilename,"ecwp:",5) )
        return( NULL );

    return Open( poOpenInfo );
}
    
/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *ECWDataset::Open( GDALOpenInfo * poOpenInfo )

{
    CNCSJP2FileView *poFileView = NULL;
    NCSError         eErr;
    CNCSError        oErr;
    int              i;
    FILE            *fpVSIL = NULL;

/* -------------------------------------------------------------------- */
/*      Handle special case of a JPEG2000 data stream in another file.  */
/* -------------------------------------------------------------------- */
    if( EQUALN(poOpenInfo->pszFilename,"J2K_SUBFILE:",12) )
    {
        int            subfile_offset=-1, subfile_size=-1;
        char *real_filename = NULL;

        if( sscanf( poOpenInfo->pszFilename, "J2K_SUBFILE:%d,%d", 
                    &subfile_offset, &subfile_size ) != 2 )
          {
              CPLError( CE_Failure, CPLE_OpenFailed, 
                        "Failed to parse J2K_SUBFILE specification." );
              return NULL;
          }

          real_filename = strstr(poOpenInfo->pszFilename,",");
          if( real_filename != NULL )
              real_filename = strstr(real_filename+1,",");
          if( real_filename != NULL )
              real_filename = real_filename++;
          else
          {
              CPLError( CE_Failure, CPLE_OpenFailed, 
                        "Failed to parse J2K_SUBFILE specification." );
              return NULL;
          }

          FILE *fpVSIL = VSIFOpenL( real_filename, "rb" );
          if( fpVSIL == NULL )
          {
              CPLError( CE_Failure, CPLE_OpenFailed, 
                        "Failed to open %s.",  real_filename );
              return NULL;
          }

          VSIIOStream *poIOStream;
          poIOStream = new VSIIOStream();
          poIOStream->Access( fpVSIL, FALSE, real_filename,
                              subfile_offset, subfile_size );

          poFileView = new CNCSJP2FileView();
          oErr = poFileView->Open( poIOStream, false );
          if( oErr.GetErrorNumber() != NCS_SUCCESS )
          {
              CPLError( CE_Failure, CPLE_AppDefined, "%s",
                        oErr.GetErrorMessage() );			
              return NULL;
          }
    }

/* -------------------------------------------------------------------- */
/*      This has to either be a file on disk ending in .ecw or a        */
/*      ecwp: protocol url.                                             */
/* -------------------------------------------------------------------- */
    else if( poOpenInfo->nHeaderBytes >= 16 
        && (memcmp( poOpenInfo->pabyHeader, jpc_header, 
                    sizeof(jpc_header) ) == 0
            || memcmp( poOpenInfo->pabyHeader, jp2_header, 
                    sizeof(jp2_header) ) == 0) )
        /* accept JPEG2000 files */;
    else if( (!EQUAL(CPLGetExtension(poOpenInfo->pszFilename),"ecw")
              || poOpenInfo->nHeaderBytes == 0)
             && !EQUALN(poOpenInfo->pszFilename,"ecwp:",5) )
        return( NULL );
    
/* -------------------------------------------------------------------- */
/*      Open the client interface.                                      */
/* -------------------------------------------------------------------- */
    if( poFileView == NULL )
    {
        poFileView = new CNCSFile();
        oErr = poFileView->Open( poOpenInfo->pszFilename, FALSE );
        eErr = oErr.GetErrorNumber();
        CPLDebug( "ECW", "NCScbmOpenFileView(%s): eErr = %d", 
                  poOpenInfo->pszFilename, (int) eErr );
        if( eErr != NCS_SUCCESS )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "%s", NCSGetErrorText(eErr) );
            return NULL;
        }
    }

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    ECWDataset 	*poDS;

    poDS = new ECWDataset();

    poDS->poFileView = poFileView;
    poDS->fpVSIL = fpVSIL;

/* -------------------------------------------------------------------- */
/*      Fetch general file information.                                 */
/* -------------------------------------------------------------------- */
    poDS->psFileInfo = poFileView->GetFileInfo();

    CPLDebug( "ECW", "FileInfo: SizeXY=%d,%d Bands=%d\n"
              "       OriginXY=%g,%g  CellIncrementXY=%g,%g\n",
              poDS->psFileInfo->nSizeX,
              poDS->psFileInfo->nSizeY,
              poDS->psFileInfo->nBands,
              poDS->psFileInfo->fOriginX,
              poDS->psFileInfo->fOriginY,
              poDS->psFileInfo->fCellIncrementX,
              poDS->psFileInfo->fCellIncrementY );

/* -------------------------------------------------------------------- */
/*      Establish raster info.                                          */
/* -------------------------------------------------------------------- */
    poDS->nRasterXSize = poDS->psFileInfo->nSizeX; 
    poDS->nRasterYSize = poDS->psFileInfo->nSizeY;

/* -------------------------------------------------------------------- */
/*      Establish the GDAL data type that corresponds.  A few NCS       */
/*      data types have no direct corresponding value in GDAL so we     */
/*      will coerce to something sufficiently similar.                  */
/* -------------------------------------------------------------------- */
    poDS->eNCSRequestDataType = poDS->psFileInfo->eCellType;
    switch( poDS->psFileInfo->eCellType )
    {
        case NCSCT_UINT8:
            poDS->eRasterDataType = GDT_Byte;
            break;

        case NCSCT_UINT16:
            poDS->eRasterDataType = GDT_UInt16;
            break;

        case NCSCT_UINT32:
        case NCSCT_UINT64:
            poDS->eRasterDataType = GDT_UInt32;
            poDS->eNCSRequestDataType = NCSCT_UINT32;
            break;

        case NCSCT_INT8:
        case NCSCT_INT16:
            poDS->eRasterDataType = GDT_Int16;
            poDS->eNCSRequestDataType = NCSCT_INT16;
            break;

        case NCSCT_INT32:
        case NCSCT_INT64:
            poDS->eRasterDataType = GDT_Int32;
            poDS->eNCSRequestDataType = NCSCT_INT32;
            break;

        case NCSCT_IEEE4:
            poDS->eRasterDataType = GDT_Float32;
            break;

        case NCSCT_IEEE8:
            poDS->eRasterDataType = GDT_Float64;
            break;
    }

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( i=0; i < poDS->psFileInfo->nBands; i++ )
        poDS->SetBand( i+1, new ECWRasterBand( poDS, i+1 ) );

    poDS->ECW2WKTProjection();

    return( poDS );
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *ECWDataset::GetProjectionRef() 

{
    if( pszProjection == NULL )
        return "";
    else
        return pszProjection;
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
/*                            ECWGetCSList()                            */
/************************************************************************/

char **ECWGetCSList()

{
/* -------------------------------------------------------------------- */
/*      Load the supporting data file with the coordinate system        */
/*      translations if we don't already have it loaded.  Note,         */
/*      currently we never unload the file, even if the driver is       */
/*      destroyed ... but we should.                                    */
/* -------------------------------------------------------------------- */
    if( !gnTriedCSFile )
    {
        const char *pszFilename = CPLFindFile( "data", "ecw_cs.dat" );
        
        gnTriedCSFile = TRUE;
        if( pszFilename != NULL )
            gpapszCSLookup = CSLLoad( pszFilename );
    }

    return gpapszCSLookup;
}

/************************************************************************/
/*                         ECW2WKTProjection()                          */
/*                                                                      */
/*      Set the dataset pszProjection string in OGC WKT format by       */
/*      looking up the ECW (GDT) coordinate system info in              */
/*      ecw_cs.dat support data file.                                   */
/*                                                                      */
/*      This code is likely still broken in some circumstances.  For    */
/*      instance, I haven't been careful about changing the linear      */
/*      projection parameters (false easting/northing) if the units     */
/*      is feet.  Lots of cases missing here, and in ecw_cs.dat.        */
/************************************************************************/

void ECWDataset::ECW2WKTProjection()

{
    if( psFileInfo == NULL )
        return;

    CPLDebug( "ECW", "projection=%s, datum=%s",
              psFileInfo->szProjection, psFileInfo->szDatum );

    if( EQUAL(psFileInfo->szProjection,"RAW") )
        return;

    if( ECWGetCSList() == NULL )
        return;

/* -------------------------------------------------------------------- */
/*      Set projection if we have it.                                   */
/* -------------------------------------------------------------------- */
    OGRSpatialReference oSRS;
    if( EQUAL(psFileInfo->szProjection,"GEODETIC") )
    {
    }
    else
    {
        const char *pszProjWKT;

        pszProjWKT = CSLFetchNameValue( gpapszCSLookup, 
                                        psFileInfo->szProjection );

        if( pszProjWKT == NULL 
            || EQUAL(pszProjWKT,"unsupported")
            || oSRS.importFromWkt( (char **) &pszProjWKT ) != OGRERR_NONE )
        {
            oSRS.SetLocalCS( psFileInfo->szProjection );
        }

        if( psFileInfo->eCellSizeUnits == ECW_CELL_UNITS_FEET )
            oSRS.SetLinearUnits( SRS_UL_US_FOOT, atof(SRS_UL_US_FOOT_CONV));
        else
            oSRS.SetLinearUnits( SRS_UL_METER, 1.0 );
    }

/* -------------------------------------------------------------------- */
/*      Set the geogcs.                                                 */
/* -------------------------------------------------------------------- */
    OGRSpatialReference oGeogCS;
    const char *pszGeogWKT;

    pszGeogWKT = CSLFetchNameValue( gpapszCSLookup, 
                                    psFileInfo->szDatum );

    if( pszGeogWKT == NULL )
        oGeogCS.SetWellKnownGeogCS( "WGS84" );
    else
    {
        oGeogCS.importFromWkt( (char **) &pszGeogWKT );
    }

    if( !oSRS.IsLocal() )
        oSRS.CopyGeogCSFrom( &oGeogCS );

/* -------------------------------------------------------------------- */
/*      Capture the resulting composite coordiante system.              */
/* -------------------------------------------------------------------- */
    if( pszProjection != NULL )
    {
        CPLFree( pszProjection );
        pszProjection = NULL;
    }
    oSRS.exportToWkt( &pszProjection );
}

#endif /* def FRMT_ecw */

/************************************************************************/
/*                          GDALRegister_ECW()                        */
/************************************************************************/

void GDALRegister_ECW()

{
#ifdef FRMT_ecw 
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "ECW" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "ECW" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "ERMapper Compressed Wavelets" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_ecw.html" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "ecw" );
        
        poDriver->pfnOpen = ECWDataset::OpenECW;
#ifdef HAVE_COMPRESS
        poDriver->pfnCreateCopy = ECWCreateCopyECW;
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES, 
                                   "Byte" );
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONOPTIONLIST, 
"<CreationOptionList>"
"   <Option name='TARGET' type='float' description='Compression Percentage' />"
"   <Option name='PROJ' type='string' description='ERMapper Projection Name'/>"
"   <Option name='DATUM' type='string' description='ERMapper Datum Name' />"
"</CreationOptionList>" );
#endif

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
#endif /* def FRMT_ecw */
}

/************************************************************************/
/*                        GDALRegister_JP2ECW()                         */
/************************************************************************/
void GDALRegister_JP2ECW()

{
#ifdef FRMT_ecw 
    GDALDriver	*poDriver;


    if( GDALGetDriverByName( "JP2ECW" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "JP2ECW" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "ERMapper JPEG2000" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_ecw.html" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "jp2" );
        
        poDriver->pfnOpen = ECWDataset::OpenJPEG2000;
#ifdef HAVE_COMPRESS
        poDriver->pfnCreateCopy = ECWCreateCopyJPEG2000;
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES, 
                                   "Byte UInt16 Int16 UInt32 Int32 Float32 Float64" );
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONOPTIONLIST, 
"<CreationOptionList>"
"   <Option name='TARGET' type='float' description='Compression Percentage' />"
"   <Option name='PROJ' type='string' description='ERMapper Projection Name'/>"
"   <Option name='DATUM' type='string' description='ERMapper Datum Name' />"
"   <Option name='PROFILE' type='string-select'>"
"       <Value>BASELINE_0</Value>"
"       <Value>BASELINE_1</Value>"
"       <Value>BASELINE_2</Value>"
"       <Value>NPJE</Value>"
"       <Value>EPJE</Value>"
"   </Option>"
"   <Option name='PROGRESSION' type='string-select'>"
"       <Value>LRCP</Value>"
"       <Value>RLCP</Value>"
"       <Value>RPCL</Value>"
"   </Option>"
"   <Option name='CODESTREAM_ONLY' type='boolean' description='No JP2 wrapper'/>"
"   <Option name='LEVELS' type='int'/>"
"   <Option name='LAYERS' type='int'/>"
"   <Option name='PRECINCT_WIDTH' type='int'/>"
"   <Option name='PRECINCT_HEIGHT' type='int'/>"
"   <Option name='TILE_WIDTH' type='int'/>"
"   <Option name='TILE_HEIGHT' type='int'/>"
"   <Option name='INCLUDE_SOP' type='boolean'/>"
"   <Option name='INCLUDE_EPH' type='boolean'/>"
"   <Option name='DECOMPRESS_LAYERS' type='int'/>"
"   <Option name='DECOMPRESS_RECONSTRUCTION_PARAMETER' type='float'/>"
"</CreationOptionList>" );
#endif

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
#endif /* def FRMT_ecw */
}

