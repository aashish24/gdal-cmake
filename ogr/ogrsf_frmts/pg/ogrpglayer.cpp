/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRPGLayer class.
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
 * Revision 1.1  2000/10/17 17:46:51  warmerda
 * New
 *
 */

#include "cpl_conv.h"
#include "ogr_pg.h"

#define CURSOR_PAGE	1

/************************************************************************/
/*                           OGRPGLayer()                               */
/************************************************************************/

OGRPGLayer::OGRPGLayer( OGRPGDataSource *poDSIn, const char * pszTableName,
                        int bUpdate )

{
    poDS = poDSIn;

    poFilterGeom = NULL;
    
    bUpdateAccess = bUpdate;
    bHasWkb = FALSE;

    iNextShapeId = 0;

    /* Eventually we may need to make these a unique name */
    pszCursorName = "OGRPGLayerReader";
    hCursorResult = NULL;

    poFeatureDefn = ReadTableDefinition( pszTableName );
}

/************************************************************************/
/*                            ~OGRPGLayer()                             */
/************************************************************************/

OGRPGLayer::~OGRPGLayer()

{
    ResetReading();

    delete poFeatureDefn;

    if( poFilterGeom != NULL )
        delete poFilterGeom;
}

/************************************************************************/
/*                        ReadTableDefinition()                         */
/************************************************************************/

OGRFeatureDefn *OGRPGLayer::ReadTableDefinition( const char * pszTable )

{
    PGresult            *hResult;
    char		szCommand[1024];
    PGconn		*hPGConn = poDS->GetPGConn();
    
/* -------------------------------------------------------------------- */
/*      Fire off commands to get back the schema of the table.          */
/* -------------------------------------------------------------------- */
    hResult = PQexec(hPGConn, "BEGIN");

    if( hResult && PQresultStatus(hResult) == PGRES_COMMAND_OK )
    {
        PQclear( hResult );
        sprintf( szCommand, 
                 "DECLARE mycursor CURSOR for "
                 "SELECT a.attname, t.typname, a.attlen "
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
    int		   iRecord;

    for( iRecord = 0; iRecord < PQntuples(hResult); iRecord++ )
    {
        const char	*pszType;
        OGRFieldDefn    oField( PQgetvalue( hResult, iRecord, 0 ), OFTString);

        if( EQUAL(oField.GetNameRef(),"ogc_fid") )
        {
            bHasFid = TRUE;
            continue;
        }
        else if( EQUAL(oField.GetNameRef(),"ogc_wkb") )
        {
            bHasWkb = TRUE;
            continue;
        }
        
        pszType = PQgetvalue(hResult, iRecord, 1 );
        
        if( EQUAL(pszType,"varchar") )
        {
            oField.SetType( OFTString );
        }
        else if( EQUALN(pszType,"int",3) )
        {
            oField.SetType( OFTInteger );
        }
        else if( EQUALN(pszType, "float", 5) ) 
        {
            oField.SetType( OFTReal );
        }
        else
        {
            oField.SetWidth( atoi(PQgetvalue(hResult,iRecord,2)) );
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

void OGRPGLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( poFilterGeom != NULL )
    {
        delete poFilterGeom;
        poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
        poFilterGeom = poGeomIn->clone();
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRPGLayer::ResetReading()

{
    PGconn	*hPGConn = poDS->GetPGConn();
    char	szCommand[1024];

    iNextShapeId = 0;

    if( hCursorResult != NULL )
    {
        PQclear( hCursorResult );

        sprintf( szCommand, "CLOSE %s", pszCursorName );

        hCursorResult = PQexec(hPGConn, szCommand);
        PQclear( hCursorResult );

        hCursorResult = PQexec(hPGConn, "COMMIT" );
        PQclear( hCursorResult );

        hCursorResult = NULL;
    }
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRPGLayer::GetNextFeature()

{
    PGconn	*hPGConn = poDS->GetPGConn();
    char	szCommand[1024];

/* -------------------------------------------------------------------- */
/*      Do we need to establish an initial query?                       */
/* -------------------------------------------------------------------- */
    if( iNextShapeId == 0 )
    {
        hCursorResult = PQexec(hPGConn, "BEGIN");
        PQclear( hCursorResult );

        sprintf( szCommand, 
                 "DECLARE %s CURSOR for "
                 "SELECT * FROM %s", 
                 pszCursorName, poFeatureDefn->GetName() );
        hCursorResult = PQexec(hPGConn, szCommand );
        PQclear( hCursorResult );

        sprintf( szCommand, "FETCH %d in %s", CURSOR_PAGE, pszCursorName );
        hCursorResult = PQexec(hPGConn, szCommand );

        nResultOffset = 0;
    }

/* -------------------------------------------------------------------- */
/*      Are we in some sort of error condition?                         */
/* -------------------------------------------------------------------- */
    if( hCursorResult == NULL 
        || PQresultStatus(hCursorResult) != PGRES_TUPLES_OK )
    {
        iNextShapeId = MAX(1,iNextShapeId);
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Do we need to fetch more records?                               */
/* -------------------------------------------------------------------- */
    if( nResultOffset >= PQntuples(hCursorResult) )
    {
        PQclear( hCursorResult );

        sprintf( szCommand, "FETCH %d in %s", CURSOR_PAGE, pszCursorName );
        hCursorResult = PQexec(hPGConn, szCommand );

        nResultOffset = 0;
    }

/* -------------------------------------------------------------------- */
/*      Are we out of results?  If so complete the transaction, and     */
/*      cleanup, but don't reset the next shapeid.                      */
/* -------------------------------------------------------------------- */
    if( nResultOffset >= PQntuples(hCursorResult) )
    {
        PQclear( hCursorResult );

        sprintf( szCommand, "CLOSE %s", pszCursorName );

        hCursorResult = PQexec(hPGConn, szCommand);
        PQclear( hCursorResult );

        hCursorResult = PQexec(hPGConn, "COMMIT" );
        PQclear( hCursorResult );

        hCursorResult = NULL;

        iNextShapeId = MAX(1,iNextShapeId);

        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create a feature from the current result.                       */
/* -------------------------------------------------------------------- */
    int		iField;
    OGRFeature *poFeature = new OGRFeature( poFeatureDefn );

    if( EQUAL(PQfname(hCursorResult,0),"ogc_fid") )
    {
        poFeature->SetFID( atoi(PQgetvalue(hCursorResult,nResultOffset,0)) );
    }
    else
        poFeature->SetFID( iNextShapeId );

    iNextShapeId++;

    for( iField = 0; 
         iField < PQnfields(hCursorResult);
         iField++ )
    {
        int	iOGRField;

        if( EQUAL(PQfname(hCursorResult,iField),"ogc_wkb") )
        {
            poFeature->SetGeometryDirectly( 
                BYTEAToGeometry( 
                    PQgetvalue( hCursorResult, 
                                nResultOffset, iField ) ) );
            continue;
        }

        iOGRField = 
            poFeatureDefn->GetFieldIndex(PQfname(hCursorResult,iField));

        if( iOGRField < 0 )
            continue;

        poFeature->SetField( iOGRField, 
                             PQgetvalue( hCursorResult, 
                                         nResultOffset, iField ) );
    }

    nResultOffset++;

    return poFeature;
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRPGLayer::GetFeature( long nFeatureId )

{
    return NULL;
}

/************************************************************************/
/*                             SetFeature()                             */
/************************************************************************/

OGRErr OGRPGLayer::SetFeature( OGRFeature *poFeature )

{
    return OGRERR_FAILURE;
}

/************************************************************************/
/*                          BYTEAToGeometry()                           */
/************************************************************************/

OGRGeometry *OGRPGLayer::BYTEAToGeometry( const char *pszBytea )

{
    GByte       *pabyWKB;
    int iSrc=0, iDst=0;
    OGRGeometry *poGeometry;

    if( pszBytea == NULL )
        return NULL;

    pabyWKB = (GByte *) CPLMalloc(strlen(pszBytea));
    while( pszBytea[iSrc] != '\0' )
    {
        if( pszBytea[iSrc] == '\\' )
        {
            if( pszBytea[iSrc+1] >= '0' && pszBytea[iSrc+1] <= '9' )
            {
                pabyWKB[iDst++] = 
                    (pszBytea[iSrc+1] - 48) * 64
                    + (pszBytea[iSrc+2] - 48) * 8
                    + (pszBytea[iSrc+3] - 48) * 1;
                iSrc += 4;
            }
            else
            {
                pabyWKB[iDst++] = pszBytea[iSrc+1];
                iSrc += 2;
            }
        }
        else
        {
            pabyWKB[iDst++] = pszBytea[iSrc++];
        }
    }

    poGeometry = NULL;
    OGRGeometryFactory::createFromWkb( pabyWKB, NULL, &poGeometry, iDst );

    CPLFree( pabyWKB );
    return poGeometry;
}

/************************************************************************/
/*                          GeometryToBYTEA()                           */
/************************************************************************/

char *OGRPGLayer::GeometryToBYTEA( OGRGeometry * poGeometry )

{
    int		nWkbSize = poGeometry->WkbSize();
    GByte	*pabyWKB;
    char	*pszTextBuf, *pszRetBuf;

    pabyWKB = (GByte *) CPLMalloc(nWkbSize);
    if( poGeometry->exportToWkb( wkbNDR, pabyWKB ) != OGRERR_NONE )
        return CPLStrdup("");

    pszTextBuf = (char *) CPLMalloc(nWkbSize*5+1);

    int  iSrc, iDst=0;

    for( iSrc = 0; iSrc < nWkbSize; iSrc++ )
    {
        if( pabyWKB[iSrc] < 40 || pabyWKB[iSrc] > 126
            || pabyWKB[iSrc] == '\\' )
        {
            sprintf( pszTextBuf+iDst, "\\\\%03o", pabyWKB[iSrc] );
            iDst += 5;
        }
        else
            pszTextBuf[iDst++] = pabyWKB[iSrc];
    }
    pszTextBuf[iDst] = '\0';

    pszRetBuf = CPLStrdup( pszTextBuf );
    CPLFree( pszTextBuf );

    return pszRetBuf;
}

/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/

OGRErr OGRPGLayer::CreateFeature( OGRFeature *poFeature )

{
    PGconn		*hPGConn = poDS->GetPGConn();
    PGresult            *hResult;
    char		szCommand[8000];
    int                 i, bNeedComma;

/* -------------------------------------------------------------------- */
/*      Form the INSERT command.  Note, we aren't watching for          */
/*      command buffer overflow for the field list, and we don't        */
/*      skip null fields the way we ought to.                           */
/* -------------------------------------------------------------------- */
    sprintf( szCommand, "INSERT INTO %s (", poFeatureDefn->GetName() );

    if( bHasWkb && poFeature->GetGeometryRef() != NULL )
        strcat( szCommand, "ogc_wkb, " );
    
    bNeedComma = FALSE;
    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
    {
        if( !bNeedComma )
            bNeedComma = TRUE;
        else
            strcat( szCommand, ", " );

        strcat( szCommand, poFeatureDefn->GetFieldDefn(i)->GetNameRef() );
    }

    strcat( szCommand, ") VALUES (" );

    /* Set the geometry */
    if( bHasWkb && poFeature->GetGeometryRef() != NULL )
    {
        char	*pszBytea = GeometryToBYTEA( poFeature->GetGeometryRef() );

        if( strlen(szCommand) + strlen(pszBytea) > sizeof(szCommand)-50 )
        {
            CPLFree( pszBytea );
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Internal command buffer to short for INSERT command." );
            return OGRERR_FAILURE;
        }

        if( pszBytea != NULL )
        {
            sprintf( szCommand + strlen(szCommand), 
                     "'%s', ", pszBytea );
            CPLFree( pszBytea );
        }
        else
            strcat( szCommand, "''," );
    }

    /* Set the other fields */
    bNeedComma = TRUE;
    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
    {
        const char *pszStrValue = poFeature->GetFieldAsString(i);

        if( i > 0 )
            strcat( szCommand, ", " );

        if( strlen(pszStrValue) + strlen(szCommand) > sizeof(szCommand)-50)
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Internal command buffer to short for INSERT command." );
            return OGRERR_FAILURE;
        }
        
        if( poFeatureDefn->GetFieldDefn(i)->GetType() == OFTString )
        {
            strcat( szCommand, "'" );
            strcat( szCommand, pszStrValue );
            strcat( szCommand, "'" );
        }
        else
        {
            strcat( szCommand, pszStrValue );
        }
    }

    strcat( szCommand, ")" );

/* -------------------------------------------------------------------- */
/*      Execute the insert.                                             */
/* -------------------------------------------------------------------- */
    hResult = PQexec(hPGConn, "BEGIN");
    PQclear( hResult );

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
#ifdef notdef
    /* Should we use this oid to get back the FID and assign back to the
       feature?  I think we are supposed to. */
    Oid	nNewOID = PQoidValue( hResult );
    printf( "nNewOID = %d\n", (int) nNewOID );
#endif
    PQclear( hResult );

    hResult = PQexec( hPGConn, "COMMIT" );
    PQclear( hResult );

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

int OGRPGLayer::GetFeatureCount( int bForce )

{
/* -------------------------------------------------------------------- */
/*      Use a more brute force mechanism if we have a spatial query     */
/*      in play.                                                        */
/* -------------------------------------------------------------------- */
    if( poFilterGeom != NULL )
        return OGRLayer::GetFeatureCount( bForce );

/* -------------------------------------------------------------------- */
/*      In theory it might be wise to cache this result, but it         */
/*      won't be trivial to work out the lifetime of the value.         */
/*      After all someone else could be adding records from another     */
/*      application when working against a database.                    */
/* -------------------------------------------------------------------- */
    PGconn		*hPGConn = poDS->GetPGConn();
    PGresult            *hResult;
    char		szCommand[1024];
    int			nCount = 0;

    hResult = PQexec(hPGConn, "BEGIN");
    PQclear( hResult );

    sprintf( szCommand, 
             "DECLARE countCursor CURSOR for "
             "SELECT count(*) FROM %s", 
             poFeatureDefn->GetName() );
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
/*                           TestCapability()                           */
/************************************************************************/

int OGRPGLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return TRUE;

    else if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return bUpdateAccess;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return poFilterGeom == NULL;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return FALSE;

    else if( EQUAL(pszCap,OLCCreateField) )
        return TRUE;

    else 
        return FALSE;
}

/************************************************************************/
/*                            CreateField()                             */
/************************************************************************/

OGRErr OGRPGLayer::CreateField( OGRFieldDefn *poField, int bApproxOK )

{
    PGconn		*hPGConn = poDS->GetPGConn();
    PGresult            *hResult;
    char		szCommand[1024];
    char		szFieldType[256];

/* -------------------------------------------------------------------- */
/*      Work out the PostgreSQL type.                                   */
/* -------------------------------------------------------------------- */
    if( poField->GetType() == OFTInteger )
    {
        strcpy( szFieldType, "INTEGER" );
    }
    else if( poField->GetType() == OFTReal )
    {
        strcpy( szFieldType, "FLOAT8" );
    }
    else if( poField->GetType() == OFTString )
    {
        if( poField->GetWidth() == 0 )
            strcpy( szFieldType, "VARCHAR" );
        else
            sprintf( szFieldType, "CHAR(%d)", poField->GetWidth() );
    }
    else
    {
        CPLError( CE_Failure, CPLE_NotSupported,
                  "Can't create fields of type %s on PostgreSQL layers.\n",
                  OGRFieldDefn::GetFieldTypeName(poField->GetType()) );

        return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Create the new field.                                           */
/* -------------------------------------------------------------------- */
    hResult = PQexec(hPGConn, "BEGIN");
    PQclear( hResult );

    sprintf( szCommand, "ALTER TABLE %s ADD COLUMN %s %s", 
             poFeatureDefn->GetName(), poField->GetNameRef(), szFieldType );
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

    poFeatureDefn->AddFieldDefn( poField );

    return OGRERR_NONE;
}
