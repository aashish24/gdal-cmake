/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRDGNLayer class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam (warmerdam@pobox.com)
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
 * Revision 1.14  2002/01/18 18:52:21  warmerda
 * set rotation angle for labels
 *
 * Revision 1.13  2002/01/15 06:39:56  warmerda
 * remove _gv_color, flesh out pen and brush style settings
 *
 * Revision 1.12  2002/01/09 14:12:12  warmerda
 * Treat SHAPE elements as polygon geometries, not linestrings.
 *
 * Revision 1.11  2001/11/09 18:14:36  warmerda
 * added size info for LABELs
 *
 * Revision 1.10  2001/11/09 15:59:23  warmerda
 * set style information for text, drop _gv_ogrfs
 *
 * Revision 1.9  2001/11/06 14:44:41  warmerda
 * Removed printf() statement.
 *
 * Revision 1.8  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.7  2001/06/19 15:50:23  warmerda
 * added feature attribute query support
 *
 * Revision 1.6  2001/03/07 19:29:46  warmerda
 * added support for stroking curves
 *
 * Revision 1.5  2001/03/07 15:20:13  warmerda
 * Only apply the _gv_color property if the color lookup is successful.
 *
 * Revision 1.4  2001/01/16 21:19:29  warmerda
 * Added preliminary text support
 *
 * Revision 1.3  2001/01/16 18:11:12  warmerda
 * majorly extended, added arc support
 *
 * Revision 1.2  2000/12/28 21:29:17  warmerda
 * use stype field
 *
 * Revision 1.1  2000/11/28 19:03:47  warmerda
 * New
 *
 */

#include "ogr_dgn.h"
#include "cpl_conv.h"
#include "ogr_featurestyle.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           OGRDGNLayer()                              */
/************************************************************************/

OGRDGNLayer::OGRDGNLayer( const char * pszName, DGNHandle hDGN )
    
{
    poFilterGeom = NULL;
    
    this->hDGN = hDGN;

    poFeatureDefn = new OGRFeatureDefn( pszName );
    
    OGRFieldDefn	oField( "", OFTInteger );

/* -------------------------------------------------------------------- */
/*      Element type                                                    */
/* -------------------------------------------------------------------- */
    oField.SetName( "DGNId" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 8 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      Element type                                                    */
/* -------------------------------------------------------------------- */
    oField.SetName( "Type" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 2 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*	Level number.							*/
/* -------------------------------------------------------------------- */
    oField.SetName( "Level" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 2 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      graphic group                                                   */
/* -------------------------------------------------------------------- */
    oField.SetName( "GraphicGroup" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 4 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      ColorIndex                                                      */
/* -------------------------------------------------------------------- */
    oField.SetName( "ColorIndex" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 3 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      Weight                                                          */
/* -------------------------------------------------------------------- */
    oField.SetName( "Weight" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 2 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      Style                                                           */
/* -------------------------------------------------------------------- */
    oField.SetName( "Style" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 1 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );
}

/************************************************************************/
/*                           ~OGRDGNLayer()                             */
/************************************************************************/

OGRDGNLayer::~OGRDGNLayer()

{
    delete poFeatureDefn;

    if( poFilterGeom != NULL )
        delete poFilterGeom;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRDGNLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

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

void OGRDGNLayer::ResetReading()

{
    iNextShapeId = 0;
    DGNRewind( hDGN );
}

/************************************************************************/
/*                          ElementToFeature()                          */
/************************************************************************/

OGRFeature *OGRDGNLayer::ElementToFeature( DGNElemCore *psElement )

{
    OGRFeature	*poFeature = new OGRFeature( poFeatureDefn );

    poFeature->SetFID( psElement->element_id );
    poFeature->SetField( "DGNId", psElement->element_id );
    poFeature->SetField( "Type", psElement->type );
    poFeature->SetField( "Level", psElement->level );
    poFeature->SetField( "GraphicGroup", psElement->graphic_group );
    poFeature->SetField( "ColorIndex", psElement->color );
    poFeature->SetField( "Weight", psElement->weight );
    poFeature->SetField( "Style", psElement->style );

    char	gv_color[128];
    int		gv_red, gv_green, gv_blue;
    char        szFSColor[128], szPen[256];

/* -------------------------------------------------------------------- */
/*      Lookup color.                                                   */
/* -------------------------------------------------------------------- */
    szFSColor[0] = '\0';
    if( DGNLookupColor( hDGN, psElement->color, 
                        &gv_red, &gv_green, &gv_blue ) )
    {
        sprintf( gv_color, "%f %f %f 1.0", 
                 gv_red / 255.0, gv_green / 255.0, gv_blue / 255.0 );

        sprintf( szFSColor, "c:#%02x%02x%02x", 
                 gv_red, gv_green, gv_blue );
    }

/* -------------------------------------------------------------------- */
/*      Generate corresponding PEN style.                               */
/* -------------------------------------------------------------------- */
    if( psElement->style == DGNS_SOLID )
        sprintf( szPen, "PEN(id:\"ogr-pen-0\"" );
    else if( psElement->style == DGNS_DOTTED )
        sprintf( szPen, "PEN(id:\"ogr-pen-5\"" );
    else if( psElement->style == DGNS_MEDIUM_DASH )
        sprintf( szPen, "PEN(id:\"ogr-pen-2\"" );
    else if( psElement->style == DGNS_LONG_DASH )
        sprintf( szPen, "PEN(id:\"ogr-pen-4\"" );
    else if( psElement->style == DGNS_DOT_DASH )
        sprintf( szPen, "PEN(id:\"ogr-pen-6\"" );
    else if( psElement->style == DGNS_SHORT_DASH )
        sprintf( szPen, "PEN(id:\"ogr-pen-3\"" );
    else if( psElement->style == DGNS_DASH_DOUBLE_DOT )
        sprintf( szPen, "PEN(id:\"ogr-pen-7\"" );
    else if( psElement->style == DGNS_LONG_DASH_SHORT_DASH )
        sprintf( szPen, "PEN(p:\"10px 5px 4px 5px\"" );
    else
        sprintf( szPen, "PEN(id:\"ogr-pen-0\"" );

    if( strlen(szFSColor) > 0 )
        sprintf( szPen+strlen(szPen), ",%s", szFSColor );

    if( psElement->weight > 1 )
        sprintf( szPen+strlen(szPen), ",w:%dpx", psElement->weight );
        
    strcat( szPen, ")" );

    switch( psElement->stype )
    {
      case DGNST_MULTIPOINT:
        if( psElement->type == DGNT_SHAPE )
        {
            OGRLinearRing	*poLine = new OGRLinearRing();
            OGRPolygon          *poPolygon = new OGRPolygon();
            DGNElemMultiPoint *psEMP = (DGNElemMultiPoint *) psElement;
            char		szFullStyle[256];
            int                 nFillColor;
            
            poLine->setNumPoints( psEMP->num_vertices );
            for( int i = 0; i < psEMP->num_vertices; i++ )
            {
                poLine->setPoint( i, 
                                  psEMP->vertices[i].x,
                                  psEMP->vertices[i].y,
                                  psEMP->vertices[i].z );
            }

            poPolygon->addRingDirectly( poLine );

            poFeature->SetGeometryDirectly( poPolygon );

            if( DGNGetShapeFillInfo( hDGN, psElement, &nFillColor ) 
                && DGNLookupColor( hDGN, nFillColor, 
                                   &gv_red, &gv_green, &gv_blue ) )
            {
                sprintf( szFullStyle, 
                         "BRUSH(fc:#%02x%02x%02x,id:\"ogr-brush-0\")",
                         gv_red, gv_green, gv_blue );

                if( nFillColor != psElement->color )
                {
                    strcat( szFullStyle, ";" );
                    strcat( szFullStyle, szPen );
                }
                poFeature->SetStyleString( szFullStyle );

                OGRStyleMgr oSMgr;
                if( !oSMgr.InitStyleString( szFullStyle ) )
                    CPLAssert( FALSE );
            }
            else
                poFeature->SetStyleString( szPen );
        }
        else if( psElement->type == DGNT_CURVE )
        {
            DGNElemMultiPoint *psEMP = (DGNElemMultiPoint *) psElement;
            OGRLineString	*poLine = new OGRLineString();
            DGNPoint		*pasPoints;
            int			nPoints;

            nPoints = 5 * psEMP->num_vertices;
            pasPoints = (DGNPoint *) CPLMalloc(sizeof(DGNPoint) * nPoints);
            
            DGNStrokeCurve( hDGN, psEMP, nPoints, pasPoints );

            poLine->setNumPoints( nPoints );
            for( int i = 0; i < nPoints; i++ )
            {
                poLine->setPoint( i, 
                                  pasPoints[i].x,
                                  pasPoints[i].y,
                                  pasPoints[i].z );
            }

            poFeature->SetGeometryDirectly( poLine );
            CPLFree( pasPoints );

            poFeature->SetStyleString( szPen );
        }
        else
        {
            OGRLineString	*poLine = new OGRLineString();
            DGNElemMultiPoint *psEMP = (DGNElemMultiPoint *) psElement;
            
            if( psEMP->num_vertices > 0 )
            {
                poLine->setNumPoints( psEMP->num_vertices );
                for( int i = 0; i < psEMP->num_vertices; i++ )
                {
                    poLine->setPoint( i, 
                                      psEMP->vertices[i].x,
                                      psEMP->vertices[i].y,
                                      psEMP->vertices[i].z );
                }
                
                poFeature->SetGeometryDirectly( poLine );
            }

            poFeature->SetStyleString( szPen );
        }
        break;

      case DGNST_ARC:
      {
          OGRLineString	*poLine = new OGRLineString();
          DGNElemArc    *psArc = (DGNElemArc *) psElement;
          DGNPoint	asPoints[90];
          int		nPoints;

          nPoints = (int) (MAX(1,ABS(psArc->sweepang) / 5) + 1);
          DGNStrokeArc( hDGN, psArc, nPoints, asPoints );

          poLine->setNumPoints( nPoints );
          for( int i = 0; i < nPoints; i++ )
          {
              poLine->setPoint( i, 
                                asPoints[i].x,
                                asPoints[i].y,
                                asPoints[i].z );
          }

          poFeature->SetGeometryDirectly( poLine );
          poFeature->SetStyleString( szPen );
      }
      break;

      case DGNST_TEXT:
      {
          OGRPoint	*poPoint = new OGRPoint();
          DGNElemText   *psText = (DGNElemText *) psElement;
          char		*pszOgrFS;

          poPoint->setX( psText->origin.x );
          poPoint->setY( psText->origin.y );
          poPoint->setZ( psText->origin.z );

          poFeature->SetGeometryDirectly( poPoint );

          pszOgrFS = (char *) CPLMalloc(strlen(psText->string) + 150);

          // setup the basic label.
          sprintf( pszOgrFS, "LABEL(t:\"%s\"",  psText->string );

          // set the color if we have it. 
          if( strlen(szFSColor) > 0 )
              sprintf( pszOgrFS+strlen(pszOgrFS), ",%s", szFSColor );

          // Add the size info in ground units.
          if( ABS(psText->height_mult) >= 6.0 )
              sprintf( pszOgrFS+strlen(pszOgrFS), ",s:%dg", 
                       (int) psText->height_mult );
          else if( ABS(psText->height_mult) > 0.1 )
              sprintf( pszOgrFS+strlen(pszOgrFS), ",s:%.3fg", 
                       psText->height_mult );
          else
              sprintf( pszOgrFS+strlen(pszOgrFS), ",s:%.12f", 
                       psText->height_mult );

          // Add the angle, if not horizontal
          if( psText->rotation != 0.0 )
              sprintf( pszOgrFS+strlen(pszOgrFS), ",a:%d", 
                       (int) (psText->rotation+0.5) );

          strcat( pszOgrFS, ")" );

          poFeature->SetStyleString( pszOgrFS );
          CPLFree( pszOgrFS );
      }
      break;

      default:
        break;
    }

    return poFeature;
}


/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRDGNLayer::GetNextFeature()

{
    DGNElemCore *psElement;

    while( (psElement = DGNReadElement( hDGN )) != NULL )
    {
        OGRFeature	*poFeature;

        if( psElement->deleted )
        {
            DGNFreeElement( hDGN, psElement );
            continue;
        }

        poFeature = ElementToFeature( psElement );
        DGNFreeElement( hDGN, psElement );

        if( poFeature == NULL )
            continue;

        if( poFeature->GetGeometryRef() == NULL )
        {
            delete poFeature;
            continue;
        }

        if( (poFilterGeom == NULL
            || poFilterGeom->Intersect( poFeature->GetGeometryRef() ) )
            && (m_poAttrQuery == NULL
                || m_poAttrQuery->Evaluate( poFeature )) )
            return poFeature;

        delete poFeature;
    }        

    return NULL;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRDGNLayer::TestCapability( const char * pszCap )

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

    else if( EQUAL(pszCap,OLCCreateField) )
        return FALSE;

    else 
        return FALSE;
}
