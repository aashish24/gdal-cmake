/******************************************************************************
 * $Id$
 *
 * Project:  SDTS Translator
 * Purpose:  Include file for entire SDTS Abstraction Layer functions.
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
 * Revision 1.5  1999/05/11 14:05:59  warmerda
 * added SDTSTransfer and SDTSPolygonreader
 *
 * Revision 1.4  1999/05/07 13:45:01  warmerda
 * major upgrade to use iso8211lib
 *
 * Revision 1.3  1999/04/21 04:38:32  warmerda
 * Added new classes including SDTSPoint
 *
 * Revision 1.2  1999/03/23 15:58:01  warmerda
 * some const fixes, and attr record fixes
 *
 * Revision 1.1  1999/03/23 13:56:14  warmerda
 * New
 *
 */

#ifndef SDTS_AL_H_INCLUDED
#define STDS_AL_H_INCLUDED

#include "cpl_conv.h"
#include "iso8211.h"

class SDTS_IREF;

#define SDTS_SIZEOF_SADR	8

int SDTSGetSADR( SDTS_IREF *, DDFField *, int, double *, double *, double * );

/************************************************************************/
/*                              SDTS_IREF                               */
/*									*/
/*      Class for reading the IREF (internal reference) module.         */
/************************************************************************/
class SDTS_IREF
{
  public:
    		SDTS_IREF();
		~SDTS_IREF();

    int         Read( const char *pszFilename );

    char	*pszXAxisName;			/* XLBL */
    char  	*pszYAxisName;			/* YLBL */

    double	dfXScale;			/* SFAX */
    double      dfYScale;			/* SFAY */

    char  	*pszCoordinateFormat;		/* HFMT */
                
};

/************************************************************************/
/*                              SDTS_CATD                               */
/*                                                                      */
/*      Class for reading the directory of other files file.            */
/************************************************************************/
class SDTS_CATDEntry;

typedef enum {
  SLTUnknown,
  SLTPoint,
  SLTLine,
  SLTAttr,
  SLTPoly
} SDTSLayerType;

class SDTS_CATD
{
    char	*pszPrefixPath;

    int		nEntries;
    SDTS_CATDEntry **papoEntries;
    
  public:
    		SDTS_CATD();
                ~SDTS_CATD();

    int         Read( const char * pszFilename );

    const char  *GetModuleFilePath( const char * pszModule );

    int		GetEntryCount() { return nEntries; }
    const char * GetEntryModule(int);
    const char * GetEntryTypeDesc(int);
    const char * GetEntryFilePath(int);
    SDTSLayerType GetEntryType(int);
};

/************************************************************************/
/*                            SDTSLineReader                            */
/*                                                                      */
/*      Class for reading any of the files lines.                       */
/************************************************************************/

class SDTSRawLine;

class SDTSLineReader
{
    DDFModule   oDDFModule;

    SDTS_IREF	*poIREF;
    
  public:
    		SDTSLineReader( SDTS_IREF * );
                ~SDTSLineReader();

    int         Open( const char * );
    SDTSRawLine *GetNextLine( void );
    void	Close();
};

/************************************************************************/
/*                              SDTSModId                               */
/*                                                                      */
/*      A simple class to encapsulate a model and record                */
/*      identifier.  Implemented in sdtslib.cpp.                        */
/************************************************************************/

class SDTSModId
{
  public:
    		SDTSModId() { szModule[0] = '\0'; nRecord = -1; }

    int		Set( DDFField * );
    const char *GetName();
    
    char	szModule[8];
    long	nRecord;
};

/************************************************************************/
/*                             SDTSRawLine                              */
/*                                                                      */
/*      Simple container for the information from a line read from a    */
/*      line file.                                                      */
/************************************************************************/
class SDTSRawLine
{
  public:
    		SDTSRawLine();
                ~SDTSRawLine();

    int         Read( SDTS_IREF *, DDFRecord * );

    SDTSModId	oLine;			/* LINE field */
                
    int		nVertices;

    double	*padfX;
    double	*padfY;
    double	*padfZ;

    SDTSModId	oLeftPoly;		/* PIDL */
    SDTSModId	oRightPoly;		/* PIDR */
    SDTSModId	oStartNode;		/* SNID */
    SDTSModId	oEndNode;		/* ENID */

#define MAX_RAWLINE_ATID	3    
    int		nAttributes;
    SDTSModId	aoATID[MAX_RAWLINE_ATID];  /* ATID (attribute) references */

    void	Dump( FILE * );
};

/************************************************************************/
/*                            SDTSAttrReader                            */
/*                                                                      */
/*      Class for reading any of the primary attribute files.           */
/************************************************************************/

class SDTSAttrRecord;

class SDTSAttrReader
{
    DDFModule	oDDFModule;

    SDTS_IREF	*poIREF;
    
  public:
    		SDTSAttrReader( SDTS_IREF * );
                ~SDTSAttrReader();

    int         Open( const char * );
    DDFField	*GetNextRecord( SDTSModId * = NULL,
                                DDFRecord ** = NULL );
    void	Close();
};

/************************************************************************/
/*                           SDTSPointReader                            */
/*                                                                      */
/*      Class for reading any of the point files.                       */
/************************************************************************/

class SDTSRawPoint;

class SDTSPointReader
{
    DDFModule	oDDFModule;
    SDTS_IREF	*poIREF;
    
  public:
    		SDTSPointReader( SDTS_IREF * );
                ~SDTSPointReader();

    int         Open( const char * );
    SDTSRawPoint *GetNextPoint( void );
    void	Close();
};

/************************************************************************/
/*                             SDTSRawPoint                             */
/*                                                                      */
/*      Simple container for the information from a point.              */
/************************************************************************/

class SDTSRawPoint
{
  public:
    		SDTSRawPoint();
                ~SDTSRawPoint();

    int         Read( SDTS_IREF *, DDFRecord * );

    SDTSModId	oPoint;			/* PNTS field */
                
    double	dfX;
    double	dfY;
    double	dfZ;

#define MAX_RAWPOINT_ATID	3    
    int		nAttributes;
    SDTSModId	aoATID[MAX_RAWPOINT_ATID];  /* ATID (attribute) references */
    SDTSModId   oAreaId;		/* ARID */
};

/************************************************************************/
/*                          SDTSPolygonReader                           */
/*                                                                      */
/*      Class for reading any of the polygon files.                     */
/************************************************************************/

class SDTSRawPolygon;

class SDTSPolygonReader
{
    DDFModule	oDDFModule;
    
  public:
    		SDTSPolygonReader();
                ~SDTSPolygonReader();

    int         Open( const char * );
    SDTSRawPolygon *GetNextPolygon( void );
    void	Close();
};

/************************************************************************/
/*                             SDTSRawPolygon                           */
/*                                                                      */
/*      Simple container for the information from a polygon             */
/************************************************************************/

class SDTSRawPolygon
{
  public:
    		SDTSRawPolygon();

    int         Read( DDFRecord * );

    SDTSModId	oPolyId;
     
#define MAX_RAWPOLYGON_ATID	3    
    int		nAttributes;
    SDTSModId	aoATID[MAX_RAWPOLYGON_ATID];  /* ATID (attribute) references */
    SDTSModId   oAreaId;		      /* ARID */
};

/************************************************************************/
/*                             SDTSTransfer                             */
/************************************************************************/

class SDTSTransfer
{
  public:
    		SDTSTransfer();
                ~SDTSTransfer();

    int		Open( const char * );
    void	Close();

    int		GetLayerCount() { return nLayers; }
    SDTSLayerType GetLayerType( int );
    int		GetLayerCATDEntry( int );

    SDTSLineReader *GetLayerLineReader( int );
    SDTSPointReader *GetLayerPointReader( int );
    SDTSAttrReader *GetLayerAttrReader( int );
    DDFModule	*GetLayerModuleReader( int );

    SDTS_CATD	*GetCATD() { return &oCATD ; }
    SDTS_IREF	*GetIREF() { return &oIREF; }
                
  private:

    SDTS_CATD	oCATD;
    SDTS_IREF	oIREF;

    int		nLayers;
    int		*panLayerCATDEntry;
};

#endif /* ndef SDTS_AL_H_INCLUDED */
