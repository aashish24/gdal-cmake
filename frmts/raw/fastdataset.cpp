/******************************************************************************
 * $Id$
 *
 * Project:  EOSAT FAST Format reader
 * Purpose:  Reads Landsat FAST-L7A, IRS 1C/1D
 * Author:   Andrey Kiselev, dron@at1895.spb.edu
 *
 ******************************************************************************
 * Copyright (c) 2002, Andrey Kiselev <dron@at1895.spb.edu>
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
 * Revision 1.10  2004/02/01 17:29:34  dron
 * Format parsing logic completely rewritten. Start using importFromUSGS().
 *
 * Revision 1.9  2003/10/17 07:08:21  dron
 * Use locale selection option in CPLScanDouble().
 *
 * Revision 1.8  2003/10/05 15:31:00  dron
 * TM projection support implemented.
 *
 * Revision 1.7  2003/07/08 21:10:19  warmerda
 * avoid warnings
 *
 * Revision 1.6  2003/03/14 17:28:10  dron
 * CPLFormCIFilename() used instead of CPLFormFilename() for FAST-L7 datasets.
 *
 * Revision 1.5  2003/03/05 15:49:59  dron
 * Fixed typo when reading SENSOR metadata record.
 *
 * Revision 1.4  2003/02/18 15:07:49  dron
 * IRS-1C/1D support added.
 *
 * Revision 1.3  2003/02/14 21:05:40  warmerda
 * Don't use path for rawdataset.h.
 *
 * Revision 1.2  2002/12/30 14:55:01  dron
 * SetProjCS() removed, added unit setting.
 *
 * Revision 1.1  2002/10/05 12:35:31  dron
 * FAST driver moved to the RAW directory.
 *
 * Revision 1.5  2002/10/04 16:06:06  dron
 * Some redundancy removed.
 *
 * Revision 1.4  2002/10/04 12:33:02  dron
 * Added calibration coefficients extraction.
 *
 * Revision 1.3  2002/09/04 06:50:37  warmerda
 * avoid static driver pointers
 *
 * Revision 1.2  2002/08/15 09:35:50  dron
 * Fixes in georeferencing
 *
 * Revision 1.1  2002/08/13 16:55:41  dron
 * Initial release
 *
 *
 */

#include "gdal_priv.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include "ogr_spatialref.h"
#include "rawdataset.h"

CPL_CVSID("$Id$");

CPL_C_START
void	GDALRegister_FAST(void);
CPL_C_END

#define ADM_STD_HEADER_SIZE	4608    // XXX: Format specification says it
#define ADM_HEADER_SIZE		5000    // should be 4608, but some vendors
                                        // ship broken large datasets.

#define ACQUISITION_DATE        "ACQUISITION DATE ="
#define ACQUISITION_DATE_SIZE   8

#define SATELLITE_NAME          "SATELLITE ="
#define SATELLITE_NAME_SIZE     10

#define SENSOR_NAME             "SENSOR ="
#define SENSOR_NAME_SIZE        10

#define FILENAME                "FILENAME ="
#define FILENAME_SIZE           29

#define PIXELS                  "PIXELS PER LINE ="
#define PIXELS_SIZE             5

#define LINES                   "LINES PER BAND ="
#define LINES_SIZE              5

#define BITS_PER_PIXEL          "OUTPUT BITS PER PIXEL ="
#define BITS_PER_PIXEL_SIZE     2

#define PROJECTION_NAME         "MAP PROJECTION ="
#define PROJECTION_NAME_SIZE    4

#define ELLIPSOID_NAME          "ELLIPSOID ="
#define ELLIPSOID_NAME_SIZE     18

#define DATUM_NAME              "DATUM ="
#define DATUM_NAME_SIZE         6

#define ZONE_NUMBER             "USGS MAP ZONE ="
#define ZONE_NUMBER_SIZE        6

#define USGS_PARAMETERS         "USGS PROJECTION PARAMETERS ="

#define CORNER_UPPER_LEFT       "UL ="
#define CORNER_UPPER_RIGHT      "UR ="
#define CORNER_LOWER_LEFT       "LL ="
#define CORNER_LOWER_RIGHT      "LR ="
#define CORNER_VALUE_SIZE       13

#define VALUE_SIZE              24

enum FASTSatellite  // Satellites:
{
    LANDSAT,	    // Landsat 7
    IRS		    // IRS 1C/1D
};

/************************************************************************/
/* ==================================================================== */
/*				FASTDataset				*/
/* ==================================================================== */
/************************************************************************/

class FASTDataset : public GDALDataset
{
    friend class FASTRasterBand;

    double      adfGeoTransform[6];
    char        *pszProjection;

    FILE	*fpHeader;
    FILE	*fpChannels[6];
    const char	*pszFilename;
    char	*pszDirname;
    GDALDataType eDataType;
    FASTSatellite iSatellite;

  public:
                FASTDataset();
		~FASTDataset();
    
    static GDALDataset *Open( GDALOpenInfo * );

    CPLErr 	GetGeoTransform( double * padfTransform );
    const char	*GetProjectionRef();
    FILE	*FOpenChannel( char *pszFilename, int iBand );
};

/************************************************************************/
/* ==================================================================== */
/*                            FASTRasterBand                            */
/* ==================================================================== */
/************************************************************************/

class FASTRasterBand : public RawRasterBand
{
    friend class FASTDataset;

  public:

    		FASTRasterBand( FASTDataset *, int, FILE *, vsi_l_offset,
				int, int, GDALDataType, int );
};


/************************************************************************/
/*                           FASTRasterBand()                           */
/************************************************************************/

FASTRasterBand::FASTRasterBand( FASTDataset *poDS, int nBand, FILE * fpRaw,
                                vsi_l_offset nImgOffset, int nPixelOffset,
                                int nLineOffset, GDALDataType eDataType,
				int bNativeOrder) :
                 RawRasterBand( poDS, nBand, fpRaw, nImgOffset, nPixelOffset,
                               nLineOffset, eDataType, bNativeOrder, FALSE)
{

}

/************************************************************************/
/* ==================================================================== */
/*				FASTDataset				*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           FASTDataset()                              */
/************************************************************************/

FASTDataset::FASTDataset()

{
    fpHeader = NULL;
    pszDirname = NULL;
    pszProjection = CPLStrdup( "" );
    nBands = 0;
}

/************************************************************************/
/*                            ~FASTDataset()                            */
/************************************************************************/

FASTDataset::~FASTDataset()

{
    int i;
    if ( pszDirname )
	CPLFree( pszDirname );
    if ( pszProjection )
	CPLFree( pszProjection );
    for ( i = 0; i < nBands; i++ )
	if ( fpChannels[i] )
	    VSIFCloseL( fpChannels[i] );
    if( fpHeader != NULL )
        VSIFClose( fpHeader );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr FASTDataset::GetGeoTransform( double * padfTransform )

{
    memcpy( padfTransform, adfGeoTransform, sizeof(double) * 6 );
    return CE_None;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *FASTDataset::GetProjectionRef()

{
    if( pszProjection )
        return pszProjection;
    else
        return "";
}
/************************************************************************/
/*                             FOpenChannel()                           */
/************************************************************************/

FILE *FASTDataset::FOpenChannel( char *pszFilename, int iBand )
{
    const char	*pszChannelFilename = NULL;
    char	*pszPrefix = CPLStrdup( CPLGetBasename( this->pszFilename ) );
    char	*pszSuffix = CPLStrdup( CPLGetExtension( this->pszFilename ) );

    switch ( iSatellite )
    {
	case LANDSAT:
	if ( pszFilename && !EQUAL( pszFilename, "" ) )
	{
	    pszChannelFilename =
                CPLFormCIFilename( pszDirname, pszFilename, NULL );
	    fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	}
	else
	    fpChannels[iBand] = NULL;
	break;
	case IRS:
	default:
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "%s.%d", pszPrefix, iBand + 1 ), pszSuffix );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "IMAGERY%d", iBand + 1 ), pszSuffix );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "imagery%d", iBand + 1 ), pszSuffix );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "IMAGERY%d.DAT", iBand + 1 ), NULL );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "imagery%d.dat", iBand + 1 ), NULL );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "IMAGERY%d.dat", iBand + 1 ), NULL );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "imagery%d.DAT", iBand + 1 ), NULL );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "BAND%d", iBand + 1 ), pszSuffix );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "band%d", iBand + 1 ), pszSuffix );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "BAND%d.DAT", iBand + 1 ), NULL );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "band%d.dat", iBand + 1 ), NULL );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "BAND%d.dat", iBand + 1 ), NULL );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	if ( fpChannels[iBand] )
	    break;
	pszChannelFilename = CPLFormFilename( pszDirname,
	    CPLSPrintf( "band%d.DAT", iBand + 1 ), NULL );
	fpChannels[iBand] = VSIFOpenL( pszChannelFilename, "rb" );
	break;
    }
    
    CPLDebug( "FAST", "Band %d filename: %s", iBand + 1, pszChannelFilename);

    CPLFree( pszPrefix );
    CPLFree( pszSuffix );
    return fpChannels[iBand];
}

static char *GetValue( const char *pszString, const char *pszName,
                       int iValueSize, int iNormalize )
{
    char    *pszTemp = strstr( pszString, pszName );

    if ( pszTemp )
    {
        pszTemp += strlen( pszName );
        pszTemp = CPLScanString( pszTemp, iValueSize, TRUE, iNormalize );
    }

    return pszTemp;
}

static long USGSMnemonicToCode( const char* pszMnemonic )
{
    if ( EQUAL(pszMnemonic, "UTM") )
        return 1L;
    else if ( EQUAL(pszMnemonic, "TM") )
        return 9L;
    else
        return 0L;
}

static long USGSEllipsoidToCode( const char* pszMnemonic )
{
    if ( EQUAL(pszMnemonic, "WGS84") || EQUAL(pszMnemonic, "WGS_84") )
        return 12L;
    else
        return 12L;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *FASTDataset::Open( GDALOpenInfo * poOpenInfo )

{
    int		i;
	
    if( poOpenInfo->fp == NULL )
        return NULL;

    if( !EQUALN((const char *) poOpenInfo->pabyHeader + 52,
		"ACQUISITION DATE =", 18) )
        return NULL;
    
/* -------------------------------------------------------------------- */
/*  Create a corresponding GDALDataset.                                 */
/* -------------------------------------------------------------------- */
    FASTDataset	*poDS;

    poDS = new FASTDataset();

    poDS->fpHeader = poOpenInfo->fp;
    poOpenInfo->fp = NULL;
    poDS->pszFilename = poOpenInfo->pszFilename;
    poDS->pszDirname = CPLStrdup( CPLGetDirname( poOpenInfo->pszFilename ) );
    
/* -------------------------------------------------------------------- */
/*  Read the administrative record.                                     */
/* -------------------------------------------------------------------- */
    char	*pszTemp;
    char	*pszHeader = (char *) CPLMalloc( ADM_HEADER_SIZE + 1 );
    size_t      nBytesRead;
 
    VSIFSeek( poDS->fpHeader, 0, SEEK_SET );
    nBytesRead = VSIFRead( pszHeader, 1, ADM_HEADER_SIZE, poDS->fpHeader );
    if ( nBytesRead < ADM_STD_HEADER_SIZE )
    {
	CPLDebug( "FAST", "Header file too short. Reading failed" );
	delete poDS;
	return NULL;
    }
    pszHeader[nBytesRead] = '\0';

    // Read acquisition date
    pszTemp = GetValue( pszHeader, ACQUISITION_DATE,
                        ACQUISITION_DATE_SIZE, TRUE );
    poDS->SetMetadataItem( "ACQUISITION_DATE", pszTemp );
    CPLFree( pszTemp );

    // Read satellite name (will read the first one only)
    pszTemp = GetValue( pszHeader, SATELLITE_NAME, SATELLITE_NAME_SIZE, TRUE );
    poDS->SetMetadataItem( "SATELLITE", pszTemp );
    if ( EQUALN(pszTemp, "LANDSAT", 7) )
	poDS->iSatellite = LANDSAT;
    else if ( EQUALN(pszTemp, "IRS", 3) )
	poDS->iSatellite = IRS;
    else
	poDS->iSatellite = IRS;
    CPLFree( pszTemp );

    // Read sensor name (will read the first one only)
    pszTemp = GetValue( pszHeader, SENSOR_NAME, SENSOR_NAME_SIZE, TRUE );
    poDS->SetMetadataItem( "SENSOR", pszTemp );
    CPLFree( pszTemp );

    // Read filenames
    pszTemp = pszHeader;
    poDS->nBands = 0;
    for ( i = 0; i < 6; i++ )
    {
        char *pszFilename = NULL ;

        if ( pszTemp )
            pszTemp = strstr( pszTemp, FILENAME );
        if ( pszTemp )
        {
            pszTemp += strlen(FILENAME);
            pszFilename = CPLScanString( pszTemp, FILENAME_SIZE, TRUE, FALSE );
        }
        else
            pszTemp = NULL;
        if ( poDS->FOpenChannel( pszFilename, poDS->nBands ) )
            poDS->nBands++;
        if ( pszFilename )
            CPLFree( pszFilename );
    }

    if ( !poDS->nBands )
    {
	CPLDebug( "FAST", "Failed to find and open band data files." );
	delete poDS;
	return NULL;
    }

    // Read number of pixels/lines and bit depth
    pszTemp = strstr( pszHeader, PIXELS );
    if ( pszTemp )
        poDS->nRasterXSize = CPLScanLong( pszTemp + strlen(PIXELS),
                                          PIXELS_SIZE );
    else
    {
        CPLDebug( "FAST", "Failed to find number of pixels in line." );
        delete poDS;
	return NULL;
    }

    pszTemp = strstr( pszHeader, LINES );
    if ( pszTemp )
        poDS->nRasterYSize = CPLScanLong( pszTemp + strlen(LINES), LINES_SIZE );
    else
    {
        CPLDebug( "FAST", "Failed to find number of lines in raster." );
        delete poDS;
	return NULL;
    }

    pszTemp = strstr( pszHeader, BITS_PER_PIXEL );
    switch( CPLScanLong(pszTemp+strlen(BITS_PER_PIXEL), BITS_PER_PIXEL_SIZE) )
    {
	case 8:
        default:
	    poDS->eDataType = GDT_Byte;
	    break;
    }

/* -------------------------------------------------------------------- */
/*  Read radiometric record.    					*/
/* -------------------------------------------------------------------- */
    // Read gains and biases. This is a trick!
    pszTemp = strstr( pszHeader, "BIASES" );// It may be "BIASES AND GAINS"
                                            // or "GAINS AND BIASES"
    // Now search for the first number occurance after that string
    for ( i = 1; i <= poDS->nBands; i++ )
    {
        char *pszValue;

        pszTemp = strpbrk( pszTemp, "-.0123456789" );
        if ( pszTemp )
        {
            pszValue = CPLScanString( pszTemp, VALUE_SIZE, TRUE, TRUE );
            poDS->SetMetadataItem( CPLSPrintf("BIAS%d", i ), pszValue );
        }
        pszTemp += VALUE_SIZE;
        if ( pszValue )
            CPLFree( pszValue );
        pszTemp = strpbrk( pszTemp, "-.0123456789" );
        if ( pszTemp )
        {
            pszValue = CPLScanString( pszTemp, VALUE_SIZE, TRUE, TRUE );
            poDS->SetMetadataItem( CPLSPrintf("GAIN%d", i ), pszValue );
        }
        pszTemp += VALUE_SIZE;
        if ( pszValue )
            CPLFree( pszValue );
    }

/* -------------------------------------------------------------------- */
/*  Read geometric record.					        */
/* -------------------------------------------------------------------- */
    OGRSpatialReference oSRS;
    long        iProjSys, iZone, iDatum;
    int         bNorth = TRUE;
    // Coordinates of pixel's centers
    double	dfULX = 0.5, dfULY = 0.5;
    double	dfURX = poDS->nRasterXSize - 0.5, dfURY = 0.5;
    double	dfLLX = 0.5, dfLLY = poDS->nRasterYSize - 0.5;
    double	dfLRX = poDS->nRasterXSize - 0.5,
                dfLRY = poDS->nRasterYSize - 0.5;
    double      adfProjParms[15];

    // Read projection name
    pszTemp = GetValue( pszHeader, PROJECTION_NAME,
                        PROJECTION_NAME_SIZE, FALSE );
    if ( pszTemp )
        iProjSys = USGSMnemonicToCode( pszTemp );
    else
        iProjSys = 1;   // UTM by default
    CPLFree( pszTemp );

    // Read ellipsoid name
    pszTemp = GetValue( pszHeader, ELLIPSOID_NAME, ELLIPSOID_NAME_SIZE, FALSE );
    if ( pszTemp )
        iDatum = USGSEllipsoidToCode( pszTemp );
    else
        iDatum = 12;    // WGS84 by default
    CPLFree( pszTemp );

    // Read zone number
    pszTemp = GetValue( pszHeader, ZONE_NUMBER, ZONE_NUMBER_SIZE, FALSE );
    if ( pszTemp )
        iZone = atoi( pszTemp );
    else
        iZone = 0L;
    CPLFree( pszTemp );

    // Read 15 USGS projection parameters
    for ( i = 0; i < 15; i++ )
        adfProjParms[i] = 0.0;
    pszTemp = strstr( pszHeader, USGS_PARAMETERS );
    if ( pszTemp )
    {
        pszTemp += strlen( USGS_PARAMETERS );
        for ( i = 0; i < 15; i++ )
        {
            pszTemp = strpbrk( pszTemp, "-.0123456789" );
            if ( pszTemp )
                adfProjParms[i] = CPLScanDouble( pszTemp, VALUE_SIZE, "C" );
            pszTemp += VALUE_SIZE;
        }
    }

    // Read corner coordinates
    pszTemp = strstr( pszHeader, CORNER_UPPER_LEFT );
    if ( pszTemp )
    {
        pszTemp += strlen( CORNER_UPPER_LEFT ) + 26;
        if ( *pszTemp == 'S' )
            bNorth = FALSE;
        pszTemp += 2;
        dfULX = CPLScanDouble( pszTemp, CORNER_VALUE_SIZE, "C" );
        pszTemp += CORNER_VALUE_SIZE + 1;
        dfULY = CPLScanDouble( pszTemp, CORNER_VALUE_SIZE, "C" );
    }

    pszTemp = strstr( pszHeader, CORNER_UPPER_RIGHT );
    if ( pszTemp )
    {
        pszTemp += strlen( CORNER_UPPER_RIGHT ) + 28;
        dfURX = CPLScanDouble( pszTemp, CORNER_VALUE_SIZE, "C" );
        pszTemp += CORNER_VALUE_SIZE + 1;
        dfURY = CPLScanDouble( pszTemp, CORNER_VALUE_SIZE, "C" );
    }

    pszTemp = strstr( pszHeader, CORNER_LOWER_LEFT );
    if ( pszTemp )
    {
        pszTemp += strlen( CORNER_LOWER_LEFT ) + 28;
        dfLLX = CPLScanDouble( pszTemp, CORNER_VALUE_SIZE, "C" );
        pszTemp += CORNER_VALUE_SIZE + 1;
        dfLLY = CPLScanDouble( pszTemp, CORNER_VALUE_SIZE, "C" );
    }

    pszTemp = strstr( pszHeader, CORNER_LOWER_RIGHT );
    if ( pszTemp )
    {
        pszTemp += strlen( CORNER_LOWER_RIGHT ) + 28;
        dfLRX = CPLScanDouble( pszTemp, CORNER_VALUE_SIZE, "C" );
        pszTemp += CORNER_VALUE_SIZE + 1;
        dfLRY = CPLScanDouble( pszTemp, CORNER_VALUE_SIZE, "C" );
    }

    // Strip out zone number from the easting values, if either
    if ( dfULX >= 1000000.0 )
        dfULX -= (double)iZone * 1000000.0;
    if ( dfURX >= 1000000.0 )
        dfURX -= (double)iZone * 1000000.0;
    if ( dfLLX >= 1000000.0 )
        dfLLX -= (double)iZone * 1000000.0;
    if ( dfLRX >= 1000000.0 )
        dfLRX -= (double)iZone * 1000000.0;

    // Create projection definition
    oSRS.importFromUSGS( iProjSys, iZone, adfProjParms, iDatum );
    oSRS.SetLinearUnits( SRS_UL_METER, 1.0 );
    
    // Read datum name
    pszTemp = GetValue( pszHeader, DATUM_NAME, DATUM_NAME_SIZE, FALSE );
    if ( EQUAL( pszTemp, "WGS84" ) )
        oSRS.SetWellKnownGeogCS( "WGS84" );
    CPLFree( pszTemp );

    if ( poDS->pszProjection )
        CPLFree( poDS->pszProjection );
    oSRS.exportToWkt( &poDS->pszProjection );

    // Calculate transformation matrix
    poDS->adfGeoTransform[1] = (dfURX - dfLLX) / (poDS->nRasterXSize - 1);
    poDS->adfGeoTransform[5] = (bNorth) ?
                                (dfURY - dfLLY) / (poDS->nRasterYSize - 1)
                                : (dfLLY - dfURY) / (poDS->nRasterYSize - 1);
    poDS->adfGeoTransform[0] = dfULX - poDS->adfGeoTransform[1] / 2;
    poDS->adfGeoTransform[3] = dfULY - poDS->adfGeoTransform[5] / 2;
    poDS->adfGeoTransform[2] = 0.0;
    poDS->adfGeoTransform[4] = 0.0;

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( i = 1; i <= poDS->nBands; i++ )
        poDS->SetBand( i, new FASTRasterBand( poDS, i, poDS->fpChannels[i - 1],
	    0, 1, poDS->nRasterXSize, poDS->eDataType, TRUE));

    CPLFree( pszHeader );

    return( poDS );
}

/************************************************************************/
/*                        GDALRegister_FAST()				*/
/************************************************************************/

void GDALRegister_FAST()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "FAST" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "FAST" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "EOSAT FAST Format" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_fast.html" );

        poDriver->pfnOpen = FASTDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

