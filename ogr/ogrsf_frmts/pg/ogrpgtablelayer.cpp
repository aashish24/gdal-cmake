/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRPGTableLayer class, access to an existing table.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
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
 * Revision 1.13  2003/05/21 03:59:42  warmerda
 * expand tabs
 *
 * Revision 1.12  2003/02/01 07:55:48  warmerda
 * avoid dependence on libpq-fs.h
 *
 * Revision 1.11  2003/01/08 22:07:14  warmerda
 * Added support for integer and real list field types
 *
 * Revision 1.10  2002/12/12 14:29:28  warmerda
 * fixed bug with creating features with no geometry in PostGIS
 *
 * Revision 1.9  2002/10/20 03:45:53  warmerda
 * quote table name in feature insert, and feature count commands
 *
 * Revision 1.8  2002/10/09 18:30:10  warmerda
 * substantial upgrade to type handling, and preservations of width/precision
 *
 * Revision 1.7  2002/10/09 14:16:32  warmerda
 * changed the way that character field widths are extracted from catalog
 *
 * Revision 1.6  2002/10/04 14:03:09  warmerda
 * added column name laundering support
 *
 * Revision 1.5  2002/09/19 17:40:42  warmerda
 * Make initial ResetReading() call to set full query expression in constructor.
 *
 * Revision 1.4  2002/05/09 17:21:54  warmerda
 * Don't add trailing command if no fields to be inserted.
 *
 * Revision 1.3  2002/05/09 17:09:08  warmerda
 * Ensure nSRSId is set before creating any features.
 *
 * Revision 1.2  2002/05/09 16:48:08  warmerda
 * upgrade to quote table and field names
 *
 * Revision 1.1  2002/05/09 16:03:46  warmerda
 * New
 *
 */

#include "cpl_conv.h"
#include "ogr_pg.h"

CPL_CVSID("$Id$");

#define CURSOR_PAGE     1

/************************************************************************/
/*                          OGRPGTableLayer()                           */
/************************************************************************/

OGRPGTableLayer::OGRPGTableLayer( OGRPGDataSource *poDSIn, 
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
/*                          ~OGRPGTableLayer()                          */
/************************************************************************/

OGRPGTableLayer::~OGRPGTableLayer()

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

OGRFeatureDefn *OGRPGTableLayer::ReadTableDefinition( const char * pszTable )

{
    PGresult            *hResult;
    char                szCommand[1024];
    PGconn              *hPGConn = poDS->GetPGConn();
    
/* -------------------------------------------------------------------- */
/*      Fire off commands to get back the schema of the table.          */
/* -------------------------------------------------------------------- */
    poDS->FlushSoftTransaction();
    hResult = PQexec(hPGConn, "BEGIN");

    if( hResult && PQresultStatus(hResult) == PGRES_COMMAND_OK )
    {
        PQclear( hResult );
        sprintf( szCommand, 
                 "DECLARE mycursor CURSOR for "
                 "SELECT a.attname, t.typname, a.attlen,"
                 "       format_type(a.atttypid,a.atttypmod) "
                 "FROM pg_class c, pg_attribute a, pg_type t "
                 "WHERE c.relname = '%s' "
                 "AND a.attnum > 0 AND a.attrelid = c.oid "
                 "AND a.atttypid = t.oid", 
                 pszTable );

        hResult = PQexec(hPGConn, szCommand );
    }

    if( hResult && PQresultStatus(hResult) == PGRES_COMMAND_OK )
    {
        PQclear( hResult );
        hResult = PQexec(hPGConn, "FETCH ALL in mycursor" );
    }

    if( !hResult || PQresultStatus(hResult) != PGRES_TUPLES_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s", PQerrorMessage(hPGConn) );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Parse the returned table information.                           */
/* -------------------------------------------------------------------- */
    OGRFeatureDefn *poDefn = new OGRFeatureDefn( pszTable );
    int            iRecord;

    for( iRecord = 0; iRecord < PQntuples(hResult); iRecord++ )
    {
        const char      *pszType, *pszFormatType;
        OGRFieldDefn    oField( PQgetvalue( hResult, iRecord, 0 ), OFTString);

        pszType = PQgetvalue(hResult, iRecord, 1 );
        pszFormatType = PQgetvalue(hResult,iRecord,3);

        if( EQUAL(oField.GetNameRef(),"ogc_fid") )
        {
            bHasFid = TRUE;
            continue;
        }
        else if( EQUAL(pszType,"geometry") )
        {
            bHasPostGISGeometry = TRUE;
            pszGeomColumn = CPLStrdup( oField.GetNameRef());
            continue;
        }
        else if( EQUAL(oField.GetNameRef(),"WKB_GEOMETRY") )
        {
            bHasWkb = TRUE;
            if( EQUAL(pszType,"OID") )
                bWkbAsOid = TRUE;
            continue;
        }

        if( EQUAL(pszType,"varchar") || EQUAL(pszType,"text") )
        {
            oField.SetType( OFTString );
        }
        else if( EQUAL(pszType,"bpchar") )
        {
            int nWidth;

            nWidth = atoi(PQgetvalue(hResult,iRecord,2));
            if( nWidth == -1 )
            {
                if( EQUALN(pszFormatType,"character(",10) )
                    nWidth = atoi(pszFormatType+10);
                else
                    nWidth = 0;
            }
            oField.SetType( OFTString );
            oField.SetWidth( nWidth );
        }
        else if( EQUAL(pszType,"numeric") )
        {
            const char *pszFormatName = PQgetvalue(hResult,iRecord,3);
            const char *pszPrecision = strstr(pszFormatName,",");
            int    nWidth, nPrecision = 0;

            nWidth = atoi(pszFormatName + 8);
            if( pszPrecision != NULL )
                nPrecision = atoi(pszPrecision+1);
            
            if( nPrecision == 0 )
                oField.SetType( OFTInteger );
            else
                oField.SetType( OFTReal );

            oField.SetWidth( nWidth );
            oField.SetPrecision( nPrecision );
        }
        else if( EQUAL(pszFormatType,"integer[]") )
        {
            oField.SetType( OFTIntegerList );
        }
        else if( EQUAL(pszFormatType,"float[]") )
        {
            oField.SetType( OFTRealList );
        }
        else if( EQUAL(pszType,"int2") )
        {
            oField.SetType( OFTInteger );
            oField.SetWidth( 5 );
        }
        else if( EQUALN(pszType,"int",3) )
        {
            oField.SetType( OFTInteger );
        }
        else if( EQUALN(pszType,"float",5) || EQUALN(pszType,"double",6) )
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
            oField.SetWidth( 8 );
        }
        
        poDefn->AddFieldDefn( &oField );
    }

    PQclear( hResult );

    hResult = PQexec(hPGConn, "CLOSE mycursor");
    PQclear( hResult );

    hResult = PQexec(hPGConn, "COMMIT");
    PQclear( hResult );

    return poDefn;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRPGTableLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( poFilterGeom != NULL )
    {
        delete poFilterGeom;
        poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
        poFilterGeom = poGeomIn->clone();

    BuildWhere();

    ResetReading();
}

/************************************************************************/
/*                             BuildWhere()                             */
/*                                                                      */
/*      Build the WHERE statement appropriate to the current set of     */
/*      criteria (spatial and attribute queries).                       */
/************************************************************************/

void OGRPGTableLayer::BuildWhere()

{
    char        szWHERE[4096];

    CPLFree( pszWHERE );
    pszWHERE = NULL;

    szWHERE[0] = '\0';

    if( poFilterGeom != NULL && bHasPostGISGeometry )
    {
        OGREnvelope  sEnvelope;

        poFilterGeom->getEnvelope( &sEnvelope );
        sprintf( szWHERE, 
                 "WHERE %s && GeometryFromText('BOX3D(%.12f %.12f, %.12f %.12f)'::box3d,%d) ",
                 pszGeomColumn, 
                 sEnvelope.MinX, sEnvelope.MinY, 
                 sEnvelope.MaxX, sEnvelope.MaxY,
                 nSRSId );
    }

    if( pszQuery != NULL )
    {
        if( strlen(szWHERE) == 0 )
            sprintf( szWHERE, "WHERE %s ", pszQuery  );
        else
            sprintf( szWHERE+strlen(szWHERE), "AND %s ", pszQuery );
    }

    pszWHERE = CPLStrdup(szWHERE);
}

/************************************************************************/
/*                      BuildFullQueryStatement()                       */
/************************************************************************/

void OGRPGTableLayer::BuildFullQueryStatement()

{
    if( pszQueryStatement != NULL )
    {
        CPLFree( pszQueryStatement );
        pszQueryStatement = NULL;
    }

    char szCommand[6000];
    char *pszFields = BuildFields();

    sprintf( szCommand, 
             "SELECT %s FROM \"%s\" %s", 
             pszFields, poFeatureDefn->GetName(), pszWHERE );
    
    CPLFree( pszFields );

    pszQueryStatement = CPLStrdup( szCommand );
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRPGTableLayer::ResetReading()

{
    BuildFullQueryStatement();

    OGRPGLayer::ResetReading();
}

/************************************************************************/
/*                            BuildFields()                             */
/*                                                                      */
/*      Build list of fields to fetch, performing any required          */
/*      transformations (such as on geometry).                          */
/************************************************************************/

char *OGRPGTableLayer::BuildFields()

{
    int         i, nSize;
    char        *pszFieldList;

    nSize = 25;
    if( pszGeomColumn )
        nSize += strlen(pszGeomColumn);

    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
        nSize += strlen(poFeatureDefn->GetFieldDefn(i)->GetNameRef()) + 4;

    pszFieldList = (char *) CPLMalloc(nSize);

    if( pszGeomColumn )
    {
        if( bHasPostGISGeometry )
        {
            sprintf( pszFieldList, "AsText(\"%s\")", pszGeomColumn );
        }
        else
        {
            sprintf( pszFieldList, "\"%s\"", pszGeomColumn );
        }
    }
    else
        pszFieldList[0] = '\0';

    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
    {
        const char *pszName = poFeatureDefn->GetFieldDefn(i)->GetNameRef();

        if( strlen(pszFieldList) > 0 )
            strcat( pszFieldList, ", " );

        strcat( pszFieldList, "\"" );
        strcat( pszFieldList, pszName );
        strcat( pszFieldList, "\"" );
    }

    CPLAssert( (int) strlen(pszFieldList) < nSize );

    return pszFieldList;
}

/************************************************************************/
/*                         SetAttributeFilter()                         */
/************************************************************************/

OGRErr OGRPGTableLayer::SetAttributeFilter( const char *pszQuery )

{
    CPLFree( this->pszQuery );
    this->pszQuery = CPLStrdup( pszQuery );

    BuildWhere();

    ResetReading();

    return OGRERR_NONE;
}


/************************************************************************/
/*                             SetFeature()                             */
/************************************************************************/

OGRErr OGRPGTableLayer::SetFeature( OGRFeature *poFeature )

{
    return OGRERR_FAILURE;
}

/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/

OGRErr OGRPGTableLayer::CreateFeature( OGRFeature *poFeature )

{
    PGconn              *hPGConn = poDS->GetPGConn();
    PGresult            *hResult;
    char                *pszCommand;
    int                 i, bNeedComma;
    unsigned int        nCommandBufSize;;
    OGRErr              eErr;

    eErr = poDS->SoftStartTransaction();
    if( eErr != OGRERR_NONE )
        return eErr;

    nCommandBufSize = 40000;
    pszCommand = (char *) CPLMalloc(nCommandBufSize);

/* -------------------------------------------------------------------- */
/*      Form the INSERT command.                                        */
/* -------------------------------------------------------------------- */
    sprintf( pszCommand, "INSERT INTO \"%s\" (", poFeatureDefn->GetName() );

    if( bHasWkb && poFeature->GetGeometryRef() != NULL )
        strcat( pszCommand, "WKB_GEOMETRY " );
    
    if( bHasPostGISGeometry && poFeature->GetGeometryRef() != NULL )
    {
        strcat( pszCommand, pszGeomColumn );
        strcat( pszCommand, " " );
    }
    
    bNeedComma = poFeature->GetGeometryRef() != NULL;
    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
    {
        if( !poFeature->IsFieldSet( i ) )
            continue;

        if( !bNeedComma )
            bNeedComma = TRUE;
        else
            strcat( pszCommand, ", " );

        sprintf( pszCommand + strlen(pszCommand), "\"%s\"",
                 poFeatureDefn->GetFieldDefn(i)->GetNameRef() );
    }

    strcat( pszCommand, ") VALUES (" );

    /* Set the geometry */
    bNeedComma = poFeature->GetGeometryRef() != NULL;
    if( bHasPostGISGeometry && poFeature->GetGeometryRef() != NULL)
    {
        char    *pszWKT = NULL;

        // Do we need to force nSRSId to be set?
        if( nSRSId == -2 )
            GetSpatialRef();

        if( poFeature->GetGeometryRef() != NULL )
            poFeature->GetGeometryRef()->exportToWkt( &pszWKT );
        
        if( pszWKT != NULL 
            && strlen(pszCommand) + strlen(pszWKT) > nCommandBufSize - 10000 )
        {
            nCommandBufSize = strlen(pszCommand) + strlen(pszWKT) + 10000;
            pszCommand = (char *) CPLRealloc(pszCommand, nCommandBufSize );
        }

        if( pszWKT != NULL )
        {
            sprintf( pszCommand + strlen(pszCommand), 
                     "GeometryFromText('%s'::TEXT,%d) ", pszWKT, nSRSId );
            OGRFree( pszWKT );
        }
        else
            strcat( pszCommand, "''" );
    }
    else if( bHasWkb && !bWkbAsOid && poFeature->GetGeometryRef() != NULL )
    {
        char    *pszBytea = GeometryToBYTEA( poFeature->GetGeometryRef() );

        if( strlen(pszCommand) + strlen(pszBytea) > nCommandBufSize - 10000 )
        {
            nCommandBufSize = strlen(pszCommand) + strlen(pszBytea) + 10000;
            pszCommand = (char *) CPLRealloc(pszCommand, nCommandBufSize );
        }

        if( pszBytea != NULL )
        {
            sprintf( pszCommand + strlen(pszCommand), 
                     "'%s'", pszBytea );
            CPLFree( pszBytea );
        }
        else
            strcat( pszCommand, "''" );
    }
    else if( bHasWkb && bWkbAsOid && poFeature->GetGeometryRef() != NULL )
    {
        Oid     oid = GeometryToOID( poFeature->GetGeometryRef() );

        if( oid != 0 )
        {
            sprintf( pszCommand + strlen(pszCommand), 
                     "'%d' ", oid );
        }
        else
            strcat( pszCommand, "''" );
    }

    /* Set the other fields */
    int nOffset = strlen(pszCommand);

    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
    {
        const char *pszStrValue = poFeature->GetFieldAsString(i);
        char *pszNeedToFree = NULL;

        if( !poFeature->IsFieldSet( i ) )
            continue;

        if( bNeedComma )
            strcat( pszCommand+nOffset, ", " );
        else
            bNeedComma = TRUE;

        // We need special formatting for integer list values.
        if( poFeatureDefn->GetFieldDefn(i)->GetType() == OFTIntegerList )
        {
            int nCount, nOff = 0, j;
            const int *panItems = poFeature->GetFieldAsIntegerList(i,&nCount);
            
            pszNeedToFree = (char *) CPLMalloc(nCount * 13 + 10);
            strcpy( pszNeedToFree, "{" );
            for( j = 0; j < nCount; j++ )
            {
                if( j != 0 )
                    strcat( pszNeedToFree+nOff, "," );

                nOff += strlen(pszNeedToFree+nOff);
                sprintf( pszNeedToFree+nOff, "%d", panItems[j] );
            }
            strcat( pszNeedToFree+nOff, "}" );
            pszStrValue = pszNeedToFree;
        }

        // We need special formatting for real list values.
        if( poFeatureDefn->GetFieldDefn(i)->GetType() == OFTRealList )
        {
            int nCount, nOff = 0, j;
            const double *padfItems =poFeature->GetFieldAsDoubleList(i,&nCount);
            
            pszNeedToFree = (char *) CPLMalloc(nCount * 40 + 10);
            strcpy( pszNeedToFree, "{" );
            for( j = 0; j < nCount; j++ )
            {
                if( j != 0 )
                    strcat( pszNeedToFree+nOff, "," );

                nOff += strlen(pszNeedToFree+nOff);
                sprintf( pszNeedToFree+nOff, "%.16g", padfItems[j] );
            }
            strcat( pszNeedToFree+nOff, "}" );
            pszStrValue = pszNeedToFree;
        }

        // Grow the command buffer?
        if( strlen(pszStrValue) + strlen(pszCommand+nOffset) + nOffset 
            > nCommandBufSize-50 )
        {
            nCommandBufSize = strlen(pszCommand) + strlen(pszStrValue) + 10000;
            pszCommand = (char *) CPLRealloc(pszCommand, nCommandBufSize );
        }

        if( poFeatureDefn->GetFieldDefn(i)->GetType() != OFTInteger
                 && poFeatureDefn->GetFieldDefn(i)->GetType() != OFTReal )
        {
            int         iChar;

            /* We need to quote and escape string fields. */
            strcat( pszCommand+nOffset, "'" );

            nOffset += strlen(pszCommand+nOffset);
            
            for( iChar = 0; pszStrValue[iChar] != '\0'; iChar++ )
            {
                if( pszStrValue[iChar] == '\\' 
                    || pszStrValue[iChar] == '\'' )
                {
                    pszCommand[nOffset++] = '\\';
                    pszCommand[nOffset++] = pszStrValue[iChar];
                }
                else
                    pszCommand[nOffset++] = pszStrValue[iChar];
            }
            pszCommand[nOffset] = '\0';
            
            strcat( pszCommand+nOffset, "'" );
        }
        else
        {
            strcat( pszCommand+nOffset, pszStrValue );
        }

        if( pszNeedToFree )
            CPLFree( pszNeedToFree );
    }

    strcat( pszCommand+nOffset, ")" );

/* -------------------------------------------------------------------- */
/*      Execute the insert.                                             */
/* -------------------------------------------------------------------- */
    hResult = PQexec(hPGConn, pszCommand);
    if( PQresultStatus(hResult) != PGRES_COMMAND_OK )
    {
        CPLDebug( "OGR_PG", "PQexec(%s)\n", pszCommand );
        CPLFree( pszCommand );

        CPLError( CE_Failure, CPLE_AppDefined, 
                  "INSERT command for new feature failed.\n%s", 
                  PQerrorMessage(hPGConn) );

        PQclear( hResult );
        
        poDS->SoftRollback();

        return OGRERR_FAILURE;
    }
    CPLFree( pszCommand );

#ifdef notdef
    /* Should we use this oid to get back the FID and assign back to the
       feature?  I think we are supposed to. */
    Oid nNewOID = PQoidValue( hResult );
    printf( "nNewOID = %d\n", (int) nNewOID );
#endif
    PQclear( hResult );

    return poDS->SoftCommit();
}


/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRPGTableLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return bUpdateAccess;

    else if( EQUAL(pszCap,OLCCreateField) )
        return bUpdateAccess;

    else 
        return OGRPGLayer::TestCapability( pszCap );
}

/************************************************************************/
/*                            CreateField()                             */
/************************************************************************/

OGRErr OGRPGTableLayer::CreateField( OGRFieldDefn *poFieldIn, int bApproxOK )

{
    PGconn              *hPGConn = poDS->GetPGConn();
    PGresult            *hResult;
    char                szCommand[1024];
    char                szFieldType[256];
    OGRFieldDefn        oField( poFieldIn );

/* -------------------------------------------------------------------- */
/*      Do we want to "launder" the column names into Postgres          */
/*      friendly format?                                                */
/* -------------------------------------------------------------------- */
    if( bLaunderColumnNames )
    {
        char    *pszSafeName = CPLStrdup( oField.GetNameRef() );
        int     i;

        for( i = 0; pszSafeName[i] != '\0'; i++ )
        {
            pszSafeName[i] = tolower( pszSafeName[i] );
            if( pszSafeName[i] == '-' || pszSafeName[i] == '#' )
                pszSafeName[i] = '_';
        }
        oField.SetName( pszSafeName );
        CPLFree( pszSafeName );
    }
    
/* -------------------------------------------------------------------- */
/*      Work out the PostgreSQL type.                                   */
/* -------------------------------------------------------------------- */
    if( oField.GetType() == OFTInteger )
    {
        if( oField.GetWidth() > 0 && bPreservePrecision )
            sprintf( szFieldType, "NUMERIC(%d,0)", oField.GetWidth() );
        else
            strcpy( szFieldType, "INTEGER" );
    }
    else if( oField.GetType() == OFTReal )
    {
        if( oField.GetWidth() > 0 && oField.GetPrecision() > 0 
            && bPreservePrecision )
            sprintf( szFieldType, "NUMERIC(%d,%d)", 
                     oField.GetWidth(), oField.GetPrecision() );
        else
            strcpy( szFieldType, "FLOAT8" );
    }
    else if( oField.GetType() == OFTString )
    {
        if( oField.GetWidth() == 0 || !bPreservePrecision )
            strcpy( szFieldType, "VARCHAR" );
        else
            sprintf( szFieldType, "CHAR(%d)", oField.GetWidth() );
    }
    else if( oField.GetType() == OFTIntegerList )
    {
        strcpy( szFieldType, "INTEGER[]" );
    }
    else if( oField.GetType() == OFTRealList )
    {
        strcpy( szFieldType, "FLOAT8[]" );
    }
    else if( bApproxOK )
    {
        CPLError( CE_Warning, CPLE_NotSupported,
                  "Can't create field %s with type %s on PostgreSQL layers.  Creating as VARCHAR.",
                  oField.GetNameRef(),
                  OGRFieldDefn::GetFieldTypeName(oField.GetType()) );
        strcpy( szFieldType, "VARCHAR" );
    }
    else
    {
        CPLError( CE_Failure, CPLE_NotSupported,
                  "Can't create field %s with type %s on PostgreSQL layers.",
                  oField.GetNameRef(),
                  OGRFieldDefn::GetFieldTypeName(oField.GetType()) );

        return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Create the new field.                                           */
/* -------------------------------------------------------------------- */
    poDS->FlushSoftTransaction();
    hResult = PQexec(hPGConn, "BEGIN");
    PQclear( hResult );

    sprintf( szCommand, "ALTER TABLE \"%s\" ADD COLUMN \"%s\" %s", 
             poFeatureDefn->GetName(), oField.GetNameRef(), szFieldType );
    hResult = PQexec(hPGConn, szCommand);
    if( PQresultStatus(hResult) != PGRES_COMMAND_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s\n%s", szCommand, PQerrorMessage(hPGConn) );

        PQclear( hResult );
        hResult = PQexec( hPGConn, "ROLLBACK" );
        PQclear( hResult );

        return OGRERR_FAILURE;
    }

    PQclear( hResult );

    hResult = PQexec(hPGConn, "COMMIT");
    PQclear( hResult );

    poFeatureDefn->AddFieldDefn( &oField );

    return OGRERR_NONE;
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/*                                                                      */
/*      If a spatial filter is in effect, we turn control over to       */
/*      the generic counter.  Otherwise we return the total count.      */
/*      Eventually we should consider implementing a more efficient     */
/*      way of counting features matching a spatial query.              */
/************************************************************************/

int OGRPGTableLayer::GetFeatureCount( int bForce )

{
/* -------------------------------------------------------------------- */
/*      Use a more brute force mechanism if we have a spatial query     */
/*      in play.                                                        */
/* -------------------------------------------------------------------- */
    if( poFilterGeom != NULL && !bHasPostGISGeometry )
        return OGRPGLayer::GetFeatureCount( bForce );

/* -------------------------------------------------------------------- */
/*      In theory it might be wise to cache this result, but it         */
/*      won't be trivial to work out the lifetime of the value.         */
/*      After all someone else could be adding records from another     */
/*      application when working against a database.                    */
/* -------------------------------------------------------------------- */
    PGconn              *hPGConn = poDS->GetPGConn();
    PGresult            *hResult;
    char                szCommand[4096];
    int                 nCount = 0;

    poDS->FlushSoftTransaction();
    hResult = PQexec(hPGConn, "BEGIN");
    PQclear( hResult );

    sprintf( szCommand, 
             "DECLARE countCursor CURSOR for "
             "SELECT count(*) FROM \"%s\" "
             "%s",
             poFeatureDefn->GetName(), pszWHERE );

    CPLDebug( "OGR_PG", "PQexec(%s)\n", 
              szCommand );

    hResult = PQexec(hPGConn, szCommand);
    PQclear( hResult );

    hResult = PQexec(hPGConn, "FETCH ALL in countCursor");
    if( hResult != NULL && PQresultStatus(hResult) == PGRES_TUPLES_OK )
        nCount = atoi(PQgetvalue(hResult,0,0));
    else
        CPLDebug( "OGR_PG", "%s; failed.", szCommand );
    PQclear( hResult );

    hResult = PQexec(hPGConn, "CLOSE countCursor");
    PQclear( hResult );

    hResult = PQexec(hPGConn, "COMMIT");
    PQclear( hResult );
    
    return nCount;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/*                                                                      */
/*      We override this to try and fetch the table SRID from the       */
/*      geometry_columns table if the srsid is -2 (meaning we           */
/*      haven't yet even looked for it).                                */
/************************************************************************/

OGRSpatialReference *OGRPGTableLayer::GetSpatialRef()

{
    if( nSRSId == -2 )
    {
        PGconn          *hPGConn = poDS->GetPGConn();
        PGresult        *hResult;
        char            szCommand[1024];

        nSRSId = -1;

        poDS->SoftStartTransaction();

        sprintf( szCommand, 
                 "SELECT srid FROM geometry_columns "
                 "WHERE f_table_name = '%s'",
                 poFeatureDefn->GetName() );
        hResult = PQexec(hPGConn, szCommand );

        if( hResult 
            && PQresultStatus(hResult) == PGRES_TUPLES_OK 
            && PQntuples(hResult) == 1 )
        {
            nSRSId = atoi(PQgetvalue(hResult,0,0));
        }

        poDS->SoftCommit();
    }

    return OGRPGLayer::GetSpatialRef();
}
