/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  The OGRPolygon geometry class.
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
 * Revision 1.4  1999/05/23 05:34:41  warmerda
 * added support for clone(), multipolygons and geometry collections
 *
 * Revision 1.3  1999/05/20 14:35:44  warmerda
 * added support for well known text format
 *
 * Revision 1.2  1999/05/17 14:38:11  warmerda
 * Added new IPolygon style methods.
 *
 * Revision 1.1  1999/03/30 21:21:05  warmerda
 * New
 *
 */

#include "ogr_geometry.h"
#include "ogr_p.h"

/************************************************************************/
/*                           OGRPolygon()                            */
/************************************************************************/

OGRPolygon::OGRPolygon()

{
    nRingCount = 0;
    papoRings = NULL;
}

/************************************************************************/
/*                           ~OGRPolygon()                           */
/************************************************************************/

OGRPolygon::~OGRPolygon()

{
    if( papoRings != NULL )
    {
        for( int i = 0; i < nRingCount; i++ )
        {
            delete papoRings[i];
        }
        OGRFree( papoRings );
    }
}

/************************************************************************/
/*                               clone()                                */
/************************************************************************/

OGRGeometry *OGRPolygon::clone()

{
    OGRPolygon	*poNewPolygon;

    poNewPolygon = new OGRPolygon;

    for( int i = 0; i < nRingCount; i++ )
    {
        poNewPolygon->addRing( papoRings[i] );
    }

    return poNewPolygon;
}

/************************************************************************/
/*                          getGeometryType()                           */
/************************************************************************/

OGRwkbGeometryType OGRPolygon::getGeometryType()

{
    return wkbPolygon;
}

/************************************************************************/
/*                            getDimension()                            */
/************************************************************************/

int OGRPolygon::getDimension()

{
    return 2;
}

/************************************************************************/
/*                       getCoordinateDimension()                       */
/************************************************************************/

int OGRPolygon::getCoordinateDimension()

{
    return 2;
}

/************************************************************************/
/*                          getGeometryName()                           */
/************************************************************************/

const char * OGRPolygon::getGeometryName()

{
    return "POLYGON";
}

/************************************************************************/
/*                          getExteriorRing()                           */
/************************************************************************/

OGRLinearRing *OGRPolygon::getExteriorRing()

{
    if( nRingCount > 0 )
        return papoRings[0];
    else
        return NULL;
}

/************************************************************************/
/*                        getNumInteriorRings()                         */
/************************************************************************/

int OGRPolygon::getNumInteriorRings()

{
    if( nRingCount > 0 )
        return nRingCount-1;
    else
        return 0;
}

/************************************************************************/
/*                          getInteriorRing()                           */
/************************************************************************/

OGRLinearRing *OGRPolygon::getInteriorRing( int iRing )

{
    if( iRing < 0 || iRing >= nRingCount-1 )
        return NULL;
    else
        return papoRings[iRing+1];
}

/************************************************************************/
/*                              addRing()                               */
/*                                                                      */
/*      Add a ring to the polygon. Note we make a copy of the ring.     */
/*      The original is still the responsibility of the caller.         */
/************************************************************************/

void OGRPolygon::addRing( OGRLinearRing * poNewRing )

{
    papoRings = (OGRLinearRing **) OGRRealloc( papoRings,
                                               sizeof(void*) * (nRingCount+1));

    papoRings[nRingCount] = new OGRLinearRing( poNewRing );

    nRingCount++;
}

/************************************************************************/
/*                              WkbSize()                               */
/*                                                                      */
/*      Return the size of this object in well known binary             */
/*      representation including the byte order, and type information.  */
/************************************************************************/

int OGRPolygon::WkbSize()

{
    int		nSize = 9;

    for( int i = 0; i < nRingCount; i++ )
    {
        nSize += papoRings[i]->_WkbSize();
    }

    return nSize;
}

/************************************************************************/
/*                           importFromWkb()                            */
/*                                                                      */
/*      Initialize from serialized stream in well known binary          */
/*      format.                                                         */
/************************************************************************/

OGRErr OGRPolygon::importFromWkb( unsigned char * pabyData,
                                  int nBytesAvailable )

{
    OGRwkbByteOrder	eByteOrder;
    int			nDataOffset;
    
    if( nBytesAvailable < 21 && nBytesAvailable != -1 )
        return OGRERR_NOT_ENOUGH_DATA;

/* -------------------------------------------------------------------- */
/*      Get the byte order byte.                                        */
/* -------------------------------------------------------------------- */
    eByteOrder = (OGRwkbByteOrder) *pabyData;
    CPLAssert( eByteOrder == wkbXDR || eByteOrder == wkbNDR );

/* -------------------------------------------------------------------- */
/*      Get the geometry feature type.  For now we assume that          */
/*      geometry type is between 0 and 255 so we only have to fetch     */
/*      one byte.                                                       */
/* -------------------------------------------------------------------- */
#ifdef DEBUG
    OGRwkbGeometryType eGeometryType;
    
    if( eByteOrder == wkbNDR )
        eGeometryType = (OGRwkbGeometryType) pabyData[1];
    else
        eGeometryType = (OGRwkbGeometryType) pabyData[4];

    CPLAssert( eGeometryType == wkbPolygon );
#endif    

/* -------------------------------------------------------------------- */
/*      Do we already have some rings?                                  */
/* -------------------------------------------------------------------- */
    if( nRingCount != 0 )
    {
        for( int iRing = 0; iRing < nRingCount; iRing++ )
            delete papoRings[iRing];

        OGRFree( papoRings );
        papoRings = NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Get the ring count.                                             */
/* -------------------------------------------------------------------- */
    memcpy( &nRingCount, pabyData + 5, 4 );
    
    if( OGR_SWAP( eByteOrder ) )
        nRingCount = CPL_SWAP32(nRingCount);

    papoRings = (OGRLinearRing **) OGRMalloc(sizeof(void*) * nRingCount);

    nDataOffset = 9;
    if( nBytesAvailable != -1 )
        nBytesAvailable -= nDataOffset;

/* -------------------------------------------------------------------- */
/*      Get the rings.                                                  */
/* -------------------------------------------------------------------- */
    for( int iRing = 0; iRing < nRingCount; iRing++ )
    {
        OGRErr	eErr;
        
        papoRings[iRing] = new OGRLinearRing();
        eErr = papoRings[iRing]->_importFromWkb( eByteOrder,
                                                 pabyData + nDataOffset,
                                                 nBytesAvailable );
        if( eErr != OGRERR_NONE )
        {
            nRingCount = iRing;
            return eErr;
        }

        if( nBytesAvailable != -1 )
            nBytesAvailable -= papoRings[iRing]->_WkbSize();

        nDataOffset += papoRings[iRing]->_WkbSize();
    }
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkb()                             */
/*                                                                      */
/*      Build a well known binary representation of this object.        */
/************************************************************************/

OGRErr	OGRPolygon::exportToWkb( OGRwkbByteOrder eByteOrder,
                                 unsigned char * pabyData )

{
    int		nOffset;
    
/* -------------------------------------------------------------------- */
/*      Set the byte order.                                             */
/* -------------------------------------------------------------------- */
    pabyData[0] = (unsigned char) eByteOrder;

/* -------------------------------------------------------------------- */
/*      Set the geometry feature type.                                  */
/* -------------------------------------------------------------------- */
    if( eByteOrder == wkbNDR )
    {
        pabyData[1] = wkbPolygon;
        pabyData[2] = 0;
        pabyData[3] = 0;
        pabyData[4] = 0;
    }
    else
    {
        pabyData[1] = 0;
        pabyData[2] = 0;
        pabyData[3] = 0;
        pabyData[4] = wkbPolygon;
    }
    
/* -------------------------------------------------------------------- */
/*      Copy in the raw data.                                           */
/* -------------------------------------------------------------------- */
    if( OGR_SWAP( eByteOrder ) )
    {
        int	nCount;

        nCount = CPL_SWAP32( nRingCount );
        memcpy( pabyData+5, &nCount, 4 );
    }
    else
    {
        memcpy( pabyData+5, &nRingCount, 4 );
    }
    
    nOffset = 9;
    
/* ==================================================================== */
/*      Serialize each of the rings.                                    */
/* ==================================================================== */
    for( int iRing = 0; iRing < nRingCount; iRing++ )
    {
        papoRings[iRing]->_exportToWkb( eByteOrder,
                                        pabyData + nOffset );

        nOffset += papoRings[iRing]->_WkbSize();
    }
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                           importFromWkt()                            */
/*                                                                      */
/*      Instantiate from well known text format.  Currently this is     */
/*      `POLYGON ((x y, x y, ...),(x y, ...),...)'.                     */
/************************************************************************/

OGRErr OGRPolygon::importFromWkt( char ** ppszInput )

{
    char	szToken[OGR_WKT_TOKEN_MAX];
    const char	*pszInput = *ppszInput;
    int		iRing;

/* -------------------------------------------------------------------- */
/*      Clear existing rings.                                           */
/* -------------------------------------------------------------------- */
    if( nRingCount > 0 )
    {
        for( iRing = 0; iRing < nRingCount; iRing++ )
            delete papoRings[iRing];
        
        nRingCount = 0;
        CPLFree( papoRings );
    }

/* -------------------------------------------------------------------- */
/*      Read and verify the ``POLYGON'' keyword token.                  */
/* -------------------------------------------------------------------- */
    pszInput = OGRWktReadToken( pszInput, szToken );

    if( !EQUAL(szToken,"POLYGON") )
        return OGRERR_CORRUPT_DATA;

/* -------------------------------------------------------------------- */
/*      The next character should be a ( indicating the start of the    */
/*      list of rings.                                                  */
/* -------------------------------------------------------------------- */
    pszInput = OGRWktReadToken( pszInput, szToken );
    if( szToken[0] != '(' )
        return OGRERR_CORRUPT_DATA;

/* ==================================================================== */
/*      Read each ring in turn.  Note that we try to reuse the same     */
/*      point list buffer from ring to ring to cut down on              */
/*      allocate/deallocate overhead.                                   */
/* ==================================================================== */
    OGRRawPoint	*paoPoints = NULL;
    int		nMaxPoints = 0, nMaxRings = 0;
    
    do
    {
        int	nPoints = 0;

/* -------------------------------------------------------------------- */
/*      Read points for one ring from input.                            */
/* -------------------------------------------------------------------- */
        pszInput = OGRWktReadPoints( pszInput, &paoPoints, &nMaxPoints,
                                     &nPoints );

        if( pszInput == NULL )
        {
            CPLFree( paoPoints );
            return OGRERR_CORRUPT_DATA;
        }
        
/* -------------------------------------------------------------------- */
/*      Do we need to grow the ring array?                              */
/* -------------------------------------------------------------------- */
        if( nRingCount == nMaxRings )
        {
            nMaxRings = nMaxRings * 2 + 1;
            papoRings = (OGRLinearRing **)
                CPLRealloc(papoRings, nMaxRings * sizeof(OGRLinearRing*));
        }

/* -------------------------------------------------------------------- */
/*      Create the new ring, and assign to ring list.                   */
/* -------------------------------------------------------------------- */
        papoRings[nRingCount] = new OGRLinearRing();
        papoRings[nRingCount]->setPoints( nPoints, paoPoints );

        nRingCount++;

/* -------------------------------------------------------------------- */
/*      Read the delimeter following the ring.                          */
/* -------------------------------------------------------------------- */
        
        pszInput = OGRWktReadToken( pszInput, szToken );
    } while( szToken[0] == ',' );

/* -------------------------------------------------------------------- */
/*      freak if we don't get a closing bracket.                        */
/* -------------------------------------------------------------------- */
    CPLFree( paoPoints );
   
    if( szToken[0] != ')' )
        return OGRERR_CORRUPT_DATA;
    
    *ppszInput = (char *) pszInput;
    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkt()                             */
/*                                                                      */
/*      Translate this structure into it's well known text format       */
/*      equivelent.  This could be made alot more CPU efficient!        */
/************************************************************************/

OGRErr OGRPolygon::exportToWkt( char ** ppszReturn )

{
    char	**papszRings;
    int		iRing, nCumulativeLength = 0;
    OGRErr	eErr;

/* -------------------------------------------------------------------- */
/*      Build a list of strings containing the stuff for each ring.     */
/* -------------------------------------------------------------------- */
    papszRings = (char **) CPLCalloc(sizeof(char *),nRingCount);

    for( iRing = 0; iRing < nRingCount; iRing++ )
    {
        eErr = papoRings[iRing]->exportToWkt( &(papszRings[iRing]) );
        if( eErr != OGRERR_NONE )
            return eErr;

        CPLAssert( EQUALN(papszRings[iRing],"LINEARRING (", 12) );
        nCumulativeLength += strlen(papszRings[iRing] + 11);
    }
    
/* -------------------------------------------------------------------- */
/*      Allocate exactly the right amount of space for the              */
/*      aggregated string.                                              */
/* -------------------------------------------------------------------- */
    *ppszReturn = (char *) VSIMalloc(nCumulativeLength + nRingCount + 11);

    if( *ppszReturn == NULL )
        return OGRERR_NOT_ENOUGH_MEMORY;

/* -------------------------------------------------------------------- */
/*      Build up the string, freeing temporary strings as we go.        */
/* -------------------------------------------------------------------- */
    strcpy( *ppszReturn, "POLYGON (" );

    for( iRing = 0; iRing < nRingCount; iRing++ )
    {								
        if( iRing > 0 )
            strcat( *ppszReturn, "," );
        
        strcat( *ppszReturn, papszRings[iRing] + 11 );
        VSIFree( papszRings[iRing] );
    }

    strcat( *ppszReturn, ")" );

    CPLFree( papszRings );

    return OGRERR_NONE;
}

/************************************************************************/
/*                              get_Area()                              */
/************************************************************************/

double OGRPolygon::get_Area()

{
    // notdef ... correct later.
    
    return 0.0;
}

/************************************************************************/
/*                              Centroid()                              */
/************************************************************************/

int OGRPolygon::Centroid( OGRPoint * )

{
    // notdef ... not implemented yet.
    
    return 0;
}

/************************************************************************/
/*                           PointOnSurface()                           */
/************************************************************************/

int OGRPolygon::PointOnSurface( OGRPoint * )

{
    // notdef ... not implemented yet.
    
    return 0;
}
