/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRSQLiteLayer class, code shared between 
 *           the direct table access, and the generic SQL results.
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
 * Revision 1.1  2004/07/09 06:25:04  warmerda
 * New
 *
 */

#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_sqlite.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           OGRSQLiteLayer()                           */
/************************************************************************/

OGRSQLiteLayer::OGRSQLiteLayer()

{
    poDS = NULL;

    poFilterGeom = NULL;

    pszGeomColumn = NULL;
    pszFIDColumn = NULL;

    hStmt = NULL;

    iNextShapeId = 0;

    poSRS = NULL;
    nSRSId = -2; // we haven't even queried the database for it yet. 
}

/************************************************************************/
/*                          ~OGRSQLiteLayer()                           */
/************************************************************************/

OGRSQLiteLayer::~OGRSQLiteLayer()

{
    if( hStmt != NULL )
    {
        sqlite3_finalize( hStmt );
        hStmt = NULL;
    }

    if( pszGeomColumn != NULL )
        CPLFree( pszGeomColumn );

    if( poFilterGeom != NULL )
        delete poFilterGeom;

    if( poFeatureDefn != NULL )
    {
        delete poFeatureDefn;
        poFeatureDefn = NULL;
    }

    if( poSRS != NULL )
        poSRS->Dereference();
}

/************************************************************************/
/*                          BuildFeatureDefn()                          */
/*                                                                      */
/*      Build feature definition from a set of column definitions       */
/*      set on a statement.  Sift out geometry and FID fields.          */
/************************************************************************/

CPLErr OGRSQLiteLayer::BuildFeatureDefn( const char *pszLayerName, 
                                         sqlite3_stmt *hStmt )

{
    poFeatureDefn = new OGRFeatureDefn( pszLayerName );
    int    nRawColumns = sqlite3_column_count( hStmt );

    panFieldOrdinals = (int *) CPLMalloc( sizeof(int) * nRawColumns );

    for( int iCol = 0; iCol < nRawColumns; iCol++ )
    {
        OGRFieldDefn    oField( sqlite3_column_name( hStmt, iCol ), 
                                OFTString );

        printf( "%s: %s\n", 
                sqlite3_column_name( hStmt, iCol ),
                sqlite3_column_decltype( hStmt, iCol ) );

        //oField.SetWidth( MAX(0,poStmt->GetColSize( iCol )) );

        if( pszGeomColumn != NULL 
            && EQUAL(oField.GetNameRef(),pszGeomColumn) )
            continue;

        // TODO: Identify primary key? 
        
        switch( sqlite3_column_type( hStmt, iCol ) )
        {
          case SQLITE_INTEGER:
            oField.SetType( OFTInteger );
            break;

          case SQLITE_FLOAT:
            oField.SetType( OFTReal );
            break;

          case SQLITE_BLOB:
            oField.SetType( OFTBinary );
            break;

          default:
            /* leave it as OFTString */;
        }

        poFeatureDefn->AddFieldDefn( &oField );
        panFieldOrdinals[poFeatureDefn->GetFieldCount() - 1] = iCol+1;
    }

    return CE_None;
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRSQLiteLayer::ResetReading()

{
    iNextShapeId = 0;
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRSQLiteLayer::GetNextFeature()

{
    for( ; TRUE; )
    {
        OGRFeature      *poFeature;

        poFeature = GetNextRawFeature();
        if( poFeature == NULL )
            return NULL;

        if( (poFilterGeom == NULL
            || poFilterGeom->Intersect( poFeature->GetGeometryRef() ) )
            && (m_poAttrQuery == NULL
                || m_poAttrQuery->Evaluate( poFeature )) )
            return poFeature;

        delete poFeature;
    }
}

/************************************************************************/
/*                         GetNextRawFeature()                          */
/************************************************************************/

OGRFeature *OGRSQLiteLayer::GetNextRawFeature()

{
    if( GetStatement() == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      If we are marked to restart then do so, and fetch a record.     */
/* -------------------------------------------------------------------- */
    int rc;

    rc = sqlite3_step( hStmt );
    if( rc != SQLITE_ROW )
    {
        // we really should check for errors 
        sqlite3_finalize( hStmt );
        hStmt = NULL;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create a feature from the current result.                       */
/* -------------------------------------------------------------------- */
    int         iField;
    OGRFeature *poFeature = new OGRFeature( poFeatureDefn );

#ifdef notdef
    if( pszFIDColumn != NULL && poStmt->GetColId(pszFIDColumn) > -1 )
        poFeature->SetFID( 
            atoi(poStmt->GetColData(poStmt->GetColId(pszFIDColumn))) );
    else
#endif
        poFeature->SetFID( iNextShapeId );

    iNextShapeId++;

/* -------------------------------------------------------------------- */
/*      Set the fields.                                                 */
/* -------------------------------------------------------------------- */
    for( iField = 0; iField < poFeatureDefn->GetFieldCount(); iField++ )
    {
        OGRFieldDefn *poFieldDefn = poFeatureDefn->GetFieldDefn( iField );
        int iRawField = panFieldOrdinals[iField] - 1;

        if( sqlite3_column_type( hStmt, iRawField ) == SQLITE_NULL )
            continue;
        
        switch( poFieldDefn->GetType() )
        {
          case OFTInteger:
            poFeature->SetField( iField, 
                                 sqlite3_column_int( hStmt, iRawField ) );
            break;

          case OFTReal:
            poFeature->SetField( iField, 
                                 sqlite3_column_double( hStmt, iRawField ) );
            break;

#ifdef notdef
          case OFTBinary:
          {
              int nBytes = sqlite3_column_bytes( hStmt, iRawField );

              poFeature->SetField( iField, 
                                   sqlite3_column_double( hStmt, iRawField ) );
          }
          break;
#endif

          case OFTString:
            poFeature->SetField( iField, 
                                 (const char *) 
                                 sqlite3_column_text( hStmt, iRawField ) );
            break;

          default:
            break;
        }
    }

/* -------------------------------------------------------------------- */
/*      Try to extract a geometry.                                      */
/* -------------------------------------------------------------------- */
#ifdef notdef
    if( pszGeomColumn != NULL )
    {
        int iField = poStmt->GetColId( pszGeomColumn );
        const char *pszGeomText = poStmt->GetColData( iField );
        OGRGeometry *poGeom = NULL;

        if( pszGeomText != NULL )
            OGRGeometryFactory::createFromWkt( (char **) &pszGeomText,
                                               NULL, &poGeom );
        
        if( poGeom != NULL )
            poFeature->SetGeometryDirectly( poGeom );
    }
#endif

    return poFeature;
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRSQLiteLayer::GetFeature( long nFeatureId )

{
    /* This should be implemented directly! */

    return OGRLayer::GetFeature( nFeatureId );
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRSQLiteLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return FALSE;

    else if( EQUAL(pszCap,OLCTransactions) )
        return FALSE;

    else 
        return FALSE;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRSQLiteLayer::GetSpatialRef()

{
    return poSRS;
}
