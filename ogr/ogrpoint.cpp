/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  The Point geometry class.
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
 * Revision 1.23  2003/06/09 13:48:54  warmerda
 * added DB2 V7.2 byte order hack
 *
 * Revision 1.22  2003/05/28 19:16:43  warmerda
 * fixed up argument names and stuff for docs
 *
 * Revision 1.21  2003/03/07 21:28:56  warmerda
 * support 0x8000 style 3D WKB flags
 *
 * Revision 1.20  2003/02/08 00:37:14  warmerda
 * try to improve documentation
 *
 * Revision 1.19  2002/09/11 13:47:17  warmerda
 * preliminary set of fixes for 3D WKB enum
 *
 * Revision 1.18  2002/05/02 19:45:36  warmerda
 * added flattenTo2D() method
 *
 * Revision 1.17  2001/11/01 17:20:33  warmerda
 * added DISABLE_OGRGEOM_TRANSFORM macro
 *
 * Revision 1.16  2001/11/01 17:01:28  warmerda
 * pass output buffer into OGRMakeWktCoordinate
 *
 * Revision 1.15  2001/09/21 16:24:20  warmerda
 * added transform() and transformTo() methods
 *
 * Revision 1.14  2001/07/18 05:03:05  warmerda
 * added CPL_CVSID
 *
 * Revision 1.13  1999/11/18 19:02:19  warmerda
 * expanded tabs
 *
 * Revision 1.12  1999/09/13 14:34:07  warmerda
 * updated to use wkbZ of 0x8000 instead of 0x80000000
 *
 * Revision 1.11  1999/09/13 02:27:33  warmerda
 * incorporated limited 2.5d support
 *
 * Revision 1.10  1999/07/27 00:48:11  warmerda
 * Added Equal() support
 *
 * Revision 1.9  1999/07/06 21:36:47  warmerda
 * tenatively added getEnvelope() and Intersect()
 *
 * Revision 1.8  1999/06/25 20:44:43  warmerda
 * implemented assignSpatialReference, carry properly
 *
 * Revision 1.7  1999/05/31 20:42:13  warmerda
 * added empty method
 *
 * Revision 1.6  1999/05/31 14:59:06  warmerda
 * added documentation
 *
 * Revision 1.5  1999/05/31 11:05:08  warmerda
 * added some documentation
 *
 * Revision 1.4  1999/05/23 05:34:40  warmerda
 * added support for clone(), multipolygons and geometry collections
 *
 * Revision 1.3  1999/05/20 14:35:44  warmerda
 * added support for well known text format
 *
 * Revision 1.2  1999/03/30 21:21:43  warmerda
 * added linearring/polygon support
 *
 * Revision 1.1  1999/03/29 21:21:10  warmerda
 * New
 *
 */

#include "ogr_geometry.h"
#include "ogr_p.h"
#include <assert.h>

CPL_CVSID("$Id$");

/************************************************************************/
/*                              OGRPoint()                              */
/************************************************************************/

/**
 * Create a (0,0) point.
 */

OGRPoint::OGRPoint()

{
    x = 0;
    y = 0;
    z = 0;
}

/************************************************************************/
/*                              OGRPoint()                              */
/*                                                                      */
/*      Initialize point to value.                                      */
/************************************************************************/

OGRPoint::OGRPoint( double xIn, double yIn, double zIn )

{
    x = xIn;
    y = yIn;
    z = zIn;
}

/************************************************************************/
/*                             ~OGRPoint()                              */
/************************************************************************/

OGRPoint::~OGRPoint()

{
}

/************************************************************************/
/*                               clone()                                */
/*                                                                      */
/*      Make a new object that is a copy of this object.                */
/************************************************************************/

OGRGeometry *OGRPoint::clone()

{
    OGRPoint    *poNewPoint = new OGRPoint( x, y, z );

    poNewPoint->assignSpatialReference( getSpatialReference() );

    return poNewPoint;
}

/************************************************************************/
/*                               empty()                                */
/************************************************************************/
void OGRPoint::empty()

{
    x = y = z = 0.0;
}

/************************************************************************/
/*                            getDimension()                            */
/************************************************************************/

int OGRPoint::getDimension()

{
    return 0;
}

/************************************************************************/
/*                       getCoordinateDimension()                       */
/************************************************************************/

int OGRPoint::getCoordinateDimension()

{
    if( z == 0 )
        return 2;
    else
        return 3;
}

/************************************************************************/
/*                          getGeometryType()                           */
/************************************************************************/

OGRwkbGeometryType OGRPoint::getGeometryType()

{
    if( z == 0 )
        return wkbPoint;
    else
        return wkbPoint25D;
}

/************************************************************************/
/*                          getGeometryName()                           */
/************************************************************************/

const char * OGRPoint::getGeometryName()

{
    return "POINT";
}

/************************************************************************/
/*                            flattenTo2D()                             */
/************************************************************************/

void OGRPoint::flattenTo2D()

{
    z = 0;
}

/************************************************************************/
/*                              WkbSize()                               */
/*                                                                      */
/*      Return the size of this object in well known binary             */
/*      representation including the byte order, and type information.  */
/************************************************************************/

int OGRPoint::WkbSize()

{
    if( z == 0)
        return 21;
    else
        return 29;
}

/************************************************************************/
/*                           importFromWkb()                            */
/*                                                                      */
/*      Initialize from serialized stream in well known binary          */
/*      format.                                                         */
/************************************************************************/

OGRErr OGRPoint::importFromWkb( unsigned char * pabyData,
                                int nSize )

{
    OGRwkbByteOrder     eByteOrder;
    
    if( nSize < 21 && nSize != -1 )
        return OGRERR_NOT_ENOUGH_DATA;

/* -------------------------------------------------------------------- */
/*      Get the byte order byte.                                        */
/* -------------------------------------------------------------------- */
    eByteOrder = DB2_V72_FIX_BYTE_ORDER((OGRwkbByteOrder) *pabyData);
    assert( eByteOrder == wkbXDR || eByteOrder == wkbNDR );

/* -------------------------------------------------------------------- */
/*      Get the geometry feature type.  For now we assume that          */
/*      geometry type is between 0 and 255 so we only have to fetch     */
/*      one byte.                                                       */
/* -------------------------------------------------------------------- */
    OGRwkbGeometryType eGeometryType;
    int                bIs3D;
    
    if( eByteOrder == wkbNDR )
    {
        eGeometryType = (OGRwkbGeometryType) pabyData[1];
        bIs3D = pabyData[4] & 0x80 || pabyData[2] & 0x80;
    }
    else
    {
        eGeometryType = (OGRwkbGeometryType) pabyData[4];
        bIs3D = pabyData[1] & 0x80 || pabyData[3] & 0x80;
    }

    assert( eGeometryType == wkbPoint );

/* -------------------------------------------------------------------- */
/*      Get the vertex.                                                 */
/* -------------------------------------------------------------------- */
    memcpy( &x, pabyData + 5, 16 );
    
    if( OGR_SWAP( eByteOrder ) )
    {
        CPL_SWAPDOUBLE( &x );
        CPL_SWAPDOUBLE( &y );
    }

    if( bIs3D )
    {
        memcpy( &z, pabyData + 5 + 16, 8 );
        if( OGR_SWAP( eByteOrder ) )
        {
            CPL_SWAPDOUBLE( &z );
        }
        
    }
    else
        z = 0;

    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkb()                             */
/*                                                                      */
/*      Build a well known binary representation of this object.        */
/************************************************************************/

OGRErr  OGRPoint::exportToWkb( OGRwkbByteOrder eByteOrder,
                               unsigned char * pabyData )

{
/* -------------------------------------------------------------------- */
/*      Set the byte order.                                             */
/* -------------------------------------------------------------------- */
    pabyData[0] = (unsigned char) eByteOrder;

/* -------------------------------------------------------------------- */
/*      Set the geometry feature type.                                  */
/* -------------------------------------------------------------------- */
    GUInt32 nGType = getGeometryType();
    
    if( eByteOrder == wkbNDR )
        nGType = CPL_LSBWORD32( nGType );
    else
        nGType = CPL_MSBWORD32( nGType );

    memcpy( pabyData + 1, &nGType, 4 );
    
/* -------------------------------------------------------------------- */
/*      Copy in the raw data.                                           */
/* -------------------------------------------------------------------- */
    memcpy( pabyData+5, &x, 16 );

    if( z != 0 )
    {
        memcpy( pabyData + 5 + 16, &z, 8 );
    }
    
/* -------------------------------------------------------------------- */
/*      Swap if needed.                                                 */
/* -------------------------------------------------------------------- */
    if( OGR_SWAP( eByteOrder ) )
    {
        CPL_SWAPDOUBLE( pabyData + 5 );
        CPL_SWAPDOUBLE( pabyData + 5 + 8 );

        if( z != 0 )
            CPL_SWAPDOUBLE( pabyData + 5 + 16 );
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                           importFromWkt()                            */
/*                                                                      */
/*      Instantiate point from well known text format ``POINT           */
/*      (x,y)''.                                                        */
/************************************************************************/

OGRErr OGRPoint::importFromWkt( char ** ppszInput )

{
    char        szToken[OGR_WKT_TOKEN_MAX];
    const char  *pszInput = *ppszInput;

/* -------------------------------------------------------------------- */
/*      Read and verify the ``POINT'' keyword token.                    */
/* -------------------------------------------------------------------- */
    pszInput = OGRWktReadToken( pszInput, szToken );

    if( !EQUAL(szToken,"POINT") )
        return OGRERR_CORRUPT_DATA;

/* -------------------------------------------------------------------- */
/*      Read the point list which should consist of exactly one point.  */
/* -------------------------------------------------------------------- */
    OGRRawPoint         *poPoints = NULL;
    double              *padfZ = NULL;
    int                 nMaxPoint = 0, nPoints = 0;

    pszInput = OGRWktReadPoints( pszInput, &poPoints, &padfZ,
                                 &nMaxPoint, &nPoints );
    if( pszInput == NULL || nPoints != 1 )
        return OGRERR_CORRUPT_DATA;

    x = poPoints[0].x;
    y = poPoints[0].y;

    CPLFree( poPoints );

    if( padfZ != NULL )
    {
        z = padfZ[0];
        CPLFree( padfZ );
    }

    *ppszInput = (char *) pszInput;
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkt()                             */
/*                                                                      */
/*      Translate this structure into it's well known text format       */
/*      equivelent.                                                     */
/************************************************************************/

OGRErr OGRPoint::exportToWkt( char ** ppszDstText )

{
    char        szTextEquiv[100];
    char        szCoordinate[80];

    OGRMakeWktCoordinate(szCoordinate, x, y, z);
    sprintf( szTextEquiv, "POINT (%s)", szCoordinate );
    *ppszDstText = CPLStrdup( szTextEquiv );
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                            getEnvelope()                             */
/************************************************************************/

void OGRPoint::getEnvelope( OGREnvelope * psEnvelope )

{
    psEnvelope->MinX = psEnvelope->MaxX = getX();
    psEnvelope->MinY = psEnvelope->MaxY = getY();
}



/**
 * \fn double OGRPoint::getX();
 *
 * Fetch X coordinate.
 *
 * Relates to the SFCOM IPoint::get_X() method.
 *
 * @return the X coordinate of this point. 
 */

/**
 * \fn double OGRPoint::getY();
 *
 * Fetch Y coordinate.
 *
 * Relates to the SFCOM IPoint::get_Y() method.
 *
 * @return the Y coordinate of this point. 
 */

/**
 * \fn double OGRPoint::getZ();
 *
 * Fetch Z coordinate.
 *
 * Relates to the SFCOM IPoint::get_Z() method.
 *
 * @return the Z coordinate of this point, or zero if it is a 2D point.
 */

/**
 * \fn void OGRPoint::setX( double xIn );
 *
 * Assign point X coordinate.
 *
 * There is no corresponding SFCOM method.
 */ 

/**
 * \fn void OGRPoint::setY( double yIn );
 *
 * Assign point Y coordinate.
 *
 * There is no corresponding SFCOM method.
 */ 

/**
 * \fn void OGRPoint::setZ( double zIn );
 *
 * Assign point Z coordinate.  Setting a zero zIn value will make the point
 * 2D, and setting a non-zero value will make the point 3D (wkbPoint|wkbZ).
 *
 * There is no corresponding SFCOM method.  
 */ 

/************************************************************************/
/*                               Equal()                                */
/************************************************************************/

OGRBoolean OGRPoint::Equal( OGRGeometry * poOther )

{
    OGRPoint    *poOPoint = (OGRPoint *) poOther;
    
    if( poOther == this )
        return TRUE;
    
    if( poOther->getGeometryType() != getGeometryType() )
        return FALSE;

    // we should eventually test the SRS.
    
    if( poOPoint->getX() != getX()
        || poOPoint->getY() != getY()
        || poOPoint->getZ() != getZ() )
        return FALSE;
    else
        return TRUE;
}

/************************************************************************/
/*                             transform()                              */
/************************************************************************/

OGRErr OGRPoint::transform( OGRCoordinateTransformation *poCT )

{
#ifdef DISABLE_OGRGEOM_TRANSFORM
    return OGRERR_FAILURE;
#else
    if( poCT->Transform( 1, &x, &y, &z ) )
    {
        assignSpatialReference( poCT->GetTargetCS() );
        return OGRERR_NONE;
    }
    else
        return OGRERR_FAILURE;
#endif
}
