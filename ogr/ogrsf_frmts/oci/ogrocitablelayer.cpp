/******************************************************************************
 * $Id$
 *
 * Project:  Oracle Spatial Driver
 * Purpose:  Implementation of the OGROCITableLayer class.  This class provides
 *           layer semantics on a table, but utilizing alot of machinery from
 *           the OGROCILayer base class.
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
 * Revision 1.21  2003/04/04 06:18:08  warmerda
 * first pass implementation of loader support
 *
 * Revision 1.20  2003/02/06 21:16:37  warmerda
 * restructure to handle elem_info indirectly like ordinals
 *
 * Revision 1.19  2003/01/15 05:47:06  warmerda
 * fleshed out SRID writing support in newly created geometries
 *
 * Revision 1.18  2003/01/15 05:36:14  warmerda
 * start work on creating feature geometry with SRID
 *
 * Revision 1.17  2003/01/14 22:15:02  warmerda
 * added logic to order rings properly
 *
 * Revision 1.16  2003/01/14 16:59:03  warmerda
 * added field truncation support
 *
 * Revision 1.15  2003/01/14 15:31:08  warmerda
 * added fallback support if no spatial index available
 *
 * Revision 1.14  2003/01/14 15:11:00  warmerda
 * Added layer creation options caching on layer.
 * Set SRID in spatial query.
 * Support user override of DIMINFO bounds in FinalizeNewLayer().
 * Support user override of indexing options, or disabling of indexing.
 *
 * Revision 1.13  2003/01/13 13:50:13  warmerda
 * dont quote table names, it doesnt seem to work with userid.tablename
 *
 * Revision 1.12  2003/01/10 22:31:53  warmerda
 * no longer encode ordinates into SQL statement when creating features
 *
 * Revision 1.11  2003/01/09 21:19:12  warmerda
 * improved data type support, get/set precision
 *
 * Revision 1.10  2003/01/07 22:24:35  warmerda
 * added SRS support
 *
 * Revision 1.9  2003/01/07 21:14:20  warmerda
 * implement GetFeature() and SetFeature()
 *
 * Revision 1.8  2003/01/07 18:16:01  warmerda
 * implement spatial filtering in Oracle, re-enable index build
 *
 * Revision 1.7  2003/01/06 18:00:34  warmerda
 * Restructure geometry translation ... collections now supported.
 * Dimension is now a layer wide attribute.
 * Update dimension info in USER_SDO_GEOM_METADATA on close.
 *
 * Revision 1.6  2003/01/02 21:51:05  warmerda
 * fix quote escaping
 *
 * Revision 1.5  2002/12/29 19:43:59  warmerda
 * avoid some warnings
 *
 * Revision 1.4  2002/12/29 03:48:58  warmerda
 * fixed memory bug in CreateFeature()
 *
 * Revision 1.3  2002/12/28 20:06:31  warmerda
 * minor CreateFeature improvements
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

static int nDiscarded = 0;
static int nHits = 0;

#define HSI_UNKNOWN  -2

/************************************************************************/
/*                          OGROCITableLayer()                          */
/************************************************************************/

OGROCITableLayer::OGROCITableLayer( OGROCIDataSource *poDSIn, 
                                    const char * pszTableName,
                                    const char * pszGeomColIn,
                                    int nSRIDIn, int bUpdate, int bNewLayerIn )

{
    poDS = poDSIn;

    pszQuery = NULL;
    pszWHERE = CPLStrdup( "" );
    pszQueryStatement = NULL;
    
    bUpdateAccess = bUpdate;
    bNewLayer = bNewLayerIn;

    iNextShapeId = 0;
    iNextFIDToWrite = 1;

    bValidTable = FALSE;
    if( bNewLayerIn )
        bHaveSpatialIndex = FALSE;
    else
        bHaveSpatialIndex = HSI_UNKNOWN;

    poFeatureDefn = ReadTableDefinition( pszTableName );
    
    CPLFree( pszGeomName );
    pszGeomName = CPLStrdup( pszGeomColIn );

    nSRID = nSRIDIn;
    poSRS = poDSIn->FetchSRS( nSRID );
    if( poSRS != NULL )
        poSRS->Reference();

    hOrdVARRAY = NULL;

    hElemInfoVARRAY = NULL;

    ResetReading();
}

/************************************************************************/
/*                         ~OGROCITableLayer()                          */
/************************************************************************/

OGROCITableLayer::~OGROCITableLayer()

{
    if( bNewLayer )
        FinalizeNewLayer();

    CPLFree( pszQuery );
    CPLFree( pszWHERE );

    if( poSRS != NULL && poSRS->Dereference() == 0 )
        delete poSRS;
}

/************************************************************************/
/*                        ReadTableDefinition()                         */
/*                                                                      */
/*      Build a schema from the named table.  Done by querying the      */
/*      catalog.                                                        */
/************************************************************************/

OGRFeatureDefn *OGROCITableLayer::ReadTableDefinition( const char * pszTable )

{
    OGROCISession      *poSession = poDS->GetSession();
    sword               nStatus;
    OGRFeatureDefn *poDefn = new OGRFeatureDefn( pszTable );

    poDefn->Reference();

/* -------------------------------------------------------------------- */
/*      Do a DescribeAll on the table.                                  */
/* -------------------------------------------------------------------- */
    OCIParam *hAttrParam = NULL;
    OCIParam *hAttrList = NULL;

    nStatus = 
        OCIDescribeAny( poSession->hSvcCtx, poSession->hError, 
                        (dvoid *) pszTable, strlen(pszTable), OCI_OTYPE_NAME, 
                        OCI_DEFAULT, OCI_PTYPE_TABLE, poSession->hDescribe );
    if( poSession->Failed( nStatus, "OCIDescribeAny" ) )
        return poDefn;

    if( poSession->Failed( 
        OCIAttrGet( poSession->hDescribe, OCI_HTYPE_DESCRIBE, 
                    &hAttrParam, 0, OCI_ATTR_PARAM, poSession->hError ),
        "OCIAttrGet(ATTR_PARAM)") )
        return poDefn;

    if( poSession->Failed( 
        OCIAttrGet( hAttrParam, OCI_DTYPE_PARAM, &hAttrList, 0, 
                    OCI_ATTR_LIST_COLUMNS, poSession->hError ),
        "OCIAttrGet(ATTR_LIST_COLUMNS)" ) )
        return poDefn;

/* -------------------------------------------------------------------- */
/*      Parse the returned table information.                           */
/* -------------------------------------------------------------------- */
    for( int iRawFld = 0; TRUE; iRawFld++ )
    {
        OGRFieldDefn    oField( "", OFTString);
        OCIParam     *hParmDesc;
        ub2          nOCIType;
        ub4          nOCILen;
        sword        nStatus;

        nStatus = OCIParamGet( hAttrList, OCI_DTYPE_PARAM,
                               poSession->hError, (dvoid**)&hParmDesc, 
                               (ub4) iRawFld+1 );
        if( nStatus != OCI_SUCCESS )
            break;

        if( poSession->GetParmInfo( hParmDesc, &oField, &nOCIType, &nOCILen )
            != CE_None )
            return poDefn;

        if( oField.GetType() == OFTBinary )
            continue;			

        if( EQUAL(oField.GetNameRef(),"OGR_FID") 
            && oField.GetType() == OFTInteger )
        {
            pszFIDName = CPLStrdup(oField.GetNameRef());
            continue;
        }

        poDefn->AddFieldDefn( &oField );
    }

    bValidTable = TRUE;

    return poDefn;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGROCITableLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

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
/*                        TestForSpatialIndex()                         */
/************************************************************************/

void OGROCITableLayer::TestForSpatialIndex( const char *pszSpatWHERE )

{
    OGROCIStringBuf oTestCmd;
    OGROCIStatement oTestStatement( poDS->GetSession() );
        
    oTestCmd.Append( "SELECT COUNT(*) FROM " );
    oTestCmd.Append( poFeatureDefn->GetName() );
    oTestCmd.Append( pszSpatWHERE );

    if( oTestStatement.Execute( oTestCmd.GetString() ) != CE_None )
        bHaveSpatialIndex = FALSE;
    else
        bHaveSpatialIndex = TRUE;
}

/************************************************************************/
/*                             BuildWhere()                             */
/*                                                                      */
/*      Build the WHERE statement appropriate to the current set of     */
/*      criteria (spatial and attribute queries).                       */
/************************************************************************/

void OGROCITableLayer::BuildWhere()

{
    OGROCIStringBuf oWHERE;

    CPLFree( pszWHERE );
    pszWHERE = NULL;

    if( poFilterGeom != NULL && bHaveSpatialIndex )
    {
        OGREnvelope  sEnvelope;

        poFilterGeom->getEnvelope( &sEnvelope );

        oWHERE.Append( " WHERE sdo_filter(" );
        oWHERE.Append( pszGeomName );
        oWHERE.Append( ", MDSYS.SDO_GEOMETRY(2003," );
        if( nSRID == -1 )
            oWHERE.Append( "NULL" );
        else
            oWHERE.Appendf( 15, "%d", nSRID );
        oWHERE.Append( ",NULL," );
        oWHERE.Append( "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3)," );
        oWHERE.Append( "MDSYS.SDO_ORDINATE_ARRAY(" );
        oWHERE.Appendf( 200, "%.16g,%.16g,%.16g,%.16g", 
                        sEnvelope.MinX, sEnvelope.MinY,
                        sEnvelope.MaxX, sEnvelope.MaxY );
        oWHERE.Append( ")), 'querytype=window') = 'TRUE' " );
    }

    if( bHaveSpatialIndex == HSI_UNKNOWN )
    {
        TestForSpatialIndex( oWHERE.GetString() );
        if( !bHaveSpatialIndex )
            oWHERE.Clear();
    }

    if( pszQuery != NULL )
    {
        if( oWHERE.GetLast() == '\0' )
            oWHERE.Append( "WHERE " );
        else
            oWHERE.Append( "AND " );

        oWHERE.Append( pszQuery );
    }

    pszWHERE = oWHERE.StealString();
}

/************************************************************************/
/*                      BuildFullQueryStatement()                       */
/************************************************************************/

void OGROCITableLayer::BuildFullQueryStatement()

{
    if( pszQueryStatement != NULL )
    {
        CPLFree( pszQueryStatement );
        pszQueryStatement = NULL;
    }

    OGROCIStringBuf oCmd;
    char *pszFields = BuildFields();

    oCmd.Append( "SELECT " );
    oCmd.Append( pszFields );
    oCmd.Append( " FROM " );
    oCmd.Append( poFeatureDefn->GetName() );
    oCmd.Append( " " );
    oCmd.Append( pszWHERE );

    pszQueryStatement = oCmd.StealString();

    CPLFree( pszFields );
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGROCITableLayer::GetFeature( long nFeatureId )

{

/* -------------------------------------------------------------------- */
/*      If we don't have an FID column scan for the desired feature.    */
/* -------------------------------------------------------------------- */
    if( pszFIDName == NULL )
        return OGROCILayer::GetFeature( nFeatureId );

/* -------------------------------------------------------------------- */
/*      Clear any existing query.                                       */
/* -------------------------------------------------------------------- */
    ResetReading();

/* -------------------------------------------------------------------- */
/*      Build query for this specific feature.                          */
/* -------------------------------------------------------------------- */
    OGROCIStringBuf oCmd;
    char *pszFields = BuildFields();

    oCmd.Append( "SELECT " );
    oCmd.Append( pszFields );
    oCmd.Append( " FROM " );
    oCmd.Append( poFeatureDefn->GetName() );
    oCmd.Append( " " );
    oCmd.Appendf( 50+strlen(pszFIDName), 
                  " WHERE \"%s\" = %ld ", 
                  pszFIDName, nFeatureId );

/* -------------------------------------------------------------------- */
/*      Execute the statement.                                          */
/* -------------------------------------------------------------------- */
    if( !ExecuteQuery( oCmd.GetString() ) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Get the feature.                                                */
/* -------------------------------------------------------------------- */
    OGRFeature *poFeature;

    poFeature = GetNextRawFeature();
    
    if( poFeature != NULL && poFeature->GetGeometryRef() != NULL )
        poFeature->GetGeometryRef()->assignSpatialReference( poSRS );

/* -------------------------------------------------------------------- */
/*      Cleanup the statement.                                          */
/* -------------------------------------------------------------------- */
    ResetReading();

/* -------------------------------------------------------------------- */
/*      verify the FID.                                                 */
/* -------------------------------------------------------------------- */
    if( poFeature != NULL && poFeature->GetFID() != nFeatureId )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "OGROCITableLayer::GetFeature(%d) ... query returned feature %d instead!",
                  nFeatureId, poFeature->GetFID() );
        delete poFeature;
        return NULL;
    }
    else
        return poFeature;
}

/************************************************************************/
/*                           GetNextFeature()                           */
/*                                                                      */
/*      We override the next feature method because we know that we     */
/*      implement the attribute query within the statement and so we    */
/*      don't have to test here.   Eventually the spatial query will    */
/*      be fully tested within the statement as well.                   */
/************************************************************************/

OGRFeature *OGROCITableLayer::GetNextFeature()

{

    for( ; TRUE; )
    {
        OGRFeature	*poFeature;

        poFeature = GetNextRawFeature();
        if( poFeature == NULL )
        {
            CPLDebug( "OCI", "Query complete, got %d hits, and %d discards.",
                      nHits, nDiscarded );
            nHits = 0;
            nDiscarded = 0;
            return NULL;
        }

        if( poFilterGeom == NULL
            || poFilterGeom->Intersect( poFeature->GetGeometryRef() ) )
        {
            nHits++;
            if( poFeature->GetGeometryRef() != NULL )
                poFeature->GetGeometryRef()->assignSpatialReference( poSRS );
            return poFeature;
        }

        if( poFilterGeom != NULL )
            nDiscarded++;

        delete poFeature;
    }
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGROCITableLayer::ResetReading()

{
    nHits = 0;
    nDiscarded = 0;

    BuildFullQueryStatement();

    OGROCILayer::ResetReading();
}

/************************************************************************/
/*                            BuildFields()                             */
/*                                                                      */
/*      Build list of fields to fetch, performing any required          */
/*      transformations (such as on geometry).                          */
/************************************************************************/

char *OGROCITableLayer::BuildFields()

{
    int		i;
    OGROCIStringBuf oFldList;

    if( pszGeomName )							
    {
        oFldList.Append( "\"" );
        oFldList.Append( pszGeomName );
        oFldList.Append( "\"" );
        iGeomColumn = 0;
    }

    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
    {
        const char *pszName = poFeatureDefn->GetFieldDefn(i)->GetNameRef();

        if( oFldList.GetLast() != '\0' )
            oFldList.Append( "," );

        oFldList.Append( "\"" );
        oFldList.Append( pszName );
        oFldList.Append( "\"" );
    }

    if( pszFIDName != NULL )
    {
        iFIDColumn = poFeatureDefn->GetFieldCount();
        oFldList.Append( ",\"" );
        oFldList.Append( pszFIDName );
        oFldList.Append( "\"" );
    }

    return oFldList.StealString();
}

/************************************************************************/
/*                         SetAttributeFilter()                         */
/************************************************************************/

OGRErr OGROCITableLayer::SetAttributeFilter( const char *pszQuery )

{
    if( (pszQuery == NULL && this->pszQuery == NULL)
        || (pszQuery != NULL && this->pszQuery != NULL
            && strcmp(pszQuery,this->pszQuery) == 0) )
        return OGRERR_NONE;
    
    CPLFree( this->pszQuery );

    if( pszQuery == NULL )
        this->pszQuery = NULL;
    else
        this->pszQuery = CPLStrdup( pszQuery );

    BuildWhere();

    ResetReading();

    return OGRERR_NONE;
}

/************************************************************************/
/*                             SetFeature()                             */
/*                                                                      */
/*      We implement SetFeature() by deleting the existing row (if      */
/*      it exists), and then using CreateFeature() to write it out      */
/*      tot he table normally.  CreateFeature() will preserve the       */
/*      existing FID if possible.                                       */
/************************************************************************/

OGRErr OGROCITableLayer::SetFeature( OGRFeature *poFeature )

{
/* -------------------------------------------------------------------- */
/*      Do some validation.                                             */
/* -------------------------------------------------------------------- */
    if( pszFIDName == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "OGROCITableLayer::SetFeature(%d) failed because there is "
                  "no apparent FID column on table %s.",
                  poFeature->GetFID(), 
                  poFeatureDefn->GetName() );

        return OGRERR_FAILURE;
    }

    if( poFeature->GetFID() == OGRNullFID )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "OGROCITableLayer::SetFeature(%d) failed because the feature "
                  "has no FID!", poFeature->GetFID() );

        return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Prepare the delete command, and execute.  We don't check the    */
/*      error result of the execute, since attempting to Set a          */
/*      non-existing feature may be OK.                                 */
/* -------------------------------------------------------------------- */
    OGROCIStringBuf     oCmdText;
    OGROCIStatement     oCmdStatement( poDS->GetSession() );

    oCmdText.Appendf( strlen(poFeatureDefn->GetName())+strlen(pszFIDName)+100,
                      "DELETE FROM %s WHERE \"%s\" = %d",
                      poFeatureDefn->GetName(), 
                      pszFIDName, 
                      poFeature->GetFID() );

    oCmdStatement.Execute( oCmdText.GetString() );

    return CreateFeature( poFeature );
}

/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/

OGRErr OGROCITableLayer::CreateFeature( OGRFeature *poFeature )

{
    OGROCISession      *poSession = poDS->GetSession();
    char		*pszCommand;
    int                 i, bNeedComma = FALSE;
    unsigned int        nCommandBufSize;;

/* -------------------------------------------------------------------- */
/*      Add extents of this geometry to the existing layer extents.     */
/* -------------------------------------------------------------------- */
    if( poFeature->GetGeometryRef() != NULL )
    {
        OGREnvelope  sThisExtent;
        
        poFeature->GetGeometryRef()->getEnvelope( &sThisExtent );
        sExtent.Merge( sThisExtent );
    }

/* -------------------------------------------------------------------- */
/*      Prepare SQL statement buffer.                                   */
/* -------------------------------------------------------------------- */
    nCommandBufSize = 2000;
    pszCommand = (char *) CPLMalloc(nCommandBufSize);

/* -------------------------------------------------------------------- */
/*      Form the INSERT command.  					*/
/* -------------------------------------------------------------------- */
    sprintf( pszCommand, "INSERT INTO %s (", poFeatureDefn->GetName() );

    if( poFeature->GetGeometryRef() != NULL )
    {
        bNeedComma = TRUE;
        strcat( pszCommand, pszGeomName );
    }
    
    if( pszFIDName != NULL )
    {
        if( bNeedComma )
            strcat( pszCommand, ", " );
        
        strcat( pszCommand, pszFIDName );
        bNeedComma = TRUE;
    }
    

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

    CPLAssert( strlen(pszCommand) < nCommandBufSize );

/* -------------------------------------------------------------------- */
/*      Set the geometry                                                */
/* -------------------------------------------------------------------- */
    bNeedComma = poFeature->GetGeometryRef() != NULL;
    if( poFeature->GetGeometryRef() != NULL)
    {
        OGRGeometry *poGeometry = poFeature->GetGeometryRef();
        char szSDO_GEOMETRY[512];
        char szSRID[128];

        if( nSRID == -1 )
            strcpy( szSRID, "NULL" );
        else
            sprintf( szSRID, "%d", nSRID );

        if( wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
        {
            OGRPoint *poPoint = (OGRPoint *) poGeometry;

            if( poGeometry->getDimension() == 2 )
                sprintf( szSDO_GEOMETRY,
                         "%s(%d,%s,MDSYS.SDO_POINT_TYPE(%.16g,%.16g),NULL,NULL)",
                         SDO_GEOMETRY, 2001, szSRID, 
                         poPoint->getX(), poPoint->getY() );
            else
                sprintf( szSDO_GEOMETRY, 
                         "%s(%d,%s,MDSYS.SDO_POINT_TYPE(%.16g,%.16g,%.16g),NULL,NULL)",
                         SDO_GEOMETRY, 2001, szSRID, 
                         poPoint->getX(), poPoint->getY(), poPoint->getZ() );
        }
        else
        {
            int  nGType;

            if( TranslateToSDOGeometry( poFeature->GetGeometryRef(), &nGType )
                == OGRERR_NONE )
                sprintf( szSDO_GEOMETRY, 
                         "%s(%d,%s,NULL,:elem_info,:ordinates)", 
                         SDO_GEOMETRY, nGType, szSRID );
            else
                sprintf( szSDO_GEOMETRY, "NULL" );
        }

        if( strlen(pszCommand) + strlen(szSDO_GEOMETRY) 
            > nCommandBufSize - 50 )
        {
            nCommandBufSize = 
                strlen(pszCommand) + strlen(szSDO_GEOMETRY) + 10000;
            pszCommand = (char *) CPLRealloc(pszCommand, nCommandBufSize );
        }

        strcat( pszCommand, szSDO_GEOMETRY );
    }

/* -------------------------------------------------------------------- */
/*      Set the FID.                                                    */
/* -------------------------------------------------------------------- */
    int nOffset = strlen(pszCommand);

    if( pszFIDName != NULL )
    {
        long  nFID;

        if( bNeedComma )
            strcat( pszCommand+nOffset, ", " );
        bNeedComma = TRUE;

        nOffset += strlen(pszCommand+nOffset);

        nFID = poFeature->GetFID();
        if( nFID == -1 )
            nFID = iNextFIDToWrite++;

        sprintf( pszCommand+nOffset, "%ld", nFID );
    }

/* -------------------------------------------------------------------- */
/*      Set the other fields.                                           */
/* -------------------------------------------------------------------- */
    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
    {
        if( !poFeature->IsFieldSet( i ) )
            continue;

        OGRFieldDefn *poFldDefn = poFeatureDefn->GetFieldDefn(i);
        const char *pszStrValue = poFeature->GetFieldAsString(i);

        if( bNeedComma )
            strcat( pszCommand+nOffset, ", " );
        else
            bNeedComma = TRUE;

        if( strlen(pszStrValue) + strlen(pszCommand+nOffset) + nOffset 
            > nCommandBufSize-50 )
        {
            nCommandBufSize = strlen(pszCommand) + strlen(pszStrValue) + 10000;
            pszCommand = (char *) CPLRealloc(pszCommand, nCommandBufSize );
        }
        
        if( poFldDefn->GetType() == OFTInteger 
            || poFldDefn->GetType() == OFTReal )
        {
            if( poFldDefn->GetWidth() > 0 && bPreservePrecision
                && (int) strlen(pszStrValue) > poFldDefn->GetWidth() )
            {
                strcat( pszCommand+nOffset, "NULL" );
                ReportTruncation( poFldDefn );
            }
            else
                strcat( pszCommand+nOffset, pszStrValue );
        }
        else 
        {
            int		iChar;

            /* We need to quote and escape string fields. */
            strcat( pszCommand+nOffset, "'" );

            nOffset += strlen(pszCommand+nOffset);
            
            for( iChar = 0; pszStrValue[iChar] != '\0'; iChar++ )
            {
                if( poFldDefn->GetWidth() != 0 && bPreservePrecision
                    && iChar >= poFldDefn->GetWidth() )
                {
                    ReportTruncation( poFldDefn );
                    break;
                }

                if( pszStrValue[iChar] == '\'' )
                {
                    pszCommand[nOffset++] = '\'';
                    pszCommand[nOffset++] = pszStrValue[iChar];
                }
                else
                    pszCommand[nOffset++] = pszStrValue[iChar];
            }
            pszCommand[nOffset] = '\0';
            
            strcat( pszCommand+nOffset, "'" );
        }
        nOffset += strlen(pszCommand+nOffset);
    }

    strcat( pszCommand+nOffset, ")" );

/* -------------------------------------------------------------------- */
/*      Prepare statement.                                              */
/* -------------------------------------------------------------------- */
    OGROCIStatement oInsert( poSession );
    int  bHaveOrdinates = strstr(pszCommand,":ordinates") != NULL;
    int  bHaveElemInfo = strstr(pszCommand,":elem_info") != NULL;

    if( oInsert.Prepare( pszCommand ) != CE_None )
    {
        CPLFree( pszCommand );
        return OGRERR_FAILURE;
    }

    CPLFree( pszCommand );

/* -------------------------------------------------------------------- */
/*      Bind and translate the elem_info if we have some.               */
/* -------------------------------------------------------------------- */
    if( bHaveElemInfo )
    {
        OCIBind *hBindOrd = NULL;
        int i;
        OCINumber oci_number; 

        // Create or clear VARRAY 
        if( hElemInfoVARRAY == NULL )
        {
            if( poSession->Failed(
                OCIObjectNew( poSession->hEnv, poSession->hError, 
                              poSession->hSvcCtx, OCI_TYPECODE_VARRAY,
                              poSession->hElemInfoTDO, (dvoid *)NULL, 
                              OCI_DURATION_SESSION,
                              FALSE, (dvoid **)&hElemInfoVARRAY),
                "OCIObjectNew(hElemInfoVARRAY)") )
                return OGRERR_FAILURE;
        }
        else
        {
            sb4  nOldCount;

            OCICollSize( poSession->hEnv, poSession->hError, 
                         hElemInfoVARRAY, &nOldCount );
            OCICollTrim( poSession->hEnv, poSession->hError, 
                         nOldCount, hElemInfoVARRAY );
        }

        // Prepare the VARRAY of ordinate values. 
	for (i = 0; i < nElemInfoCount; i++)
	{
            if( poSession->Failed( 
                OCINumberFromInt( poSession->hError, 
                                  (dvoid *) (panElemInfo + i),
                                  (uword)sizeof(int),
                                  OCI_NUMBER_SIGNED,
                                  &oci_number),
                "OCINumberFromInt") )
                return OGRERR_FAILURE;

            if( poSession->Failed( 
                OCICollAppend( poSession->hEnv, poSession->hError,
                               (dvoid *) &oci_number,
                               (dvoid *)0, hElemInfoVARRAY),
                "OCICollAppend") )
                return OGRERR_FAILURE;
	}

        // Do the binding.
        if( poSession->Failed( 
            OCIBindByName( oInsert.GetStatement(), &hBindOrd, 
                           poSession->hError,
                           (text *) ":elem_info", (sb4) -1, (dvoid *) 0, 
                           (sb4) 0, SQLT_NTY, (dvoid *)0, (ub2 *)0, 
                           (ub2 *)0, (ub4)0, (ub4 *)0, 
                           (ub4)OCI_DEFAULT),
            "OCIBindByName(:elem_info)") )
            return OGRERR_FAILURE;

        if( poSession->Failed(
            OCIBindObject( hBindOrd, poSession->hError, 
                           poSession->hElemInfoTDO,
                           (dvoid **)&hElemInfoVARRAY, (ub4 *)0, 
                           (dvoid **)0, (ub4 *)0),
            "OCIBindObject(:elem_info)" ) )
            return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Bind and translate the ordinates if we have some.               */
/* -------------------------------------------------------------------- */
    if( bHaveOrdinates )
    {
        OCIBind *hBindOrd = NULL;
        int i;
        OCINumber oci_number; 

        // Create or clear VARRAY 
        if( hOrdVARRAY == NULL )
        {
            if( poSession->Failed(
                OCIObjectNew( poSession->hEnv, poSession->hError, 
                              poSession->hSvcCtx, OCI_TYPECODE_VARRAY,
                              poSession->hOrdinatesTDO, (dvoid *)NULL, 
                              OCI_DURATION_SESSION,
                              FALSE, (dvoid **)&hOrdVARRAY),
                "OCIObjectNew(hOrdVARRAY)") )
                return OGRERR_FAILURE;
        }
        else
        {
            sb4  nOldCount;

            OCICollSize( poSession->hEnv, poSession->hError, 
                         hOrdVARRAY, &nOldCount );
            OCICollTrim( poSession->hEnv, poSession->hError, 
                         nOldCount, hOrdVARRAY );
        }

        // Prepare the VARRAY of ordinate values. 
	for (i = 0; i < nOrdinalCount; i++)
	{
            if( poSession->Failed( 
                OCINumberFromReal( poSession->hError, 
                                   (dvoid *) (padfOrdinals + i),
                                   (uword)sizeof(double),
                                   &oci_number),
                "OCINumberFromReal") )
                return OGRERR_FAILURE;

            if( poSession->Failed( 
                OCICollAppend( poSession->hEnv, poSession->hError,
                               (dvoid *) &oci_number,
                               (dvoid *)0, hOrdVARRAY),
                "OCICollAppend") )
                return OGRERR_FAILURE;
	}

        // Do the binding.
        if( poSession->Failed( 
            OCIBindByName( oInsert.GetStatement(), &hBindOrd, 
                           poSession->hError,
                           (text *) ":ordinates", (sb4) -1, (dvoid *) 0, 
                           (sb4) 0, SQLT_NTY, (dvoid *)0, (ub2 *)0, 
                           (ub2 *)0, (ub4)0, (ub4 *)0, 
                           (ub4)OCI_DEFAULT),
            "OCIBindByName(:ordinates)") )
            return OGRERR_FAILURE;

        if( poSession->Failed(
            OCIBindObject( hBindOrd, poSession->hError, 
                           poSession->hOrdinatesTDO,
                           (dvoid **)&hOrdVARRAY, (ub4 *)0, 
                           (dvoid **)0, (ub4 *)0),
            "OCIBindObject(:ordinates)" ) )
            return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Execute the insert.                                             */
/* -------------------------------------------------------------------- */
    if( oInsert.Execute( NULL ) != CE_None )
        return OGRERR_FAILURE;
    else
        return OGRERR_NONE;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGROCITableLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return bUpdateAccess;

    else if( EQUAL(pszCap,OLCCreateField) )
        return bUpdateAccess;

    else 
        return OGROCILayer::TestCapability( pszCap );
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/*                                                                      */
/*      If a spatial filter is in effect, we turn control over to       */
/*      the generic counter.  Otherwise we return the total count.      */
/*      Eventually we should consider implementing a more efficient     */
/*      way of counting features matching a spatial query.              */
/************************************************************************/

int OGROCITableLayer::GetFeatureCount( int bForce )

{
/* -------------------------------------------------------------------- */
/*      Use a more brute force mechanism if we have a spatial query     */
/*      in play.                                                        */
/* -------------------------------------------------------------------- */
    if( poFilterGeom != NULL )
        return OGROCILayer::GetFeatureCount( bForce );

/* -------------------------------------------------------------------- */
/*      In theory it might be wise to cache this result, but it         */
/*      won't be trivial to work out the lifetime of the value.         */
/*      After all someone else could be adding records from another     */
/*      application when working against a database.                    */
/* -------------------------------------------------------------------- */
    OGROCISession      *poSession = poDS->GetSession();
    OGROCIStatement    oGetCount( poSession );
    char               szCommand[1024];
    char               **papszResult;

    sprintf( szCommand, "SELECT COUNT(*) FROM %s %s", 
             poFeatureDefn->GetName(), pszWHERE );

    oGetCount.Execute( szCommand );

    papszResult = oGetCount.SimpleFetchRow();

    if( CSLCount(papszResult) < 1 )
    {
        CPLDebug( "OCI", "Fast get count failed, doing hard way." );
        return OGROCILayer::GetFeatureCount( bForce );
    }
    
    return atoi(papszResult[0]);
}

/************************************************************************/
/*                          FinalizeNewLayer()                          */
/*                                                                      */
/*      Our main job here is to update the USER_SDO_GEOM_METADATA       */
/*      table to include the correct array of dimension object with     */
/*      the appropriate extents for this layer.  We may also do         */
/*      spatial indexing at this point.                                 */
/************************************************************************/

void OGROCITableLayer::FinalizeNewLayer()

{
    OGROCIStringBuf  sDimUpdate;

/* -------------------------------------------------------------------- */
/*      If the dimensions are degenerate (all zeros) then we assume     */
/*      there were no geometries, and we don't bother setting the       */
/*      dimensions.                                                     */
/* -------------------------------------------------------------------- */
    if( sExtent.MaxX == 0 && sExtent.MinX == 0
        && sExtent.MaxY == 0 && sExtent.MinY == 0 )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "Layer %s appears to have no geometry ... not setting SDO DIMINFO metadata.", 
                  poFeatureDefn->GetName() );
        return;
                  
    }

/* -------------------------------------------------------------------- */
/*      Establish the extents and resolution to use.                    */
/* -------------------------------------------------------------------- */
    double           dfResSize;
    double           dfXMin, dfXMax, dfXRes;
    double           dfYMin, dfYMax, dfYRes;
    double           dfZMin, dfZMax, dfZRes;

    if( sExtent.MaxX - sExtent.MinX > 400 )
        dfResSize = 0.001;
    else
        dfResSize = 0.0000001;

    dfXMin = sExtent.MinX - dfResSize * 3;
    dfXMax = sExtent.MaxX + dfResSize * 3;
    dfXRes = dfResSize;
    ParseDIMINFO( "DIMINFO_X", &dfXMin, &dfXMax, &dfXRes );
    
    dfYMin = sExtent.MinY - dfResSize * 3;
    dfYMax = sExtent.MaxY + dfResSize * 3;
    dfYRes = dfResSize;
    ParseDIMINFO( "DIMINFO_Y", &dfYMin, &dfYMax, &dfYRes );
    
    dfZMin = -100000.0;
    dfZMax = 100000.0;
    dfZRes = 0.002;
    ParseDIMINFO( "DIMINFO_Z", &dfZMin, &dfZMax, &dfZRes );
    
/* -------------------------------------------------------------------- */
/*      Prepare dimension update statement.                             */
/* -------------------------------------------------------------------- */
    sDimUpdate.Append( "UPDATE USER_SDO_GEOM_METADATA SET DIMINFO = " );
    sDimUpdate.Append( "MDSYS.SDO_DIM_ARRAY(" );

    sDimUpdate.Appendf(200,
                       "MDSYS.SDO_DIM_ELEMENT('X',%.16g,%.16g,%.12g)",
                       dfXMin, dfXMax, dfXRes );
    sDimUpdate.Appendf(200,
                       ",MDSYS.SDO_DIM_ELEMENT('Y',%.16g,%.16g,%.12g)",
                       dfYMin, dfYMax, dfYRes );

    if( nDimension == 3 )
    {
        sDimUpdate.Appendf(200,
                           ",MDSYS.SDO_DIM_ELEMENT('Z',%.16g,%.16g,%.12g)",
                           dfZMin, dfZMax, dfZRes );
    }

    sDimUpdate.Append( ")" );

    sDimUpdate.Appendf( strlen(poFeatureDefn->GetName()) + 100,
                        " WHERE table_name = '%s'", 
                        poFeatureDefn->GetName() );

/* -------------------------------------------------------------------- */
/*      Execute the metadata update.                                    */
/* -------------------------------------------------------------------- */
    OGROCIStatement oExecStatement( poDS->GetSession() );

    if( oExecStatement.Execute( sDimUpdate.GetString() ) != CE_None )
        return;

/* -------------------------------------------------------------------- */
/*      If the user has disabled INDEX support then don't create the    */
/*      index.                                                          */
/* -------------------------------------------------------------------- */
    if( !CSLFetchBoolean( papszOptions, "INDEX", TRUE ) )
        return;

/* -------------------------------------------------------------------- */
/*      Establish an index name.  For some reason Oracle 8.1.7 does     */
/*      not support spatial index names longer than 18 characters so    */
/*      we magic up an index name if it would be too long.              */
/* -------------------------------------------------------------------- */
    char  szIndexName[20];

    if( strlen(poFeatureDefn->GetName()) < 15 )
        sprintf( szIndexName, "%s_idx", poFeatureDefn->GetName() );
    else if( strlen(poFeatureDefn->GetName()) < 17 )
        sprintf( szIndexName, "%si", poFeatureDefn->GetName() );
    else
    {
        int i, nHash = 0;
        const char *pszSrcName = poFeatureDefn->GetName();

        for( i = 0; pszSrcName[i] != '\0'; i++ )
            nHash = (nHash + i * pszSrcName[i]) % 987651;
        
        sprintf( szIndexName, "OSI_%d", nHash );
    }

    poDS->GetSession()->CleanName( szIndexName );

/* -------------------------------------------------------------------- */
/*      Try creating an index on the table now.  Use a simple 5         */
/*      level quadtree based index.  Would R-tree be a better default?  */
/* -------------------------------------------------------------------- */

// Disable for now, spatial index creation always seems to cause me to 
// lose my connection to the database!
    OGROCIStringBuf  sIndexCmd;

    sIndexCmd.Appendf( 10000, "CREATE INDEX \"%s\" ON %s(\"%s\") "
                       "INDEXTYPE IS MDSYS.SPATIAL_INDEX ",
                       szIndexName, 
                       poFeatureDefn->GetName(), 
                       pszGeomName );

    if( CSLFetchNameValue( papszOptions, "INDEX_PARAMETERS" ) != NULL )
    {
        sIndexCmd.Append( " PARAMETERS( '" );
        sIndexCmd.Append( CSLFetchNameValue(papszOptions,"INDEX_PARAMETERS") );
        sIndexCmd.Append( "' )" );
    }

    if( oExecStatement.Execute( sIndexCmd.GetString() ) != CE_None )
    {
        char szDropCommand[2000];
        sprintf( szDropCommand, "DROP INDEX \"%s\"", szIndexName );
        oExecStatement.Execute( szDropCommand );
    }
}

