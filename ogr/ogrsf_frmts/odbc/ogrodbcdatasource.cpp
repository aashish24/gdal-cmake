/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRODBCDataSource class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.4  2004/01/05 22:23:40  warmerda
 * added ExecuteSQL implementation via OGRODBCSelectLayer
 *
 * Revision 1.3  2003/11/10 20:08:50  warmerda
 * support explicit tables list, or GetTables() fallback
 *
 * Revision 1.2  2003/10/06 19:18:30  warmerda
 * Added userid/password support
 *
 * Revision 1.1  2003/09/25 17:08:37  warmerda
 * New
 *
 */

#include "ogr_odbc.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");
/************************************************************************/
/*                         OGRODBCDataSource()                          */
/************************************************************************/

OGRODBCDataSource::OGRODBCDataSource()

{
    pszName = NULL;
    papoLayers = NULL;
    nLayers = 0;

    nKnownSRID = 0;
    panSRID = NULL;
    papoSRS = NULL;
}

/************************************************************************/
/*                         ~OGRODBCDataSource()                         */
/************************************************************************/

OGRODBCDataSource::~OGRODBCDataSource()

{
    int         i;

    CPLFree( pszName );

    for( i = 0; i < nLayers; i++ )
        delete papoLayers[i];
    
    CPLFree( papoLayers );

    for( i = 0; i < nKnownSRID; i++ )
    {
        if( papoSRS[i] != NULL && papoSRS[i]->Dereference() == 0 )
            delete papoSRS[i];
    }
    CPLFree( panSRID );
    CPLFree( papoSRS );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

int OGRODBCDataSource::Open( const char * pszNewName, int bUpdate,
                              int bTestOpen )

{
    CPLAssert( nLayers == 0 );

/* -------------------------------------------------------------------- */
/*      Strip off any comma delimeted set of tables names to access     */
/*      from the end of the string first.                               */
/* -------------------------------------------------------------------- */
    char *pszWrkName = CPLStrdup( pszNewName );
    char **papszTables = NULL;
    char *pszComma;

    while( (pszComma = strrchr( pszWrkName, ',' )) != NULL )
    {
        papszTables = CSLAddString( papszTables, pszComma + 1 );
        *pszComma = '\0';
    }

/* -------------------------------------------------------------------- */
/*      Split out userid, password and DSN.  The general form is        */
/*      user/password@dsn.  But if there are no @ characters the        */
/*      whole thing is assumed to be a DSN.                             */
/* -------------------------------------------------------------------- */
    char *pszUserid = NULL;
    char *pszPassword = NULL;
    char *pszDSN = NULL;

    if( strstr(pszWrkName+5,"@") == NULL )
    {
        pszDSN = CPLStrdup(pszWrkName+5);
    }
    else
    {
        char *pszTarget;

        pszDSN = CPLStrdup(strstr(pszWrkName+5,"@")+1);
        if( pszWrkName[5] == '/' )
        {
            pszPassword = CPLStrdup(pszWrkName + 6);
            pszTarget = strstr(pszPassword,"@");
            *pszTarget = '\0';
        }
        else
        {
            pszUserid = CPLStrdup(pszWrkName+5);
            pszTarget = strstr(pszUserid,"@");
            *pszTarget = '\0';

            pszTarget = strstr(pszUserid,"/");
            if( pszTarget != NULL )
            {
                *pszTarget = '\0';
                pszPassword = CPLStrdup(pszTarget+1);
            }
        }
    }

    CPLFree( pszWrkName );

/* -------------------------------------------------------------------- */
/*      Initialize based on the DSN.                                    */
/* -------------------------------------------------------------------- */
    CPLDebug( "ODBC", "EstablishSession(%s,%s,%s)", 
              pszDSN, pszUserid, pszPassword );

    if( !oSession.EstablishSession( pszDSN, pszUserid, pszPassword ) )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Unable to initialize ODBC connection to DSN %s,\n"
                  "%s", 
                  pszNewName+5, oSession.GetLastError() );
        CPLFree( pszDSN );
        CPLFree( pszUserid );
        CPLFree( pszPassword );
        return FALSE;
    }

    CPLFree( pszDSN );
    CPLFree( pszUserid );
    CPLFree( pszPassword );

    pszName = CPLStrdup( pszNewName );
    
    bDSUpdate = bUpdate;

/* -------------------------------------------------------------------- */
/*      If we have an explicit list of requested tables, use them       */
/*      (non-spatial).                                                  */
/* -------------------------------------------------------------------- */
    if( papszTables != NULL )
    {
        for( int iTable = 0; papszTables[iTable] != NULL; iTable++ )
            OpenTable( papszTables[iTable], NULL, bUpdate );
        return TRUE;
    }

/* -------------------------------------------------------------------- */
/*      If we have a GEOMETRY_COLUMN tables, initialize on the basis    */
/*      of that.                                                        */
/* -------------------------------------------------------------------- */
    CPLODBCStatement oStmt( &oSession );

    oStmt.Append( "SELECT f_table_name, f_geometry_column, geometry_type"
                  " FROM geometry_columns" );
    if( oStmt.ExecuteSQL() )
    {
        while( oStmt.Fetch() )
        {
            OpenTable( oStmt.GetColData(0), oStmt.GetColData(1), bUpdate );
        }

        return TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Otherwise our final resort is to return all tables as           */
/*      non-spatial tables.                                             */
/* -------------------------------------------------------------------- */
    CPLODBCStatement oTableList( &oSession );
    
    if( oTableList.GetTables() )
    {
        while( oTableList.Fetch() )
        {
            OpenTable( oTableList.GetColData(2), NULL, bUpdate );
        }
    }
    
    return TRUE;
}

/************************************************************************/
/*                             OpenTable()                              */
/************************************************************************/

int OGRODBCDataSource::OpenTable( const char *pszNewName, 
                                  const char *pszGeomCol,
                                  int bUpdate )

{
/* -------------------------------------------------------------------- */
/*      Create the layer object.                                        */
/* -------------------------------------------------------------------- */
    OGRODBCTableLayer  *poLayer;

    poLayer = new OGRODBCTableLayer( this );

    if( poLayer->Initialize( pszNewName, pszGeomCol ) )
    {
        delete poLayer;
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Add layer to data source layer list.                            */
/* -------------------------------------------------------------------- */
    papoLayers = (OGRODBCLayer **)
        CPLRealloc( papoLayers,  sizeof(OGRODBCLayer *) * (nLayers+1) );
    papoLayers[nLayers++] = poLayer;
    
    return TRUE;
}

/************************************************************************/
/*                            DeleteLayer()                             */
/************************************************************************/

void OGRODBCDataSource::DeleteLayer( const char *pszLayerName )

{
#ifdef notdef
    int iLayer;

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
    CPLDebug( "OGR_PG", "DeleteLayer(%s)", pszLayerName );

    delete papoLayers[iLayer];
    memmove( papoLayers + iLayer, papoLayers + iLayer + 1, 
             sizeof(void *) * (nLayers - iLayer - 1) );
    nLayers--;

/* -------------------------------------------------------------------- */
/*      Remove from the database.                                       */
/* -------------------------------------------------------------------- */
    PGresult            *hResult;
    char                szCommand[1024];

    hResult = PQexec(hPGConn, "BEGIN");
    PQclear( hResult );

    if( bHavePostGIS )
    {
        sprintf( szCommand, 
                 "SELECT DropGeometryColumn('%s','%s','wkb_geometry')",
                 pszDBName, pszLayerName );

        hResult = PQexec( hPGConn, szCommand );
        PQclear( hResult );
    }

    sprintf( szCommand, "DROP TABLE %s", pszLayerName );
    hResult = PQexec( hPGConn, szCommand );
    PQclear( hResult );
    
    sprintf( szCommand, "DROP SEQUENCE %s_ogc_fid_seq", pszLayerName );
    hResult = PQexec( hPGConn, szCommand );
    PQclear( hResult );
    
    hResult = PQexec(hPGConn, "COMMIT");
    PQclear( hResult );
#endif
}

/************************************************************************/
/*                            CreateLayer()                             */
/************************************************************************/

#ifdef notdef
OGRLayer *
OGRODBCDataSource::CreateLayer( const char * pszLayerName,
                              OGRSpatialReference *poSRS,
                              OGRwkbGeometryType eType,
                              char ** papszOptions )

{
    PGresult            *hResult;
    char                szCommand[1024];
    const char          *pszGeomType;

/* -------------------------------------------------------------------- */
/*      Do we already have this layer?  If so, should we blow it        */
/*      away?                                                           */
/* -------------------------------------------------------------------- */
    int iLayer;

    for( iLayer = 0; iLayer < nLayers; iLayer++ )
    {
        if( EQUAL(pszLayerName,papoLayers[iLayer]->GetLayerDefn()->GetName()) )
        {
            if( CSLFetchNameValue( papszOptions, "OVERWRITE" ) != NULL
                && !EQUAL(CSLFetchNameValue(papszOptions,"OVERWRITE"),"NO") )
            {
                DeleteLayer( pszLayerName );
            }
            else
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "Layer %s already exists, CreateLayer failed.\n"
                          "Use the layer creation option OVERWRITE=YES to "
                          "replace it.",
                          pszLayerName );
                return NULL;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Handle the GEOM_TYPE option.                                    */
/* -------------------------------------------------------------------- */
    pszGeomType = CSLFetchNameValue( papszOptions, "GEOM_TYPE" );
    if( pszGeomType == NULL )
    {
        if( bHavePostGIS )
            pszGeomType = "geometry";
        else
            pszGeomType = "bytea";
    }

    if( bHavePostGIS && !EQUAL(pszGeomType,"geometry") )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Can't override GEOM_TYPE in PostGIS enabled databases.\n"
                  "Creation of layer %s with GEOM_TYPE %s has failed.",
                  pszLayerName, pszGeomType );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Try to get the SRS Id of this spatial reference system,         */
/*      adding tot the srs table if needed.                             */
/* -------------------------------------------------------------------- */
    int nSRSId = -1;

    if( poSRS != NULL )
        nSRSId = FetchSRSId( poSRS );

/* -------------------------------------------------------------------- */
/*      Create a basic table with the FID.  Also include the            */
/*      geometry if this is not a PostGIS enabled table.                */
/* -------------------------------------------------------------------- */
    hResult = PQexec(hPGConn, "BEGIN");
    PQclear( hResult );

    if( !bHavePostGIS )
        sprintf( szCommand, 
                 "CREATE TABLE \"%s\" ( "
                 "   OGC_FID SERIAL, "
                 "   WKB_GEOMETRY %s )",
                 pszLayerName, pszGeomType );
    else
        sprintf( szCommand, 
                 "CREATE TABLE \"%s\" ( OGC_FID SERIAL )", 
                 pszLayerName );

    CPLDebug( "OGR_PG", "PQexec(%s)", szCommand );
    hResult = PQexec(hPGConn, szCommand);
    if( PQresultStatus(hResult) != PGRES_COMMAND_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s\n%s", szCommand, PQerrorMessage(hPGConn) );

        PQclear( hResult );
        hResult = PQexec( hPGConn, "ROLLBACK" );
        PQclear( hResult );

        return NULL;
    }
    
    PQclear( hResult );

/* -------------------------------------------------------------------- */
/*      Eventually we should be adding this table to a table of         */
/*      "geometric layers", capturing the WKT projection, and           */
/*      perhaps some other housekeeping.                                */
/* -------------------------------------------------------------------- */
    if( bHavePostGIS )
    {
        const char *pszGeometryType;

        switch( wkbFlatten(eType) )
        {
          case wkbPoint:
            pszGeometryType = "POINT";
            break;

          case wkbLineString:
            pszGeometryType = "LINESTRING";
            break;

          case wkbPolygon:
            pszGeometryType = "POLYGON";
            break;

          case wkbMultiPoint:
            pszGeometryType = "MULTIPOINT";
            break;

          case wkbMultiLineString:
            pszGeometryType = "MULTILINESTRING";
            break;

          case wkbMultiPolygon:
            pszGeometryType = "MULTIPOLYGON";
            break;

          case wkbGeometryCollection:
            pszGeometryType = "GEOMETRYCOLLECTION";
            break;

          default:
            pszGeometryType = "GEOMETRY";
            break;

        }

        sprintf( szCommand, 
                 "SELECT AddGeometryColumn('%s','%s','wkb_geometry',%d,'%s',%d)",
                 pszDBName, pszLayerName, nSRSId, pszGeometryType, 3 );

        CPLDebug( "OGR_PG", "PQexec(%s)", szCommand );
        hResult = PQexec(hPGConn, szCommand);

        if( !hResult 
            || PQresultStatus(hResult) != PGRES_TUPLES_OK )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "AddGeometryColumn failed for layer %s, layer creation has failed.",
                      pszLayerName );
            
            PQclear( hResult );

            hResult = PQexec(hPGConn, "ROLLBACK");
            PQclear( hResult );

            return NULL;
        }
    }

/* -------------------------------------------------------------------- */
/*      Complete, and commit the transaction.                           */
/* -------------------------------------------------------------------- */
    hResult = PQexec(hPGConn, "COMMIT");
    PQclear( hResult );

/* -------------------------------------------------------------------- */
/*      Create the layer object.                                        */
/* -------------------------------------------------------------------- */
    OGRODBCTableLayer     *poLayer;

    poLayer = new OGRODBCTableLayer( this, pszLayerName, TRUE, nSRSId );

    poLayer->SetLaunderFlag( CSLFetchBoolean(papszOptions,"LAUNDER",FALSE) );
    poLayer->SetPrecisionFlag( CSLFetchBoolean(papszOptions,"PRECISION",TRUE));

/* -------------------------------------------------------------------- */
/*      Add layer to data source layer list.                            */
/* -------------------------------------------------------------------- */
    papoLayers = (OGRODBCLayer **)
        CPLRealloc( papoLayers,  sizeof(OGRODBCLayer *) * (nLayers+1) );
    
    papoLayers[nLayers++] = poLayer;

    return poLayer;
}
#endif

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRODBCDataSource::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,ODsCCreateLayer) )
        return TRUE;
    else
        return FALSE;
}

/************************************************************************/
/*                              GetLayer()                              */
/************************************************************************/

OGRLayer *OGRODBCDataSource::GetLayer( int iLayer )

{
    if( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];
}
/************************************************************************/
/*                             ExecuteSQL()                             */
/************************************************************************/

OGRLayer * OGRODBCDataSource::ExecuteSQL( const char *pszSQLCommand,
                                          OGRGeometry *poSpatialFilter,
                                          const char *pszDialect )

{
    CPLODBCStatement *poStmt = new CPLODBCStatement( &oSession );

    poStmt->Append( pszSQLCommand );
    if( !poStmt->ExecuteSQL() )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s", oSession.GetLastError() );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Are there result columns for this statement?                    */
/* -------------------------------------------------------------------- */
    if( poStmt->GetColCount() == 0 )
    {
        delete poStmt;
        CPLErrorReset();
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create a results layer.  It will take ownership of the          */
/*      statement.                                                      */
/* -------------------------------------------------------------------- */
    OGRODBCSelectLayer *poLayer = NULL;
        
    poLayer = new OGRODBCSelectLayer( this, poStmt );

    if( poSpatialFilter != NULL )
        poLayer->SetSpatialFilter( poSpatialFilter );
    
    return poLayer;
}

/************************************************************************/
/*                          ReleaseResultSet()                          */
/************************************************************************/

void OGRODBCDataSource::ReleaseResultSet( OGRLayer * poLayer )

{
    delete poLayer;
}
