/******************************************************************************
 * $Id$
 *
 * Project:  OGR
 * Purpose:  Implements OGRGMLLayer class.
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
 * Revision 1.7  2003/03/06 20:30:28  warmerda
 * use GML/OGR geometry translations from ogr_geometry.h now
 *
 * Revision 1.6  2003/01/17 20:39:40  warmerda
 * added bounding rectangle support
 *
 * Revision 1.5  2003/01/10 16:23:11  warmerda
 * assign FIDs if not provided in CreateFeature()
 *
 * Revision 1.4  2002/04/24 19:32:07  warmerda
 * Make a copy of the passed OGRSpatialReference in the constructor.
 *
 * Revision 1.3  2002/03/06 20:08:24  warmerda
 * add accelerated GetFeatureCount and GetExtent
 *
 * Revision 1.2  2002/01/25 21:00:31  warmerda
 * fix some small bugs found by MS VC++
 *
 * Revision 1.1  2002/01/25 20:37:02  warmerda
 * New
 *
 */

#include "ogr_gml.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           OGRGMLLayer()                              */
/************************************************************************/

OGRGMLLayer::OGRGMLLayer( const char * pszName,
                          OGRSpatialReference *poSRSIn, int bWriterIn,
                          OGRwkbGeometryType eReqType,
                          OGRGMLDataSource *poDSIn )

{
    poFilterGeom = NULL;

    if( poSRSIn == NULL )
        poSRS = NULL;
    else
        poSRS = poSRSIn->Clone();
    
    iNextGMLId = 0;
    nTotalGMLCount = -1;
    
    poDS = poDSIn;

    poFeatureDefn = new OGRFeatureDefn( pszName );
    poFeatureDefn->SetGeomType( eReqType );

    bWriter = bWriterIn;

/* -------------------------------------------------------------------- */
/*      Reader's should get the corresponding GMLFeatureClass and       */
/*      cache it.                                                       */
/* -------------------------------------------------------------------- */
    if( !bWriter )
        poFClass = poDS->GetReader()->GetClass( pszName );
    else
        poFClass = NULL;
}

/************************************************************************/
/*                           ~OGRGMLLayer()                           */
/************************************************************************/

OGRGMLLayer::~OGRGMLLayer()

{
    delete poFeatureDefn;

    if( poSRS != NULL )
        delete poSRS;

    if( poFilterGeom != NULL )
        delete poFilterGeom;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRGMLLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

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

void OGRGMLLayer::ResetReading()

{
    iNextGMLId = 0;
    poDS->GetReader()->ResetReading();
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRGMLLayer::GetNextFeature()

{
    GMLFeature	*poGMLFeature;

    if( iNextGMLId == 0 )
        ResetReading();

/* -------------------------------------------------------------------- */
/*      Get the next GML feature of our class.                          */
/* -------------------------------------------------------------------- */
    poGMLFeature = poDS->GetReader()->NextFeature();

    while( poGMLFeature != NULL && poGMLFeature->GetClass() != poFClass )
    {
        delete poGMLFeature;
        poGMLFeature = poDS->GetReader()->NextFeature();
    }
    
    if( poGMLFeature == NULL )
        return NULL;
    
/* -------------------------------------------------------------------- */
/*      Create a corresponding OGR feature.                             */
/* -------------------------------------------------------------------- */
    OGRFeature *poOGRFeature = new OGRFeature( GetLayerDefn() );
    int	iField;

    poOGRFeature->SetFID( iNextGMLId++ );

    if( poGMLFeature->GetGeometry() != NULL )
        poOGRFeature->SetGeometryDirectly( 
            OGRGeometryFactory::createFromGML( poGMLFeature->GetGeometry() ) );

    for( iField = 0; iField < poFClass->GetPropertyCount(); iField++ )
    {
        const char *pszProperty = poGMLFeature->GetProperty( iField );

        if( pszProperty != NULL )
            poOGRFeature->SetField( iField, pszProperty );
    }

/* -------------------------------------------------------------------- */
/*      Cleanup and return.                                             */
/* -------------------------------------------------------------------- */
    delete poGMLFeature;

    return poOGRFeature;
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/************************************************************************/

int OGRGMLLayer::GetFeatureCount( int bForce )

{
    if( poFClass == NULL )
        return 0;

    if( poFilterGeom != NULL || m_poAttrQuery != NULL )
        return OGRLayer::GetFeatureCount( bForce );
    else
        return poFClass->GetFeatureCount();
}

/************************************************************************/
/*                             GetExtent()                              */
/************************************************************************/

OGRErr OGRGMLLayer::GetExtent(OGREnvelope *psExtent, int bForce )

{
    double dfXMin, dfXMax, dfYMin, dfYMax;

    if( poFClass != NULL && 
        poFClass->GetExtents( &dfXMin, &dfXMax, &dfYMin, &dfYMax ) )
    {
        psExtent->MinX = dfXMin;
        psExtent->MaxX = dfXMax;
        psExtent->MinY = dfYMin;
        psExtent->MaxY = dfYMax;

        return OGRERR_NONE;
    }
    else 
        return OGRLayer::GetExtent( psExtent, bForce );
}

/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/

OGRErr OGRGMLLayer::CreateFeature( OGRFeature *poFeature )

{
    FILE	*fp = poDS->GetOutputFP();

    if( !bWriter )
        return OGRERR_FAILURE;

    VSIFPrintf( fp, "  <gml:featureMember>\n" );

    if( poFeature->GetFID() == OGRNullFID )
        poFeature->SetFID( iNextGMLId++ );

    VSIFPrintf( fp, "    <%s fid=\"%d\">\n", 
                poFeatureDefn->GetName(),
                poFeature->GetFID() );

    // Write all "set" fields. 
    for( int iField = 0; iField < poFeatureDefn->GetFieldCount(); iField++ )
    {
        
        OGRFieldDefn *poField = poFeatureDefn->GetFieldDefn( iField );

        if( poFeature->IsFieldSet( iField ) )
            VSIFPrintf( fp, "      <%s>%s</%s>\n", 
                        poField->GetNameRef(), 
                        poFeature->GetFieldAsString( iField ), 
                        poField->GetNameRef() );
    }

    // Write out Geometry - for now it isn't indented properly.
    if( poFeature->GetGeometryRef() != NULL )
    {
        char	*pszGeometry;
        OGREnvelope sGeomBounds;

        pszGeometry = poFeature->GetGeometryRef()->exportToGML();
        VSIFPrintf( fp, "      <gml:geometryProperty>%s</gml:geometryProperty>\n",
                    pszGeometry );
        CPLFree( pszGeometry );

        poFeature->GetGeometryRef()->getEnvelope( &sGeomBounds );
        poDS->GrowExtents( &sGeomBounds );
    }

    VSIFPrintf( fp, "    </%s>\n", poFeatureDefn->GetName() );
    VSIFPrintf( fp, "  </gml:featureMember>\n" );

    return OGRERR_NONE;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRGMLLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCSequentialWrite) )
        return bWriter;

    else if( EQUAL(pszCap,OLCCreateField) )
        return bWriter && iNextGMLId == 0;

    else if( EQUAL(pszCap,OLCFastGetExtent) )
    {
        double	dfXMin, dfXMax, dfYMin, dfYMax;

        if( poFClass == NULL )
            return FALSE;

        return poFClass->GetExtents( &dfXMin, &dfXMax, &dfYMin, &dfYMax );
    }

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
    {
        if( poFClass == NULL || poFilterGeom != NULL || m_poAttrQuery != NULL )
            return FALSE;

        return poFClass->GetFeatureCount() != -1;
    }

    else 
        return FALSE;
}

/************************************************************************/
/*                            CreateField()                             */
/************************************************************************/

OGRErr OGRGMLLayer::CreateField( OGRFieldDefn *poField, int bApproxOK )

{
    int		iNewField;

    if( !bWriter || iNextGMLId != 0 )
        return OGRERR_FAILURE;

    poFeatureDefn->AddFieldDefn( poField );

    return OGRERR_NONE;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRGMLLayer::GetSpatialRef()

{
    return poSRS;
}

