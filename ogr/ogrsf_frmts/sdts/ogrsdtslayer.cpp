/******************************************************************************
 * $Id$
 *
 * Project:  SDTSReader
 * Purpose:  Implements OGRSDTSLayer class.
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
 * Revision 1.1  1999/09/22 13:32:16  warmerda
 * New
 *
 */

#include "ogr_sdts.h"
#include "cpl_conv.h"

/************************************************************************/
/*                            OGRSDTSLayer()                            */
/*                                                                      */
/*      Note that the OGRSDTSLayer assumes ownership of the passed      */
/*      OGRFeatureDefn object.                                          */
/************************************************************************/

OGRSDTSLayer::OGRSDTSLayer( SDTSTransfer * poTransferIn, int iLayerIn )

{
    poFilterGeom = NULL;

    poTransfer = poTransferIn;
    iLayer = iLayerIn;

    poReader = poTransfer->GetLayerIndexedReader( iLayer );

/* -------------------------------------------------------------------- */
/*      Define the schema.                                              */
/* -------------------------------------------------------------------- */
    int		iCATDEntry = poTransfer->GetLayerCATDEntry( iLayer );
    
    poFeatureDefn =
        new OGRFeatureDefn(poTransfer->GetCATD()->GetEntryModule(iCATDEntry));

    OGRFieldDefn oRecId( "RCID", OFTInteger );
    poFeatureDefn->AddFieldDefn( &oRecId );

    if( poTransfer->GetLayerType(iLayer) == SLTPoint )
    {
        poFeatureDefn->SetGeomType( wkbPoint );
    }
    else if( poTransfer->GetLayerType(iLayer) == SLTLine )
    {
        poFeatureDefn->SetGeomType( wkbLineString );
    }
    else if( poTransfer->GetLayerType(iLayer) == SLTPoly )
    {
        poFeatureDefn->SetGeomType( wkbPolygon );
    }
    else if( poTransfer->GetLayerType(iLayer) == SLTAttr )
    {
        poFeatureDefn->SetGeomType( wkbNone );
    }
}

/************************************************************************/
/*                           ~OGRSDTSLayer()                           */
/************************************************************************/

OGRSDTSLayer::~OGRSDTSLayer()

{
    delete poFeatureDefn;

    if( poFilterGeom != NULL )
        delete poFilterGeom;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRSDTSLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

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

void OGRSDTSLayer::ResetReading()

{
    poReader->Rewind();
}

/************************************************************************/
/*                      GetNextUnfilteredFeature()                      */
/************************************************************************/

OGRFeature * OGRSDTSLayer::GetNextUnfilteredFeature()

{
/* -------------------------------------------------------------------- */
/*      If not done before we need to assemble the geometry for a       */
/*      polygon layer.                                                  */
/* -------------------------------------------------------------------- */
    if( poTransfer->GetLayerType(iLayer) == SLTPoly )
    {
        ((SDTSPolygonReader *) poReader)->AssembleRings(poTransfer);
    }

/* -------------------------------------------------------------------- */
/*      Fetch the next sdts style feature object from the reader.       */
/* -------------------------------------------------------------------- */
    SDTSFeature	*poSDTSFeature = poReader->GetNextFeature();
    OGRFeature  *poFeature;

    if( poSDTSFeature == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create the OGR feature.                                         */
/* -------------------------------------------------------------------- */
    poFeature = new OGRFeature( poFeatureDefn );

    switch( poTransfer->GetLayerType(iLayer) )
    {
/* -------------------------------------------------------------------- */
/*      Translate point feature specific information and geometry.      */
/* -------------------------------------------------------------------- */
      case SLTPoint:
      {
          SDTSRawPoint	*poPoint = (SDTSRawPoint *) poSDTSFeature;
          
          poFeature->SetGeometryDirectly( new OGRPoint( poPoint->dfX,
                                                        poPoint->dfY,
                                                        poPoint->dfZ ) );
      }
      break;

/* -------------------------------------------------------------------- */
/*      Translate line feature specific information and geometry.       */
/* -------------------------------------------------------------------- */
      case SLTLine:
      {
          SDTSRawLine	*poLine = (SDTSRawLine *) poSDTSFeature;
          OGRLineString *poOGRLine = new OGRLineString();

          poOGRLine->setPoints( poLine->nVertices,
                                poLine->padfX, poLine->padfY, poLine->padfZ );
          poFeature->SetGeometryDirectly( poOGRLine );
      }
      break;

/* -------------------------------------------------------------------- */
/*      Translate polygon feature specific information and geometry.    */
/* -------------------------------------------------------------------- */
      case SLTPoly:
      {
          SDTSRawPolygon *poPoly = (SDTSRawPolygon *) poSDTSFeature;
          OGRPolygon *poOGRPoly = new OGRPolygon();

          for( int iRing = 0; iRing < poPoly->nRings; iRing++ )
          {
              OGRLinearRing *poRing = new OGRLinearRing();
              int	    nVertices;

              if( iRing == poPoly->nRings - 1 )
                  nVertices = poPoly->nVertices - poPoly->panRingStart[iRing];
              else
                  nVertices = poPoly->panRingStart[iRing+1]
                            - poPoly->panRingStart[iRing];

              poRing->setPoints( nVertices,
                                 poPoly->padfX + poPoly->panRingStart[iRing],
                                 poPoly->padfY + poPoly->panRingStart[iRing],
                                 poPoly->padfZ + poPoly->panRingStart[iRing] );

              poOGRPoly->addRingDirectly( poRing );
          }

          poFeature->SetGeometryDirectly( poOGRPoly );
      }
      break;

      default:
        break;
    }

/* -------------------------------------------------------------------- */
/*      Translate the record id.                                        */
/* -------------------------------------------------------------------- */
    poFeature->SetFID( poSDTSFeature->oModId.nRecord );
    poFeature->SetField( 0, (int) poSDTSFeature->oModId.nRecord );

    if( !poReader->IsIndexed() )
        delete poSDTSFeature;

    return poFeature;
}


/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRSDTSLayer::GetNextFeature()

{
    OGRFeature	*poFeature = NULL;
    
/* -------------------------------------------------------------------- */
/*      Read features till we find one that satisfies our current       */
/*      spatial criteria.                                               */
/* -------------------------------------------------------------------- */
    while( TRUE )
    {
        poFeature = GetNextUnfilteredFeature();
        if( poFeature == NULL
            || poFilterGeom == NULL
            || poFeature->GetGeometryRef() == NULL 
            || poFilterGeom->Intersect( poFeature->GetGeometryRef() ) )
            break;

        delete poFeature;
    }

    return poFeature;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRSDTSLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return FALSE;

    else if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return FALSE;

    else 
        return FALSE;
}

