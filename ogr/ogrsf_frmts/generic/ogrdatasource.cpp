/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  The generic portions of the OGRDataSource class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999,  Les Technologies SoftMap Inc.
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
 * Revision 1.13  2003/03/19 20:35:49  warmerda
 * Added support for reference counting.
 * Added support for joins from tables in other datasources.
 *
 * Revision 1.12  2003/03/19 05:12:34  warmerda
 * fixed memory leak
 *
 * Revision 1.11  2003/03/05 05:13:49  warmerda
 * added getlayerbyname, implement join support
 *
 * Revision 1.10  2003/03/04 05:47:49  warmerda
 * added CREATE INDEX support
 *
 * Revision 1.9  2003/03/03 05:06:27  warmerda
 * added support for DeleteDataSource and DeleteLayer
 *
 * Revision 1.8  2002/09/26 18:16:19  warmerda
 * added C entry points
 *
 * Revision 1.7  2002/05/01 18:26:27  warmerda
 * Fixed reporting of error on table.
 *
 * Revision 1.6  2002/04/29 19:35:50  warmerda
 * fixes for selecting FID
 *
 * Revision 1.5  2002/04/25 03:42:04  warmerda
 * fixed spatial filter support on SQL results
 *
 * Revision 1.4  2002/04/25 02:24:45  warmerda
 * added ExecuteSQL method
 *
 * Revision 1.3  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.2  2000/08/21 16:37:43  warmerda
 * added constructor, and initialization of styletable
 *
 * Revision 1.1  1999/11/04 21:10:51  warmerda
 * New
 *
 */

#include "ogrsf_frmts.h"
#include "ogr_api.h"
#include "ogr_p.h"
#include "ogr_gensql.h"
#include "ogr_attrind.h"

CPL_C_START
#include "swq.h"
CPL_C_END

CPL_CVSID("$Id$");

/************************************************************************/
/*                           ~OGRDataSource()                           */
/************************************************************************/

OGRDataSource::OGRDataSource()

{
    m_poStyleTable = NULL;
    m_nRefCount = 0;
}

/************************************************************************/
/*                           ~OGRDataSource()                           */
/************************************************************************/

OGRDataSource::~OGRDataSource()

{
}

/************************************************************************/
/*                           OGR_DS_Destroy()                           */
/************************************************************************/

void OGR_DS_Destroy( OGRDataSourceH hDS )

{
    delete (OGRDataSource *) hDS;
}

/************************************************************************/
/*                             Reference()                              */
/************************************************************************/

int OGRDataSource::Reference()

{
    return ++m_nRefCount;
}

/************************************************************************/
/*                          OGR_DS_Reference()                          */
/************************************************************************/

int OGR_DS_Reference( OGRDataSourceH hDataSource )

{
    return ((OGRDataSource *) hDataSource)->Reference();
}

/************************************************************************/
/*                            Dereference()                             */
/************************************************************************/

int OGRDataSource::Dereference()

{
    return --m_nRefCount;
}

/************************************************************************/
/*                         OGR_DS_Dereference()                         */
/************************************************************************/

int OGR_DS_Dereference( OGRDataSourceH hDataSource )

{
    return ((OGRDataSource *) hDataSource)->Dereference();
}

/************************************************************************/
/*                            GetRefCount()                             */
/************************************************************************/

int OGRDataSource::GetRefCount() const

{
    return m_nRefCount;
}

/************************************************************************/
/*                         OGR_DS_GetRefCount()                         */
/************************************************************************/

int OGR_DS_GetRefCount( OGRDataSourceH hDataSource )

{
    return ((OGRDataSource *) hDataSource)->GetRefCount();
}

/************************************************************************/
/*                         GetSummaryRefCount()                         */
/************************************************************************/

int OGRDataSource::GetSummaryRefCount() const

{
    int nSummaryCount = m_nRefCount;
    int iLayer;
    OGRDataSource *poUseThis = (OGRDataSource *) this;

    for( iLayer=0; iLayer < poUseThis->GetLayerCount(); iLayer++ )
        nSummaryCount += poUseThis->GetLayer( iLayer )->GetRefCount();

    return nSummaryCount;
}

/************************************************************************/
/*                     OGR_DS_GetSummaryRefCount()                      */
/************************************************************************/

int OGR_DS_GetSummaryRefCount( OGRDataSourceH hDataSource )

{
    return ((OGRDataSource *) hDataSource)->GetSummaryRefCount();
}

/************************************************************************/
/*                            CreateLayer()                             */
/************************************************************************/

OGRLayer *OGRDataSource::CreateLayer( const char * pszName,
                                      OGRSpatialReference * poSpatialRef,
                                      OGRwkbGeometryType eType,
                                      char ** )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "CreateLayer() not supported by this data source." );
              
    return NULL;
}

/************************************************************************/
/*                         OGR_DS_CreateLayer()                         */
/************************************************************************/

OGRLayerH OGR_DS_CreateLayer( OGRDataSourceH hDS, 
                              const char * pszName,
                              OGRSpatialReferenceH hSpatialRef,
                              OGRwkbGeometryType eType,
                              char ** papszOptions )

{
    return ((OGRDataSource *)hDS)->CreateLayer( 
        pszName, (OGRSpatialReference *) hSpatialRef, eType, papszOptions );
}

/************************************************************************/
/*                            DeleteLayer()                             */
/************************************************************************/

OGRErr OGRDataSource::DeleteLayer( int iLayer )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "DeleteLayer() not supported by this data source." );
              
    return OGRERR_UNSUPPORTED_OPERATION;
}

/************************************************************************/
/*                         OGR_DS_DeleteLayer()                         */
/************************************************************************/

OGRErr OGR_DS_DeleteLayer( OGRDataSourceH hDS, int iLayer )

{
    return ((OGRDataSource *) hDS)->DeleteLayer( iLayer );
}

/************************************************************************/
/*                           GetLayerByName()                           */
/************************************************************************/

OGRLayer *OGRDataSource::GetLayerByName( const char *pszName )

{
    int  i;

    /* first a case sensitive check */
    for( i = 0; i < GetLayerCount(); i++ )
    {
        OGRLayer *poLayer = GetLayer(i);

        if( strcmp( pszName, poLayer->GetLayerDefn()->GetName() ) == 0 )
            return poLayer;
    }

    /* then case insensitive */
    for( i = 0; i < GetLayerCount(); i++ )
    {
        OGRLayer *poLayer = GetLayer(i);

        if( EQUAL( pszName, poLayer->GetLayerDefn()->GetName() ) )
            return poLayer;
    }

    return NULL;
}

/************************************************************************/
/*                       OGR_DS_GetLayerByName()                        */
/************************************************************************/

OGRLayerH OGR_DS_GetLayerByName( OGRDataSourceH hDS, const char *pszName )

{
    return (OGRLayerH) ((OGRDataSource *) hDS)->GetLayerByName( pszName );
}

/************************************************************************/
/*                       ProcessSQLCreateIndex()                        */
/*                                                                      */
/*      The correct syntax for creating an index in our dialect of      */
/*      SQL is:                                                         */
/*                                                                      */
/*        CREATE INDEX ON <layername> USING <columnname>                */
/************************************************************************/

OGRErr OGRDataSource::ProcessSQLCreateIndex( const char *pszSQLCommand )

{
    char **papszTokens = CSLTokenizeString( pszSQLCommand );

/* -------------------------------------------------------------------- */
/*      Do some general syntax checking.                                */
/* -------------------------------------------------------------------- */
    if( CSLCount(papszTokens) != 6 
        || !EQUAL(papszTokens[0],"CREATE")
        || !EQUAL(papszTokens[1],"INDEX")
        || !EQUAL(papszTokens[2],"ON")
        || !EQUAL(papszTokens[4],"USING") )
    {
        CSLDestroy( papszTokens );
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Syntax error in CREATE INDEX command.\n"
                  "Was '%s'\n"
                  "Should be of form 'CREATE INDEX ON <table> USING <field>'",
                  pszSQLCommand );
        return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Find the named layer.                                           */
/* -------------------------------------------------------------------- */
    int  i;
    OGRLayer *poLayer;

    for( i = 0; i < GetLayerCount(); i++ )
    {
        poLayer = GetLayer(i);
        
        if( EQUAL(poLayer->GetLayerDefn()->GetName(),papszTokens[3]) )
            break;
    }

    if( i >= GetLayerCount() )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "CREATE INDEX ON failed, no such layer as `%s'.",
                  papszTokens[3] );
        CSLDestroy( papszTokens );
        return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Does this layer even support attribute indexes?                 */
/* -------------------------------------------------------------------- */
    if( poLayer->GetIndex() == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "CREATE INDEX ON not supported by this driver." );
        CSLDestroy( papszTokens );
        return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Find the named field.                                           */
/* -------------------------------------------------------------------- */
    for( i = 0; i < poLayer->GetLayerDefn()->GetFieldCount(); i++ )
    {
        if( EQUAL(papszTokens[5],
                  poLayer->GetLayerDefn()->GetFieldDefn(i)->GetNameRef()) )
            break;
    }

    CSLDestroy( papszTokens );

    if( i >= poLayer->GetLayerDefn()->GetFieldCount() )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "`%s' failed, field not found.",
                  pszSQLCommand );
        return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Attempt to create the index.                                    */
/* -------------------------------------------------------------------- */
    OGRErr eErr;

    eErr = poLayer->GetIndex()->CreateIndex( i );
    if( eErr == OGRERR_NONE )
        eErr = poLayer->GetIndex()->IndexAllFeatures( i );

    return eErr;
}

/************************************************************************/
/*                             ExecuteSQL()                             */
/************************************************************************/

OGRLayer * OGRDataSource::ExecuteSQL( const char *pszSQLCommand,
                                      OGRGeometry *poSpatialFilter,
                                      const char *pszDialect )

{
    const char *pszError;
    swq_select *psSelectInfo = NULL;

/* -------------------------------------------------------------------- */
/*      Handle CREATE INDEX statements specially.                       */
/* -------------------------------------------------------------------- */
    if( EQUALN(pszSQLCommand,"CREATE INDEX",12) )
    {
        ProcessSQLCreateIndex( pszSQLCommand );
        return NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Preparse the SQL statement.                                     */
/* -------------------------------------------------------------------- */
    pszError = swq_select_preparse( pszSQLCommand, &psSelectInfo );
    if( pszError != NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "SQL: %s", pszError );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Validate that all the source tables are recognised, count       */
/*      fields.                                                         */
/* -------------------------------------------------------------------- */
    int  nFieldCount = 0, iTable;

    for( iTable = 0; iTable < psSelectInfo->table_count; iTable++ )
    {
        swq_table_def *psTableDef = psSelectInfo->table_defs + iTable;
        OGRLayer *poSrcLayer;
        OGRDataSource *poTableDS = this;

        if( psTableDef->data_source != NULL )
        {
            poTableDS = (OGRDataSource *) 
                OGROpenShared( psTableDef->data_source, FALSE, NULL );
            if( poTableDS == NULL )
            {
                if( strlen(CPLGetLastErrorMsg()) == 0 )
                    CPLError( CE_Failure, CPLE_AppDefined, 
                              "Unable to open secondary datasource\n"
                              "`%s' required by JOIN.",
                              psTableDef->data_source );

                swq_select_free( psSelectInfo );
                return NULL;
            }

            // This drops explicit reference, but leave it open for use by
            // code in ogr_gensql.cpp
            poTableDS->Dereference();
        }

        poSrcLayer = poTableDS->GetLayerByName( psTableDef->table_name );

        if( poSrcLayer == NULL )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "SELECT from table %s failed, no such table/featureclass.",
                      psTableDef->table_name );
            swq_select_free( psSelectInfo );
            return NULL;
        }

        nFieldCount += poSrcLayer->GetLayerDefn()->GetFieldCount();
    }
    
/* -------------------------------------------------------------------- */
/*      Build the field list for all indicated tables.                  */
/* -------------------------------------------------------------------- */
    swq_field_list sFieldList;
    int            nFIDIndex;

    memset( &sFieldList, 0, sizeof(sFieldList) );
    sFieldList.table_count = psSelectInfo->table_count;
    sFieldList.table_defs = psSelectInfo->table_defs;

    sFieldList.count = 0;
    sFieldList.names = (char **) CPLMalloc( sizeof(char *) * (nFieldCount+1) );
    sFieldList.types = (swq_field_type *)  
        CPLMalloc( sizeof(swq_field_type) * (nFieldCount+1) );
    sFieldList.table_ids = (int *) 
        CPLMalloc( sizeof(int) * (nFieldCount+1) );
    sFieldList.ids = (int *) 
        CPLMalloc( sizeof(int) * (nFieldCount+1) );
    
    for( iTable = 0; iTable < psSelectInfo->table_count; iTable++ )
    {
        swq_table_def *psTableDef = psSelectInfo->table_defs + iTable;
        OGRDataSource *poTableDS = this;
        OGRLayer *poSrcLayer;
        int      iField;

        if( psTableDef->data_source != NULL )
        {
            poTableDS = (OGRDataSource *) 
                OGROpenShared( psTableDef->data_source, FALSE, NULL );
            CPLAssert( poTableDS != NULL );
            poTableDS->Dereference();
        }

        poSrcLayer = poTableDS->GetLayerByName( psTableDef->table_name );

        for( iField = 0; 
             iField < poSrcLayer->GetLayerDefn()->GetFieldCount();
             iField++ )
        {
            OGRFieldDefn *poFDefn=poSrcLayer->GetLayerDefn()->GetFieldDefn(iField);
            int iOutField = sFieldList.count++;
            sFieldList.names[iOutField] = (char *) poFDefn->GetNameRef();
            if( poFDefn->GetType() == OFTInteger )
                sFieldList.types[iOutField] = SWQ_INTEGER;
            else if( poFDefn->GetType() == OFTReal )
                sFieldList.types[iOutField] = SWQ_FLOAT;
            else if( poFDefn->GetType() == OFTString )
                sFieldList.types[iOutField] = SWQ_STRING;
            else
                sFieldList.types[iOutField] = SWQ_OTHER;

            sFieldList.table_ids[iOutField] = iTable;
            sFieldList.ids[iOutField] = iField;
        }

        if( iTable == 0 )
            nFIDIndex = poSrcLayer->GetLayerDefn()->GetFieldCount();
    }

/* -------------------------------------------------------------------- */
/*      Expand '*' in 'SELECT *' now before we add the pseudo field     */
/*      'FID'.                                                          */
/* -------------------------------------------------------------------- */
    pszError = 
        swq_select_expand_wildcard( psSelectInfo, &sFieldList );

    if( pszError != NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "SQL: %s", pszError );
        return NULL;
    }

    sFieldList.names[sFieldList.count] = "FID";
    sFieldList.types[sFieldList.count] = SWQ_INTEGER;
    sFieldList.table_ids[sFieldList.count] = 0;
    sFieldList.ids[sFieldList.count] = nFIDIndex;
    
    sFieldList.count++;

/* -------------------------------------------------------------------- */
/*      Finish the parse operation.                                     */
/* -------------------------------------------------------------------- */
    
    pszError = swq_select_parse( psSelectInfo, &sFieldList, 0 );

    CPLFree( sFieldList.names );
    CPLFree( sFieldList.types );
    CPLFree( sFieldList.table_ids );
    CPLFree( sFieldList.ids );

    if( pszError != NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "SQL: %s", pszError );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Everything seems OK, try to instantiate a results layer.        */
/* -------------------------------------------------------------------- */
    OGRGenSQLResultsLayer *poResults;

    poResults = new OGRGenSQLResultsLayer( this, psSelectInfo, 
                                           poSpatialFilter );

    // Eventually, we should keep track of layers to cleanup.

    return poResults;
}

/************************************************************************/
/*                         OGR_DS_ExecuteSQL()                          */
/************************************************************************/

OGRLayerH OGR_DS_ExecuteSQL( OGRDataSourceH hDS, 
                             const char *pszSQLCommand,
                             OGRGeometryH hSpatialFilter,
                             const char *pszDialect )

{
    return (OGRLayerH) 
        ((OGRDataSource *)hDS)->ExecuteSQL( pszSQLCommand,
                                            (OGRGeometry *) hSpatialFilter,
                                            pszDialect );
}

/************************************************************************/
/*                          ReleaseResultSet()                          */
/************************************************************************/

void OGRDataSource::ReleaseResultSet( OGRLayer * poLayer )

{
    delete poLayer;
}

/************************************************************************/
/*                      OGR_DS_ReleaseResultSet()                       */
/************************************************************************/

void OGR_DS_ReleaseResultSet( OGRDataSourceH hDS, OGRLayerH hLayer )

{
    ((OGRDataSource *) hDS)->ReleaseResultSet( (OGRLayer *) hLayer );
}

/************************************************************************/
/*                       OGR_DS_TestCapability()                        */
/************************************************************************/

int OGR_DS_TestCapability( OGRDataSourceH hDS, const char *pszCap )

{
    return ((OGRDataSource *) hDS)->TestCapability( pszCap );
}

/************************************************************************/
/*                        OGR_DS_GetLayerCount()                        */
/************************************************************************/

int OGR_DS_GetLayerCount( OGRDataSourceH hDS )

{
    return ((OGRDataSource *)hDS)->GetLayerCount();
}

/************************************************************************/
/*                          OGR_DS_GetLayer()                           */
/************************************************************************/

OGRLayerH OGR_DS_GetLayer( OGRDataSourceH hDS, int iLayer )

{
    return (OGRLayerH) ((OGRDataSource*)hDS)->GetLayer( iLayer );
}

/************************************************************************/
/*                           OGR_DS_GetName()                           */
/************************************************************************/

const char *OGR_DS_GetName( OGRDataSourceH hDS )

{
    return ((OGRDataSource *)hDS)->GetName();
}

