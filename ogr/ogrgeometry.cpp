/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements a few base methods on OGRGeometry.
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
 * Revision 1.2  1999/05/31 11:05:08  warmerda
 * added some documentation
 *
 * Revision 1.1  1999/05/20 14:35:00  warmerda
 * New
 *
 */

#include "ogr_geometry.h"
#include "ogr_p.h"
#include <assert.h>

/************************************************************************/
/*                            OGRGeometry()                             */
/************************************************************************/

OGRGeometry::OGRGeometry()

{
    poSRS = NULL;
}

/************************************************************************/
/*                            ~OGRGeometry()                            */
/************************************************************************/

OGRGeometry::~OGRGeometry()

{
    if( poSRS != NULL )
        delete poSRS;
}


/************************************************************************/
/*                       assignSpatialReference()                       */
/*                                                                      */
/*      Assign a spatial reference without trying to reproject the      */
/*      points.                                                         */
/************************************************************************/

void OGRGeometry::assignSpatialReference( OGRSpatialReference * poSR )

{
    poSRS = poSR;
}


/************************************************************************/
/*                            dumpReadable()                            */
/************************************************************************/

/**
 * Dump geometry in well known text format to indicated output file.
 */

void OGRGeometry::dumpReadable( FILE * fp, const char * pszPrefix )

{
    char	*pszWkt = NULL;
    
    if( pszPrefix == NULL )
        pszPrefix = "";

    if( exportToWkt( &pszWkt ) == OGRERR_NONE )
    {
        fprintf( fp, "%s%s\n", pszPrefix, pszWkt );
        CPLFree( pszWkt );
    }
}


/**
 * \fn int OGRGeometry::getDimension();
 *
 * Get the dimension of this object.
 *
 * This method corresponds to the SFCOM IGeometry::GetDimension() method.
 * It indicates the dimension of the object, but does not indicate the
 * dimension of the underlying space (as indicated by
 * OGRGeometry::getCoordinateDimension().
 *
 * @return 0 for points, 1 for lines and 2 for surfaces.
 */

/**
 * \fn int OGRGeometry::getCoordinateDimension();
 *
 * Get the dimension of the coordinates in this object.
 *
 * This method corresponds to the SFCOM IGeometry::GetDimension() method.
 *
 * @return in practice this always returns 2 indicating that coordinates are
 * specified within a two dimensional space.
 */

/**
 * \fn OGRBoolean OGRGeometry::IsEmpty();
 *
 * Returns TRUE (non-zero) if the object has no points.  Normally this
 * returns FALSE except between when an object is instantiated and points
 * have been assigned.
 *
 * This method relates to the SFCOM IGeometry::IsEmpty() method.
 *
 * NOTE: This method is hardcoded to return FALSE at this time.
 *
 * @return TRUE if object is empty, otherwise FALSE.
 */

/**
 * \fn OGRBoolean OGRGeometry::IsSimple();
 *
 * Returns TRUE if the geometry is simple.
 * 
 * Returns TRUE if the geometry has no anomalous geometric points, such
 * as self intersection or self tangency. The description of each
 * instantiable geometric class will include the specific conditions that
 * cause an instance of that class to be classified as not simple.
 *
 * This method relates to the SFCOM IGeometry::IsSimple() method.
 *
 * NOTE: This method is hardcoded to return TRUE at this time.
 *
 * @return TRUE if object is simple, otherwise FALSE.
 */

/**
 * \fn int OGRGeometry::WkbSize();
 *
 * Returns size of related binary representation.
 *
 * This method returns the exact number of bytes required to hold the
 * well known binary representation of this geometry object.  Its computation
 * may be slightly expensive for complex geometries.
 *
 * This method relates to the SFCOM IWks::WkbSize() method.
 *
 * @return size of binary representation in bytes.
 */

/**
 * \fn OGRErr OGRGeometry::importFromWkb( unsigned char * pabyData, int nSize);
 *
 * Assign geometry from well known binary data.
 *
 * The object must have already been instantiated as the correct derived
 * type of geometry object to match the binaries type.  This method is used
 * by the OGRGeometryFactory class, but not normally called by application
 * code.  
 * 
 * This method relates to the SFCOM IWks::ImportFromWKB() method.
 *
 * @param pabyData the binary input data.
 * @param nSize the size of pabyData in bytes, or zero if not known.
 *
 * @return OGRERR_NONE if all goes well, otherwise any of
 * OGRERR_NOT_ENOUGH_DATA, OGRERR_UNSUPPORTED_GEOMETRY_TYPE, or
 * OGRERR_CORRUPT_DATA may be returned.
 */

/**
 * \fn OGRErr OGRGeometry::exportToWkb( OGRwkbByteOrder eOrder,
 *                                      unsigned char * pabyDstBuffer );
 *
 * Convert a geometry into well known binary format.
 *
 * This method relates to the SFCOM IWks::ExportToWKB() method.
 *
 * @param eOrder One of wkbXDR or wkbNDR indicating MSB or LSB byte order
 *               respectively.
 * @param pabyDstBuffer a buffer into which the binary representation is
 *                      written.  This buffer must be at least
 *                      OGRGeometry::WkbSize() byte in size.
 *
 * @return Currently OGRERR_NONE is always returned.
 */

/**
 * \fn OGRErr OGRGeometry::importFromWkt( char ** ppszSrcText );
 *
 * Assign geometry from well known text data.
 *
 * The object must have already been instantiated as the correct derived
 * type of geometry object to match the text type.  This method is used
 * by the OGRGeometryFactory class, but not normally called by application
 * code.  
 * 
 * This method relates to the SFCOM IWks::ImportFromWKT() method.
 *
 * @param ppszSrcText pointer to a pointer to the source text.  The pointer is
 *                    updated to pointer after the consumed text.
 *
 * @return OGRERR_NONE if all goes well, otherwise any of
 * OGRERR_NOT_ENOUGH_DATA, OGRERR_UNSUPPORTED_GEOMETRY_TYPE, or
 * OGRERR_CORRUPT_DATA may be returned.
 */

/**
 * \fn OGRErr OGRGeometry::exportToWkt( char ** ppszDstText );
 *
 * Convert a geometry into well known text format.
 *
 * This method relates to the SFCOM IWks::ExportToWKT() method.
 *
 * @param ppszDstText a text buffer is allocated by the program, and assigned
 *                    to the passed pointer.
 *
 * @return Currently OGRERR_NONE is always returned.
 */

/**
 * \fn OGRwkbGeometryType OGRGeometry::getGeometryType();
 *
 * Fetch geometry type.
 *
 * @return the geometry type code.
 */

/**
 * \fn const char * OGRGeometry::getGeometryName();
 *
 * Fetch WKT name for geometry type.
 *
 * There is no SFCOM analog to this method.  
 *
 * @return name used for this geometry type in well known text format.  The
 * returned pointer is to a static internal string and should not be modified
 * or freed.
 */

/**
 * \fn OGRGeometry *OGRGeometry::clone();
 *
 * Make a copy of this object.
 *
 * This method relates to the SFCOM IGeometry::clone() method.
 *
 * @return a new object instance with the same geometry, and spatial
 * reference system as the original.
 */

/**
 * \fn void OGRGeometry::assignSpatialReference( OGRSpatialReference * poSR );
 *
 * Assign spatial reference to this object.
 *
 * This method isn't currently implemented.  When implemented it will
 * assign the spatial reference system without reprojecting the geometry.
 *
 * This is similar to the SFCOM IGeometry::put_SpatialReference() method.
 *
 * @param poSR new spatial reference system to apply.
 */

/**
 * \fn OGRSpatialReference *OGRGeometry::getSpatialReference();
 *
 * Returns spatial reference system for object.
 *
 * This method relates to the SFCOM IGeometry::get_SpatialReference() method.
 *
 * @return a reference to the spatial reference object.  The object may be
 * shared with many geometry objects, and should not be modified.
 */

