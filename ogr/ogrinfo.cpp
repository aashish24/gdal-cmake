/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Simple client for viewing OGR driver data.
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
 * Revision 1.10  2001/07/17 15:00:21  danmo
 * Report layer extent in ReportOnLayer().
 *
 * Revision 1.9  2001/06/26 20:58:45  warmerda
 * added spatial query option
 *
 * Revision 1.8  2001/06/19 15:48:36  warmerda
 * added feature attribute query support
 *
 * Revision 1.7  2000/03/14 21:37:49  warmerda
 * report layer geometry type
 *
 * Revision 1.6  1999/11/18 19:02:19  warmerda
 * expanded tabs
 *
 * Revision 1.5  1999/11/04 21:07:22  warmerda
 * Changed to OGRRegisterAll().
 *
 * Revision 1.4  1999/09/29 16:36:41  warmerda
 * added srs reporting
 *
 * Revision 1.3  1999/09/22 13:31:48  warmerda
 * added sdts
 *
 * Revision 1.2  1999/09/13 14:34:20  warmerda
 * added feature reporting
 *
 * Revision 1.1  1999/09/09 20:40:19  warmerda
 * New
 *
 */

#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "cpl_string.h"

int     bReadOnly = FALSE;
int     bVerbose = TRUE;

static void Usage();

static void ReportOnLayer( OGRLayer *, const char *, OGRGeometry * );

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc, char ** papszArgv )

{
    const char *pszWHERE = NULL;
    const char  *pszDataSource = NULL;
    char        **papszLayers = NULL;
    OGRGeometry *poSpatialFilter = NULL;
    
/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    OGRRegisterAll();
    
/* -------------------------------------------------------------------- */
/*      Processing command line arguments.                              */
/* -------------------------------------------------------------------- */
    for( int iArg = 1; iArg < nArgc; iArg++ )
    {
        if( EQUAL(papszArgv[iArg],"-ro") )
            bReadOnly = TRUE;
        else if( EQUAL(papszArgv[iArg],"-q") )
            bVerbose = FALSE;
        else if( EQUAL(papszArgv[iArg],"-spat") 
                 && papszArgv[iArg+1] != NULL 
                 && papszArgv[iArg+2] != NULL 
                 && papszArgv[iArg+3] != NULL 
                 && papszArgv[iArg+4] != NULL )
        {
            OGRLinearRing  oRing;

            oRing.addPoint( atof(papszArgv[iArg+1]), atof(papszArgv[iArg+2]) );
            oRing.addPoint( atof(papszArgv[iArg+1]), atof(papszArgv[iArg+4]) );
            oRing.addPoint( atof(papszArgv[iArg+3]), atof(papszArgv[iArg+4]) );
            oRing.addPoint( atof(papszArgv[iArg+3]), atof(papszArgv[iArg+2]) );
            oRing.addPoint( atof(papszArgv[iArg+1]), atof(papszArgv[iArg+2]) );

            poSpatialFilter = new OGRPolygon();
            ((OGRPolygon *) poSpatialFilter)->addRing( &oRing );
            iArg += 4;
        }
        else if( EQUAL(papszArgv[iArg],"-where") && papszArgv[iArg+1] != NULL )
        {
            pszWHERE = papszArgv[++iArg];
        }
        else if( papszArgv[iArg][0] == '-' )
        {
            Usage();
        }
        else if( pszDataSource == NULL )
            pszDataSource = papszArgv[iArg];
        else
            papszLayers = CSLAddString( papszLayers, papszArgv[iArg] );
    }

    if( pszDataSource == NULL )
        Usage();

/* -------------------------------------------------------------------- */
/*      Open data source.                                               */
/* -------------------------------------------------------------------- */
    OGRDataSource       *poDS;
    OGRSFDriver         *poDriver;

    poDS = OGRSFDriverRegistrar::Open( pszDataSource, !bReadOnly, &poDriver );
    if( poDS == NULL && !bReadOnly )
    {
        poDS = OGRSFDriverRegistrar::Open( pszDataSource, FALSE, &poDriver );
        if( poDS != NULL && bVerbose )
        {
            printf( "Had to open data source read-only.\n" );
            bReadOnly = TRUE;
        }
    }

/* -------------------------------------------------------------------- */
/*      Report failure                                                  */
/* -------------------------------------------------------------------- */
    if( poDS == NULL )
    {
        OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
        
        printf( "FAILURE:\n"
                "Unable to open datasource `%s' with the following drivers.\n",
                pszDataSource );

        for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
        {
            printf( "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
        }

        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Some information messages.                                      */
/* -------------------------------------------------------------------- */
    if( bVerbose )
        printf( "INFO: Open of `%s'\n"
                "using driver `%s' successful.\n",
                pszDataSource, poDriver->GetName() );

    if( bVerbose && !EQUAL(pszDataSource,poDS->GetName()) )
    {
        printf( "INFO: Internal data source name `%s'\n"
                "      different from user name `%s'.\n",
                poDS->GetName(), pszDataSource );
    }

/* -------------------------------------------------------------------- */
/*      Process each data source layer.                                 */
/* -------------------------------------------------------------------- */
    for( int iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++ )
    {
        OGRLayer        *poLayer = poDS->GetLayer(iLayer);

        if( poLayer == NULL )
        {
            printf( "FAILURE: Couldn't fetch advertised layer %d!\n",
                    iLayer );
            exit( 1 );
        }

        if( CSLCount(papszLayers) == 0 )
        {
            printf( "%d: %s",
                    iLayer+1,
                    poLayer->GetLayerDefn()->GetName() );

            if( poLayer->GetLayerDefn()->GetGeomType() != wkbUnknown )
                printf( " (%s)", 
                        OGRGeometryTypeToName( 
                            poLayer->GetLayerDefn()->GetGeomType() ) );

            printf( "\n" );
        }
        else if( CSLFindString( papszLayers,
                                poLayer->GetLayerDefn()->GetName() ) != -1 )
        {
            ReportOnLayer( poLayer, pszWHERE, poSpatialFilter );
        }
    }

/* -------------------------------------------------------------------- */
/*      Close down.                                                     */
/* -------------------------------------------------------------------- */
    delete poDS;

#ifdef DBMALLOC
    malloc_dump(1);
#endif
    
    return 0;
}

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( "Usage: ogrinfo [-ro] [-q] [-where restricted_where]\n"
            "               [-spat xmin ymin xmax ymax]\n"
            "               datasource_name [layer [layer ...]]\n");
    exit( 1 );
}

/************************************************************************/
/*                           ReportOnLayer()                            */
/************************************************************************/

static void ReportOnLayer( OGRLayer * poLayer, const char *pszWHERE, 
                           OGRGeometry *poSpatialFilter )

{
    OGRFeatureDefn      *poDefn = poLayer->GetLayerDefn();

/* -------------------------------------------------------------------- */
/*      Set filters if provided.                                        */
/* -------------------------------------------------------------------- */
    if( pszWHERE != NULL )
        poLayer->SetAttributeFilter( pszWHERE );

    if( poSpatialFilter != NULL )
        poLayer->SetSpatialFilter( poSpatialFilter );

/* -------------------------------------------------------------------- */
/*      Report various overall information.                             */
/* -------------------------------------------------------------------- */
    printf( "\n" );
    
    printf( "Layer name: %s\n", poDefn->GetName() );

    printf( "Geometry: %s\n", 
            OGRGeometryTypeToName( poDefn->GetGeomType() ) );

    printf( "Feature Count: %d\n", poLayer->GetFeatureCount() );

    OGREnvelope oExt;
    if (poLayer->GetExtent(&oExt, TRUE) == OGRERR_NONE)
    {
        printf("Extent: (%f, %f) - (%f, %f)\n", 
               oExt.MinX, oExt.MinY, oExt.MaxX, oExt.MaxY);
    }

    if( bVerbose )
    {
        char    *pszWKT;
        
        if( poLayer->GetSpatialRef() == NULL )
            pszWKT = CPLStrdup( "(NULL)" );
        else
            poLayer->GetSpatialRef()->exportToWkt( &pszWKT );

        printf( "Layer SRS WKT: %s\n", pszWKT );
        CPLFree( pszWKT );
    }
    
    for( int iAttr = 0; iAttr < poDefn->GetFieldCount(); iAttr++ )
    {
        OGRFieldDefn    *poField = poDefn->GetFieldDefn( iAttr );

        printf( "%s: %s (%d.%d)\n",
                poField->GetNameRef(),
                poField->GetFieldTypeName( poField->GetType() ),
                poField->GetWidth(),
                poField->GetPrecision() );
    }

/* -------------------------------------------------------------------- */
/*      Read, and dump features.                                        */
/* -------------------------------------------------------------------- */
    OGRFeature  *poFeature;

    while( (poFeature = poLayer->GetNextFeature()) != NULL )
    {
        poFeature->DumpReadable( stdout );
        delete poFeature;
    }
}
