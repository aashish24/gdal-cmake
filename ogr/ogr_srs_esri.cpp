/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  OGRSpatialReference translation to/from ESRI .prj definitions.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
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
 * Revision 1.26  2003/08/03 01:30:34  warmerda
 * Added mapping for EUR datum as per:
 * http://bugzilla.remotesensing.org/show_bug.cgi?id=371
 *
 * Revision 1.25  2003/06/23 14:49:17  warmerda
 * added InitDatumMappingTable, and use of gdal_datum.csv file
 *
 * Revision 1.24  2003/06/17 14:39:00  warmerda
 * Translate Equirectangular to/from Equidistant_Cylindrical
 *
 * Revision 1.23  2003/05/21 02:59:41  warmerda
 * morphToEsri() now sets specific units constants for Foot_US and Degree.
 * morphToEsri() converts albers parameters to match expected ESRI values.
 *
 * Revision 1.22  2003/05/08 21:52:55  warmerda
 * fixed C morphToESRI func, added ESRI unit remapping
 *
 * Revision 1.21  2003/02/25 04:53:16  warmerda
 * Added support for the LAMBERT projection.  Added support for defining a
 * GEOGCS from the SPHEROID if there is no known DATUM.   Fixed bug with
 * GREATBRITIAN_GRID.
 *
 * Revision 1.20  2003/02/14 22:15:04  warmerda
 * expand tabs
 *
 * Revision 1.19  2003/01/24 20:15:34  warmerda
 * added polar stereographic support
 *
 * Revision 1.18  2002/12/16 17:07:42  warmerda
 * dont alter projection parameter units ... ESRI was right!
 *
 * Revision 1.17  2002/12/01 21:16:21  warmerda
 * added logic to correct angular projection parameter units when needed
 *
 * Revision 1.16  2002/11/29 22:10:15  warmerda
 * added logic to map ESRI LCC to LCC1SP or LCC2SP in WKT and vice versa
 *
 * Revision 1.15  2002/11/25 03:28:16  warmerda
 * added/improved documentation
 *
 * Revision 1.14  2002/11/12 19:42:08  warmerda
 * added state plane and BNG support
 *
 * Revision 1.13  2002/04/25 20:56:28  warmerda
 * expanded tabs
 *
 * Revision 1.12  2002/03/12 18:11:38  warmerda
 * ESRI WKT false easting/northing not necessariliy in meters
 *
 * Revision 1.11  2002/03/05 14:25:14  warmerda
 * expand tabs
 *
 * Revision 1.10  2001/11/09 21:06:40  warmerda
 * stripctparms may not results in null root
 *
 * Revision 1.9  2001/10/11 19:27:54  warmerda
 * worked on esri morphing
 *
 * Revision 1.8  2001/10/10 20:42:43  warmerda
 * added ESRI WKT morphing support
 *
 * Revision 1.7  2001/07/19 18:25:07  warmerda
 * expanded tabs
 *
 * Revision 1.6  2001/07/18 05:03:05  warmerda
 * added CPL_CVSID
 *
 * Revision 1.5  2001/03/16 22:15:48  warmerda
 * added support for reading WKT in importFromEPSG
 *
 * Revision 1.4  2001/01/26 14:56:11  warmerda
 * added Transverse Mercator .prj support
 *
 * Revision 1.3  2001/01/19 21:10:46  warmerda
 * replaced tabs
 *
 * Revision 1.2  2000/11/17 17:25:37  warmerda
 * added improved utm support
 *
 * Revision 1.1  2000/11/09 06:22:15  warmerda
 * New
 *
 */

#include "ogr_spatialref.h"
#include "ogr_p.h"
#include "cpl_csv.h"

CPL_CVSID("$Id$");

static char *apszProjMapping[] = {
    "Albers", SRS_PT_ALBERS_CONIC_EQUAL_AREA,
    "Cassini", SRS_PT_CASSINI_SOLDNER,
    "Hotine_Oblique_Mercator_Azimuth_Natural_Origin", 
                                        SRS_PT_HOTINE_OBLIQUE_MERCATOR,
    "Lambert_Conformal_Conic", SRS_PT_LAMBERT_CONFORMAL_CONIC_2SP,
    "Lambert_Conformal_Conic", SRS_PT_LAMBERT_CONFORMAL_CONIC_1SP,
    "Van_der_Grinten_I", SRS_PT_VANDERGRINTEN,
    SRS_PT_TRANSVERSE_MERCATOR, SRS_PT_TRANSVERSE_MERCATOR,
    "Gauss_Kruger", SRS_PT_TRANSVERSE_MERCATOR,
    "Mercator", SRS_PT_MERCATOR_1SP,
    "Equidistant_Cylindrical", SRS_PT_EQUIRECTANGULAR,
    NULL, NULL }; 
 
static char *apszAlbersMapping[] = {
    SRS_PP_CENTRAL_MERIDIAN, SRS_PP_LONGITUDE_OF_CENTER, 
    SRS_PP_LATITUDE_OF_ORIGIN, SRS_PP_LATITUDE_OF_CENTER,
    NULL, NULL };

static char **papszDatumMapping = NULL;
 
static char *apszUnitMapping[] = {
    "Meter", "meter",
    "Meter", "metre",
    "Foot", "foot",
    "Foot", "feet",
    "Foot_US", SRS_UL_US_FOOT,
    "Degree", "degree",
    "Degree", "degrees",
    "Degree", SRS_UA_DEGREE,
    "Radian", SRS_UA_RADIAN,
    NULL, NULL }; 
 
/* -------------------------------------------------------------------- */
/*      Table relating USGS and ESRI state plane zones.                 */
/* -------------------------------------------------------------------- */
static int anUsgsEsriZones[] =
{
  101, 3101,
  102, 3126,
  201, 3151,
  202, 3176,
  203, 3201,
  301, 3226,
  302, 3251,
  401, 3276,
  402, 3301,
  403, 3326,
  404, 3351,
  405, 3376,
  406, 3401,
  407, 3426,
  501, 3451,
  502, 3476,
  503, 3501,
  600, 3526,
  700, 3551,
  901, 3601,
  902, 3626,
  903, 3576,
 1001, 3651,
 1002, 3676,
 1101, 3701,
 1102, 3726,
 1103, 3751,
 1201, 3776,
 1202, 3801,
 1301, 3826,
 1302, 3851,
 1401, 3876,
 1402, 3901,
 1501, 3926,
 1502, 3951,
 1601, 3976,
 1602, 4001,
 1701, 4026,
 1702, 4051,
 1703, 6426,
 1801, 4076,
 1802, 4101,
 1900, 4126,
 2001, 4151,
 2002, 4176,
 2101, 4201,
 2102, 4226,
 2103, 4251,
 2111, 6351,
 2112, 6376,
 2113, 6401,
 2201, 4276,
 2202, 4301,
 2203, 4326,
 2301, 4351,
 2302, 4376,
 2401, 4401,
 2402, 4426,
 2403, 4451,
 2500,    0,
 2501, 4476,
 2502, 4501,
 2503, 4526,
 2600,    0,
 2601, 4551,
 2602, 4576,
 2701, 4601,
 2702, 4626,
 2703, 4651,
 2800, 4676,
 2900, 4701,
 3001, 4726,
 3002, 4751,
 3003, 4776,
 3101, 4801,
 3102, 4826,
 3103, 4851,
 3104, 4876,
 3200, 4901,
 3301, 4926,
 3302, 4951,
 3401, 4976,
 3402, 5001,
 3501, 5026,
 3502, 5051,
 3601, 5076,
 3602, 5101,
 3701, 5126,
 3702, 5151,
 3800, 5176,
 3900,    0,
 3901, 5201,
 3902, 5226,
 4001, 5251,
 4002, 5276,
 4100, 5301,
 4201, 5326,
 4202, 5351,
 4203, 5376,
 4204, 5401,
 4205, 5426,
 4301, 5451,
 4302, 5476,
 4303, 5501,
 4400, 5526,
 4501, 5551,
 4502, 5576,
 4601, 5601,
 4602, 5626,
 4701, 5651,
 4702, 5676,
 4801, 5701,
 4802, 5726,
 4803, 5751,
 4901, 5776,
 4902, 5801,
 4903, 5826,
 4904, 5851,
 5001, 6101,
 5002, 6126,
 5003, 6151,
 5004, 6176,
 5005, 6201,
 5006, 6226,
 5007, 6251,
 5008, 6276,
 5009, 6301,
 5010, 6326,
 5101, 5876,
 5102, 5901,
 5103, 5926,
 5104, 5951,
 5105, 5976,
 5201, 6001,
 5200, 6026,
 5200, 6076,
 5201, 6051,
 5202, 6051,
 5300,    0, 
 5400,    0
};

void OGREPSGDatumNameMassage( char ** ppszDatum );

/************************************************************************/
/*                           ESRIToUSGSZone()                           */
/*                                                                      */
/*      Convert ESRI style state plane zones to USGS style state        */
/*      plane zones.                                                    */
/************************************************************************/

static int ESRIToUSGSZone( int nESRIZone )

{
    int         nPairs = sizeof(anUsgsEsriZones) / (2*sizeof(int));
    int         i;
    
    for( i = 0; i < nPairs; i++ )
    {
        if( anUsgsEsriZones[i*2+1] == nESRIZone )
            return anUsgsEsriZones[i*2];
    }

    return 0;
}

/************************************************************************/
/*                       InitDatumMappingTable()                        */
/************************************************************************/

static void InitDatumMappingTable()

{
    static char *apszDefaultDatumMapping[] = {
        "6267", "North_American_1927", SRS_DN_NAD27,
        "6269", "North_American_1983", SRS_DN_NAD83,
        NULL, NULL, NULL }; 

    if( papszDatumMapping != NULL )
        return;

/* -------------------------------------------------------------------- */
/*      Try to open the datum.csv file.                                 */
/* -------------------------------------------------------------------- */
    const char  *pszFilename = CSVFilename("gdal_datum.csv");
    FILE * fp = VSIFOpen( pszFilename, "rb" );

/* -------------------------------------------------------------------- */
/*      Use simple default set if we can't find the file.               */
/* -------------------------------------------------------------------- */
    if( fp == NULL )
    {
        papszDatumMapping = apszDefaultDatumMapping;
        return;
    }

/* -------------------------------------------------------------------- */
/*      Figure out what fields we are interested in.                    */
/* -------------------------------------------------------------------- */
    char **papszFieldNames = CSVReadParseLine( fp );
    int  nDatumCodeField = CSLFindString( papszFieldNames, "DATUM_CODE" );
    int  nEPSGNameField = CSLFindString( papszFieldNames, "DATUM_NAME" );
    int  nESRINameField = CSLFindString( papszFieldNames, "ESRI_DATUM_NAME" );

    CSLDestroy( papszFieldNames );

    if( nDatumCodeField == -1 || nEPSGNameField == -1 || nESRINameField == -1 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Failed to find required field in datum.csv in InitDatumMappingTable(), using default table setup." );
        
        papszDatumMapping = apszDefaultDatumMapping;
        return;
    }
    
/* -------------------------------------------------------------------- */
/*      Read each line, adding a detail line for each.                  */
/* -------------------------------------------------------------------- */
    int nMappingCount = 0;
    const int nMaxDatumMappings = 1000;
    char **papszFields;
    papszDatumMapping = (char **)CPLCalloc(sizeof(char*),nMaxDatumMappings*3);

    for( papszFields = CSVReadParseLine( fp );
         papszFields != NULL;
         papszFields = CSVReadParseLine( fp ) )
    {
        int nFieldCount = CSLCount(papszFields);

        CPLAssert( nMappingCount+1 < nMaxDatumMappings );

        if( MAX(nEPSGNameField,MAX(nDatumCodeField,nESRINameField)) 
            < nFieldCount 
            && nMaxDatumMappings > nMappingCount+1 )
        {
            papszDatumMapping[nMappingCount*3+0] = 
                CPLStrdup( papszFields[nDatumCodeField] );
            papszDatumMapping[nMappingCount*3+1] = 
                CPLStrdup( papszFields[nESRINameField] );
            papszDatumMapping[nMappingCount*3+2] = 
                CPLStrdup( papszFields[nEPSGNameField] );
            OGREPSGDatumNameMassage( &(papszDatumMapping[nMappingCount*3+2]) );

            nMappingCount++;
        }
        CSLDestroy( papszFields );
    }

    VSIFClose( fp );

    papszDatumMapping[nMappingCount*3+0] = NULL;
    papszDatumMapping[nMappingCount*3+1] = NULL;
    papszDatumMapping[nMappingCount*3+2] = NULL;
}


/************************************************************************/
/*                         OSRImportFromESRI()                          */
/************************************************************************/

OGRErr OSRImportFromESRI( OGRSpatialReferenceH hSRS, char **papszPrj )

{
    return ((OGRSpatialReference *) hSRS)->importFromESRI( papszPrj );
}

/************************************************************************/
/*                              OSR_GDV()                               */
/*                                                                      */
/*      Fetch a particular parameter out of the parameter list, or      */
/*      the indicated default if it isn't available.  This is a         */
/*      helper function for importFromESRI().                           */
/************************************************************************/

static double OSR_GDV( char **papszNV, const char * pszField, 
                       double dfDefaultValue )

{
    int         iLine;

    if( papszNV == NULL || papszNV[0] == NULL )
        return dfDefaultValue;

    if( EQUALN(pszField,"PARAM_",6) )
    {
        int     nOffset;

        for( iLine = 0; 
             papszNV[iLine] != NULL && !EQUALN(papszNV[iLine],"Paramet",7);
             iLine++ ) {}

        for( nOffset=atoi(pszField+6); 
             papszNV[iLine] != NULL && nOffset > 0; 
             nOffset--, iLine++ ) {}

        if( papszNV[iLine] != NULL )
        {
            char        **papszTokens, *pszLine = papszNV[iLine];
            double      dfValue;
            
            int         i;
            
            // Trim comments.
            for( i=0; pszLine[i] != '\0'; i++ )
            {
                if( pszLine[i] == '/' && pszLine[i+1] == '*' )
                    pszLine[i] = '\0';
            }

            papszTokens = CSLTokenizeString(papszNV[iLine]);
            if( CSLCount(papszTokens) == 3 )
            {
                dfValue = ABS(atof(papszTokens[0]))
                    + atof(papszTokens[1]) / 60.0
                    + atof(papszTokens[2]) / 3600.0;

                if( atof(papszTokens[0]) < 0.0 )
                    dfValue *= -1;
            }
            else
                dfValue = atof(papszTokens[0]);

            CSLDestroy( papszTokens );

            return dfValue;
        }
        else
            return dfDefaultValue;
    }
    else
    {
        for( iLine = 0; 
             papszNV[iLine] != NULL && 
                 !EQUALN(papszNV[iLine],pszField,strlen(pszField));
             iLine++ ) {}

        if( papszNV[iLine] == NULL )
            return dfDefaultValue;
        else
            return atof( papszNV[iLine] + strlen(pszField) );
    }
}

/************************************************************************/
/*                              OSR_GDS()                               */
/************************************************************************/

static const char*OSR_GDS( char **papszNV, const char * pszField, 
                           const char *pszDefaultValue )

{
    int         iLine;

    if( papszNV == NULL || papszNV[0] == NULL )
        return pszDefaultValue;

    for( iLine = 0; 
         papszNV[iLine] != NULL && 
             !EQUALN(papszNV[iLine],pszField,strlen(pszField));
         iLine++ ) {}

    if( papszNV[iLine] == NULL )
        return pszDefaultValue;
    else
    {
        static char     szResult[80];
        char    **papszTokens;
        
        papszTokens = CSLTokenizeString(papszNV[iLine]);

        if( CSLCount(papszTokens) > 1 )
            strncpy( szResult, papszTokens[1], sizeof(szResult));
        else
            strncpy( szResult, pszDefaultValue, sizeof(szResult));
        
        CSLDestroy( papszTokens );
        return szResult;
    }
}

/************************************************************************/
/*                          importFromESRI()                            */
/************************************************************************/

/**
 * Import coordinate system from ESRI .prj format(s).
 *
 * This function will read the text loaded from an ESRI .prj file, and
 * translate it into an OGRSpatialReference definition.  This should support
 * many (but by no means all) old style (Arc/Info 7.x) .prj files, as well
 * as the newer pseudo-OGC WKT .prj files.  Note that new style .prj files
 * are in OGC WKT format, but require some manipulation to correct datum
 * names, and units on some projection parameters.  This is addressed within
 * importFromESRI() by an automatical call to morphFromESRI(). 
 *
 * Currently only GEOGRAPHIC, UTM, STATEPLANE, GREATBRITIAN_GRID, ALBERS, 
 * EQUIDISTANT_CONIC, and TRANSVERSE (mercator) projections are supported
 * from old style files. 
 *
 * At this time there is no equivelent exportToESRI() method.  Writing old
 * style .prj files is not supported by OGRSpatialReference. However the
 * morphToESRI() and exportToWkt() methods can be used to generate output
 * suitable to write to new style (Arc 8) .prj files. 
 *
 * This function is the equilvelent of the C function OSRImportFromESRI().
 *
 * @param papszPrj NULL terminated list of strings containing the definition.
 *
 * @return OGRERR_NONE on success or an error code in case of failure. 
 */

OGRErr OGRSpatialReference::importFromESRI( char **papszPrj )

{
    if( papszPrj == NULL || papszPrj[0] == NULL )
        return OGRERR_CORRUPT_DATA;

/* -------------------------------------------------------------------- */
/*      Some newer ESRI products, like ArcPad, produce .prj files       */
/*      with WKT in them.  Check if that appears to be the case.        */
/*                                                                      */
/*      ESRI uses an odd datum naming scheme, so some further           */
/*      massaging may be required.                                      */
/* -------------------------------------------------------------------- */
    if( EQUALN(papszPrj[0],"GEOGCS",6)
        || EQUALN(papszPrj[0],"PROJCS",6)
        || EQUALN(papszPrj[0],"LOCAL_CS",8) )
    {
        char    *pszWKT;
        OGRErr  eErr;

        pszWKT = papszPrj[0];
        eErr = importFromWkt( &pszWKT );
        if( eErr == OGRERR_NONE )
            eErr = morphFromESRI();
        return eErr;
    }

/* -------------------------------------------------------------------- */
/*      Operate on the basis of the projection name.                    */
/* -------------------------------------------------------------------- */
    const char *pszProj = OSR_GDS( papszPrj, "Projection", NULL );

    if( pszProj == NULL )
    {
        CPLDebug( "OGR_ESRI", "Can't find Projection\n" );
        return OGRERR_CORRUPT_DATA;
    }

    else if( EQUAL(pszProj,"GEOGRAPHIC") )
    {
    }
    
    else if( EQUAL(pszProj,"utm") )
    {
        if( (int) OSR_GDV( papszPrj, "zone", 0.0 ) != 0 )
        {
            double      dfYShift = OSR_GDV( papszPrj, "Yshift", 0.0 );

            SetUTM( (int) OSR_GDV( papszPrj, "zone", 0.0 ),
                    dfYShift >= 0.0 );
        }
        else
        {
            double      dfCentralMeridian, dfRefLat;
            int         nZone;

            dfCentralMeridian = OSR_GDV( papszPrj, "PARAM_1", 0.0 );
            dfRefLat = OSR_GDV( papszPrj, "PARAM_2", 0.0 );

            nZone = (int) ((dfCentralMeridian+183) / 6.0 + 0.0000001);
            SetUTM( nZone, dfRefLat >= 0.0 );
        }
    }

    else if( EQUAL(pszProj,"STATEPLANE") )
    {
        int nZone = (int) OSR_GDV( papszPrj, "zone", 0.0 );
        nZone = ESRIToUSGSZone( nZone );

        if( nZone != 0 )
        {
            if( EQUAL(OSR_GDS( papszPrj, "Datum", "NAD83"),"NAD27") )
                SetStatePlane( nZone, FALSE );
            else
                SetStatePlane( nZone, TRUE );
        }
    }

    else if( EQUAL(pszProj,"GREATBRITIAN_GRID") 
             || EQUAL(pszProj,"GREATBRITAIN_GRID") )
    {
        const char *pszWkt = 
            "PROJCS[\"OSGB 1936 / British National Grid\",GEOGCS[\"OSGB 1936\",DATUM[\"OSGB_1936\",SPHEROID[\"Airy 1830\",6377563.396,299.3249646]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",49],PARAMETER[\"central_meridian\",-2],PARAMETER[\"scale_factor\",0.999601272],PARAMETER[\"false_easting\",400000],PARAMETER[\"false_northing\",-100000],UNIT[\"metre\",1]]";

        importFromWkt( (char **) &pszWkt );
    }

    else if( EQUAL(pszProj,"ALBERS") )
    {
        SetACEA( OSR_GDV( papszPrj, "PARAM_1", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_4", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_5", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_6", 0.0 ) );
    }

    else if( EQUAL(pszProj,"LAMBERT") )
    {
        SetLCC( OSR_GDV( papszPrj, "PARAM_1", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_2", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_4", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_3", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_5", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_6", 0.0 ) );
    }

    else if( EQUAL(pszProj,"EQUIDISTANT_CONIC") )
    {
        int     nStdPCount = (int) OSR_GDV( papszPrj, "PARAM_1", 0.0 );

        if( nStdPCount == 1 )
        {
            SetEC( OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_4", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_5", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_6", 0.0 ) );
        }
        else
        {
            SetEC( OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_5", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_4", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_5", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_7", 0.0 ) );
        }
    }

    else if( EQUAL(pszProj,"TRANSVERSE") )
    {
        SetTM( OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_1", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_4", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_5", 0.0 ) );
    }

    else if( EQUAL(pszProj,"POLAR") )
    {
        SetPS( OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_1", 0.0 ), 
               1.0,
               OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_4", 0.0 ) );
    }

    else
    {
        CPLDebug( "OGR_ESRI", "Unsupported projection: %s", pszProj );
        SetLocalCS( pszProj );
    }

/* -------------------------------------------------------------------- */
/*      Try to translate the datum/spheroid.                            */
/* -------------------------------------------------------------------- */
    if( !IsLocal() && GetAttrNode( "GEOGCS" ) == NULL )
    {
        const char *pszDatum;

        pszDatum = OSR_GDS( papszPrj, "Datum", "");

        if( EQUAL(pszDatum,"NAD27") || EQUAL(pszDatum,"NAD83")
            || EQUAL(pszDatum,"WGS84") || EQUAL(pszDatum,"WGS72") )
        {
            SetWellKnownGeogCS( pszDatum );
        }
        else if( EQUAL( pszDatum, "EUR" ) )
        {
            SetWellKnownGeogCS( "EPSG:4230" );
        }
        else
        {
            const char *pszSpheroid;

            pszSpheroid = OSR_GDS( papszPrj, "Spheroid", "");
            
            if( EQUAL(pszSpheroid,"INT1909") )
            {
                OGRSpatialReference oGCS;
                oGCS.importFromEPSG( 4022 );
                CopyGeogCSFrom( &oGCS );
            }
            else if( EQUAL(pszSpheroid,"AIRY") )
            {
                OGRSpatialReference oGCS;
                oGCS.importFromEPSG( 4001 );
                CopyGeogCSFrom( &oGCS );
            }
            else if( EQUAL(pszSpheroid,"CLARKE1866") )
            {
                OGRSpatialReference oGCS;
                oGCS.importFromEPSG( 4008 );
                CopyGeogCSFrom( &oGCS );
            }
            else
            {
                // If we don't know, default to WGS84 so there is something there.
                SetWellKnownGeogCS( "WGS84" );
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Linear units translation                                        */
/* -------------------------------------------------------------------- */
    if( IsLocal() || IsProjected() )
    {
        const char *pszValue;

        pszValue = OSR_GDS( papszPrj, "Units", NULL );
        if( pszValue == NULL )
            SetLinearUnits( SRS_UL_METER, 1.0 );
        else if( EQUAL(pszValue,"FEET") )
            SetLinearUnits( SRS_UL_FOOT, atof(SRS_UL_FOOT_CONV) );
        else
            SetLinearUnits( pszValue, 1.0 );
    }
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                            morphToESRI()                             */
/************************************************************************/

/**
 * Convert in place from ESRI WKT format.
 *
 * The value notes of this coordinate system as modified in various manners
 * to adhere more closely to the WKT standard.  This mostly involves
 * translating a variety of ESRI names for projections, arguments and
 * datums to "standard" names, as defined by Adam Gawne-Cain's reference
 * translation of EPSG to WKT for the CT specification.
 *
 * This does the same as the C function OSRMorphToESRI().
 *
 * @return OGRERR_NONE unless something goes badly wrong.
 */

OGRErr OGRSpatialReference::morphToESRI()

{
    OGRErr      eErr;

/* -------------------------------------------------------------------- */
/*      Strip all CT parameters (AXIS, AUTHORITY, TOWGS84, etc).        */
/* -------------------------------------------------------------------- */
    eErr = StripCTParms();
    if( eErr != OGRERR_NONE )
        return eErr;

    if( GetRoot() == NULL )
        return OGRERR_NONE;

/* -------------------------------------------------------------------- */
/*      Translate PROJECTION keywords that are misnamed.                */
/* -------------------------------------------------------------------- */
    GetRoot()->applyRemapper( "PROJECTION", 
                              apszProjMapping+1, apszProjMapping, 2 );

/* -------------------------------------------------------------------- */
/*      Translate DATUM keywords that are misnamed.                     */
/* -------------------------------------------------------------------- */
    InitDatumMappingTable();

    GetRoot()->applyRemapper( "DATUM", 
                              papszDatumMapping+2, papszDatumMapping+1, 3 );

/* -------------------------------------------------------------------- */
/*      Translate UNIT keywords that are misnamed, or even the wrong    */
/*      case.                                                           */
/* -------------------------------------------------------------------- */
    GetRoot()->applyRemapper( "UNIT", 
                              apszUnitMapping+1, apszUnitMapping, 2 );

/* -------------------------------------------------------------------- */
/*      reset constants for decimal degrees to the exact string ESRI    */
/*      expects when encountered to ensure a matchup.                   */
/* -------------------------------------------------------------------- */
    OGR_SRSNode *poUnit = GetAttrNode( "GEOGCS|UNIT" );
    
    if( poUnit != NULL && poUnit->GetChildCount() >= 2 
        && ABS(GetAngularUnits()-0.0174532925199433) < 0.00000000001 )
    {
        poUnit->GetChild(0)->SetValue("Degree");
        poUnit->GetChild(1)->SetValue("0.017453292519943295");
    }

/* -------------------------------------------------------------------- */
/*      Make sure we reproduce US Feet exactly too.                     */
/* -------------------------------------------------------------------- */
    poUnit = GetAttrNode( "PROJCS|UNIT" );
    
    if( poUnit != NULL && poUnit->GetChildCount() >= 2 
        && ABS(GetLinearUnits()- 0.30480060960121924) < 0.000000000000001)
    {
        poUnit->GetChild(0)->SetValue("Foot_US");
        poUnit->GetChild(1)->SetValue("0.30480060960121924");
    }

/* -------------------------------------------------------------------- */
/*      Remap parameters used for Albers.                               */
/* -------------------------------------------------------------------- */
    const char *pszProjection = GetAttrValue("PROJECTION");
    
    if( pszProjection != NULL && EQUAL(pszProjection,"Albers") )
        GetRoot()->applyRemapper( 
            "PARAMETER", apszAlbersMapping + 1, apszAlbersMapping + 0, 2 );

/* -------------------------------------------------------------------- */
/*      Try to insert a D_ in front of the datum name.                  */
/* -------------------------------------------------------------------- */
    OGR_SRSNode *poDatum;

    poDatum = GetAttrNode( "DATUM" );
    if( poDatum != NULL )
        poDatum = poDatum->GetChild(0);

    if( poDatum != NULL )
    {
        if( !EQUALN(poDatum->GetValue(),"D_",2) )
        {
            char *pszNewValue;

            pszNewValue = (char *) CPLMalloc(strlen(poDatum->GetValue())+3);
            strcpy( pszNewValue, "D_" );
            strcat( pszNewValue, poDatum->GetValue() );
            poDatum->SetValue( pszNewValue );
            CPLFree( pszNewValue );
        }
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                           OSRMorphToESRI()                           */
/************************************************************************/

OGRErr OSRMorphToESRI( OGRSpatialReferenceH hSRS )

{
    return ((OGRSpatialReference *) hSRS)->morphToESRI();
}

/************************************************************************/
/*                           morphFromESRI()                            */
/*                                                                      */
/*      modify this definition from the ESRI definition of WKT to       */
/*      the "Standard" definition.                                      */
/************************************************************************/

/**
 * Convert in place to ESRI WKT format.
 *
 * The value nodes of this coordinate system as modified in various manners
 * more closely map onto the ESRI concept of WKT format.  This includes
 * renaming a variety of projections and arguments, and stripping out 
 * nodes note recognised by ESRI (like AUTHORITY and AXIS). 
 *
 * This does the same as the C function OSRMorphFromESRI().
 *
 * @return OGRERR_NONE unless something goes badly wrong.
 */

OGRErr OGRSpatialReference::morphFromESRI()

{
    OGRErr      eErr = OGRERR_NONE;

    if( GetRoot() == NULL )
        return OGRERR_NONE;

/* -------------------------------------------------------------------- */
/*      Translate DATUM keywords that are oddly named.                  */
/* -------------------------------------------------------------------- */
    InitDatumMappingTable();

    GetRoot()->applyRemapper( "DATUM", 
                              papszDatumMapping+1, papszDatumMapping+2, 3 );

/* -------------------------------------------------------------------- */
/*      Try to remove any D_ in front of the datum name.                */
/* -------------------------------------------------------------------- */
    OGR_SRSNode *poDatum;

    poDatum = GetAttrNode( "DATUM" );
    if( poDatum != NULL )
        poDatum = poDatum->GetChild(0);

    if( poDatum != NULL )
    {
        if( EQUALN(poDatum->GetValue(),"D_",2) )
        {
            char *pszNewValue = CPLStrdup( poDatum->GetValue() + 2 );
            poDatum->SetValue( pszNewValue );
            CPLFree( pszNewValue );
        }
    }

/* -------------------------------------------------------------------- */
/*      Split Lambert_Conformal_Conic into 1SP or 2SP form.             */
/*                                                                      */
/*      See bugzilla.remotesensing.org/show_bug.cgi?id=187              */
/* -------------------------------------------------------------------- */
    const char *pszProjection = GetAttrValue("PROJECTION");
    
    if( pszProjection != NULL
        && EQUAL(pszProjection,"Lambert_Conformal_Conic") )
    {
        if( GetProjParm( "Scale_Factor", 2.0 ) == 2.0 )
            SetNode( "PROJCS|PROJECTION", 
                     SRS_PT_LAMBERT_CONFORMAL_CONIC_2SP );
        else
            SetNode( "PROJCS|PROJECTION", 
                     SRS_PT_LAMBERT_CONFORMAL_CONIC_1SP );
    }

/* -------------------------------------------------------------------- */
/*      Remap albers parameters.                                        */
/* -------------------------------------------------------------------- */
    if( pszProjection != NULL && EQUAL(pszProjection,"Albers") )
        GetRoot()->applyRemapper( 
            "PARAMETER", apszAlbersMapping + 0, apszAlbersMapping + 1, 2 );

/* -------------------------------------------------------------------- */
/*      Translate PROJECTION keywords that are misnamed.                */
/* -------------------------------------------------------------------- */
    GetRoot()->applyRemapper( "PROJECTION", 
                              apszProjMapping, apszProjMapping+1, 2 );
    
/* -------------------------------------------------------------------- */
/*      Translate DATUM keywords that are misnamed.                     */
/* -------------------------------------------------------------------- */
    InitDatumMappingTable();

    GetRoot()->applyRemapper( "DATUM", 
                              papszDatumMapping+1, papszDatumMapping+2, 3 );

    return eErr;
}

/************************************************************************/
/*                          OSRMorphFromESRI()                          */
/************************************************************************/

OGRErr OSRMorphFromESRI( OGRSpatialReferenceH hSRS )

{
    return ((OGRSpatialReference *) hSRS)->morphFromESRI();
}
