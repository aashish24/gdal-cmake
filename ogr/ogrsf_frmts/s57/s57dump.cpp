/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Simple client for viewing S57 driver data.
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
 * Revision 1.10  2001/08/30 03:48:43  warmerda
 * preliminary implementation of S57 Update Support
 *
 * Revision 1.9  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.8  2000/06/16 18:10:05  warmerda
 * expanded tabs
 *
 * Revision 1.7  1999/11/26 15:17:01  warmerda
 * fixed lname to lnam
 *
 * Revision 1.6  1999/11/26 15:08:38  warmerda
 * added setoptions, and LNAM support
 *
 * Revision 1.5  1999/11/18 19:01:25  warmerda
 * expanded tabs
 *
 * Revision 1.4  1999/11/18 18:57:45  warmerda
 * utilize s57filecollector
 *
 * Revision 1.3  1999/11/16 21:47:32  warmerda
 * updated class occurance collection
 *
 * Revision 1.2  1999/11/08 22:23:00  warmerda
 * added object class support
 *
 * Revision 1.1  1999/11/03 22:12:43  warmerda
 * New
 *
 */

#include "s57.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc, char ** papszArgv )

{
    char        **papszOptions = NULL;
    int		bUpdate = TRUE;
    
    if( nArgc < 2 )
    {
        printf( "Usage: s57dump [-split] [-lnam] [-no-update] filename\n" );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Process commandline arguments.                                  */
/* -------------------------------------------------------------------- */
    for( int iArg = 1; iArg < nArgc-1; iArg++ )
    {
        if( EQUAL(papszArgv[iArg],"-split") )
            papszOptions =
                CSLSetNameValue( papszOptions, "SPLIT_MULTIPOINT", "ON" );
        else if( EQUAL(papszArgv[iArg],"-no-update") )
            bUpdate = FALSE;
        else if( EQUALN(papszArgv[iArg],"-lnam",4) )
            papszOptions =
                CSLSetNameValue( papszOptions, "LNAM_REFS", "ON" );
    }
    
/* -------------------------------------------------------------------- */
/*      Load the class definitions into the registrar.                  */
/* -------------------------------------------------------------------- */
    S57ClassRegistrar   oRegistrar;
    int                 bRegistrarLoaded;

    bRegistrarLoaded = oRegistrar.LoadInfo( "/home/warmerda/data/s57", TRUE );

/* -------------------------------------------------------------------- */
/*      Get a list of candidate files.                                  */
/* -------------------------------------------------------------------- */
    char        **papszFiles;
    int         iFile;

    papszFiles = S57FileCollector( papszArgv[nArgc-1] );

    for( iFile = 0; papszFiles != NULL && papszFiles[iFile] != NULL; iFile++ )
    {
        printf( "Found: %s\n", papszFiles[iFile] );
    }

    for( iFile = 0; papszFiles != NULL && papszFiles[iFile] != NULL; iFile++ )
    {
        printf( "<------------------------------------------------------------------------->\n" );
        printf( "\nFile: %s\n\n", papszFiles[iFile] );
        
        S57Reader       oReader( papszFiles[iFile] );

        oReader.SetOptions( papszOptions );
        
        if( !oReader.Open( FALSE ) )
            continue;

        if( bRegistrarLoaded )
        {
            int i, anClassList[MAX_CLASSES];
            
            for( i = 0; i < MAX_CLASSES; i++ )
                anClassList[i] = 0;
        
            oReader.CollectClassList(anClassList, MAX_CLASSES);

            oReader.SetClassBased( &oRegistrar );

            printf( "Classes found:\n" );
            for( i = 0; i < MAX_CLASSES; i++ )
            {
                if( anClassList[i] == 0 )
                    continue;
                
                oRegistrar.SelectClass( i );
                printf( "%d: %s/%s\n",
                        i,
                        oRegistrar.GetAcronym(),
                        oRegistrar.GetDescription() );
            
                oReader.AddFeatureDefn(
                    oReader.GenerateObjectClassDefn( &oRegistrar, i ) );
            }
        }
        else
        {
            oReader.AddFeatureDefn(
                oReader.GenerateGeomFeatureDefn( wkbPoint ) );
            oReader.AddFeatureDefn(
                oReader.GenerateGeomFeatureDefn( wkbLineString ) );
            oReader.AddFeatureDefn(
                oReader.GenerateGeomFeatureDefn( wkbPolygon ) );
            oReader.AddFeatureDefn(
                oReader.GenerateGeomFeatureDefn( wkbNone ) );
        }
    
        OGRFeature      *poFeature;
        int             nFeatures = 0;
        DDFModule	oUpdate;

        if( bUpdate )
            oReader.FindAndApplyUpdates(papszFiles[iFile]);
    
        while( (poFeature = oReader.ReadNextFeature()) != NULL )
        {
            poFeature->DumpReadable( stdout );
            nFeatures++;
            delete poFeature;
        }

        printf( "Feature Count: %d\n", nFeatures );
    }

    return 0;
}

