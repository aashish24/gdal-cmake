/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Classes for manipulating simple features that is not specific
 *           to a particular interface technology.
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
 * Revision 1.6  1999/05/23 05:34:40  warmerda
 * added support for clone(), multipolygons and geometry collections
 *
 * Revision 1.5  1999/05/20 14:35:44  warmerda
 * added support for well known text format
 *
 * Revision 1.4  1999/05/17 14:39:13  warmerda
 * Added ICurve, and some other IGeometry and related methods.
 *
 * Revision 1.3  1999/05/14 13:30:59  warmerda
 * added IsEmpty() and IsSimple()
 *
 * Revision 1.2  1999/03/30 21:21:43  warmerda
 * added linearring/polygon support
 *
 * Revision 1.1  1999/03/29 21:21:10  warmerda
 * New
 *
 */

#ifndef _OGR_GEOMETRY_H_INCLUDED
#define _OGR_GEOMETRY_H_INLLUDED

#include "ogr_core.h"
#include "ogr_spatialref.h"

enum OGRwkbGeometryType
{
    wkbUnknown = 0,		// non-standard
    wkbPoint = 1,		// rest are standard WKB type codes
    wkbLineString = 2,
    wkbPolygon = 3,
    wkbMultiPoint = 4,
    wkbMultiLineString = 5,
    wkbMultiPolygon = 6,
    wkbGeometryCollection = 7
};

enum OGRwkbByteOrder
{
    wkbXDR = 0,
    wkbNDR = 1
};

class OGRRawPoint
{
  public:
    double	x;
    double	y;
};

/************************************************************************/
/*                             OGRGeometry                              */
/************************************************************************/
class OGRGeometry
{
  private:
    OGRSpatialReference * poSRS;		// may be NULL
    
  public:
    		OGRGeometry();
    virtual	~OGRGeometry();
                        
    // standard IGeometry
    virtual int	getDimension() = 0;
    virtual int	getCoordinateDimension() = 0;
    virtual OGRBoolean	IsEmpty() { return 0; } 
    virtual OGRBoolean	IsSimple() { return 1; }

    // IWks Interface
    virtual int	WkbSize() = 0;
    virtual OGRErr importFromWkb( unsigned char *, int=-1 )=0;
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * ) = 0;
    virtual OGRErr importFromWkt( char ** ) = 0;
    virtual OGRErr exportToWkt( char ** ) = 0;
    
    // non-standard
    virtual OGRwkbGeometryType getGeometryType() = 0;
    virtual const char *getGeometryName() = 0;
    virtual void   dumpReadable( FILE *, const char * = NULL );
    virtual OGRGeometry *clone() = 0;

    void    assignSpatialReference( OGRSpatialReference * poSR );
    OGRSpatialReference *getSpatialReference( void );
   
#ifdef notdef
    
    // I presume all these should be virtual?  How many
    // should be pure?
    OGREnvelope	*getEnvelope();

    OGRGeometry *getBoundary();

    OGRBoolean	Equal( OGRGeometry * );
    OGRBoolean	Disjoint( OGRGeometry * );
    OGRBoolean	Intersect( OGRGeometry * );
    OGRBoolean	Touch( OGRGeometry * );
    OGRBoolean	Cross( OGRGeometry * );
    OGRBoolean	Within( OGRGeometry * );
    OGRBoolean	Contains( OGRGeometry * );
    OGRBoolean	Overlap( OGRGeometry * );
    OGRBoolean	Relate( OGRGeometry *, const char * );

    double	Distance( OGRGeometry * );
    OGRGeometry *Buffer( double );
    OGRGeometry *ConvexHull();
    OGRGeometry *Intersection(OGRGeometry *);
    OGRGeometry *Union( OGRGeometry * );
    OGRGeometry *Difference( OGRGeometry * );
    OGRGeometry *SymmetricDifference( OGRGeometry * );
#endif    
};

/************************************************************************/
/*                               OGRPoint                               */
/************************************************************************/
class OGRPoint : public OGRGeometry
{
    double	x;
    double	y;

  public:
    		OGRPoint();
                OGRPoint( double, double );
                OGRPoint( OGRRawPoint & );
                OGRPoint( OGRRawPoint * );
    virtual     ~OGRPoint();

    // IWks Interface
    virtual int	WkbSize();
    virtual OGRErr importFromWkb( unsigned char *, int=-1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * );
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** );
    
    // IGeometry
    virtual int	getDimension();
    virtual int	getCoordinateDimension();

    // IPoint
    double	getX() { return x; }
    double	getY() { return y; }

    // Non standard
    void	setX( double xIn ) { x = xIn; }
    void	setY( double yIn ) { y = yIn; }

    // Non standard from OGRGeometry
    virtual OGRGeometry *clone();
    virtual const char *getGeometryName();
    virtual OGRwkbGeometryType getGeometryType();

};

/************************************************************************/
/*                               OGRCurve                               */
/*                                                                      */
/*      This is a pure virtual class.                                   */
/************************************************************************/

class OGRCurve : public OGRGeometry
{
  protected:
    int 	nPointCount;
    OGRRawPoint	*paoPoints;

  public:
    		OGRCurve();
    virtual     ~OGRCurve();

    // ICurve methods
    virtual double get_Length();
    virtual void StartPoint(OGRPoint *);
    virtual void EndPoint(OGRPoint *);
    virtual int  get_IsClosed();
    virtual void Value( double, OGRPoint * );

    // IWks Interface
    virtual int	WkbSize();
    virtual OGRErr importFromWkb( unsigned char *, int = -1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * );
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** );

    // IGeometry interface
    virtual int	getDimension();
    virtual int	getCoordinateDimension();

    // ILineString methods
    int		getNumPoints() { return nPointCount; }
    void	getPoint( int, OGRPoint * );
    double	getX( int i ) { return paoPoints[i].x; }
    double	getY( int i ) { return paoPoints[i].y; }

    // non-standard
    void	setNumPoints( int );
    void	setPoint( int, OGRPoint * );
    void	setPoint( int, double, double );
    void	setPoints( int, OGRRawPoint * );
    void	addPoint( OGRPoint * );
    void	addPoint( double, double );
};

/************************************************************************/
/*                            OGRLineString                             */
/************************************************************************/
class OGRLineString : public OGRCurve
{
  public:
    		OGRLineString();
    virtual     ~OGRLineString();

    // non standard.
    virtual OGRwkbGeometryType getGeometryType();
    virtual const char *getGeometryName();
    virtual OGRGeometry *clone();
   
};

/************************************************************************/
/*                            OGRLinearRing                             */
/*                                                                      */
/*      This is an alias for OGRLineString for now.                     */
/************************************************************************/

class OGRLinearRing : public OGRLineString
{
  private:
    friend class OGRPolygon; 
    
    // These are not IWks compatible ... just a convenience for OGRPolygon.
    virtual int	_WkbSize();
    virtual OGRErr _importFromWkb( OGRwkbByteOrder, unsigned char *, int=-1 );
    virtual OGRErr _exportToWkb( OGRwkbByteOrder, unsigned char * );
    
  public:
    			OGRLinearRing();
    			OGRLinearRing( OGRLinearRing * );

    // Non standard.
    virtual const char *getGeometryName();
    virtual OGRwkbGeometryType getGeometryType();
    virtual OGRGeometry *clone();
    
    // IWks Interface - Note this isnt really a first class object
    // for the purposes of WKB form.  These methods always fail since this
    // object cant be serialized on its own. 
    virtual int	WkbSize();
    virtual OGRErr importFromWkb( unsigned char *, int=-1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * );
};

/************************************************************************/
/*                              OGRSurface                              */
/************************************************************************/

class OGRSurface : public OGRGeometry
{
  public:
    virtual double      get_Area() = 0;
    virtual int         Centroid( OGRPoint * ) = 0;
    virtual int         PointOnSurface( OGRPoint * ) = 0;
};

/************************************************************************/
/*                              OGRPolygon                              */
/************************************************************************/
class OGRPolygon : public OGRSurface
{
    int		nRingCount;
    OGRLinearRing **papoRings;
    
  public:
    		OGRPolygon();
    virtual     ~OGRPolygon();

    // Non standard (OGRGeometry).
    virtual const char *getGeometryName();
    virtual OGRwkbGeometryType getGeometryType();
    virtual OGRGeometry *clone();
    
    // ISurface Interface
    virtual double      get_Area();
    virtual int         Centroid( OGRPoint * );
    virtual int         PointOnSurface( OGRPoint * );
    
    // IWks Interface
    virtual int	WkbSize();
    virtual OGRErr importFromWkb( unsigned char *, int = -1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * );
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** );

    // IGeometry
    virtual int	getDimension();
    virtual int	getCoordinateDimension();

    // Non standard
    void    	addRing( OGRLinearRing * );

    OGRLinearRing *getExteriorRing();
    int		getNumInteriorRings();
    OGRLinearRing *getInteriorRing( int );
	

};

/************************************************************************/
/*                        OGRGeometryCollection                         */
/************************************************************************/

class OGRGeometryCollection : public OGRGeometry
{
    int		nGeomCount;
    OGRGeometry **papoGeoms;
    
  public:
    		OGRGeometryCollection();
    virtual     ~OGRGeometryCollection();

    // Non standard (OGRGeometry).
    virtual const char *getGeometryName();
    virtual OGRwkbGeometryType getGeometryType();
    virtual OGRGeometry *clone();
    
    // IWks Interface
    virtual int	WkbSize();
    virtual OGRErr importFromWkb( unsigned char *, int = -1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * );
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** );

    // IGeometry methods
    virtual int	getDimension();
    virtual int	getCoordinateDimension();

    // IGeometryCollection
    int		getNumGeometries();
    OGRGeometry *getGeometryRef( int );

    // Non standard
    virtual OGRErr addGeometry( OGRGeometry * );
    
};

/************************************************************************/
/*                           OGRMultiPolygon                            */
/************************************************************************/

class OGRMultiPolygon : public OGRGeometryCollection
{
  public:
    // Non standard (OGRGeometry).
    virtual const char *getGeometryName();
    virtual OGRwkbGeometryType getGeometryType();
    virtual OGRGeometry *clone();
    
    // Non standard
    virtual OGRErr addGeometry( OGRGeometry * );
};


/************************************************************************/
/*                          OGRGeometryFactory                          */
/************************************************************************/
class OGRGeometryFactory
{
  public:
    static OGRErr createFromWkb( unsigned char *, OGRSpatialReference *,
                                 OGRGeometry **, int = -1 );
    static OGRErr createFromWkt( const char *, OGRSpatialReference *,
                                 OGRGeometry ** );
};

#endif /* ndef _OGR_GEOMETRY_H_INCLUDED */
