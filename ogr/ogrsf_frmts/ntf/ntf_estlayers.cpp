/******************************************************************************
 * $Id$
 *
 * Project:  NTF Translator
 * Purpose:  NTFFileReader methods related to establishing the schemas
 *           of features that could occur in this product and the functions
 *           for actually performing the NTFRecord to OGRFeature conversion.
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
 * Revision 1.1  1999/08/28 03:13:35  warmerda
 * New
 *
 */

#include <stdarg.h>
#include "ntf.h"
#include "cpl_string.h"

#define MAX_LINK	200

/************************************************************************/
/*                       TranslateBasedataPoint()                       */
/************************************************************************/

static OGRFeature *TranslateBasedataPoint( NTFFileReader *poReader,
                                           OGRNTFLayer *poLayer,
                                           NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) < 2
        || papoGroup[0]->GetType() != NRT_POINTREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // POINT_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // Geometry
    int		nGeomId;
    
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1],
                                                             &nGeomId));

    // GEOM_ID
    poFeature->SetField( 1, nGeomId );

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "FC", 2, "PN", 3, "NU", 4, "CM", 5,
                                    "UN", 6, "OR", 7,
                                    NULL );

    return poFeature;
}

/************************************************************************/
/*                       TranslateBasedataLine()                        */
/************************************************************************/

static OGRFeature *TranslateBasedataLine( NTFFileReader *poReader,
                                          OGRNTFLayer *poLayer,
                                          NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) < 2
        || papoGroup[0]->GetType() != NRT_LINEREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // LINE_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // Geometry
    int		nGeomId;
    
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1],
                                                             &nGeomId));

    // GEOM_ID
    poFeature->SetField( 2, nGeomId );

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "FC", 1, "PN", 3, "NU", 4, "RB", 5,
                                    NULL );

    return poFeature;
}

/************************************************************************/
/*                  TranslateBoundarylineCollection()                   */
/************************************************************************/

static OGRFeature *TranslateBoundarylineCollection( NTFFileReader *poReader,
                                                    OGRNTFLayer *poLayer,
                                                    NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) != 2 
        || papoGroup[0]->GetType() != NRT_COLLECT
        || papoGroup[1]->GetType() != NRT_ATTREC )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // COLL_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // NUM_PARTS
    int		nNumLinks = atoi(papoGroup[0]->GetField( 9, 12 ));
    
    if( nNumLinks > MAX_LINK )
        return poFeature;
    
    poFeature->SetField( 1, nNumLinks );

    // POLY_ID
    int		i, anList[MAX_LINK];

    for( i = 0; i < nNumLinks; i++ )
        anList[i] = atoi(papoGroup[0]->GetField( 15+i*8, 20+i*8 ));

    poFeature->SetField( 2, nNumLinks, anList );

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "AI", 3, "OP", 4, "NM", 5,
                                    NULL );

    return poFeature;
}

/************************************************************************/
/*                     TranslateBoundarylinePoly()                      */
/************************************************************************/

static OGRFeature *TranslateBoundarylinePoly( NTFFileReader *poReader,
                                              OGRNTFLayer *poLayer,
                                              NTFRecord **papoGroup )

{
/* ==================================================================== */
/*      Traditional POLYGON record groups.                              */
/* ==================================================================== */
    if( CSLCount((char **) papoGroup) == 4 
        && papoGroup[0]->GetType() == NRT_POLYGON
        && papoGroup[1]->GetType() == NRT_ATTREC 
        && papoGroup[2]->GetType() == NRT_CHAIN 
        && papoGroup[3]->GetType() == NRT_GEOMETRY )
    {
        
        OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

        // POLY_ID
        poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

        // NUM_PARTS
        int		nNumLinks = atoi(papoGroup[2]->GetField( 9, 12 ));
    
        if( nNumLinks > MAX_LINK )
            return poFeature;
    
        poFeature->SetField( 4, nNumLinks );

        // DIR
        int		i, anList[MAX_LINK];

        for( i = 0; i < nNumLinks; i++ )
            anList[i] = atoi(papoGroup[2]->GetField( 19+i*7, 19+i*7 ));

        poFeature->SetField( 5, nNumLinks, anList );

        // GEOM_ID_OF_LINK
        for( i = 0; i < nNumLinks; i++ )
            anList[i] = atoi(papoGroup[2]->GetField( 13+i*7, 18+i*7 ));

        poFeature->SetField( 6, nNumLinks, anList );

        // RingStart
        int	nRingList = 0;
        poFeature->SetField( 7, 1, &nRingList );

        // Attributes
        poReader->ApplyAttributeValues( poFeature, papoGroup,
                                        "FC", 1, "PI", 2, "HA", 3,
                                        NULL );

        // Read point geometry
        poFeature->SetGeometryDirectly(
            poReader->ProcessGeometry(papoGroup[3]));

        return poFeature;
    }

/* ==================================================================== */
/*      CPOLYGON Group                                                  */
/* ==================================================================== */

/* -------------------------------------------------------------------- */
/*      First we do validation of the grouping.                         */
/* -------------------------------------------------------------------- */
    int		iRec;
    
    for( iRec = 0;
         papoGroup[iRec] != NULL && papoGroup[iRec+1] != NULL
             && papoGroup[iRec]->GetType() == NRT_POLYGON
             && papoGroup[iRec+1]->GetType() == NRT_CHAIN;
         iRec += 2 ) {}

    if( CSLCount((char **) papoGroup) != iRec + 3 )
        return NULL;

    if( papoGroup[iRec]->GetType() != NRT_CPOLY
        || papoGroup[iRec+1]->GetType() != NRT_ATTREC
        || papoGroup[iRec+2]->GetType() != NRT_GEOMETRY )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Collect the chains for each of the rings, and just aggregate    */
/*      these into the master list without any concept of where the     */
/*      boundaries are.  The boundary information will be emmitted      */
/*	in the RingStart field.						*/
/* -------------------------------------------------------------------- */
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );
    int		nNumLink = 0;
    int		anDirList[MAX_LINK*2], anGeomList[MAX_LINK*2];
    int		anRingStart[MAX_LINK], nRings = 0;

    for( iRec = 0;
         papoGroup[iRec] != NULL && papoGroup[iRec+1] != NULL
             && papoGroup[iRec]->GetType() == NRT_POLYGON
             && papoGroup[iRec+1]->GetType() == NRT_CHAIN;
         iRec += 2 )
    {
        int		i, nLineCount;

        nLineCount = atoi(papoGroup[iRec+1]->GetField(9,12));

        anRingStart[nRings++] = nNumLink;
        
        for( i = 0; i < nLineCount && nNumLink < MAX_LINK*2; i++ )
        {
            anDirList[nNumLink] =
                atoi(papoGroup[iRec+1]->GetField( 19+i*7, 19+i*7 ));
            anGeomList[nNumLink] =
                atoi(papoGroup[iRec+1]->GetField( 13+i*7, 18+i*7 ));
            nNumLink++;
        }

        if( nNumLink == MAX_LINK*2 )
        {
            delete poFeature;
            return NULL;
        }
    }

    // NUM_PART
    poFeature->SetField( 4, nNumLink );

    // DIR
    poFeature->SetField( 5, nNumLink, anDirList );

    // GEOM_ID_OF_LINK
    poFeature->SetField( 6, nNumLink, anGeomList );

    // RingStart
    poFeature->SetField( 7, nRings, anRingStart );

    
/* -------------------------------------------------------------------- */
/*	collect information for whole complex polygon.			*/
/* -------------------------------------------------------------------- */
    // POLY_ID
    poFeature->SetField( 0, atoi(papoGroup[iRec]->GetField( 3, 8 )) );

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "FC", 1, "PI", 2, "HA", 3,
                                    NULL );

    // point geometry for seed.
    poFeature->SetGeometryDirectly(
        poReader->ProcessGeometry(papoGroup[iRec+2]));

    return poFeature;
}

/************************************************************************/
/*                     TranslateBoundarylineLink()                      */
/************************************************************************/

static OGRFeature *TranslateBoundarylineLink( NTFFileReader *poReader,
                                              OGRNTFLayer *poLayer,
                                              NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) != 2 
        || papoGroup[0]->GetType() != NRT_GEOMETRY
        || papoGroup[1]->GetType() != NRT_ATTREC )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // Geometry
    int		nGeomId;
    
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[0],
                                                             &nGeomId));

    // GEOM_ID
    poFeature->SetField( 0, nGeomId );

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "FC", 1, "LK", 2, "HW", 3,
                                    NULL );

    return poFeature;
}

/************************************************************************/
/*                      TranslateMeridianPoint()                        */
/************************************************************************/

static OGRFeature *TranslateMeridianPoint( NTFFileReader *poReader,
                                           OGRNTFLayer *poLayer,
                                           NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) < 2
        || papoGroup[0]->GetType() != NRT_POINTREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // POINT_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // Geometry
    int		nGeomId;
    
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1],
                                                             &nGeomId));

    // GEOM_ID
    poFeature->SetField( 1, nGeomId );

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "FC", 2, "PN", 3, "OS", 4, "JN", 5,
                                    "RT", 6, "SI", 7, "PI", 8, "NM", 9,
                                    "DA", 10, 
                                    NULL );

    return poFeature;
}

/************************************************************************/
/*                       TranslateMeridianLine()                        */
/************************************************************************/

static OGRFeature *TranslateMeridianLine( NTFFileReader *poReader,
                                          OGRNTFLayer *poLayer,
                                          NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) < 2
        || papoGroup[0]->GetType() != NRT_LINEREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // LINE_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // Geometry
    int		nGeomId;
    
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1],
                                                             &nGeomId));

    // GEOM_ID
    poFeature->SetField( 2, nGeomId );

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "FC", 1, "OM", 3, "RN", 4, "TR", 5,
                                    "RI", 6, "LC", 7, "RC", 8, "LC", 9,
                                    "RC", 10, 
                                    NULL );

    return poFeature;
}

/************************************************************************/
/*                       TranslateStrategiNode()                        */
/*                                                                      */
/*      Also used for Meridian and BaseData.GB nodes.                   */
/************************************************************************/

static OGRFeature *TranslateStrategiNode( NTFFileReader *poReader,
                                          OGRNTFLayer *poLayer,
                                          NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) != 1 
        || papoGroup[0]->GetType() != NRT_NODEREC )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // NODE_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // GEOM_ID_OF_POINT
    poFeature->SetField( 1, atoi(papoGroup[0]->GetField( 9, 14 )) );

    // NUM_LINKS
    int		nNumLinks = atoi(papoGroup[0]->GetField( 15, 18 ));
    
    if( nNumLinks > MAX_LINK )
        return poFeature;
    
    poFeature->SetField( 2, nNumLinks );

    // DIR
    int		i, anList[MAX_LINK];

    for( i = 0; i < nNumLinks; i++ )
        anList[i] = atoi(papoGroup[0]->GetField( 19+i*12, 19+i*12 ));

    poFeature->SetField( 3, nNumLinks, anList );

    // GEOM_ID_OF_POINT
    for( i = 0; i < nNumLinks; i++ )
        anList[i] = atoi(papoGroup[0]->GetField( 19+i*12+1, 19+i*12+6 ));

    poFeature->SetField( 4, nNumLinks, anList );

    // ORIENT
    double	adfList[MAX_LINK];

    for( i = 0; i < nNumLinks; i++ )
        adfList[i] =
            atoi(papoGroup[0]->GetField( 19+i*12+7, 19+i*12+10 )) * 0.1;

    poFeature->SetField( 5, nNumLinks, adfList );

    // LEVEL
    for( i = 0; i < nNumLinks; i++ )
        anList[i] = atoi(papoGroup[0]->GetField( 19+i*12+11, 19+i*12+11 ));

    poFeature->SetField( 6, nNumLinks, anList );
    
    return poFeature;
}

/************************************************************************/
/*                       TranslateStrategiText()                        */
/*                                                                      */
/*      Also used for Meridian and BaseData text.                       */
/************************************************************************/

static OGRFeature *TranslateStrategiText( NTFFileReader *poReader,
                                          OGRNTFLayer *poLayer,
                                          NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) < 4
        || papoGroup[0]->GetType() != NRT_TEXTREC
        || papoGroup[1]->GetType() != NRT_TEXTPOS 
        || papoGroup[2]->GetType() != NRT_TEXTREP 
        || papoGroup[3]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // POINT_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // FONT
    poFeature->SetField( 2, atoi(papoGroup[2]->GetField( 9, 12 )) );

    // TEXT_HT
    poFeature->SetField( 3, atoi(papoGroup[2]->GetField( 13, 15 )) * 0.1 );

    // DIG_POSTN
    poFeature->SetField( 4, atoi(papoGroup[2]->GetField( 16, 16 )) );

    // ORIENT
    poFeature->SetField( 5, atoi(papoGroup[2]->GetField( 17, 20 )) * 0.1 );

    // Geometry
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[3]));

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "FC", 1, "TX", 6,
                                    NULL );

    return poFeature;
}

/************************************************************************/
/*                      TranslateStrategiPoint()                        */
/************************************************************************/

static OGRFeature *TranslateStrategiPoint( NTFFileReader *poReader,
                                           OGRNTFLayer *poLayer,
                                           NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) < 2
        || papoGroup[0]->GetType() != NRT_POINTREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // POINT_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // Geometry
    int		nGeomId;
    
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1],
                                                             &nGeomId));

    // GEOM_ID
    poFeature->SetField( 10, nGeomId );

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "FC", 1, "PN", 2, "NU", 3, "RB", 4,
                                    "RU", 5, "AN", 6, "AO", 7, "CM", 8,
                                    "UN", 9, 
                                    NULL );

    return poFeature;
}

/************************************************************************/
/*                       TranslateStrategiLine()                        */
/************************************************************************/

static OGRFeature *TranslateStrategiLine( NTFFileReader *poReader,
                                          OGRNTFLayer *poLayer,
                                          NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) < 2
        || papoGroup[0]->GetType() != NRT_LINEREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // LINE_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // Geometry
    int		nGeomId;
    
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1],
                                                             &nGeomId));

    // GEOM_ID
    poFeature->SetField( 3, nGeomId );

    // Attributes
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "FC", 1,
                                    "PN", 2,
                                    NULL );

    return poFeature;
}

/************************************************************************/
/*                      TranslateLandrangerPoint()                      */
/************************************************************************/

static OGRFeature *TranslateLandrangerPoint( NTFFileReader *poReader,
                                             OGRNTFLayer *poLayer,
                                             NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) != 2
        || papoGroup[0]->GetType() != NRT_POINTREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // POINT_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // FEAT_CODE
    poFeature->SetField( 1, atoi(papoGroup[0]->GetField( 17, 20 )) );

    // HEIGHT
    poFeature->SetField( 2, atoi(papoGroup[0]->GetField( 11, 16 )) );

    // Geometry
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1]));

    return poFeature;
}

/************************************************************************/
/*                      TranslateLandrangerLine()                       */
/************************************************************************/

static OGRFeature *TranslateLandrangerLine( NTFFileReader *poReader,
                                            OGRNTFLayer *poLayer,
                                            NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) != 2
        || papoGroup[0]->GetType() != NRT_LINEREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    if( papoGroup[0]->GetType() != NRT_LINEREC
        || papoGroup[1] == NULL
        || papoGroup[1]->GetType() != NRT_GEOMETRY
        || papoGroup[2] != NULL )
        return NULL;

    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // LINE_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // FEAT_CODE
    poFeature->SetField( 1, atoi(papoGroup[0]->GetField( 17, 20 )) );

    // HEIGHT
    poFeature->SetField( 2, atoi(papoGroup[0]->GetField( 11, 16 )) );

    // Geometry
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1]));

    return poFeature;
}

/************************************************************************/
/*                      TranslateLandlinePoint()                        */
/************************************************************************/

static OGRFeature *TranslateLandlinePoint( NTFFileReader *poReader,
                                           OGRNTFLayer *poLayer,
                                           NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) < 2
        || papoGroup[0]->GetType() != NRT_POINTREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // POINT_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // FEAT_CODE
    poFeature->SetField( 1, atoi(papoGroup[0]->GetField( 17, 20 )) );

    // ORIENT
    poFeature->SetField( 2, atof(papoGroup[0]->GetField( 11, 16 )) );

    // DISTANCE
    poReader->ApplyAttributeValues( poFeature, papoGroup,
                                    "DT", 3,
                                    NULL );

    // Geometry
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1]));

    return poFeature;
}

/************************************************************************/
/*                       TranslateLandlineLine()                        */
/************************************************************************/

static OGRFeature *TranslateLandlineLine( NTFFileReader *poReader,
                                          OGRNTFLayer *poLayer,
                                          NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) != 2
        || papoGroup[0]->GetType() != NRT_LINEREC
        || papoGroup[1]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );

    // LINE_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );

    // FEAT_CODE
    poFeature->SetField( 1, atoi(papoGroup[0]->GetField( 17, 20 )) );

    // Geometry
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[1]));

    return poFeature;
}

/************************************************************************/
/*                       TranslateLandlineName()                        */
/************************************************************************/

static OGRFeature *TranslateLandlineName( NTFFileReader *poReader,
                                          OGRNTFLayer *poLayer,
                                          NTFRecord **papoGroup )

{
    if( CSLCount((char **) papoGroup) != 3 
        || papoGroup[0]->GetType() != NRT_NAMEREC
        || papoGroup[1]->GetType() != NRT_NAMEPOSTN
        || papoGroup[2]->GetType() != NRT_GEOMETRY )
        return NULL;
        
    OGRFeature	*poFeature = new OGRFeature( poLayer->GetLayerDefn() );
        
    // NAME_ID
    poFeature->SetField( 0, atoi(papoGroup[0]->GetField( 3, 8 )) );
        
    // TEXT_CODE
    poFeature->SetField( 1, papoGroup[0]->GetField( 0, 12 ) );
        
    // TEXT
    int 	nNumChar = atoi(papoGroup[0]->GetField(13,14));
    poFeature->SetField( 2, papoGroup[0]->GetField( 15, 15+nNumChar-1) );
    
    // FONT
    poFeature->SetField( 3, atoi(papoGroup[1]->GetField( 3, 6 )) );

    // TEXT_HT
    poFeature->SetField( 4, atoi(papoGroup[1]->GetField(7,9)) * 0.1 );
        
    // TEXT_HT
    poFeature->SetField( 5, atoi(papoGroup[1]->GetField(10,10)) );
        
    // ORIENT
    poFeature->SetField( 6, atof(papoGroup[1]->GetField( 11, 14 )) * 0.1 );

    // Geometry
    poFeature->SetGeometryDirectly(poReader->ProcessGeometry(papoGroup[2]));

    return poFeature;
}

/************************************************************************/
/*                           EstablishLayer()                           */
/*                                                                      */
/*      Establish one layer based on a simplified description of the    */
/*      fields to be present.                                           */
/************************************************************************/

void NTFFileReader::EstablishLayer( const char * pszLayerName,
                                    OGRwkbGeometryType eGeomType,
                                    NTFFeatureTranslator pfnTranslator,
                                    int nLeadRecordType,
                                    ... )

{
    va_list	hVaArgs;
    OGRFeatureDefn *poDefn;
    OGRNTFLayer		*poLayer;

/* -------------------------------------------------------------------- */
/*      Does this layer already exist?  If so, we do nothing            */
/*      ... note that we don't check the definition.                    */
/* -------------------------------------------------------------------- */
    poLayer = poDS->GetNamedLayer(pszLayerName);

/* ==================================================================== */
/*      Create a new layer matching the request if we don't aleady      */
/*      have one.                                                       */
/* ==================================================================== */
    if( poLayer == NULL )
    {
/* -------------------------------------------------------------------- */
/*      Create a new feature definition.				*/
/* -------------------------------------------------------------------- */
        poDefn = new OGRFeatureDefn( pszLayerName );
        poDefn->SetGeomType( eGeomType );
    
/* -------------------------------------------------------------------- */
/*      Fetch definitions of each field in turn.                        */
/* -------------------------------------------------------------------- */
        va_start(hVaArgs, nLeadRecordType);
        while( TRUE )
        {
            const char	*pszFieldName = va_arg(hVaArgs, const char *);
            OGRFieldType     eType;
            int		 nWidth, nPrecision;
            
            if( pszFieldName == NULL )
                break;
            
            eType = (OGRFieldType) va_arg(hVaArgs, int);
            nWidth = va_arg(hVaArgs, int);
            nPrecision = va_arg(hVaArgs, int);
            
            OGRFieldDefn	 oFieldDefn( pszFieldName, eType );
            oFieldDefn.SetWidth( nWidth );
            oFieldDefn.SetPrecision( nPrecision );
            
            poDefn->AddFieldDefn( &oFieldDefn );
        }
        
        va_end(hVaArgs);

/* -------------------------------------------------------------------- */
/*      Add the TILE_REF attribute.                                     */
/* -------------------------------------------------------------------- */
        OGRFieldDefn     oTileID( "TILE_REF", OFTString );

        oTileID.SetWidth( 10 );

        poDefn->AddFieldDefn( &oTileID );
            
/* -------------------------------------------------------------------- */
/*      Create the layer, and give over to the data source object to    */
/*      maintain.                                                       */
/* -------------------------------------------------------------------- */
        poLayer = new OGRNTFLayer( poDS, poDefn, pfnTranslator );
        poDS->AddLayer( poLayer );
    }

/* -------------------------------------------------------------------- */
/*      Register this translator with this file reader for handling     */
/*      the indicate record type.                                       */
/* -------------------------------------------------------------------- */
    apoTypeTranslation[nLeadRecordType] = poLayer;
}

/************************************************************************/
/*                          EstablishLayers()                           */
/*                                                                      */
/*      This method is responsible for creating any missing             */
/*      OGRNTFLayers needed for the current product based on the        */
/*      product name.                                                   */
/*                                                                      */
/*      NOTE: Any changes to the order of attribute fields in the       */
/*      following EstablishLayer() calls must also result in updates    */
/*      to the translate functions.  Changes of names, widths and to    */
/*      some extent types can be done without side effects.             */
/************************************************************************/

void NTFFileReader::EstablishLayers()

{
    if( poDS == NULL || fp == NULL )
        return;

    if( GetProductId() == NPC_LANDLINE )
    {
        EstablishLayer( "LANDLINE_POINT", wkbPoint,
                        TranslateLandlinePoint, NRT_POINTREC,
                        "POINT_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "ORIENT", OFTReal, 5, 1,
                        "DISTANCE", OFTReal, 6, 3,
                        NULL );
                        
        EstablishLayer( "LANDLINE_LINE", wkbLineString,
                        TranslateLandlineLine, NRT_LINEREC,
                        "LINE_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        NULL );
                        
        EstablishLayer( "LANDLINE_NAME", wkbPoint,
                        TranslateLandlineName, NRT_NAMEREC,
                        "NAME_ID", OFTInteger, 6, 0,
                        "TEXT_CODE", OFTString, 3, 0,
                        "TEXT", OFTString, 0, 0,
                        "FONT", OFTInteger, 4, 0,
                        "TEXT_HT", OFTReal, 4, 1,
                        "DIG_POSTN", OFTInteger, 1, 0,
                        "ORIENT", OFTReal, 5, 1,
                        NULL );
    }
    else if( GetProductId() == NPC_LANDRANGER_CONT )
    {
        EstablishLayer( "PANORAMA_POINT", wkbPoint,
                        TranslateLandrangerPoint, NRT_POINTREC,
                        "POINT_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "HEIGHT", OFTInteger, 6, 0,
                        NULL );
                        
        EstablishLayer( "PANORAMA_CONTOUR", wkbLineString,
                        TranslateLandrangerLine, NRT_LINEREC,
                        "LINE_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "HEIGHT", OFTInteger, 6, 0,
                        NULL );
    }
    else if( GetProductId() == NPC_STRATEGI )
    {
        EstablishLayer( "STRATEGI_POINT", wkbPoint,
                        TranslateStrategiPoint, NRT_POINTREC,
                        "POINT_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "PN_ProperName", OFTString, 0, 0, 
                        "NU_FeatureNumber", OFTString, 0, 0, 
                        "RB", OFTString, 1, 0, 
                        "RU", OFTString, 1, 0, 
                        "AN", OFTString, 0, 0, 
                        "AO", OFTString, 0, 0, 
                        "CM_CountyName", OFTString, 0, 0, 
                        "UN_UnitaryName", OFTString, 0, 0, 
                        "GEOM_ID", OFTInteger, 6, 0,
                        NULL );
        
        EstablishLayer( "STRATEGI_LINE", wkbLineString,
                        TranslateStrategiLine, NRT_LINEREC,
                        "LINE_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "PN_ProperName", OFTString, 0, 0,
                        "GEOM_ID", OFTInteger, 6, 0,
                        NULL );

        EstablishLayer( "STRATEGI_TEXT", wkbPoint,
                        TranslateStrategiText, NRT_TEXTREC,
                        "TEXT_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "FONT", OFTInteger, 4, 0,
                        "TEXT_HT", OFTReal, 5, 1,
                        "DIG_POSTN", OFTInteger, 1, 0, 
                        "ORIENT", OFTReal, 5, 1,
                        "TEXT", OFTString, 0, 0, 
                        NULL );

        EstablishLayer( "STRATEGI_NODE", wkbUnknown,
                        TranslateStrategiNode, NRT_NODEREC,
                        "NODE_ID", OFTInteger, 6, 0,
                        "GEOM_ID_OF_POINT", OFTInteger, 6, 0,
                        "NUM_LINKS", OFTInteger, 4, 0,
                        "DIR", OFTIntegerList, 1, 0,
                        "GEOM_ID_OF_LINK", OFTIntegerList, 6, 0,
                        "ORIENT", OFTRealList, 5, 1,
                        "LEVEL", OFTIntegerList, 1, 0,
                        NULL );
    }
    else if( GetProductId() == NPC_MERIDIAN )
    {
        EstablishLayer( "MERIDIAN_POINT", wkbPoint,
                        TranslateMeridianPoint, NRT_POINTREC,
                        "POINT_ID", OFTInteger, 6, 0,
                        "GEOM_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "PN_ProperName", OFTString, 0, 0, 
                        "OSMDR", OFTString, 13, 0,
                        "JN_JunctionName", OFTString, 0, 0,
                        "RT_Roundabout", OFTString, 1, 0,
                        "SI_StationId", OFTString, 13, 0,
                        "PI_GlobalId", OFTInteger, 6, 0, 
                        "NM_AdminName", OFTString, 0, 0, 
                        "DA_DLUA_ID", OFTString, 13, 0, 
                        NULL );
        
        EstablishLayer( "MERIDIAN_LINE", wkbLineString,
                        TranslateMeridianLine, NRT_LINEREC,
                        "LINE_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "GEOM_ID", OFTInteger, 6, 0,
                        "OSMDR", OFTString, 13, 0, 
                        "RN_RoadNumber", OFTString, 0, 0, 
                        "TR_TrunkRoad", OFTString, 1, 0, 
                        "RI_RailId", OFTString, 13, 0, 
                        "LC_LeftCounty", OFTInteger, 6, 0, 
                        "RC_RightCounty", OFTInteger, 6, 0, 
                        "LC_LeftDistrict", OFTInteger, 6, 0, 
                        "RC_RightDistrict", OFTInteger, 6, 0, 
                        NULL );

        EstablishLayer( "MERIDIAN_TEXT", wkbPoint,
                        TranslateStrategiText, NRT_TEXTREC,
                        "TEXT_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "FONT", OFTInteger, 4, 0,
                        "TEXT_HT", OFTReal, 5, 1,
                        "DIG_POSTN", OFTInteger, 1, 0, 
                        "ORIENT", OFTReal, 5, 1,
                        "TEXT", OFTString, 0, 0, 
                        NULL );

        EstablishLayer( "MERIDIAN_NODE", wkbUnknown,
                        TranslateStrategiNode, NRT_NODEREC,
                        "NODE_ID", OFTInteger, 6, 0,
                        "GEOM_ID_OF_POINT", OFTInteger, 6, 0,
                        "NUM_LINKS", OFTInteger, 4, 0,
                        "DIR", OFTIntegerList, 1, 0,
                        "GEOM_ID_OF_LINK", OFTIntegerList, 6, 0,
                        "ORIENT", OFTRealList, 5, 1,
                        "LEVEL", OFTIntegerList, 1, 0,
                        NULL );
    }
    else if( GetProductId() == NPC_BOUNDARYLINE )
    {
        EstablishLayer( "BOUNDARYLINE_LINK", wkbLineString,
                        TranslateBoundarylineLink, NRT_GEOMETRY,
                        "GEOM_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "GLOBAL_LINK_ID", OFTInteger, 10, 0,
                        "HWM_FLAG", OFTInteger, 1, 0, 
                        NULL );

        EstablishLayer( "BOUNDARYLINE_POLY", wkbPoint,
                        TranslateBoundarylinePoly, NRT_POLYGON,
                        "POLY_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "GLOBAL_SEED_ID", OFTInteger, 6, 0,
                        "HECTARES", OFTReal, 9, 3,
                        "NUM_PARTS", OFTInteger, 4, 0, 
                        "DIR", OFTIntegerList, 1, 0,
                        "GEOM_ID_OF_LINK", OFTIntegerList, 6, 0,
                        "RingStart", OFTIntegerList, 6, 0,
                        NULL );

        EstablishLayer( "BOUNDARYLINE_COLLECTIONS", wkbUnknown,
                        TranslateBoundarylineCollection, NRT_COLLECT,
                        "COLL_ID", OFTInteger, 6, 0,
                        "NUM_PARTS", OFTInteger, 4, 0,
                        "POLY_ID", OFTIntegerList, 6, 0,
                        "ADMIN_AREA_ID", OFTInteger, 6, 0, 
                        "OPCS_CODE", OFTString, 6, 0,
                        "NM_AdminName", OFTString, 0, 0,
                        NULL );
    }
    else if( GetProductId() == NPC_BASEDATA )
    {
        EstablishLayer( "BASEDATA_POINT", wkbPoint,
                        TranslateBasedataPoint, NRT_POINTREC,
                        "POINT_ID", OFTInteger, 6, 0,
                        "GEOM_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "PN_ProperName", OFTString, 0, 0, 
                        "NU_FeatureNumber", OFTString, 0, 0, 
                        "CM_CountyName", OFTString, 0, 0, 
                        "UN_UnitaryName", OFTString, 0, 0, 
                        "ORIENT", OFTRealList, 5, 1,
                        NULL );
        
        EstablishLayer( "BASEDATA_LINE", wkbLineString,
                        TranslateBasedataLine, NRT_LINEREC,
                        "LINE_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "GEOM_ID", OFTInteger, 6, 0,
                        "PN_ProperName", OFTString, 0, 0, 
                        "NU_FeatureNumber", OFTString, 0, 0, 
                        "RB", OFTString, 1, 0, 
                        NULL );

        EstablishLayer( "BASEDATA_TEXT", wkbPoint,
                        TranslateStrategiText, NRT_TEXTREC,
                        "TEXT_ID", OFTInteger, 6, 0,
                        "FEAT_CODE", OFTInteger, 4, 0,
                        "FONT", OFTInteger, 4, 0,
                        "TEXT_HT", OFTReal, 5, 1,
                        "DIG_POSTN", OFTInteger, 1, 0, 
                        "ORIENT", OFTReal, 5, 1,
                        "TEXT", OFTString, 0, 0, 
                        NULL );

        EstablishLayer( "BASEDATA_NODE", wkbUnknown,
                        TranslateStrategiNode, NRT_NODEREC,
                        "NODE_ID", OFTInteger, 6, 0,
                        "GEOM_ID_OF_POINT", OFTInteger, 6, 0,
                        "NUM_LINKS", OFTInteger, 4, 0,
                        "DIR", OFTIntegerList, 1, 0,
                        "GEOM_ID_OF_LINK", OFTIntegerList, 6, 0,
                        "ORIENT", OFTRealList, 5, 1,
                        "LEVEL", OFTIntegerList, 1, 0,
                        NULL );
    }
}
