/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  SFCTable class, client side abstraction for an OLE DB spatial
 *           table based on ATL CTable. 
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Les Technologies SoftMap Inc.
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
 * Revision 1.2  1999/06/08 15:41:16  warmerda
 * added working blob/geometry support
 *
 * Revision 1.1  1999/06/08 03:50:43  warmerda
 * New
 *
 */

#ifndef SFCTABLE_H_INCLUDED
#define SFCTABLE_H_INCLUDED

#include <atldbcli.h>

class OGRFeature;
class OGRGeometry;

/************************************************************************/
/*                               SFCTable                               */
/************************************************************************/

/**
 * Abstract representation of a rowset (table) with spatial features.
 *
 * This class is intended to simplify access to spatial rowsets, and to
 * centralize all the rules for selecting geometry columns, getting the
 * spatial reference system of a rowset, and special feature access short
 * cuts with selected providers.  It is based on the ATL CTable class
 * with a dynamic accessor. 
 */

class SFCTable : public CTable<CDynamicAccessor>
{
  private:
    int         bTriedToIdentify;
    int         iBindColumn;       
    int         iGeomColumn;       /* -1 means there is none
                                      this is paoColumnInfo index, not ord. */

    void        IdentifyGeometry(); /* find the geometry column */

    BYTE        *pabyLastGeometry;

  public:
    		SFCTable();
    virtual     ~SFCTable();

    void        ReleaseIUnknowns();
    
    /** Get the spatial reference system of this rowset */
    const char *GetSpatialRefWKT();

    /** Which column contains the geometry? */
    int		GetGeometryColumn();

    /** Does this table have a geometry column? */
    int         HasGeometry();

    /** Get geometry type */
//    OGRwkbGeometryType GetGeometryType();

    /** Fetch the raw geometry data for the last row read */
    BYTE        *GetWKBGeometry( int * pnSize );

    /** Fetch the geometry as an Object */
    OGRGeometry *GetOGRGeometry();

    /** Fetch the whole record as a feature */
    OGRFeature  *GetOGRFeature();
};

#endif /* ndef SFCTABLE_H_INCLUDED */
