/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRMySQLTableLayer class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.16  2006/01/31 06:17:15  hobu
 * move SRS fetching to mysqllayer.cpp
 *
 * Revision 1.15  2006/01/27 01:08:29  fwarmerdam
 * Various fixes for when the geometry columns or srid tables are
 * missing.  Avoid issue errors in this case, they are optional.
 *
 * Revision 1.14  2006/01/19 06:31:44  hobu
 * Use MBRIntersects for spatial filters.
 * Test the envelope rather than the entire geometry.
 *
 * Revision 1.13  2006/01/19 03:51:23  hobu
 * Handle spatial filters
 *
 * Revision 1.12  2006/01/16 16:08:39  hobu
 * Detect geometry column.
 * Query geometry column.
 * Detect geometry type.
 * Query geometry type.
 * Detect spatial reference.
 * Query spatial reference.
 *
 * Revision 1.11  2005/12/28 06:24:52  hobu
 * remove debugging lint
 *
 * Revision 1.10  2005/12/28 06:22:08  hobu
 * ReadTableDefinitions() wasn't setting widths properly for varchar, char, and decimal types.
 * Also, varchar handling case was never being selected.
 *
 * Revision 1.9  2005/09/21 01:00:01  fwarmerdam
 * fixup OGRFeatureDefn and OGRSpatialReference refcount handling
 *
 * Revision 1.8  2005/08/30 23:53:16  fwarmerdam
 * implement binary field support
 *
 * Revision 1.7  2005/07/25 16:08:50  fwarmerdam
 * Fixed recognision of some integer types, such as mediumint.
 *
 * Revision 1.6  2005/07/25 14:39:55  fwarmerdam
 * Added FID column related debug statement.
 *
 * Revision 1.5  2005/06/27 17:00:50  fwarmerdam
 * Fixed bug with cleaning up hResultSet in GetFeature(id).
 *
 * Revision 1.4  2005/02/24 21:54:36  fwarmerdam
 * added support for decimal fields.
 *
 * Revision 1.3  2005/02/22 12:54:27  fwarmerdam
 * use OGRLayer base spatial filter support
 *
 * Revision 1.2  2004/10/08 20:50:48  fwarmerdam
 * avoid leak in GetFeatureCount()
 *
 * Revision 1.1  2004/10/07 20:56:15  fwarmerdam
 * New
 *
 */

#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_mysql.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                         OGRMySQLTableLayer()                         */
/************************************************************************/

OGRMySQLTableLayer::OGRMySQLTableLayer( OGRMySQLDataSource *poDSIn, 
                                  const char * pszTableName,
                                  int bUpdate, int nSRSIdIn )

{
    poDS = poDSIn;

    pszQuery = NULL;
    pszWHERE = CPLStrdup( "" );
    pszQueryStatement = NULL;
    
    bUpdateAccess = bUpdate;

    iNextShapeId = 0;

    nSRSId = nSRSIdIn;

    poFeatureDefn = ReadTableDefinition( pszTableName );
    
    ResetReading();

    bLaunderColumnNames = TRUE;
}

/************************************************************************/
/*                        ~OGRMySQLTableLayer()                         */
/************************************************************************/

OGRMySQLTableLayer::~OGRMySQLTableLayer()

{
    CPLFree( pszQuery );
    CPLFree( pszWHERE );
}

/************************************************************************/
/*                        ReadTableDefinition()                         */
/*                                                                      */
/*      Build a schema from the named table.  Done by querying the      */
/*      catalog.                                                        */
/************************************************************************/

OGRFeatureDefn *OGRMySQLTableLayer::ReadTableDefinition( const char *pszTable )

{
    MYSQL_RES    *hResult;
    char         szCommand[1024];
    
/* -------------------------------------------------------------------- */
/*      Fire off commands to get back the schema of the table.          */
/* -------------------------------------------------------------------- */
    sprintf( szCommand, "DESCRIBE %s", pszTable );
    pszGeomColumnTable = CPLStrdup(pszTable);
    if( mysql_query( poDS->GetConn(), szCommand ) )
    {
        poDS->ReportError( "DESCRIBE Failed" );
        return FALSE;
    }

    hResult = mysql_store_result( poDS->GetConn() );
    if( hResult == NULL )
    {
        poDS->ReportError( "mysql_store_result() failed on DESCRIBE result." );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Parse the returned table information.                           */
/* -------------------------------------------------------------------- */
    OGRFeatureDefn *poDefn = new OGRFeatureDefn( pszTable );
    char           **papszRow;

    poDefn->Reference();

    while( (papszRow = mysql_fetch_row( hResult )) != NULL )
    {
        const char      *pszType;
        OGRFieldDefn    oField( papszRow[0], OFTString);

        pszType = papszRow[1];
        
        if( pszType == NULL )
            continue;

        if( EQUAL(pszType,"varbinary")
            || (strlen(pszType)>3 && EQUAL(pszType+strlen(pszType)-4,"blob")))
        {
            oField.SetType( OFTBinary );
        }
        else if( EQUAL(pszType,"varchar") 
                 || (strlen(pszType)>3 && EQUAL(pszType+strlen(pszType)-4,"text"))
                 || EQUAL(pszType+strlen(pszType)-4,"enum") 
                 || EQUAL(pszType+strlen(pszType)-4,"set") )
        {


            oField.SetType( OFTString );
            char ** papszTokens;

            papszTokens = CSLTokenizeString2(pszType,"(),",0);
            
            /* width is the second */
            oField.SetWidth(atoi(papszTokens[1]));

            CSLDestroy( papszTokens );
            oField.SetType( OFTString );
        }
        else if( EQUALN(pszType,"char",4)  )
        {
            oField.SetType( OFTString );
            char ** papszTokens;

            papszTokens = CSLTokenizeString2(pszType,"(),",0);
            
            /* width is the second */
            oField.SetWidth(atoi(papszTokens[1]));

            CSLDestroy( papszTokens );
            oField.SetType( OFTString );

        }
        else if( EQUALN(pszType,"varchar",6)  )
        {
            /* 
               pszType is usually in the form "varchar(15)" 
               so we'll split it up and get the width and precision
            */
            
            oField.SetType( OFTString );
            char ** papszTokens;

            papszTokens = CSLTokenizeString2(pszType,"(),",0);
            
            /* width is the second */
            oField.SetWidth(atoi(papszTokens[1]));

            CSLDestroy( papszTokens );
            oField.SetType( OFTString );
        }
        else if( EQUALN(pszType,"int",3) )
        {
            oField.SetType( OFTInteger );
        }
        else if( EQUAL(pszType,"tinyint") )
        {
            oField.SetType( OFTInteger );
        }
        else if( EQUAL(pszType,"smallint") )
        {
            oField.SetType( OFTInteger );
        }
        else if( EQUALN(pszType,"mediumint",9) )
        {
            oField.SetType( OFTInteger );
        }
        else if( EQUALN(pszType,"bigint",6) )
        {
            oField.SetType( OFTInteger );
        }
        else if( EQUALN(pszType,"decimal",7) )
        {
            /* 
               pszType is usually in the form "decimal(15,2)" 
               so we'll split it up and get the width and precision
            */
            oField.SetType( OFTReal );
            char ** papszTokens;

            papszTokens = CSLTokenizeString2(pszType,"(),",0);
            
            /* width is the second and precision is the third */
            oField.SetWidth(atoi(papszTokens[1]));
            oField.SetPrecision(atoi(papszTokens[2]));
            CSLDestroy( papszTokens );


        }
        else if( EQUAL(pszType,"float") )
        {
            oField.SetType( OFTReal );
        }
        else if( EQUAL(pszType,"double") )
        {
            oField.SetType( OFTReal );
        }
        else if( EQUAL(pszType,"decimal") )
        {
            oField.SetType( OFTReal );
        }
        else if( EQUAL(pszType, "date") ) 
        {
            oField.SetType( OFTString );
            oField.SetWidth( 10 );
        }
        else if( EQUAL(pszType, "time") ) 
        {
            oField.SetType( OFTString );
            oField.SetWidth( 10 );
        }
        else if( EQUAL(pszType, "year") ) 
        {
            oField.SetType( OFTString );
            oField.SetWidth( 10 );
        }
        else if( EQUAL(pszType, "datetime") ) 
        {
            oField.SetType( OFTString );
            oField.SetWidth( 20 );
        }
        else if( EQUAL(pszType, "geometry") ) 
        {
            pszGeomColumn = CPLStrdup(papszRow[0]);
            continue;
        }
        // Is this an integer primary key field?
        if( !bHasFid && papszRow[3] != NULL && EQUAL(papszRow[3],"PRI") 
            && oField.GetType() == OFTInteger )
        {
            bHasFid = TRUE;
            pszFIDColumn = CPLStrdup(oField.GetNameRef());
            continue;
        }

        poDefn->AddFieldDefn( &oField );
    }

    // set to none for now... if we have a geometry column it will be set layer.
    poDefn->SetGeomType( wkbNone );
    mysql_free_result( hResult );


    if( bHasFid )
        CPLDebug( "MySQL", "table %s has FID column %s.",
                  pszTable, pszFIDColumn );
    else
        CPLDebug( "MySQL", 
                  "table %s has no FID column, FIDs will not be reliable!",
                  pszTable );

    if (pszGeomColumn) 
    {
        char*        pszType=NULL;
        
        // set to unknown first
        poDefn->SetGeomType( wkbUnknown );
        
        sprintf(szCommand, "SELECT type FROM geometry_columns WHERE f_table_name='%s'",
                pszTable );
        
        hResult = NULL;
        if( !mysql_query( poDS->GetConn(), szCommand ) )
            hResult = mysql_store_result( poDS->GetConn() );

        papszRow = NULL;
        if( hResult != NULL )
            papszRow = mysql_fetch_row( hResult );

        if( papszRow != NULL && papszRow[0] != NULL )
        {
            pszType = papszRow[0];

            OGRwkbGeometryType nGeomType = wkbUnknown;

            // check only standard OGC geometry types
            if ( EQUAL(pszType, "POINT") )
                nGeomType = wkbPoint;
            else if ( EQUAL(pszType,"LINESTRING"))
                nGeomType = wkbLineString;
            else if ( EQUAL(pszType,"POLYGON"))
                nGeomType = wkbPolygon;
            else if ( EQUAL(pszType,"MULTIPOINT"))
                nGeomType = wkbMultiPoint;
            else if ( EQUAL(pszType,"MULTILINESTRING"))
                nGeomType = wkbMultiLineString;
            else if ( EQUAL(pszType,"MULTIPOLYGON"))
                nGeomType = wkbMultiPolygon;
            else if ( EQUAL(pszType,"GEOMETRYCOLLECTION"))
                nGeomType = wkbGeometryCollection;

            poDefn->SetGeomType( nGeomType );

        } 

        if( hResult != NULL )
            mysql_free_result( hResult );   //Free our query results for finding type.
    } 
    
    return poDefn;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRMySQLTableLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( !InstallFilter( poGeomIn ) )
        return;

    BuildWhere();

    ResetReading();
}



/************************************************************************/
/*                             BuildWhere()                             */
/*                                                                      */
/*      Build the WHERE statement appropriate to the current set of     */
/*      criteria (spatial and attribute queries).                       */
/************************************************************************/

void OGRMySQLTableLayer::BuildWhere()

{
    char        szWHERE[4096];
    
    CPLFree( pszWHERE );
    pszWHERE = NULL;

    szWHERE[0] = '\0';

    if( m_poFilterGeom != NULL && pszGeomColumn )
    {
        char szEnvelope[4096];
        OGREnvelope  sEnvelope;
        szEnvelope[0] = '\0';
        
        //POLYGON((MINX MINY, MAXX MINY, MAXX MAXY, MINX MAXY, MINX MINY))
        m_poFilterGeom->getEnvelope( &sEnvelope );
        
        sprintf(szEnvelope,
                "POLYGON((%.12f %.12f, %.12f %.12f, %.12f %.12f, %.12f %.12f, %.12f %.12f))",
                sEnvelope.MinX, sEnvelope.MinY,
                sEnvelope.MaxX, sEnvelope.MinY,
                sEnvelope.MaxX, sEnvelope.MaxY,
                sEnvelope.MinX, sEnvelope.MaxY,
                sEnvelope.MinX, sEnvelope.MinY);

        sprintf( szWHERE,
                 "WHERE MBRIntersects(GeomFromText('%s'), %s)",
                 szEnvelope,
                 pszGeomColumn);

    }

    if( pszQuery != NULL )
    {
        if( strlen(szWHERE) == 0 )
            sprintf( szWHERE, "WHERE %s ", pszQuery  );
        else
            sprintf( szWHERE+strlen(szWHERE), "&& %s ", pszQuery );
    }

    pszWHERE = CPLStrdup(szWHERE);
}

/************************************************************************/
/*                      BuildFullQueryStatement()                       */
/************************************************************************/

void OGRMySQLTableLayer::BuildFullQueryStatement()

{
    if( pszQueryStatement != NULL )
    {
        CPLFree( pszQueryStatement );
        pszQueryStatement = NULL;
    }

    char *pszFields = BuildFields();

    pszQueryStatement = (char *) 
        CPLMalloc(strlen(pszFields)+strlen(pszWHERE)
                  +strlen(poFeatureDefn->GetName()) + 40);
    sprintf( pszQueryStatement,
             "SELECT %s FROM %s %s", 
             pszFields, poFeatureDefn->GetName(), pszWHERE );
    
    CPLFree( pszFields );
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRMySQLTableLayer::ResetReading()

{
    BuildFullQueryStatement();

    OGRMySQLLayer::ResetReading();
}

/************************************************************************/
/*                            BuildFields()                             */
/*                                                                      */
/*      Build list of fields to fetch, performing any required          */
/*      transformations (such as on geometry).                          */
/************************************************************************/

char *OGRMySQLTableLayer::BuildFields()

{
    int         i, nSize;
    char        *pszFieldList;

    nSize = 25;
    if( pszGeomColumn )
        nSize += strlen(pszGeomColumn);

    if( bHasFid )
        nSize += strlen(pszFIDColumn);

    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
        nSize += strlen(poFeatureDefn->GetFieldDefn(i)->GetNameRef()) + 4;

    pszFieldList = (char *) CPLMalloc(nSize);
    pszFieldList[0] = '\0';

    if( bHasFid && poFeatureDefn->GetFieldIndex( pszFIDColumn ) == -1 )
        sprintf( pszFieldList, "%s", pszFIDColumn );

    if( pszGeomColumn )
    {
        if( strlen(pszFieldList) > 0 )
            strcat( pszFieldList, ", " );

		/* ------------------------------------------------------------ */
		/*      Make sure we return AsText(WKB_GEOMETRY) WKT_GEOMETRY   */
		/*      This way the column is returned column is named         */
		/*      correctly and the RecordToFeature machinery can get it  */
		/* ------------------------------------------------------------ */            
        sprintf( pszFieldList+strlen(pszFieldList), 
                 "AsBinary(%s) %s", pszGeomColumn, pszGeomColumn );
    }

    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
    {
        const char *pszName = poFeatureDefn->GetFieldDefn(i)->GetNameRef();

        if( strlen(pszFieldList) > 0 )
            strcat( pszFieldList, ", " );

        strcat( pszFieldList, pszName );
    }

    CPLAssert( (int) strlen(pszFieldList) < nSize );

    return pszFieldList;
}

/************************************************************************/
/*                         SetAttributeFilter()                         */
/************************************************************************/

OGRErr OGRMySQLTableLayer::SetAttributeFilter( const char *pszQuery )

{
    CPLFree( this->pszQuery );

    if( pszQuery == NULL || strlen(pszQuery) == 0 )
        this->pszQuery = NULL;
    else
        this->pszQuery = CPLStrdup( pszQuery );

    BuildWhere();

    ResetReading();

    return OGRERR_NONE;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRMySQLTableLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return bHasFid;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return TRUE;

    else 
        return OGRMySQLLayer::TestCapability( pszCap );
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRMySQLTableLayer::GetFeature( long nFeatureId )

{
    if( pszFIDColumn == NULL )
        return OGRMySQLLayer::GetFeature( nFeatureId );

/* -------------------------------------------------------------------- */
/*      Discard any existing resultset.                                 */
/* -------------------------------------------------------------------- */
    ResetReading();

/* -------------------------------------------------------------------- */
/*      Prepare query command that will just fetch the one record of    */
/*      interest.                                                       */
/* -------------------------------------------------------------------- */
    char        *pszFieldList = BuildFields();
    char        *pszCommand = (char *) CPLMalloc(strlen(pszFieldList)+2000);

    sprintf( pszCommand, 
             "SELECT %s FROM %s WHERE %s = %ld", 
             pszFieldList, poFeatureDefn->GetName(), pszFIDColumn, 
             nFeatureId );
    CPLFree( pszFieldList );

/* -------------------------------------------------------------------- */
/*      Issue the command.                                              */
/* -------------------------------------------------------------------- */
    if( mysql_query( poDS->GetConn(), pszCommand ) )
    {
        poDS->ReportError( pszCommand );
        return NULL;
    }
    CPLFree( pszCommand );

    hResultSet = mysql_store_result( poDS->GetConn() );
    if( hResultSet == NULL )
    {
        poDS->ReportError( "mysql_store_result() failed on query." );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Fetch the result record.                                        */
/* -------------------------------------------------------------------- */
    char **papszRow;
    unsigned long *panLengths;

    papszRow = mysql_fetch_row( hResultSet );
    if( papszRow == NULL )
        return NULL;

    panLengths = mysql_fetch_lengths( hResultSet );

/* -------------------------------------------------------------------- */
/*      Transform into a feature.                                       */
/* -------------------------------------------------------------------- */
    iNextShapeId = nFeatureId;

    OGRFeature *poFeature = RecordToFeature( papszRow, panLengths );

    iNextShapeId = 0;

/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
    mysql_free_result( hResultSet );
    hResultSet = NULL;

    return poFeature;
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/*                                                                      */
/*      If a spatial filter is in effect, we turn control over to       */
/*      the generic counter.  Otherwise we return the total count.      */
/*      Eventually we should consider implementing a more efficient     */
/*      way of counting features matching a spatial query.              */
/************************************************************************/

int OGRMySQLTableLayer::GetFeatureCount( int bForce )

{
/* -------------------------------------------------------------------- */
/*      Ensure any active long result is interrupted.                   */
/* -------------------------------------------------------------------- */
    poDS->InterruptLongResult();
    
/* -------------------------------------------------------------------- */
/*      Issue the appropriate select command.                           */
/* -------------------------------------------------------------------- */
    MYSQL_RES    *hResult;
    const char         *pszCommand;

    pszCommand = CPLSPrintf( "SELECT COUNT(*) FROM %s %s", 
                             poFeatureDefn->GetName(), pszWHERE );

    if( mysql_query( poDS->GetConn(), pszCommand ) )
    {
        poDS->ReportError( pszCommand );
        return FALSE;
    }

    hResult = mysql_store_result( poDS->GetConn() );
    if( hResult == NULL )
    {
        poDS->ReportError( "mysql_store_result() failed on SELECT COUNT(*)." );
        return FALSE;
    }
    
/* -------------------------------------------------------------------- */
/*      Capture the result.                                             */
/* -------------------------------------------------------------------- */
    char **papszRow = mysql_fetch_row( hResult );
    int nCount = 0;

    if( papszRow != NULL && papszRow[0] != NULL )
        nCount = atoi(papszRow[0]);

    mysql_free_result( hResult );
    
    return nCount;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/*                                                                      */
/*      We override this to try and fetch the table SRID from the       */
/*      geometry_columns table if the srsid is -2 (meaning we           */
/*      haven't yet even looked for it).                                */
/************************************************************************/

OGRSpatialReference *OGRMySQLTableLayer::GetSpatialRef()

{

/* 
    if( nSRSId == -2 )
    {
        MYSQL_RES    *hResult;
        char         szCommand[1024];
    
        sprintf( szCommand, 
                 "SELECT srid FROM geometry_columns "
                 "WHERE f_table_name = '%s'",
                 poFeatureDefn->GetName() );

        if( mysql_query( poDS->GetConn(), szCommand ) )
            return NULL;

        hResult = mysql_store_result( poDS->GetConn() );
        if( hResult == NULL )
            return NULL;
            
        nSRSId = -1;
        char **papszRow = mysql_fetch_row( hResult );

        if( papszRow != NULL && papszRow[0] != NULL )
        {
            nSRSId = atoi(papszRow[0]);
        }

        mysql_free_result( hResult );
    }
 */
/* -------------------------------------------------------------------- */
/*      Try to find in the existing table.                              */
/* -------------------------------------------------------------------- */

/*    MYSQL_RES    *hResult;
    char         szCommand[1024];
    char  *pszWKT = NULL;
        
    sprintf( szCommand,
             "SELECT srtext FROM spatial_ref_sys WHERE srid = %d",
             nSRSId );
    if( mysql_query( poDS->GetConn(), szCommand ) )
    {
        poDS->ReportError( "SELECT srid Failed" );
        return NULL;
    }     
    
    hResult = mysql_store_result( poDS->GetConn() );
    if( hResult == NULL )
    {
        poDS->ReportError( "mysql_store_result() failed on SELECT SRID result." );
        return FALSE;
    }   

    char **papszRow = mysql_fetch_row( hResult );

    if( papszRow != NULL && papszRow[0] != NULL )
    {
        pszWKT =papszRow[0];
    }
    
    poSRS = new OGRSpatialReference();
    if( poSRS->importFromWkt( &pszWKT ) != OGRERR_NONE )
    {
        delete poSRS;
        poSRS = NULL;
    }
    
    mysql_free_result( hResult );
    return poSRS;
    */
    FetchSRS();
    return poSRS;
    
    //return OGRMySQLLayer::GetSpatialRef();
}
