/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Private definitions for OGR/PostgreSQL driver.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
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
 * Revision 1.11  2002/10/09 18:30:10  warmerda
 * substantial upgrade to type handling, and preservations of width/precision
 *
 * Revision 1.10  2002/10/04 14:03:09  warmerda
 * added column name laundering support
 *
 * Revision 1.9  2002/05/09 16:03:19  warmerda
 * major upgrade to support SRS better and add ExecuteSQL
 *
 * Revision 1.8  2002/01/13 16:23:16  warmerda
 * capture dbname= parameter for use in AddGeometryColumn() call
 *
 * Revision 1.7  2001/11/15 21:19:58  warmerda
 * added soft transaction semantics
 *
 * Revision 1.6  2001/09/28 04:03:52  warmerda
 * partially upraded to PostGIS 0.6
 *
 * Revision 1.5  2001/06/26 20:59:13  warmerda
 * implement efficient spatial and attribute query support
 *
 * Revision 1.4  2001/06/19 22:29:12  warmerda
 * upgraded to include PostGIS support
 *
 * Revision 1.3  2001/06/19 15:50:23  warmerda
 * added feature attribute query support
 *
 * Revision 1.2  2000/11/23 06:03:35  warmerda
 * added Oid support
 *
 * Revision 1.1  2000/10/17 17:46:51  warmerda
 * New
 *
 */

#ifndef _OGR_PG_H_INCLUDED
#define _OGR_PG_H_INLLUDED

#include "ogrsf_frmts.h"
#include "libpq-fe.h"

/************************************************************************/
/*                            OGRPGLayer                                */
/************************************************************************/

class OGRPGDataSource;
    
class OGRPGLayer : public OGRLayer
{
  protected:
    OGRFeatureDefn     *poFeatureDefn;

    // Layer spatial reference system, and srid.
    OGRSpatialReference *poSRS;
    int                 nSRSId;

    OGRGeometry		*poFilterGeom;

    int			iNextShapeId;

    char               *GeometryToBYTEA( OGRGeometry * );
    OGRGeometry        *BYTEAToGeometry( const char * );
    Oid                 GeometryToOID( OGRGeometry * );
    OGRGeometry        *OIDToGeometry( Oid );

    OGRPGDataSource    *poDS;

    char               *pszQueryStatement;

    char	       *pszCursorName;
    PGresult	       *hCursorResult;
    int                 bCursorActive;

    int                 nResultOffset;

    int			bHasWkb;
    int                 bWkbAsOid;
    int                 bHasFid;
    int			bHasPostGISGeometry;
    char		*pszGeomColumn;

  public:
    			OGRPGLayer();
    virtual             ~OGRPGLayer();

    virtual void	ResetReading();
    virtual OGRFeature *GetNextRawFeature();
    virtual OGRFeature *GetNextFeature();

    virtual OGRFeature *GetFeature( long nFeatureId );
    
    OGRFeatureDefn *	GetLayerDefn() { return poFeatureDefn; }

    virtual OGRErr      StartTransaction();
    virtual OGRErr      CommitTransaction();
    virtual OGRErr      RollbackTransaction();

    virtual OGRSpatialReference *GetSpatialRef();

    virtual int         TestCapability( const char * );
};

/************************************************************************/
/*                           OGRPGTableLayer                            */
/************************************************************************/

class OGRPGTableLayer : public OGRPGLayer
{
    int			bUpdateAccess;

    OGRFeatureDefn     *ReadTableDefinition(const char *);

    void		BuildWhere(void);
    char 	       *BuildFields(void);
    void                BuildFullQueryStatement(void);

    char	        *pszQuery;
    char		*pszWHERE;

    int			bLaunderColumnNames;
    int			bPreservePrecision;
    
  public:
    			OGRPGTableLayer( OGRPGDataSource *,
                                         const char * pszName,
                                         int bUpdate, int nSRSId = -2 );
    			~OGRPGTableLayer();

    virtual void	ResetReading();
    virtual int         GetFeatureCount( int );

    virtual OGRGeometry *GetSpatialFilter() { return poFilterGeom; }
    virtual void	SetSpatialFilter( OGRGeometry * );

    virtual OGRErr      SetAttributeFilter( const char * );

    virtual OGRErr      SetFeature( OGRFeature *poFeature );
    virtual OGRErr      CreateFeature( OGRFeature *poFeature );
    
    virtual OGRErr      CreateField( OGRFieldDefn *poField,
                                     int bApproxOK = TRUE );
    
    virtual OGRSpatialReference *GetSpatialRef();

    virtual int         TestCapability( const char * );

    // follow methods are not base class overrides
    void		SetLaunderFlag( int bFlag ) 
				{ bLaunderColumnNames = bFlag; }
    void		SetPrecisionFlag( int bFlag ) 
				{ bPreservePrecision = bFlag; }
};

/************************************************************************/
/*                           OGRPGResultLayer                           */
/************************************************************************/

class OGRPGResultLayer : public OGRPGLayer
{
    void                BuildFullQueryStatement(void);

    char		*pszRawStatement;

    PGresult            *hInitialResult;

    int                 nFeatureCount;

  public:
    			OGRPGResultLayer( OGRPGDataSource *,
                                          const char * pszRawStatement,
                                          PGresult *hInitialResult );
    virtual             ~OGRPGResultLayer();

    OGRGeometry *	GetSpatialFilter() { return poFilterGeom; }
    void		SetSpatialFilter( OGRGeometry * ) {}

    OGRFeatureDefn     *ReadResultDefinition();

    virtual void	ResetReading();
    virtual int         GetFeatureCount( int );
};

/************************************************************************/
/*                           OGRPGDataSource                            */
/************************************************************************/

class OGRPGDataSource : public OGRDataSource
{
    OGRPGLayer        **papoLayers;
    int			nLayers;
    
    char	       *pszName;
    char               *pszDBName;

    int			bDSUpdate;
    int			bHavePostGIS;
    int			nSoftTransactionLevel;

    PGconn		*hPGConn;

    void		DeleteLayer( const char *pszLayerName );

    Oid                 nGeometryOID;

    // We maintain a list of known SRID to reduce the number of trips to
    // the database to get SRSes. 
    int                 nKnownSRID;
    int                *panSRID;
    OGRSpatialReference **papoSRS;
    
  public:
    			OGRPGDataSource();
    			~OGRPGDataSource();

    PGconn             *GetPGConn() { return hPGConn; }

    int                 FetchSRSId( OGRSpatialReference * poSRS );
    OGRSpatialReference *FetchSRS( int nSRSId );
    OGRErr              InitializeMetadataTables();

    int			Open( const char *, int bUpdate, int bTestOpen );
    int                 OpenTable( const char *, int bUpdate, int bTestOpen );

    const char	        *GetName() { return pszName; }
    int			GetLayerCount() { return nLayers; }
    OGRLayer		*GetLayer( int );

    virtual OGRLayer    *CreateLayer( const char *, 
                                      OGRSpatialReference * = NULL,
                                      OGRwkbGeometryType = wkbUnknown,
                                      char ** = NULL );

    int                 TestCapability( const char * );

    OGRErr              SoftStartTransaction();
    OGRErr              SoftCommit();
    OGRErr              SoftRollback();
    
    OGRErr              FlushSoftTransaction();

    Oid                 GetGeometryOID() { return nGeometryOID; }

    virtual OGRLayer *  ExecuteSQL( const char *pszSQLCommand,
                                    OGRGeometry *poSpatialFilter,
                                    const char *pszDialect );
    virtual void        ReleaseResultSet( OGRLayer * poLayer );
};

/************************************************************************/
/*                             OGRPGDriver                              */
/************************************************************************/

class OGRPGDriver : public OGRSFDriver
{
  public:
    		~OGRPGDriver();
                
    const char *GetName();
    OGRDataSource *Open( const char *, int );

    virtual OGRDataSource *CreateDataSource( const char *pszName,
                                             char ** = NULL );
    
    int                 TestCapability( const char * );
};


#endif /* ndef _OGR_PG_H_INCLUDED */


