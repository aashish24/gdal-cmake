/******************************************************************************
 * $Id$
 *
 * Project:  Oracle Spatial Driver
 * Purpose:  Implementation of the OGROCIDataSource class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.9  2003/01/10 19:30:14  warmerda
 * Another related initialization fix.
 *
 * Revision 1.8  2003/01/10 16:40:33  warmerda
 * initialize SRS table
 *
 * Revision 1.7  2003/01/07 22:24:35  warmerda
 * added SRS support
 *
 * Revision 1.6  2003/01/06 17:58:20  warmerda
 * Fix FID support, add DIM creation keyword
 *
 * Revision 1.5  2003/01/02 21:48:52  warmerda
 * uppercaseify layer names when creating tables
 *
 * Revision 1.4  2002/12/29 19:43:59  warmerda
 * avoid some warnings
 *
 * Revision 1.3  2002/12/29 03:19:48  warmerda
 * fixed extraction of database name
 *
 * Revision 1.2  2002/12/28 04:38:36  warmerda
 * converted to unix file conventions
 *
 * Revision 1.1  2002/12/28 04:07:27  warmerda
 * New
 *
 */

#include "ogr_oci.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                          OGROCIDataSource()                          */
/************************************************************************/

OGROCIDataSource::OGROCIDataSource()

{
    pszName = NULL;
    pszDBName = NULL;
    papoLayers = NULL;
    nLayers = 0;
    poSession = NULL;
    papoSRS = NULL;
    panSRID = NULL;
    nKnownSRID = 0;
}

/************************************************************************/
/*                         ~OGROCIDataSource()                          */
/************************************************************************/

OGROCIDataSource::~OGROCIDataSource()

{
    int         i;

    CPLFree( pszName );
    CPLFree( pszDBName );

    for( i = 0; i < nLayers; i++ )
        delete papoLayers[i];
    
    CPLFree( papoLayers );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

int OGROCIDataSource::Open( const char * pszNewName, int bUpdate,
                            int bTestOpen )

{
    CPLAssert( nLayers == 0 && poSession == NULL );

/* -------------------------------------------------------------------- */
/*      Verify postgresql prefix.                                       */
/* -------------------------------------------------------------------- */
    if( !EQUALN(pszNewName,"OCI:",3) )
    {
        if( !bTestOpen )
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "%s does not conform to Oracle OCI driver naming convention,"
                      " OCI:*\n" );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Try to parse out name, password and database name.              */
/* -------------------------------------------------------------------- */
    char *pszUserid;
    const char *pszPassword = "";
    const char *pszDatabase = "";
    int   i;

    pszUserid = CPLStrdup( pszNewName + 4 );
    for( i = 0; 
         pszUserid[i] != '\0' && pszUserid[i] != '/' && pszUserid[i] != '@';
         i++ ) {}

    if( pszUserid[i] == '/' )
    {
        pszUserid[i++] = '\0';
        pszPassword = pszUserid + i;
        for( ; pszUserid[i] != '\0' && pszUserid[i] != '@'; i++ ) {}
    }

    if( pszUserid[i] == '@' )
    {
        pszUserid[i++] = '\0';
        pszDatabase = pszUserid + i;
    }

/* -------------------------------------------------------------------- */
/*      Try to establish connection.                                    */
/* -------------------------------------------------------------------- */
    CPLDebug( "OCI", "Userid=%s, Password=%s, Database=%s", 
              pszUserid, pszPassword, pszDatabase );

    poSession = OGRGetOCISession( pszUserid, pszPassword, pszDatabase );
    if( poSession == NULL )
        return FALSE;

    pszName = CPLStrdup( pszNewName );
    
    bDSUpdate = bUpdate;

/* -------------------------------------------------------------------- */
/*      Get a list of available spatial tables and make them into       */
/*      layers.                                                         */
/* -------------------------------------------------------------------- */
    OGROCIStatement oGetTables( poSession );

    if( oGetTables.Execute( 
        "SELECT TABLE_NAME, COLUMN_NAME, SRID FROM USER_SDO_GEOM_METADATA" ) 
        == CE_None )
    {
        char **papszRow;

        while( (papszRow = oGetTables.SimpleFetchRow()) != NULL )
        {
            int nSRID = -1;

            if( papszRow[2] != NULL )
                nSRID = atoi(papszRow[2]);

            OpenTable( papszRow[0], papszRow[1], nSRID, bUpdate, FALSE );
        }
    }

    return TRUE;
}

/************************************************************************/
/*                             OpenTable()                              */
/************************************************************************/

int OGROCIDataSource::OpenTable( const char *pszNewName, 
                                 const char *pszGeomCol,
                                 int nSRID, int bUpdate, int bTestOpen )

{
/* -------------------------------------------------------------------- */
/*      Create the layer object.                                        */
/* -------------------------------------------------------------------- */
    OGROCILayer	*poLayer;

    poLayer = new OGROCITableLayer( this, pszNewName, pszGeomCol, nSRID, 
                                    bUpdate, FALSE );

/* -------------------------------------------------------------------- */
/*      Add layer to data source layer list.                            */
/* -------------------------------------------------------------------- */
    papoLayers = (OGROCILayer **)
        CPLRealloc( papoLayers,  sizeof(OGROCILayer *) * (nLayers+1) );
    papoLayers[nLayers++] = poLayer;

    return TRUE;
}

/************************************************************************/
/*                            DeleteLayer()                             */
/************************************************************************/

void OGROCIDataSource::DeleteLayer( const char *pszLayerName )

{
    int	iLayer;

/* -------------------------------------------------------------------- */
/*      Try to find layer.                                              */
/* -------------------------------------------------------------------- */
    for( iLayer = 0; iLayer < nLayers; iLayer++ )
    {
        if( EQUAL(pszLayerName,papoLayers[iLayer]->GetLayerDefn()->GetName()) )
            break;
    }

    if( iLayer == nLayers )
        return;

/* -------------------------------------------------------------------- */
/*      Blow away our OGR structures related to the layer.  This is     */
/*      pretty dangerous if anything has a reference to this layer!     */
/* -------------------------------------------------------------------- */
    CPLDebug( "OCI", "DeleteLayer(%s)", pszLayerName );

    delete papoLayers[iLayer];
    memmove( papoLayers + iLayer, papoLayers + iLayer + 1, 
             sizeof(void *) * (nLayers - iLayer - 1) );
    nLayers--;

/* -------------------------------------------------------------------- */
/*      Remove from the database.                                       */
/* -------------------------------------------------------------------- */
    OGROCIStatement oCommand( poSession );
    char	    szCommand[1024];

    sprintf( szCommand, "DROP TABLE \"%s\"", pszLayerName );
    oCommand.Execute( szCommand );

    sprintf( szCommand, 
             "DELETE FROM USER_SDO_GEOM_METADATA WHERE TABLE_NAME = '%s'", 
             pszLayerName );
    oCommand.Execute( szCommand );
}

/************************************************************************/
/*                            CreateLayer()                             */
/************************************************************************/

OGRLayer *
OGROCIDataSource::CreateLayer( const char * pszLayerName,
                              OGRSpatialReference *poSRS,
                              OGRwkbGeometryType eType,
                              char ** papszOptions )

{
    char		szCommand[1024];
    char               *pszSafeLayerName = CPLStrdup(pszLayerName);
    int                 i;
    
    for( i = 0; pszSafeLayerName[i] != '\0'; i++ )
        pszSafeLayerName[i] = toupper(pszSafeLayerName[i]);

/* -------------------------------------------------------------------- */
/*      Do we already have this layer?  If so, should we blow it        */
/*      away?                                                           */
/* -------------------------------------------------------------------- */
    int	iLayer;

    for( iLayer = 0; iLayer < nLayers; iLayer++ )
    {
        if( EQUAL(pszLayerName,papoLayers[iLayer]->GetLayerDefn()->GetName()) )
        {
            if( CSLFetchNameValue( papszOptions, "OVERWRITE" ) != NULL
                && !EQUAL(CSLFetchNameValue(papszOptions,"OVERWRITE"),"NO") )
            {
                DeleteLayer( pszSafeLayerName );
            }
            else
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "Layer %s already exists, CreateLayer failed.\n"
                          "Use the layer creation option OVERWRITE=YES to "
                          "replace it.",
                          pszLayerName );
                CPLFree( pszSafeLayerName );
                return NULL;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Try to get the SRS Id of this spatial reference system,         */
/*      adding tot the srs table if needed.                             */
/* -------------------------------------------------------------------- */
    char szSRSId[100];

    if( poSRS != NULL )
        sprintf( szSRSId, "%d", FetchSRSId( poSRS ) );
    else
        strcpy( szSRSId, "NULL" );

/* -------------------------------------------------------------------- */
/*      Create a basic table with the FID.  Also include the            */
/*      geometry if this is not a PostGIS enabled table.                */
/* -------------------------------------------------------------------- */
    OGROCIStatement oStatement( poSession );
    
    sprintf( szCommand, 
             "CREATE TABLE \"%s\" ( "
             "OGR_FID NUMBER, "
             "ORA_GEOMETRY %s )",
             pszSafeLayerName, SDO_GEOMETRY );

    if( oStatement.Execute( szCommand ) != CE_None )
    {
        CPLFree( pszSafeLayerName );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Add the table to the user_sdo_geom_metadata table without       */
/*      setting the dimension information.  Do that later when          */
/*      requesting an index to be built.                                */
/* -------------------------------------------------------------------- */

    sprintf( szCommand, 
             "INSERT INTO USER_SDO_GEOM_METADATA VALUES "
             "( '%s', '%s', NULL, %s )", 
             pszSafeLayerName, "ORA_GEOMETRY", szSRSId );

    oStatement.Execute( szCommand );

/* -------------------------------------------------------------------- */
/*      Create the layer object.                                        */
/* -------------------------------------------------------------------- */
    OGROCITableLayer	*poLayer;

    poLayer = new OGROCITableLayer( this, pszSafeLayerName, "ORA_GEOMETRY",
                                    -1, TRUE, TRUE );

    poLayer->SetLaunderFlag( CSLFetchBoolean(papszOptions,"LAUNDER",FALSE) );
    poLayer->SetPrecisionFlag( CSLFetchBoolean(papszOptions,"PRECISION",TRUE));

    if( CSLFetchNameValue(papszOptions,"DIM") != NULL )
        ((OGROCITableLayer *) poLayer)->SetDimension(
            atoi(CSLFetchNameValue(papszOptions,"DIM")) );

/* -------------------------------------------------------------------- */
/*      Add layer to data source layer list.                            */
/* -------------------------------------------------------------------- */
    papoLayers = (OGROCILayer **)
        CPLRealloc( papoLayers,  sizeof(OGROCILayer *) * (nLayers+1) );
    
    papoLayers[nLayers++] = poLayer;

    CPLFree( pszSafeLayerName );

    return poLayer;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGROCIDataSource::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,ODsCCreateLayer) && bDSUpdate )
        return TRUE;
    else
        return FALSE;
}

/************************************************************************/
/*                              GetLayer()                              */
/************************************************************************/

OGRLayer *OGROCIDataSource::GetLayer( int iLayer )

{
    if( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];
}

/************************************************************************/
/*                             ExecuteSQL()                             */
/************************************************************************/

OGRLayer * OGROCIDataSource::ExecuteSQL( const char *pszSQLCommand,
                                        OGRGeometry *poSpatialFilter,
                                        const char *pszDialect )

{
/* -------------------------------------------------------------------- */
/*      Just execute simple command.                                    */
/* -------------------------------------------------------------------- */
    if( !EQUALN(pszSQLCommand,"SELECT",6) )
    {
        OGROCIStatement oCommand( poSession );

        oCommand.Execute( pszSQLCommand, OCI_COMMIT_ON_SUCCESS );

        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Otherwise instantiate a layer.                                  */
/* -------------------------------------------------------------------- */
    else
    {
        OGROCIStatement oCommand( poSession );
        
        if( oCommand.Execute( pszSQLCommand, OCI_DESCRIBE_ONLY ) == CE_None )
            return new OGROCISelectLayer( this, pszSQLCommand, &oCommand );
        else
            return NULL;
    }
}

/************************************************************************/
/*                          ReleaseResultSet()                          */
/************************************************************************/

void OGROCIDataSource::ReleaseResultSet( OGRLayer * poLayer )

{
    delete poLayer;
}

/************************************************************************/
/*                              FetchSRS()                              */
/*                                                                      */
/*      Return a SRS corresponding to a particular id.  Note that       */
/*      reference counting should be honoured on the returned           */
/*      OGRSpatialReference, as handles may be cached.                  */
/************************************************************************/

OGRSpatialReference *OGROCIDataSource::FetchSRS( int nId )

{
    if( nId < 0 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      First, we look through our SRID cache, is it there?             */
/* -------------------------------------------------------------------- */
    int  i;

    for( i = 0; i < nKnownSRID; i++ )
    {
        if( panSRID[i] == nId )
            return papoSRS[i];
    }

/* -------------------------------------------------------------------- */
/*      Try looking up in MDSYS.CS_SRS table.                           */
/* -------------------------------------------------------------------- */
    OGROCIStatement oStatement( GetSession() );
    char            szSelect[200], **papszResult;

    sprintf( szSelect, "SELECT WKTEXT FROM MDSYS.CS_SRS WHERE SRID = %d", nId );

    if( oStatement.Execute( szSelect ) != CE_None )
        return NULL;

    papszResult = oStatement.SimpleFetchRow();
    if( CSLCount(papszResult) != 1 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Turn into a spatial reference.                                  */
/* -------------------------------------------------------------------- */
    char *pszWKT = papszResult[0];
    OGRSpatialReference *poSRS = NULL;

    poSRS = new OGRSpatialReference();
    if( poSRS->importFromWkt( &pszWKT ) != OGRERR_NONE )
    {
        delete poSRS;
        poSRS = NULL;
    }

/* -------------------------------------------------------------------- */
/*      Add to the cache.                                               */
/* -------------------------------------------------------------------- */
    panSRID = (int *) CPLRealloc(panSRID,sizeof(int) * (nKnownSRID+1) );
    papoSRS = (OGRSpatialReference **) 
        CPLRealloc(papoSRS, sizeof(void*) * (nKnownSRID + 1) );
    panSRID[nKnownSRID] = nId;
    papoSRS[nKnownSRID] = poSRS;

    return poSRS;
}

/************************************************************************/
/*                             FetchSRSId()                             */
/*                                                                      */
/*      Fetch the id corresponding to an SRS, and if not found, add     */
/*      it to the table.                                                */
/************************************************************************/

int OGROCIDataSource::FetchSRSId( OGRSpatialReference * poSRS )

{
    char		*pszWKT = NULL;
    int			nSRSId;

    if( poSRS == NULL )
        return -1;

/* -------------------------------------------------------------------- */
/*      Convert SRS into old style format (SF-SQL 1.0).                 */
/* -------------------------------------------------------------------- */
    OGRSpatialReference *poSRS2 = poSRS->Clone();
    
    poSRS2->StripCTParms();

/* -------------------------------------------------------------------- */
/*      Translate SRS to WKT.                                           */
/* -------------------------------------------------------------------- */
    if( poSRS2->exportToWkt( &pszWKT ) != OGRERR_NONE )
    {
        delete poSRS2;
        return -1;
    }
    
    delete poSRS2;
    
/* -------------------------------------------------------------------- */
/*      Try to find in the existing table.                              */
/* -------------------------------------------------------------------- */
    OGROCIStringBuf     oCmdText;
    OGROCIStatement     oCmdStatement( GetSession() );
    char                **papszResult = NULL;

    oCmdText.Append( "SELECT SRID FROM MDSYS.CS_SRS WHERE WKTEXT = '" );
    oCmdText.Append( pszWKT );
    oCmdText.Append( "'" );

    if( oCmdStatement.Execute( oCmdText.GetString() ) == CE_None )
        papszResult = oCmdStatement.SimpleFetchRow() ;

/* -------------------------------------------------------------------- */
/*      We got it!  Return it.                                          */
/* -------------------------------------------------------------------- */
    if( CSLCount(papszResult) == 1 )
    {
        CPLFree( pszWKT );
        return atoi( papszResult[0] );
    }
    
/* -------------------------------------------------------------------- */
/*      Get the current maximum srid in the srs table.                  */
/* -------------------------------------------------------------------- */
    if( oCmdStatement.Execute("SELECT MAX(SRID) FROM MDSYS.CS_SRS") == CE_None )
        papszResult = oCmdStatement.SimpleFetchRow();
    else
        papszResult = NULL;
        
    if( CSLCount(papszResult) == 1 )
        nSRSId = atoi(papszResult[0]) + 1;
    else
        nSRSId = 1;

/* -------------------------------------------------------------------- */
/*      Try adding the SRS to the SRS table.                            */
/* -------------------------------------------------------------------- */
    oCmdText.Clear();
    oCmdText.Append( "INSERT INTO MDSYS.CS_SRS (SRID, WKTEXT, CS_NAME) " );
    oCmdText.Appendf( 100, " VALUES (%d,'", nSRSId );
    oCmdText.Append( pszWKT );
    oCmdText.Append( "', '" );
    oCmdText.Append( poSRS->GetRoot()->GetChild(0)->GetValue() );
    oCmdText.Append( "' )" );

    CPLFree( pszWKT );

    if( oCmdStatement.Execute( oCmdText.GetString() ) != CE_None )
        return -1;
    else
        return nSRSId;
}

