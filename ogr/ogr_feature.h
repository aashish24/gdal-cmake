/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Class for representing a whole feature, and layer schemas.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999,  Les Technologies SoftMap Inc.
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
 * Revision 1.4  1999/07/07 04:23:07  danmo
 * Fixed typo in  #define _OGR_..._H_INCLUDED  line
 *
 * Revision 1.3  1999/07/05 17:18:39  warmerda
 * added docs
 *
 * Revision 1.2  1999/06/11 19:21:27  warmerda
 * Fleshed out operational definitions
 *
 * Revision 1.1  1999/05/31 17:14:53  warmerda
 * New
 *
 */

#ifndef _OGR_FEATURE_H_INCLUDED
#define _OGR_FEATURE_H_INCLUDED

#include "ogr_geometry.h"

/**
 * \file ogr_feature.h
 *
 * Simple feature classes.
 */

/**
 * List of feature field types.  This list is likely to be extended in the
 * future ... avoid coding applications based on the assumption that all
 * field types can be known.
 */

enum OGRFieldType
{
    OFTInteger = 0,
    OFTIntegerList = 1,
    OFTReal = 2,
    OFTRealList = 3,
    OFTString = 4,
    OFTStringList = 5,
    OFTWideString = 6,
    OFTWideStringList = 7,
    OFTBinary = 8,
};

/**
 * Display justification for field values.
 */

enum OGRJustification
{
    OJUndefined = 0,
    OJLeft = 1,
    OJRight = 2
};

/************************************************************************/
/*                               OGRField                               */
/************************************************************************/

typedef union {
    int		Integer;
    double	Real;
    char       *String;
    // wchar	*WideString;
    
    union {
        int	nCount;
        int	*paList;
    } IntegerList;
    
    union {
        int	nCount;
        double	*paList;
    } RealList;
    
    union {
        int	nCount;
        char	*paList;
    } StringList;

//    union {
//        int	nCount;
//        wchar	*paList;
//    } WideStringList;
} OGRField;

/************************************************************************/
/*                             OGRFieldDefn                             */
/************************************************************************/

/**
 * Definition of an attribute of an OGRFeatureDefn.
 */

class OGRFieldDefn
{
  private:
    char		*pszName;
    OGRFieldType	eType;			
    OGRJustification	eJustify;		
    int			nWidth;			/* zero is variable */
    int			nPrecision;
    OGRField		uDefault;

    void		Initialize( const char *, OGRFieldType );
    
  public:
        		OGRFieldDefn( const char *, OGRFieldType );
                        OGRFieldDefn( OGRFieldDefn * );
    			~OGRFieldDefn();

    void		SetName( const char * );
    const char	       *GetNameRef() { return pszName; }

    OGRFieldType	GetType() { return eType; }
    void                SetType( OGRFieldType eTypeIn ) { eType = eTypeIn;}
    static const char  *GetFieldTypeName( OGRFieldType );

    OGRJustification    GetJustify() { return eJustify; }
    void                SetJustify( OGRJustification eJustifyIn )
					        { eJustify = eJustifyIn; }

    int			GetWidth() { return nWidth; }
    void                SetWidth( int nWidthIn ) { nWidth = nWidthIn; }

    int			GetPrecision() { return nPrecision; }
    void                SetPrecision( int nPrecisionIn )
        				        { nPrecision = nPrecisionIn; }

    void		SetDefault( const OGRField * );
    const OGRField     *GetDefaultRef() { return &uDefault; }
};

/************************************************************************/
/*                            OGRFeatureDefn                            */
/************************************************************************/

/**
 * Definition of a feature class or feature layer.
 *
 * This object contains schema information for a set of OGRFeatures.  In
 * table based systems, an OGRFeatureDefn is essentially a layer.  In more
 * object oriented approaches (such as SF CORBA) this can represent a class
 * of features but doesn't necessarily relate to all of a layer, or just one
 * layer.
 *
 * This object also can contain some other information such as a name, the
 * base geometry type and potentially other metadata.
 *
 * It is reasonable for different translators to derive classes from
 * OGRFeatureDefn with additional translator specific information. 
 */

class OGRFeatureDefn
{
  private:
    int		nRefCount;
    
    int		nFieldCount;
    OGRFieldDefn **papoFieldDefn;

    OGRwkbGeometryType eGeomType;

    char	*pszFeatureClassName;
    
  public:
    		OGRFeatureDefn( const char * pszName = NULL );
    virtual    ~OGRFeatureDefn();

    const char	*GetName() { return pszFeatureClassName; }

    int		GetFieldCount() { return nFieldCount; }
    OGRFieldDefn *GetFieldDefn( int i );
    int		GetFieldIndex( const char * );

    void	AddFieldDefn( OGRFieldDefn * );

    OGRwkbGeometryType GetGeomType() { return eGeomType; }
    void        SetGeomType( OGRwkbGeometryType );

    int		Reference() { return ++nRefCount; }
    int		Dereference() { return --nRefCount; }
    int		GetReferenceCount() { return nRefCount; }
};

/************************************************************************/
/*                              OGRFeature                              */
/************************************************************************/

/**
 * A simple feature, including geometry and attributes.
 */

class OGRFeature
{
  private:
    
    OGRFeatureDefn 	*poDefn;
    OGRGeometry		*poGeometry;
    OGRField		*pauFields;
    
  public:
    			OGRFeature( OGRFeatureDefn * );
    virtual            ~OGRFeature();                        

    OGRFeatureDefn     *GetDefnRef() { return poDefn; }
    
    OGRErr		SetGeometryDirectly( OGRGeometry * );
    OGRErr		SetGeometry( OGRGeometry * );
    OGRGeometry        *GetGeometryRef() { return poGeometry; }

    OGRFeature	       *Clone();

    int			GetFieldCount() { return poDefn->GetFieldCount(); }
    OGRFieldDefn       *GetFieldDefnRef( int iField )
        			      { return poDefn->GetFieldDefn(iField); }
    int			GetFieldIndex( const char * pszName)
                                      { return poDefn->GetFieldIndex(pszName);}
    
    OGRField	       *GetRawFieldRef( int i ) { return pauFields + i; }

    int			GetFieldAsInteger( int i );
    double		GetFieldAsDouble( int i );
    const char	       *GetFieldAsString( int i );

    void		SetField( int i, int nValue );
    void		SetField( int i, double dfValue );
    void		SetField( int i, const char * pszValue );
    void		SetField( int i, OGRField * puValue );

    void		DumpReadable( FILE * );
};

#endif /* ndef _OGR_FEATURE_H_INCLUDED */
