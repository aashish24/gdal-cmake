/******************************************************************************
 * $Id$
 *
 * Project:  OGR
 * Purpose:  Implements OGRAVCLayer class.  This is the base class for E00
 *           and binary coverage layer implementations.  It provides some base
 *           layer operations, and methods for transforming between OGR 
 *           features, and the in memory structures of the AVC library.
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
 * Revision 1.2  2002/02/14 23:01:04  warmerda
 * added region and attribute support
 *
 * Revision 1.1  2002/02/13 20:48:18  warmerda
 * New
 *
 */

#include "ogr_avc.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           OGRAVCLayer()                           */
/************************************************************************/

OGRAVCLayer::OGRAVCLayer( AVCFileType eSectionTypeIn, 
                          OGRAVCDataSource *poDSIn )

{
    poFilterGeom = NULL;
    eSectionType = eSectionTypeIn;
    
    poDS = poDSIn;
}

/************************************************************************/
/*                          ~OGRAVCLayer()                           */
/************************************************************************/

OGRAVCLayer::~OGRAVCLayer()

{
    if( poFeatureDefn != NULL )
    {
        delete poFeatureDefn;
        poFeatureDefn = NULL;
    }

    if( poFilterGeom != NULL )
        delete poFilterGeom;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRAVCLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( poFilterGeom != NULL )
    {
        delete poFilterGeom;
        poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
        poFilterGeom = poGeomIn->clone();

    ResetReading();
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRAVCLayer::TestCapability( const char * pszCap )

{
    return FALSE;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRAVCLayer::GetSpatialRef()

{
    return poDS->GetSpatialRef();
}

/************************************************************************/
/*                       SetupFeatureDefinition()                       */
/************************************************************************/

int OGRAVCLayer::SetupFeatureDefinition( const char *pszName )

{
    switch( eSectionType )
    {
      case AVCFileARC:
        {
            poFeatureDefn = new OGRFeatureDefn( pszName );
            poFeatureDefn->SetGeomType( wkbLineString );

            OGRFieldDefn	oUserId( "UserId", OFTInteger );
            OGRFieldDefn	oFNode( "FNode", OFTInteger );
            OGRFieldDefn	oTNode( "TNode", OFTInteger );
            OGRFieldDefn	oLPoly( "LPoly", OFTInteger );
            OGRFieldDefn	oRPoly( "RPoly", OFTInteger );

            poFeatureDefn->AddFieldDefn( &oUserId );
            poFeatureDefn->AddFieldDefn( &oFNode );
            poFeatureDefn->AddFieldDefn( &oTNode );
            poFeatureDefn->AddFieldDefn( &oLPoly );
            poFeatureDefn->AddFieldDefn( &oRPoly );
        }
        return TRUE;

      case AVCFilePAL:
      case AVCFileRPL:
        {
            poFeatureDefn = new OGRFeatureDefn( pszName );
            poFeatureDefn->SetGeomType( wkbPolygon );

            OGRFieldDefn	oArcIds( "ArcIds", OFTIntegerList );
            poFeatureDefn->AddFieldDefn( &oArcIds );
        }
        return TRUE;

      case AVCFileCNT:
        {
            poFeatureDefn = new OGRFeatureDefn( pszName );
            poFeatureDefn->SetGeomType( wkbPoint );

            OGRFieldDefn	oLabelIds( "LabelIds", OFTIntegerList );
            poFeatureDefn->AddFieldDefn( &oLabelIds );
        }
        return TRUE;

      case AVCFileLAB:
        {
            poFeatureDefn = new OGRFeatureDefn( pszName );
            poFeatureDefn->SetGeomType( wkbPoint );

            OGRFieldDefn	oPolyId( "PolyId", OFTInteger );
            poFeatureDefn->AddFieldDefn( &oPolyId );
        }
        return TRUE;

      default:
        poFeatureDefn = NULL;
        return FALSE;
    }
}

/************************************************************************/
/*                          TranslateFeature()                          */
/*                                                                      */
/*      Translate the AVC structure for a feature to the the            */
/*      corresponding OGR definition.  It is assumed that the passed    */
/*      in feature is of a type matching the section type               */
/*      established by SetupFeatureDefinition().                        */
/************************************************************************/

OGRFeature *OGRAVCLayer::TranslateFeature( void *pAVCFeature )

{
    switch( eSectionType )
    {
/* ==================================================================== */
/*      ARC                                                             */
/* ==================================================================== */
      case AVCFileARC:
      {
          AVCArc *psArc = (AVCArc *) pAVCFeature;

/* -------------------------------------------------------------------- */
/*      Create feature.                                                 */
/* -------------------------------------------------------------------- */
          OGRFeature *poOGRFeature = new OGRFeature( GetLayerDefn() );
          poOGRFeature->SetFID( psArc->nArcId );

/* -------------------------------------------------------------------- */
/*      Apply the line geometry.                                        */
/* -------------------------------------------------------------------- */
          OGRLineString *poLine = new OGRLineString();

          poLine->setNumPoints( psArc->numVertices );
          for( int iVert = 0; iVert < psArc->numVertices; iVert++ )
              poLine->setPoint( iVert, 
                                psArc->pasVertices[iVert].x, 
                                psArc->pasVertices[iVert].y );

          poOGRFeature->SetGeometryDirectly( poLine );

/* -------------------------------------------------------------------- */
/*      Apply attributes.                                               */
/* -------------------------------------------------------------------- */
          poOGRFeature->SetField( 0, psArc->nUserId );
          poOGRFeature->SetField( 1, psArc->nFNode );
          poOGRFeature->SetField( 2, psArc->nTNode );
          poOGRFeature->SetField( 3, psArc->nLPoly );
          poOGRFeature->SetField( 4, psArc->nRPoly );
          return poOGRFeature;
      }

/* ==================================================================== */
/*      PAL (Polygon)                                                   */
/*      RPL (Region)                                                    */
/* ==================================================================== */
      case AVCFilePAL:
      case AVCFileRPL:
      {
          AVCPal *psPAL = (AVCPal *) pAVCFeature;

/* -------------------------------------------------------------------- */
/*      Create feature.                                                 */
/* -------------------------------------------------------------------- */
          OGRFeature *poOGRFeature = new OGRFeature( GetLayerDefn() );
          poOGRFeature->SetFID( psPAL->nPolyId );

/* -------------------------------------------------------------------- */
/*      Apply attributes.                                               */
/* -------------------------------------------------------------------- */
          // Setup ArcId list. 
          int	       *panArcs, i;

          panArcs = (int *) CPLMalloc(sizeof(int) * psPAL->numArcs );
          for( i = 0; i < psPAL->numArcs; i++ )
              panArcs[i] = psPAL->pasArcs[i].nArcId;
          poOGRFeature->SetField( 0, psPAL->numArcs, panArcs );
          CPLFree( panArcs );

          return poOGRFeature;
      }

/* ==================================================================== */
/*      CNT (Centroid)                                                  */
/* ==================================================================== */
      case AVCFileCNT:
      {
          AVCCnt *psCNT = (AVCCnt *) pAVCFeature;

/* -------------------------------------------------------------------- */
/*      Create feature.                                                 */
/* -------------------------------------------------------------------- */
          OGRFeature *poOGRFeature = new OGRFeature( GetLayerDefn() );
          poOGRFeature->SetFID( psCNT->nPolyId );

/* -------------------------------------------------------------------- */
/*      Apply Geometry                                                  */
/* -------------------------------------------------------------------- */
          poOGRFeature->SetGeometryDirectly( 
              new OGRPoint( psCNT->sCoord.x, psCNT->sCoord.y ) );

/* -------------------------------------------------------------------- */
/*      Apply attributes.                                               */
/* -------------------------------------------------------------------- */
          poOGRFeature->SetField( 0, psCNT->numLabels, psCNT->panLabelIds );

          return poOGRFeature;
      }

/* ==================================================================== */
/*      LAB (Label)                                                     */
/* ==================================================================== */
      case AVCFileLAB:
      {
          AVCLab *psLAB = (AVCLab *) pAVCFeature;

/* -------------------------------------------------------------------- */
/*      Create feature.                                                 */
/* -------------------------------------------------------------------- */
          OGRFeature *poOGRFeature = new OGRFeature( GetLayerDefn() );
          poOGRFeature->SetFID( psLAB->nValue );

/* -------------------------------------------------------------------- */
/*      Apply Geometry                                                  */
/* -------------------------------------------------------------------- */
          poOGRFeature->SetGeometryDirectly( 
              new OGRPoint( psLAB->sCoord1.x, psLAB->sCoord1.y ) );

/* -------------------------------------------------------------------- */
/*      Apply attributes.                                               */
/* -------------------------------------------------------------------- */
          poOGRFeature->SetField( 0, psLAB->nPolyId );

          return poOGRFeature;
      }

      default:
        return NULL;
    }
}

/************************************************************************/
/*                       AppendTableDefinition()                        */
/*                                                                      */
/*      Add fields to this layers feature definition based on the       */
/*      definition from the coverage.                                   */
/************************************************************************/

int OGRAVCLayer::AppendTableDefinition( AVCTableDef *psTableDef )

{
    for( int iField = 0; iField < psTableDef->numFields; iField++ )
    {
        AVCFieldInfo *psFInfo = psTableDef->pasFieldDef + iField;
        char	szFieldName[128];

        /* Strip off white space */
        strcpy( szFieldName, psFInfo->szName );
        if( strstr(szFieldName," ") != NULL )
            *(strstr(szFieldName," ")) = '\0';
        
        OGRFieldDefn  oFDefn( szFieldName, OFTInteger );

        if( psFInfo->nIndex < 0 )
            continue;

        switch( psFInfo->nType1*10 )
        {
          case AVC_FT_DATE:
          case AVC_FT_CHAR:
            oFDefn.SetType( OFTString );
            oFDefn.SetWidth( psFInfo->nFmtWidth );
            break;

          case AVC_FT_FIXINT:
          case AVC_FT_BININT:
            oFDefn.SetType( OFTInteger );
            oFDefn.SetWidth( psFInfo->nFmtWidth );
            break;

          case AVC_FT_FIXNUM:
          case AVC_FT_BINFLOAT:
            oFDefn.SetType( OFTReal );
            oFDefn.SetWidth( psFInfo->nFmtWidth );
            if( psFInfo->nFmtPrec > 0 )
                oFDefn.SetPrecision( psFInfo->nFmtPrec );
            break;
        }

        poFeatureDefn->AddFieldDefn( &oFDefn );
    }

    return TRUE;
}

/************************************************************************/
/*                        TranslateTableFields()                        */
/************************************************************************/

int OGRAVCLayer::TranslateTableFields( OGRFeature *poFeature, 
                                       int nFieldBase, 
                                       AVCTableDef *psTableDef,
                                       AVCField *pasFields )

{
    int	iOutField = nFieldBase;

    for( int iField=0; iField < psTableDef->numFields; iField++ )
    {
        AVCFieldInfo *psFInfo = psTableDef->pasFieldDef + iField;
        int           nType = psFInfo->nType1 * 10;

        if( psFInfo->nIndex < 0 )
            continue;
        
        if (nType ==  AVC_FT_DATE || nType == AVC_FT_CHAR ||
            nType == AVC_FT_FIXINT || nType == AVC_FT_FIXNUM)
        {
            poFeature->SetField( iOutField++, pasFields[iField].pszStr );
        }
        else if (nType == AVC_FT_BININT && psFInfo->nSize == 4)
        {
            poFeature->SetField( iOutField++, pasFields[iField].nInt32 );
        }
        else if (nType == AVC_FT_BININT && psFInfo->nSize == 2)
        {
            poFeature->SetField( iOutField++, pasFields[iField].nInt16 );
        }
        else if (nType == AVC_FT_BINFLOAT && psFInfo->nSize == 4)
        {
            poFeature->SetField( iOutField++, pasFields[iField].fFloat );
        }
        else if (nType == AVC_FT_BINFLOAT && psFInfo->nSize == 8)
        {
            poFeature->SetField( iOutField++, pasFields[iField].dDouble );
        }
        else
        {
            CPLAssert( FALSE );
            return FALSE;
        }
    }

    return TRUE;
}
