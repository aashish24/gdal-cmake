/******************************************************************************
 * $Id$
 *
 * Project:  USGS DOQ Driver (First Generation Format)
 * Purpose:  Implementation of DOQ1Dataset
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
 * Revision 1.3  2000/01/06 20:58:21  warmerda
 * Fixed initial poOpenInfo check.
 *
 * Revision 1.2  1999/12/30 02:39:43  warmerda
 * Added projections support.
 *
 * Revision 1.1  1999/12/29 20:43:05  warmerda
 * New
 */

#include "rawdataset.h"
#include "cpl_string.h"

static GDALDriver	*poDOQ1Driver = NULL;

static double DOQGetField( unsigned char *, int );

CPL_C_START
void	GDALRegister_DOQ1(void);
CPL_C_END

#define UTM_FORMAT \
"PROJCS[\"%s / UTM zone 1N\",GEOGCS[%s,PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",%d],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",0],%s]"

#define WGS84_DATUM \
"\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]]"

#define WGS72_DATUM \
"\"WGS 72\",DATUM[\"WGS_1972\",SPHEROID[\"NWL 10D\",6378135,298.26]]"

#define NAD27_DATUM \
"\"NAD27\",DATUM[\"North_American_Datum_1927\",SPHEROID[\"Clarke 1866\",6378206.4,294.978698213901]]"

#define NAD83_DATUM \
"\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101]]"

/************************************************************************/
/* ==================================================================== */
/*				DOQ1Dataset				*/
/* ==================================================================== */
/************************************************************************/

class DOQ1Dataset : public RawDataset
{
    FILE	*fpImage;	// image data file.
    
    double	dfULX, dfULY;
    double	dfXPixelSize, dfYPixelSize;

    char	*pszProjection;
    
  public:
    		DOQ1Dataset();
    	        ~DOQ1Dataset();

    CPLErr 	GetGeoTransform( double * padfTransform );
    const char  *GetProjectionRef( void );
    
    static GDALDataset *Open( GDALOpenInfo * );
};

/************************************************************************/
/*                            DOQ1Dataset()                             */
/************************************************************************/

DOQ1Dataset::DOQ1Dataset()
{
    pszProjection = NULL;
    fpImage = NULL;
}

/************************************************************************/
/*                            ~DOQ1Dataset()                            */
/************************************************************************/

DOQ1Dataset::~DOQ1Dataset()

{
    CPLFree( pszProjection );
    if( fpImage != NULL )
        VSIFClose( fpImage );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr DOQ1Dataset::GetGeoTransform( double * padfTransform )

{
    padfTransform[0] = dfULX;
    padfTransform[1] = dfXPixelSize;
    padfTransform[2] = 0.0;
    padfTransform[3] = dfULY;
    padfTransform[4] = 0.0;
    padfTransform[5] = -1 * dfYPixelSize;
    
    return( CE_None );
}

/************************************************************************/
/*                        GetProjectionString()                         */
/************************************************************************/

const char *DOQ1Dataset::GetProjectionRef()

{
    return pszProjection;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *DOQ1Dataset::Open( GDALOpenInfo * poOpenInfo )

{
    int		nWidth, nHeight, nBandStorage, nBandTypes;
    
/* -------------------------------------------------------------------- */
/*	We assume the user is pointing to the binary (ie. .bil) file.	*/
/* -------------------------------------------------------------------- */
    if( poOpenInfo->nHeaderBytes < 212 || poOpenInfo->fp == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*	Attempt to extract a few key values from the header.		*/
/* -------------------------------------------------------------------- */
    nWidth = (int) DOQGetField(poOpenInfo->pabyHeader + 150, 6);
    nHeight = (int) DOQGetField(poOpenInfo->pabyHeader + 144, 6);
    nBandStorage = (int) DOQGetField(poOpenInfo->pabyHeader + 162, 3);
    nBandTypes = (int) DOQGetField(poOpenInfo->pabyHeader + 156, 3);

/* -------------------------------------------------------------------- */
/*      Do these values look coherent for a DOQ file?  It would be      */
/*      nice to do a more comprehensive test than this!                 */
/* -------------------------------------------------------------------- */
    if( nWidth < 500 || nWidth > 25000
        || nHeight < 500 || nHeight > 25000
        || nBandStorage < 0 || nBandStorage > 4
        || nBandTypes < 1 || nBandTypes > 9 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Check the configuration.  We don't currently handle all         */
/*      variations, only the common ones.                               */
/* -------------------------------------------------------------------- */
    if( nBandTypes > 5 )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "DOQ Data Type (%d) is not a supported configuration.\n",
                  nBandTypes );
        return NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    DOQ1Dataset 	*poDS;

    poDS = new DOQ1Dataset();

    poDS->poDriver = poDOQ1Driver;

/* -------------------------------------------------------------------- */
/*      Capture some information from the file that is of interest.     */
/* -------------------------------------------------------------------- */
    poDS->nRasterXSize = nWidth;
    poDS->nRasterYSize = nHeight;
    
/* -------------------------------------------------------------------- */
/*      Assume ownership of the file handled from the GDALOpenInfo.     */
/* -------------------------------------------------------------------- */
    poDS->fpImage = poOpenInfo->fp;
    poOpenInfo->fp = NULL;

/* -------------------------------------------------------------------- */
/*      Compute layout of data.                                         */
/* -------------------------------------------------------------------- */
    int		nSkipBytes, nBytesPerPixel, nBytesPerLine, i;

    if( nBandTypes < 5 )
        nBytesPerPixel = 1;
    else if( nBandTypes == 5 )
        nBytesPerPixel = 3;

    nBytesPerLine = nBytesPerPixel * nWidth;
    nSkipBytes = 4 * nBytesPerLine;
    
/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    poDS->nBands = nBytesPerPixel;
    poDS->papoBands = (GDALRasterBand **)
        VSICalloc(sizeof(GDALRasterBand *),poDS->nBands);

    for( i = 0; i < poDS->nBands; i++ )
    {
        poDS->papoBands[i] =
            new RawRasterBand( poDS, i+1, poDS->fpImage,
                               nSkipBytes + i, nBytesPerPixel, nBytesPerLine,
                               GDT_Byte, TRUE );
    }

/* -------------------------------------------------------------------- */
/*      Establish the projection string.                                */
/* -------------------------------------------------------------------- */
    if( ((int) DOQGetField(poOpenInfo->pabyHeader + 195, 3)) != 1 )
        poDS->pszProjection = VSIStrdup("");
    else
    {
        const char *pszDatumLong, *pszDatumShort;
        const char *pszUnits;
        int	   nZone;

        nZone = (int) DOQGetField(poOpenInfo->pabyHeader + 198, 6);

        if( ((int) DOQGetField(poOpenInfo->pabyHeader + 204, 3)) == 1 )
            pszUnits = "UNIT[\"US survey foot\",0.304800609601219]";
        else
            pszUnits = "UNIT[\"metre\",1]";

        switch( (int) DOQGetField(poOpenInfo->pabyHeader + 167, 2) )
        {
          case 1:
            pszDatumLong = NAD27_DATUM;
            pszDatumShort = "NAD 27";
            break;
            
          case 2:
            pszDatumLong = WGS72_DATUM;
            pszDatumShort = "WGS 72";
            break;
            
          case 3:
            pszDatumLong = WGS84_DATUM;
            pszDatumShort = "WGS 84";
            break;
            
          case 4:
            pszDatumLong = NAD83_DATUM;
            pszDatumShort = "NAD 83";
            break;

          default:
            pszDatumLong = "DATUM[\"unknown\"]";
            pszDatumShort = "unknown";
            break;
        }
        
        poDS->pszProjection = 
            CPLStrdup(CPLSPrintf( UTM_FORMAT, pszDatumShort, pszDatumLong,
                                  nZone * 6 - 183, pszUnits ));
    }
    
/* -------------------------------------------------------------------- */
/*      Read the georeferencing information.                            */
/* -------------------------------------------------------------------- */
    unsigned char	abyRecordData[500];
    
    if( VSIFSeek( poDS->fpImage, nBytesPerLine * 2, SEEK_SET ) != 0
        || VSIFRead(abyRecordData,sizeof(abyRecordData),1,poDS->fpImage) != 1 )
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Header read error on %s.\n",
                  poOpenInfo->pszFilename );
        return NULL;
    }

    poDS->dfULX = DOQGetField( abyRecordData + 288, 24 );
    poDS->dfULY = DOQGetField( abyRecordData + 312, 24 );

    if( VSIFSeek( poDS->fpImage, nBytesPerLine * 3, SEEK_SET ) != 0
        || VSIFRead(abyRecordData,sizeof(abyRecordData),1,poDS->fpImage) != 1 )
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Header read error on %s.\n",
                  poOpenInfo->pszFilename );
        return NULL;
    }

    poDS->dfXPixelSize = DOQGetField( abyRecordData + 59, 12 );
    poDS->dfYPixelSize = DOQGetField( abyRecordData + 71, 12 );

    poDS->dfULX -= poDS->dfXPixelSize / 2;
    poDS->dfULY += poDS->dfYPixelSize / 2;

    return( poDS );
}

/************************************************************************/
/*                         GDALRegister_DOQ1()                          */
/************************************************************************/

void GDALRegister_DOQ1()

{
    GDALDriver	*poDriver;

    if( poDOQ1Driver == NULL )
    {
        poDOQ1Driver = poDriver = new GDALDriver();
        
        poDriver->pszShortName = "DOQ1";
        poDriver->pszLongName = "USGS DOQ (Old Style)";
        
        poDriver->pfnOpen = DOQ1Dataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

/************************************************************************/
/*                            DOQGetField()                             */
/************************************************************************/

static double DOQGetField( unsigned char *pabyData, int nBytes )

{
    char	szWork[128];
    int		i;

    strncpy( szWork, (const char *) pabyData, nBytes );
    szWork[nBytes] = '\0';

    for( i = 0; i < nBytes; i++ )
    {
        if( szWork[i] == 'D' || szWork[i] == 'd' )
            szWork[i] = 'E';
    }

    return atof(szWork);
}
