/******************************************************************************
 * $Id$
 *
 * Project:  ESRI .hdr Driver
 * Purpose:  Implementation of EHdrDataset
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
 * Revision 1.8  2000/10/30 20:49:40  warmerda
 * Added error test to ensure the user isn't selecting the .hdr file
 * directly instead of the image data file.
 *
 * Revision 1.7  2000/08/15 19:28:26  warmerda
 * added help topic
 *
 * Revision 1.6  2000/07/17 17:10:24  warmerda
 * fixed default geotransform to match expected values for raw images
 *
 * Revision 1.5  2000/07/07 15:29:09  warmerda
 * Removed the restriction that all lines must have two or more tokens.
 * In Spot GeoSPOT files there are comment lines (#...) with just one
 * field on them.
 *
 * Revision 1.4  2000/06/20 17:35:58  warmerda
 * added overview support
 *
 * Revision 1.3  2000/02/28 16:32:20  warmerda
 * use SetBand method
 *
 * Revision 1.2  1999/08/12 18:23:33  warmerda
 * Added the ability to handle the NBITS and BYTEORDER flags.
 *
 * Revision 1.1  1999/07/23 19:34:34  warmerda
 * New
 */

#include "rawdataset.h"
#include "cpl_string.h"

static GDALDriver	*poEHdrDriver = NULL;

CPL_C_START
void	GDALRegister_EHdr(void);
CPL_C_END

/************************************************************************/
/* ==================================================================== */
/*				EHdrDataset				*/
/* ==================================================================== */
/************************************************************************/

class EHdrDataset : public RawDataset
{
    FILE	*fpImage;	// image data file.
    
    double	dfULXMap;
    double	dfULYMap;
    double	dfXDim;
    double	dfYDim;

  public:
    		EHdrDataset();
    	        ~EHdrDataset();
    
    CPLErr 	GetGeoTransform( double * padfTransform );
    
    static GDALDataset *Open( GDALOpenInfo * );
};

/************************************************************************/
/*                            EHdrDataset()                             */
/************************************************************************/

EHdrDataset::EHdrDataset()
{
    fpImage = NULL;
}

/************************************************************************/
/*                            ~EHdrDataset()                            */
/************************************************************************/

EHdrDataset::~EHdrDataset()

{
    if( fpImage != NULL )
        VSIFClose( fpImage );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr EHdrDataset::GetGeoTransform( double * padfTransform )

{
    padfTransform[0] = dfULXMap - dfXDim * 0.5;
    padfTransform[1] = dfXDim;
    padfTransform[2] = 0.0;
    padfTransform[3] = dfULYMap + dfYDim * 0.5;
    padfTransform[4] = 0.0;
    padfTransform[5] = - dfYDim;

    return( CE_None );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *EHdrDataset::Open( GDALOpenInfo * poOpenInfo )

{
    int		i, bSelectedHDR;
    char	*pszHDRFilename;
    
/* -------------------------------------------------------------------- */
/*	We assume the user is pointing to the binary (ie. .bil) file.	*/
/* -------------------------------------------------------------------- */
    if( poOpenInfo->nHeaderBytes < 1 && poOpenInfo->fp != NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Now we need to tear apart the filename to form a .HDR           */
/*      filename.                                                       */
/* -------------------------------------------------------------------- */
    pszHDRFilename = (char *) CPLMalloc(strlen(poOpenInfo->pszFilename)+5);
    strcpy( pszHDRFilename, poOpenInfo->pszFilename );;

    for( i = strlen(pszHDRFilename)-1; i > 0; i-- )
    {
        if( pszHDRFilename[i] == '.' )
        {
            pszHDRFilename[i] = '\0';
            break;
        }
    }

    strcat( pszHDRFilename, ".hdr" );

    bSelectedHDR = EQUAL(pszHDRFilename,poOpenInfo->pszFilename);

/* -------------------------------------------------------------------- */
/*      Do we have a .hdr file?                                         */
/* -------------------------------------------------------------------- */
    FILE	*fp;

    fp = VSIFOpen( pszHDRFilename, "r" );
    if( fp == NULL )
    {
        strcpy( pszHDRFilename + strlen(pszHDRFilename)-4, ".HDR" );
        fp = VSIFOpen( pszHDRFilename, "r" );
    }

    CPLFree( pszHDRFilename );
    
    if( fp == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Is this file an ESRI header file?  Read a few lines of text     */
/*      searching for something starting with nrows or ncols.           */
/* -------------------------------------------------------------------- */
    const char *	pszLine;
    int			nRows = -1, nCols = -1, nBands = 1;
    int			nSkipBytes = 0;
    double		dfULXMap=0.5, dfULYMap = 0.5;
    double		dfXDim = 1.0, dfYDim = 1.0;
    int			nLineCount = 0;
    GDALDataType	eDataType = GDT_Byte;
    char		chByteOrder = 'M';

    while( (pszLine = CPLReadLine( fp )) )
    {
        char	**papszTokens;

        nLineCount++;

        if( nLineCount > 1000 || strlen(pszLine) > 1000 )
            break;

        papszTokens = CSLTokenizeString( pszLine );
        if( CSLCount( papszTokens ) < 2 )
        {
            CSLDestroy( papszTokens );
            continue;
        }
        
        if( EQUAL(papszTokens[0],"ncols") )
        {
            nCols = atoi(papszTokens[1]);
        }
        else if( EQUAL(papszTokens[0],"nrows") )
        {
            nRows = atoi(papszTokens[1]);
        }
        else if( EQUAL(papszTokens[0],"skipbytes") )
        {
            nSkipBytes = atoi(papszTokens[1]);
        }
        else if( EQUAL(papszTokens[0],"ulxmap") )
        {
            dfULXMap = atof(papszTokens[1]);
        }
        else if( EQUAL(papszTokens[0],"ulymap") )
        {
            dfULYMap = atof(papszTokens[1]);
        }
        else if( EQUAL(papszTokens[0],"xdim") )
        {
            dfXDim = atof(papszTokens[1]);
        }
        else if( EQUAL(papszTokens[0],"ydim") )
        {
            dfYDim = atof(papszTokens[1]);
        }
        else if( EQUAL(papszTokens[0],"NBITS") )
        {
            if( atoi(papszTokens[1]) == 16 )
                eDataType = GDT_UInt16;
            else if( atoi(papszTokens[1]) == 32 )
                eDataType = GDT_UInt32;
        }
        else if( EQUAL(papszTokens[0],"byteorder") )
        {
            chByteOrder = papszTokens[1][0];
        }

        CSLDestroy( papszTokens );
    }
    
    VSIFClose( fp );

/* -------------------------------------------------------------------- */
/*      Did we get the required keywords?  If not we return with        */
/*      this never having been considered to be a match. This isn't     */
/*      an error!                                                       */
/* -------------------------------------------------------------------- */
    if( nRows == -1 || nCols == -1 )
        return NULL;
    
/* -------------------------------------------------------------------- */
/*      Has the user selected the .hdr file to open?                    */
/* -------------------------------------------------------------------- */
    if( bSelectedHDR )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "The selected file is an ESRI BIL header file, but to\n"
                  "open ESRI BIL datasets, the data file should be selected\n"
                  "instead of the .hdr file.  Please try again selecting\n"
                "the data file (often with the extension .bil) corresponding\n"
                  "to the header file: %s\n", 
                  poOpenInfo->pszFilename );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    EHdrDataset 	*poDS;

    poDS = new EHdrDataset();

    poDS->poDriver = poEHdrDriver;
    poDS->dfULXMap = dfULXMap;
    poDS->dfULYMap = dfULYMap;
    poDS->dfXDim = dfXDim;
    poDS->dfYDim = dfYDim;
    
/* -------------------------------------------------------------------- */
/*      Capture some information from the file that is of interest.     */
/* -------------------------------------------------------------------- */
    poDS->nRasterXSize = nCols;
    poDS->nRasterYSize = nRows;
    
/* -------------------------------------------------------------------- */
/*      Assume ownership of the file handled from the GDALOpenInfo.     */
/* -------------------------------------------------------------------- */
    poDS->fpImage = poOpenInfo->fp;
    poOpenInfo->fp = NULL;

/* -------------------------------------------------------------------- */
/*      Compute the line offset.                                        */
/* -------------------------------------------------------------------- */
    int		nLineOffset;
    
    nLineOffset = 0;
    for( i = 0; i < nBands; i++ )
    {
        nLineOffset += (GDALGetDataTypeSize(eDataType)/8) * nCols;
    }
    
/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    poDS->nBands = nBands;;
    for( i = 0; i < poDS->nBands; i++ )
    {
        poDS->SetBand( i+1, 
            new RawRasterBand( poDS, i+1, poDS->fpImage,
                               nSkipBytes, GDALGetDataTypeSize(eDataType)/8,
                               nLineOffset, eDataType,
#ifdef CPL_LSB                               
                               chByteOrder == 'I'
#else
                               chByteOrder == 'M'
#endif        
                               ));
    }

/* -------------------------------------------------------------------- */
/*      Check for overviews.                                            */
/* -------------------------------------------------------------------- */
    poDS->oOvManager.Initialize( poDS, poOpenInfo->pszFilename );

    return( poDS );
}

/************************************************************************/
/*                         GDALRegister_EHdr()                          */
/************************************************************************/

void GDALRegister_EHdr()

{
    GDALDriver	*poDriver;

    if( poEHdrDriver == NULL )
    {
        poEHdrDriver = poDriver = new GDALDriver();
        
        poDriver->pszShortName = "EHdr";
        poDriver->pszLongName = "ESRI .hdr Labelled";
        poDriver->pszHelpTopic = "frmt_various.html#EHdr";
        
        poDriver->pfnOpen = EHdrDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

