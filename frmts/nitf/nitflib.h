/******************************************************************************
 * $Id$
 *
 * Project:  NITF Read/Write Library
 * Purpose:  Main GDAL independent include file for NITF support.  
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 **********************************************************************
 * Copyright (c) 2002, Frank Warmerdam
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
 * Revision 1.10  2004/04/15 20:52:53  warmerda
 * added metadata support
 *
 * Revision 1.9  2004/04/02 20:44:37  warmerda
 * preserve APBB (actual bits per pixel) field as metadata
 *
 * Revision 1.8  2003/05/29 19:50:57  warmerda
 * added TRE in image, and RPC00B support
 *
 * Revision 1.7  2002/12/18 20:16:04  warmerda
 * support writing IGEOLO
 *
 * Revision 1.6  2002/12/18 06:35:15  warmerda
 * implement nodata support for mapped data
 *
 * Revision 1.5  2002/12/17 21:23:15  warmerda
 * implement LUT reading and writing
 *
 * Revision 1.4  2002/12/17 20:03:08  warmerda
 * added rudimentary NITF 1.1 support
 *
 * Revision 1.3  2002/12/17 05:26:26  warmerda
 * implement basic write support
 *
 * Revision 1.2  2002/12/03 04:43:54  warmerda
 * lots of work
 *
 * Revision 1.1  2002/12/02 06:09:29  warmerda
 * New
 *
 */

#ifndef NITFLIB_H_INCLUDED
#define NITFLIB_H_INCLUDED

#include "cpl_port.h"
#include "cpl_error.h"

CPL_C_START

typedef struct { 
    char szSegmentType[3]; /* one of "IM", ... */

    GUInt32 nSegmentHeaderStart;
    GUInt32 nSegmentHeaderSize;
    GUInt32 nSegmentStart;
    GUInt32 nSegmentSize;

    void *hAccess;
} NITFSegmentInfo;

typedef struct { 
    int	nLocId;
    int nLocOffset;
    int nLocSize;
} NITFLocation;


typedef struct {
    FILE    *fp;

    char    szVersion[10];

    int     nSegmentCount;
    NITFSegmentInfo *pasSegmentInfo;

    char    *pachHeader;

    int     nTREBytes;
    char    *pachTRE;

    GUInt32 *apanVQLUT[4];

    int     nLocCount;
    NITFLocation *pasLocations;

    char    **papszMetadata;
    
} NITFFile;

/* -------------------------------------------------------------------- */
/*      File level prototypes.                                          */
/* -------------------------------------------------------------------- */
NITFFile CPL_DLL *NITFOpen( const char *pszFilename, int bUpdatable );
void     CPL_DLL  NITFClose( NITFFile * );

NITFFile CPL_DLL *NITFCreate( const char *pszFilename, 
                              int nPixels, int nLines, int nBands, 
                              int nBitsPerSample, const char *pszPVType,
                              char **papszOptions );

const char CPL_DLL *NITFFindTRE( const char *pszTREData, int nTREBytes, 
                                 const char *pszTag, int *pnFoundTRESize );

/* -------------------------------------------------------------------- */
/*      Image level access.                                             */
/* -------------------------------------------------------------------- */
typedef struct {
    char      szIREPBAND[3];
    char      szISUBCAT[7];

    int       nSignificantLUTEntries;
    int       nLUTLocation;
    unsigned char *pabyLUT;

} NITFBandInfo;

typedef struct {
    NITFFile  *psFile;
    int        iSegment;
    char      *pachHeader;

    int        nRows;
    int        nCols;
    int        nBands;
    int        nBitsPerSample;

    NITFBandInfo *pasBandInfo;
    
    char       chIMODE;

    int        nBlocksPerRow;
    int        nBlocksPerColumn;
    int        nBlockWidth;
    int        nBlockHeight;

    char       szPVType[4];
    char       szIREP[9];
    char       szICAT[9];
    int        nABPP; /* signficant bits per pixel */

    char       chICORDS;
    int        nZone;
    double     dfULX;
    double     dfULY;
    double     dfURX;
    double     dfURY;
    double     dfLRX;
    double     dfLRY;
    double     dfLLX;
    double     dfLLY;

    char       *pszComments;
    char       szIC[3];
    char       szCOMRAT[5];

    int        bNoDataSet;
    int        nNoDataValue;

    int     nTREBytes;
    char    *pachTRE;

    /* Internal information not for application use. */
    
    int        nWordSize;
    int        nPixelOffset;
    int        nLineOffset;
    int        nBlockOffset;
    int        nBandOffset;

    GUInt32    *panBlockStart;

    char       **papszMetadata;
    
} NITFImage;

NITFImage CPL_DLL *NITFImageAccess( NITFFile *, int iSegment );
void      CPL_DLL  NITFImageDeaccess( NITFImage * );

int       CPL_DLL  NITFReadImageBlock( NITFImage *, int nBlockX, int nBlockY,
                                       int nBand, void *pData );
int       CPL_DLL  NITFReadImageLine( NITFImage *, int nLine, int nBand, 
                                      void *pData );
int       CPL_DLL  NITFWriteImageBlock( NITFImage *, int nBlockX, int nBlockY,
                                        int nBand, void *pData );
int       CPL_DLL  NITFWriteImageLine( NITFImage *, int nLine, int nBand, 
                                       void *pData );
int       CPL_DLL  NITFWriteLUT( NITFImage *psImage, int nBand, int nColors, 
                                 unsigned char *pabyLUT );
int       CPL_DLL  NITFWriteIGEOLO( NITFImage *psImage, char chICORDS, 
                                    double dfULX, double dfULY,
                                    double dfURX, double dfURY,
                                    double dfLRX, double dfLRY,
                                    double dfLLX, double dfLLY );


#define BLKREAD_OK    0
#define BLKREAD_NULL  1
#define BLKREAD_FAIL  2

/* -------------------------------------------------------------------- */
/*      These are really intended to be private helper stuff for the    */
/*      library.                                                        */
/* -------------------------------------------------------------------- */
char *NITFGetField( char *pszTarget, const char *pszSource, 
                    int nStart, int nLength );

/* -------------------------------------------------------------------- */
/*      location ids from the location table (from MIL-STD-2411-1).     */
/* -------------------------------------------------------------------- */

typedef enum {
    LID_HeaderComponent = 128,
    LID_LocationComponent = 129,
    LID_CoverageSectionSubheader = 130,
    LID_CompressionSectionSubsection = 131,
    LID_CompressionLookupSubsection = 132,
    LID_CompressionParameterSubsection = 133,
    LID_ColorGrayscaleSectionSubheader = 134,
    LID_ColormapSubsection = 135,
    LID_ImageDescriptionSubheader = 136,
    LID_ImageDisplayParametersSubheader = 137,
    LID_MaskSubsection = 138,
    LID_ColorConverterSubsection = 139,
    LID_SpatialDataSubsection = 140,
    LID_AttributeSectionSubheader = 141,
    LID_AttributeSubsection = 142,
    LID_ExplicitArealCoverageTable = 143,
    LID_RelatedImagesSectionSubheader = 144,
    LID_RelatedImagesSubsection = 145,
    LID_ReplaceUpdateSectionSubheader = 146,
    LID_ReplaceUpdateTable = 147,
    LID_BoundaryRectangleSectionSubheader = 148,
    LID_BoundaryRectangleTable = 149,
    LID_FrameFileIndexSectionSubHeader = 150,
    LID_FrameFileIndexSubsection = 151,
    LID_ColorTableIndexSectionSubheader = 152,
    LID_ColorTableIndexRecord = 153
} NITFLocId;

/* -------------------------------------------------------------------- */
/*      RPC structure, and function to fill it.                         */
/* -------------------------------------------------------------------- */
typedef struct  {
    int		SUCCESS;

    double	ERR_BIAS;
    double      ERR_RAND;

    double      LINE_OFF;
    double      SAMP_OFF;
    double      LAT_OFF;
    double      LONG_OFF;
    double      HEIGHT_OFF;

    double      LINE_SCALE;
    double      SAMP_SCALE;
    double      LAT_SCALE;
    double      LONG_SCALE;
    double      HEIGHT_SCALE;

    double      LINE_NUM_COEFF[20];
    double      LINE_DEN_COEFF[20];
    double      SAMP_NUM_COEFF[20];
    double      SAMP_DEN_COEFF[20];
} NITFRPC00BInfo;

int CPL_DLL NITFReadRPC00B( NITFImage *psImage, NITFRPC00BInfo * );
int CPL_DLL NITFRPCGeoToImage(NITFRPC00BInfo *, double, double, double,
                              double *, double *);

CPL_C_END

#endif /* ndef NITFLIB_H_INCLUDED */

