/******************************************************************************
 * $Id$
 *
 * Project:  NOAA Polar Orbiter Level 1b Dataset Reader (AVHRR)
 * Purpose:  Can read NOAA-9(F)-NOAA-17(M) AVHRR datasets
 * Author:   Andrey Kiselev, dron@at1895.spb.edu
 *
 ******************************************************************************
 * Copyright (c) 2002, Andrey Kiselev <a_kissel@eudoramail.com>
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
 * Revision 1.15  2002/11/27 13:45:45  dron
 * Fixed problem with L1B signature handling.
 *
 * Revision 1.14  2002/10/04 16:23:56  dron
 * Memory leak fixed.
 *
 * Revision 1.13  2002/09/04 06:50:37  warmerda
 * avoid static driver pointers
 *
 * Revision 1.12  2002/08/31 10:01:36  dron
 * Fixes in function declarations
 *
 * Revision 1.11  2002/08/29 17:05:22  dron
 * Fixes in channels description.
 *
 * Revision 1.10  2002/08/28 15:16:31  dron
 * Band descriptions added
 *
 * Revision 1.9  2002/06/26 12:28:36  dron
 * 8-bit selective extract subsets supported
 *
 * Revision 1.8  2002/06/25 18:11:26  dron
 * Added support for 16-bit selective extract subsets
 *
 * Revision 1.7  2002/06/12 21:12:25  warmerda
 * update to metadata based driver info
 *
 * Revision 1.6  2002/06/07 18:02:41  warmerda
 * don't redeclare i
 *
 * Revision 1.5  2002/06/04 16:02:28  dron
 * Fixes in georeferencing, metadata, NOAA-K/L/M support.
 * Many thanks to Markus Neteler for intensive testing.
 *
 * Revision 1.4  2002/05/21 17:37:09  dron
 * Additional meteadata.
 *
 * Revision 1.3  2002/05/18 14:01:21  dron
 * NOAA-15 fixes, georeferencing
 *
 * Revision 1.2  2002/05/16 01:26:57  warmerda
 * move up variable declaration to avoid VC++ error
 *
 * Revision 1.1  2002/05/08 16:32:20  dron
 * NOAA Polar Orbiter Dataset reader added (not full implementation yet).
 *
 *
 */

#include "gdal_priv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

CPL_C_START
void	GDALRegister_L1B(void);
CPL_C_END

enum {		// Spacecrafts:
    TIROSN,	// TIROS-N
    NOAA6,	// NOAA-6(A)
    NOAAB,	// NOAA-B
    NOAA7,	// NOAA-7(C)
    NOAA8,	// NOAA-8(E)
    NOAA9,	// NOAA-9(F)
    NOAA10,	// NOAA-10(G)
    NOAA11,	// NOAA-11(H)
    NOAA12,	// NOAA-12(D)
    NOAA13,	// NOAA-13(I)
    NOAA14,	// NOAA-14(J)
    NOAA15,	// NOAA-15(K)
    NOAA16,	// NOAA-16(L)
    NOAA17,	// NOAA-17(M)
};

enum {		// Types of datasets
    HRPT,
    LAC,
    GAC
};

enum {		// Data format
    PACKED10BIT,
    UNPACKED8BIT,
    UNPACKED16BIT
};

enum {		// Receiving stations names:
    DU,		// Dundee, Scotland, UK
    GC,		// Fairbanks, Alaska, USA (formerly Gilmore Creek)
    HO,		// Honolulu, Hawaii, USA
    MO,		// Monterey, California, USA
    WE,		// Western Europe CDA, Lannion, France
    SO,		// SOCC (Satellite Operations Control Center), Suitland, Maryland, USA
    WI,		// Wallops Island, Virginia, USA
    UNKNOWN_STATION
};

enum {		// Data processing centers:
    CMS,	// Centre de Meteorologie Spatiale - Lannion, France
    DSS,	// Dundee Satellite Receiving Station - Dundee, Scotland, UK
    NSS,	// NOAA/NESDIS - Suitland, Maryland, USA
    UKM,	// United Kingdom Meteorological Office - Bracknell, England, UK
    UNKNOWN_CENTER
};

enum {		// AVHRR Earth location indication
	ASCEND,
	DESCEND
};

const char *paszChannelsDesc[] =				// AVHRR band widths
{								// NOAA-7 -- NOAA-17 channels
"AVHRR Channel 1:  0.58  micrometers -- 0.68 micrometers",
"AVHRR Channel 2:  0.725 micrometers -- 1.10 micrometers",
"AVHRR Channel 3:  3.55  micrometers -- 3.93 micrometers",
"AVHRR Channel 4:  10.3  micrometers -- 11.3 micrometers",
"AVHRR Channel 5:  11.5  micrometers -- 12.5 micrometers",	// not in NOAA-6,-8,-10
"AVHRR Channel 5:  11.4  micrometers -- 12.4 micrometers",	// NOAA-13
								// NOAA-15 -- NOAA-17
"AVHRR Channel 3A: 1.58  micrometers -- 1.64 micrometers",
"AVHRR Channel 3B: 3.55  micrometers -- 3.93 micrometers"
};

#define TBM_HEADER_SIZE 122

/************************************************************************/
/* ==================================================================== */
/*			TimeCode (helper class)				*/
/* ==================================================================== */
/************************************************************************/

class TimeCode {
    long	lYear;
    long	lDay;
    long	lMillisecond;
    char 	pszString[100];

  public:
    void SetYear(long year)
    {
        lYear = year;
    }
    void SetDay(long day)
    {
	lDay = day;
    }
    void SetMillisecond(long millisecond)
    {
	lMillisecond = millisecond;
    }
    char* PrintTime()
    {
	sprintf(pszString, "year: %ld, day: %ld, millisecond: %ld", lYear, lDay, lMillisecond);
	return pszString;
    }
};

/************************************************************************/
/* ==================================================================== */
/*				L1BDataset				*/
/* ==================================================================== */
/************************************************************************/

class L1BDataset : public GDALDataset
{
    friend class L1BRasterBand;

    GByte	 pabyTBMHeader[TBM_HEADER_SIZE];
    char	pszRevolution[6]; // Five-digit number identifying spacecraft revolution
    int		iSource; // Source of data (receiving station name)
    int		iProcCenter; // Data processing center
    TimeCode	sStartTime;
    TimeCode	sStopTime;

    GDAL_GCP    *pasGCPList;
    GDAL_GCP	*pasCorners;
    int         nGCPCount;
    int		iGCPOffset;
    int		iGCPCodeOffset;
    int		nGCPPerLine;
    int		iLocationIndicator;
    double	dfGCPStart, dfGCPStep;
    double	dfTLDist, dfTRDist, dfBLDist, dfBRDist;

    int		nBufferSize;
    int		iSpacecraftID;
    int		iDataType;	// LAC, GAC, HRPT
    int		iDataFormat;	// 10-bit packed or 16-bit unpacked
    int		nRecordDataStart;
    int		nRecordDataEnd;
    int		nDataStartOffset;
    int		nRecordSize;
    GUInt16	iInstrumentStatus;
    GUInt32	iChannels;

    double      adfGeoTransform[6];
    char        *pszProjection;
    int		bProjDetermined;

    FILE	*fp;

    void        ProcessRecordHeaders();
    void	FetchNOAA9GCPs(GDAL_GCP *pasGCPList, GInt16 *piRecordHeader, int iLine);
    void	FetchNOAA15GCPs(GDAL_GCP *pasGCPList, GInt32 *piRecordHeader, int iLine);
    void	UpdateCorners(GDAL_GCP *psGCP);
    void	FetchNOAA9TimeCode(TimeCode *psTime, GByte *piRecordHeader, int *iLocInd);
    void	FetchNOAA15TimeCode(TimeCode *psTime, GUInt16 *piRecordHeader, int *intLocInd);
    void	ComputeGeoref();
    void	ProcessDatasetHeader();
    
  public:
                L1BDataset();
		~L1BDataset();
    
    virtual int   GetGCPCount();
    virtual const char *GetGCPProjection();
    virtual const GDAL_GCP *GetGCPs();
    static GDALDataset *Open( GDALOpenInfo * );

    CPLErr 	GetGeoTransform( double * padfTransform );
    const char *GetProjectionRef();

};

/************************************************************************/
/* ==================================================================== */
/*                            L1BRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class L1BRasterBand : public GDALRasterBand
{
    friend class L1BDataset;

  public:

    		L1BRasterBand( L1BDataset *, int );
    
//    virtual const char *GetUnitType();
//    virtual double GetNoDataValue( int *pbSuccess = NULL );
    virtual CPLErr IReadBlock( int, int, void * );
};


/************************************************************************/
/*                           L1BRasterBand()                            */
/************************************************************************/

L1BRasterBand::L1BRasterBand( L1BDataset *poDS, int nBand )

{
    this->poDS = poDS;
    this->nBand = nBand;
    eDataType = GDT_UInt16;

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;

/*    nBlocksPerRow = 1;
    nBlocksPerColumn = 0;*/

}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr L1BRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                      void * pImage )
{
    L1BDataset *poGDS = (L1BDataset *) poDS;
    GUInt32 iword, jword;
    GUInt32 *iRawScan = NULL;			// Packed scanline buffer
    GUInt16 *iScan1 = NULL, *iScan2 = NULL;	// Unpacked 16-bit scanline buffers
    GByte *byScan = NULL;			// Unpacked 8-bit scanline buffer
    int iDataOffset, i, j;
	    
/* -------------------------------------------------------------------- */
/*      Seek to data.                                                   */
/* -------------------------------------------------------------------- */
    iDataOffset = (poGDS->iLocationIndicator == DESCEND)?
	    poGDS->nDataStartOffset + nBlockYOff * poGDS->nRecordSize:
	    poGDS->nDataStartOffset +
	    (poGDS->GetRasterYSize() - nBlockYOff - 1) * poGDS->nRecordSize;
    VSIFSeek(poGDS->fp,	iDataOffset, SEEK_SET);

/* -------------------------------------------------------------------- */
/*      Read data into the buffer.					*/
/* -------------------------------------------------------------------- */
    switch (poGDS->iDataFormat)
    {
        case PACKED10BIT:
        // Read packed scanline
        iRawScan = (GUInt32 *)CPLMalloc(poGDS->nRecordSize);
        VSIFRead(iRawScan, 1, poGDS->nRecordSize, poGDS->fp);
        iScan1 = (GUInt16 *)CPLMalloc(poGDS->nBufferSize);
        j = 0;
        for(i = poGDS->nRecordDataStart / (int)sizeof(iRawScan[0]);
	    i < poGDS->nRecordDataEnd / (int)sizeof(iRawScan[0]); i++)
        {
            iword = iRawScan[i];
#ifdef CPL_LSB
            CPL_SWAP32PTR(&iword);
#endif
            jword = iword & 0x3FF00000;
            iScan1[j++] = jword >> 20;
            jword = iword & 0x000FFC00;
            iScan1[j++] = jword >> 10;
            iScan1[j++] = iword & 0x000003FF;
        }
        CPLFree(iRawScan);
	break;
	case UNPACKED16BIT:
	iScan1 = (GUInt16 *)CPLMalloc(poGDS->GetRasterXSize()
			              * poGDS->nBands * sizeof(GUInt16));
	iScan2 = (GUInt16 *)CPLMalloc(poGDS->nRecordSize);
	VSIFRead(iScan2, 1, poGDS->nRecordSize, poGDS->fp);
        for (i = 0; i < poGDS->GetRasterXSize() * poGDS->nBands; i++)
	{
	    iScan1[i] = iScan2[poGDS->nRecordDataStart / (int)sizeof(iScan2[0]) + i];
#ifdef CPL_LSB
	    CPL_SWAP16PTR(&iScan1[i]);
#endif
	}
	CPLFree(iScan2);
	break;
	case UNPACKED8BIT:
	iScan1 = (GUInt16 *)CPLMalloc(poGDS->GetRasterXSize()
			              * poGDS->nBands * sizeof(GUInt16));
	byScan = (GByte *)CPLMalloc(poGDS->nRecordSize);
	VSIFRead(byScan, 1, poGDS->nRecordSize, poGDS->fp);
        for (i = 0; i < poGDS->GetRasterXSize() * poGDS->nBands; i++)
	    iScan1[i] = byScan[poGDS->nRecordDataStart / (int)sizeof(byScan[0]) + i];
	CPLFree(byScan);
        break;
        default: // Should never be here
	break;
    }
    
    int nBlockSize = nBlockXSize * nBlockYSize;
    if (poGDS->iLocationIndicator == DESCEND)
        for( i = 0, j = 0; i < nBlockSize; i++ )
        {
            ((GUInt16 *) pImage)[i] = iScan1[j + nBand - 1];
            j += poGDS->nBands;
        }
    else
	for ( i = nBlockSize - 1, j = 0; i >= 0; i-- )
        {
            ((GUInt16 *) pImage)[i] = iScan1[j + nBand - 1];
            j += poGDS->nBands;
        }
    
    CPLFree(iScan1);
    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*				L1BDataset				*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           L1BDataset()                           */
/************************************************************************/

L1BDataset::L1BDataset()

{
    fp = NULL;
    nGCPCount = 0;
    pasGCPList = NULL;
    pasCorners = (GDAL_GCP *) CPLCalloc( 4, sizeof(GDAL_GCP) );
    pszProjection = "GEOGCS[\"WGS 72\",DATUM[\"WGS_1972\",SPHEROID[\"WGS 72\",6378135,298.26,AUTHORITY[\"EPSG\",7043]],TOWGS84[0,0,4.5,0,0,0.554,0.2263],AUTHORITY[\"EPSG\",6322]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",8901]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",9108]],AXIS[\"Lat\",\"NORTH\"],AXIS[\"Long\",\"EAST\"],AUTHORITY[\"EPSG\",4322]]";
    bProjDetermined = FALSE; // FIXME
    nBands = 0;
    iLocationIndicator = DESCEND; // XXX: should be initialised
    iChannels = 0;
    iInstrumentStatus = 0;
}

/************************************************************************/
/*                            ~L1BDataset()                         */
/************************************************************************/

L1BDataset::~L1BDataset()

{
    if ( pasGCPList )
        CPLFree( pasGCPList );
    if ( pasCorners )
	CPLFree( pasCorners );
    if( fp != NULL )
        VSIFClose( fp );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr L1BDataset::GetGeoTransform( double * padfTransform )

{
    memcpy( padfTransform, adfGeoTransform, sizeof(double) * 6 );
    return CE_None;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *L1BDataset::GetProjectionRef()

{
    if( bProjDetermined )
        return pszProjection;
    else
        return "";
}

/************************************************************************/
/*                            GetGCPCount()                             */
/************************************************************************/

int L1BDataset::GetGCPCount()

{
    return nGCPCount;
}

/************************************************************************/
/*                          GetGCPProjection()                          */
/************************************************************************/

const char *L1BDataset::GetGCPProjection()

{
    if( nGCPCount > 0 )
        return pszProjection;
    else
        return "";
}

/************************************************************************/
/*                               GetGCPs()                              */
/************************************************************************/

const GDAL_GCP *L1BDataset::GetGCPs()
{
    return pasGCPList;
}

/************************************************************************/
/*                            ComputeGeoref()				*/
/************************************************************************/

void L1BDataset::ComputeGeoref()
{
    if (nGCPCount >= 3 && bProjDetermined)
    {
        int bApproxOK = TRUE;
        GDALGCPsToGeoTransform( 4, pasCorners, adfGeoTransform, bApproxOK );
    }
    else
    {
        adfGeoTransform[0] = adfGeoTransform[2] = adfGeoTransform[3] = adfGeoTransform[4] = 0;
	adfGeoTransform[1] = adfGeoTransform[5] = 1;
    }
}

/************************************************************************/
/*	Fetch timecode from the record header (NOAA9-NOAA14 version)	*/
/************************************************************************/

void L1BDataset::FetchNOAA9TimeCode(TimeCode *psTime, GByte *piRecordHeader, int *iLocInd)
{
    GUInt32 lTemp;

    lTemp = ((piRecordHeader[2] >> 1) & 0x7F);
    psTime->SetYear((lTemp > 77)?(lTemp + 1900):(lTemp + 2000)); // Avoid `Year 2000' problem
    psTime->SetDay((GUInt32)(piRecordHeader[2] & 0x01) << 8 | (GUInt32)piRecordHeader[3]);
    psTime->SetMillisecond(
	((GUInt32)(piRecordHeader[4] & 0x07) << 24) | ((GUInt32)piRecordHeader[5] << 16) |
	((GUInt32)piRecordHeader[6] << 8) | (GUInt32)piRecordHeader[7]);
    *iLocInd = ((piRecordHeader[8] & 0x02) == 0)?ASCEND:DESCEND;
}

/************************************************************************/
/*	Fetch timecode from the record header (NOAA15-NOAA17 version)	*/
/************************************************************************/

void L1BDataset::FetchNOAA15TimeCode(TimeCode *psTime, GUInt16 *piRecordHeader, int *iLocInd)
{
    GUInt16 iTemp;
    GUInt32 lTemp;

#ifdef CPL_LSB
    iTemp = piRecordHeader[1];
    psTime->SetYear(CPL_SWAP16(iTemp));
    iTemp = piRecordHeader[2];
    psTime->SetDay(CPL_SWAP16(iTemp));
    lTemp = (GUInt32)CPL_SWAP16(piRecordHeader[4]) << 16 |
	(GUInt32)CPL_SWAP16(piRecordHeader[5]);
    psTime->SetMillisecond(lTemp);
    *iLocInd = ((CPL_SWAP16(piRecordHeader[6]) & 0x8000) == 0)?ASCEND:DESCEND; // FIXME: hemisphere
#else
    psTime->SetYear(piRecordHeader[1]);
    psTime->SetDay(piRecordHeader[2]);
    psTime->SetMillisecond((GUInt32)piRecordHeader[4] << 16 | (GUInt32)piRecordHeader[5]);
    *iLocInd = ((piRecordHeader[6] & 0x8000) == 0)?ASCEND:DESCEND;
#endif
}

/************************************************************************/
/*		Is this GCP closer to one of the corners?		*/
/************************************************************************/

void L1BDataset::UpdateCorners(GDAL_GCP *psGCP)
{
    double tldist, trdist, bldist, brdist;

    // Will cycle through all GCPs to find ones closest to corners
    tldist = psGCP->dfGCPPixel * psGCP->dfGCPPixel +
    psGCP->dfGCPLine * psGCP->dfGCPLine;
    if (tldist < dfTLDist)
    {
        memcpy(&pasCorners[0], psGCP, sizeof(GDAL_GCP));
	dfTLDist = tldist;
    }
    else
    {
	trdist = (GetRasterXSize() - psGCP->dfGCPPixel) *
	    (GetRasterXSize() - psGCP->dfGCPPixel) + psGCP->dfGCPLine * psGCP->dfGCPLine;
	if (trdist < dfTRDist)
	{
	    memcpy(&pasCorners[1], psGCP, sizeof(GDAL_GCP));
	    dfTRDist = trdist;
	}
	else
	{
	    bldist=psGCP->dfGCPPixel*psGCP->dfGCPPixel+
	        (GetRasterYSize() - psGCP->dfGCPLine) * (GetRasterYSize() - psGCP->dfGCPLine);
	    if (bldist < dfBLDist)
	    {
	        memcpy(&pasCorners[2], psGCP, sizeof(GDAL_GCP));
	        dfBLDist = bldist;
	    }
	    else
	    {
	        bldist=psGCP->dfGCPPixel*psGCP->dfGCPPixel+
	        (GetRasterYSize() - psGCP->dfGCPLine) * (GetRasterYSize() - psGCP->dfGCPLine);
		brdist = (GetRasterXSize() - psGCP->dfGCPPixel) *
	            (GetRasterXSize() - psGCP->dfGCPPixel) +
		    (GetRasterYSize() - psGCP->dfGCPLine) *
		    (GetRasterYSize() - psGCP->dfGCPLine);
		if (brdist < dfBRDist)
	        {
		    memcpy(&pasCorners[3], psGCP, sizeof(GDAL_GCP));
		    dfBRDist = brdist;
	        }
	    }
	 }
    }
}

/************************************************************************/
/* Fetch the GCPs from the individual scanlines (NOAA9-NOAA14 version)	*/
/************************************************************************/

void L1BDataset::FetchNOAA9GCPs(GDAL_GCP *pasGCPList, GInt16 *piRecordHeader, int iLine)
{
    int		nGoodGCPs, iGCPPos, j;
    GInt16	iTemp;
    double	dfPixel;
    
    nGoodGCPs = ((GByte)*(piRecordHeader + iGCPCodeOffset) <= nGCPPerLine)?
	    (GByte)*(piRecordHeader + iGCPCodeOffset):nGCPPerLine;
    dfPixel = (iLocationIndicator == DESCEND)?dfGCPStart: GetRasterXSize() - dfGCPStart;
    j = iGCPOffset / (int)sizeof(piRecordHeader[0]);
    iGCPPos = iGCPOffset / (int)sizeof(piRecordHeader[0]) + 2 * nGoodGCPs;
    while ( j < iGCPPos )
    {
#ifdef CPL_LSB
	iTemp = piRecordHeader[j++];
	pasGCPList[nGCPCount].dfGCPY = CPL_SWAP16(iTemp) / 128.0;
        iTemp = piRecordHeader[j++];
        pasGCPList[nGCPCount].dfGCPX = CPL_SWAP16(iTemp) / 128.0;
#else
        pasGCPList[nGCPCount].dfGCPY = piRecordHeader[j++] / 128.0;
        pasGCPList[nGCPCount].dfGCPX = piRecordHeader[j++] / 128.0;
#endif
	if (pasGCPList[nGCPCount].dfGCPX < -180 || pasGCPList[nGCPCount].dfGCPX > 180 ||
	    pasGCPList[nGCPCount].dfGCPY < -90 || pasGCPList[nGCPCount].dfGCPY > 90)
	    continue;
	pasGCPList[nGCPCount].pszId = NULL;
	pasGCPList[nGCPCount].dfGCPZ = 0.0;
	pasGCPList[nGCPCount].dfGCPPixel = dfPixel;
	dfPixel += (iLocationIndicator == DESCEND)?dfGCPStep:-dfGCPStep;
	pasGCPList[nGCPCount].dfGCPLine = (double)((iLocationIndicator == DESCEND)?
            iLine:GetRasterYSize() - iLine - 1) + 0.5;
        UpdateCorners(&pasGCPList[nGCPCount]);
        nGCPCount++;
    }
}

/************************************************************************/
/* Fetch the GCPs from the individual scanlines (NOAA15-NOAA17 version)	*/
/************************************************************************/

void L1BDataset::FetchNOAA15GCPs(GDAL_GCP *pasGCPList, GInt32 *piRecordHeader, int iLine)
{
    int		j, iGCPPos;
    GUInt32	lTemp;
    double	dfPixel;
    
    dfPixel = (iLocationIndicator == DESCEND)?dfGCPStart: GetRasterXSize() - dfGCPStart;
    j = iGCPOffset / (int)sizeof(piRecordHeader[0]);
    iGCPPos = iGCPOffset / (int)sizeof(piRecordHeader[0]) + 2 * nGCPPerLine;
    while ( j < iGCPPos )
    {
#ifdef CPL_LSB
	lTemp = piRecordHeader[j++];
        pasGCPList[nGCPCount].dfGCPY = CPL_SWAP32(lTemp) / 10000.0;
	lTemp = piRecordHeader[j++];
        pasGCPList[nGCPCount].dfGCPX = CPL_SWAP32(lTemp) / 10000.0;
#else
        pasGCPList[nGCPCount].dfGCPY = piRecordHeader[j++] / 10000.0;
        pasGCPList[nGCPCount].dfGCPX = piRecordHeader[j++] / 10000.0;
#endif
	if (pasGCPList[nGCPCount].dfGCPX < -180 || pasGCPList[nGCPCount].dfGCPX > 180 ||
	    pasGCPList[nGCPCount].dfGCPY < -90 || pasGCPList[nGCPCount].dfGCPY > 90)
	    continue;
	pasGCPList[nGCPCount].pszId = NULL;
	pasGCPList[nGCPCount].dfGCPZ = 0.0;
	pasGCPList[nGCPCount].dfGCPPixel = dfPixel;
	dfPixel += (iLocationIndicator == DESCEND)?dfGCPStep:-dfGCPStep;
	pasGCPList[nGCPCount].dfGCPLine = (double)((iLocationIndicator == DESCEND)?
            iLine:GetRasterYSize() - iLine - 1) + 0.5;
        UpdateCorners(&pasGCPList[nGCPCount]);
        nGCPCount++;
    }
}

/************************************************************************/
/*			ProcessRecordHeaders()					*/
/************************************************************************/

void L1BDataset::ProcessRecordHeaders()
{
    int		iLine, iLocInd;
    void	*piRecordHeader;

    piRecordHeader = CPLMalloc(nRecordDataStart);
    pasGCPList = (GDAL_GCP *) CPLCalloc( GetRasterYSize() * nGCPPerLine, sizeof(GDAL_GCP) );
    GDALInitGCPs( GetRasterYSize() * nGCPPerLine, pasGCPList );
    dfTLDist = dfTRDist = dfBLDist = dfBRDist = GetRasterXSize() * GetRasterXSize() +
	    				GetRasterYSize() * GetRasterYSize();
    VSIFSeek(fp, nDataStartOffset, SEEK_SET);
    VSIFRead(piRecordHeader, 1, nRecordDataStart, fp);
    if (iSpacecraftID <= NOAA14)
        FetchNOAA9TimeCode(&sStartTime, (GByte *) piRecordHeader, &iLocInd);
    else
	FetchNOAA15TimeCode(&sStartTime, (GUInt16 *) piRecordHeader, &iLocInd);
    iLocationIndicator = iLocInd;
    VSIFSeek(fp, nDataStartOffset + (GetRasterYSize() - 1) * nRecordSize, SEEK_SET);
    VSIFRead(piRecordHeader, 1, nRecordDataStart, fp);
    if (iSpacecraftID <= NOAA14)
        FetchNOAA9TimeCode(&sStopTime, (GByte *) piRecordHeader, &iLocInd);
    else
	FetchNOAA15TimeCode(&sStopTime, (GUInt16 *) piRecordHeader, &iLocInd);
    for ( iLine = 0; iLine < GetRasterYSize(); iLine++ )
    {
        VSIFSeek(fp, nDataStartOffset + iLine * nRecordSize, SEEK_SET);
	VSIFRead(piRecordHeader, 1, nRecordDataStart, fp);
	if (iSpacecraftID <= NOAA14)
            FetchNOAA9GCPs(pasGCPList, (GInt16 *)piRecordHeader, iLine);
	else
	    FetchNOAA15GCPs(pasGCPList, (GInt32 *)piRecordHeader, iLine);
    }
    ComputeGeoref();
    CPLFree(piRecordHeader);
}

/************************************************************************/
/*			ProcessDatasetHeader()				*/
/************************************************************************/

void L1BDataset::ProcessDatasetHeader()
{
    GUInt16 *piHeader;
    piHeader = (GUInt16 *)CPLMalloc(nDataStartOffset);
    VSIFSeek( fp, 0, SEEK_SET );
    VSIFRead( piHeader, 1, nDataStartOffset, fp );
    if (iSpacecraftID > NOAA14)
    {
	iInstrumentStatus = (piHeader + 512)[58];
#ifdef CPL_LSB
	CPL_SWAP16PTR(&iInstrumentStatus);
#endif
    }
    CPLFree( piHeader );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *L1BDataset::Open( GDALOpenInfo * poOpenInfo )

{
    int i = 0;

    if( poOpenInfo->fp == NULL /*|| poOpenInfo->nHeaderBytes < 200*/ )
        return NULL;

    // XXX: Signature is not very good
    if( !EQUALN((const char *) poOpenInfo->pabyHeader + 33, ".", 1) ||
	!EQUALN((const char *) poOpenInfo->pabyHeader + 38, ".", 1) || 
	!EQUALN((const char *) poOpenInfo->pabyHeader + 41, ".", 1) || 
	!EQUALN((const char *) poOpenInfo->pabyHeader + 48, ".", 1) || 
	!EQUALN((const char *) poOpenInfo->pabyHeader + 54, ".", 1) ||
	!EQUALN((const char *) poOpenInfo->pabyHeader + 60, ".", 1) ||
	!EQUALN((const char *) poOpenInfo->pabyHeader + 69, ".", 1) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    L1BDataset 	*poDS;
    VSIStatBuf  sStat;

    poDS = new L1BDataset();

    poDS->fp = poOpenInfo->fp;
    poOpenInfo->fp = NULL;
    
/* -------------------------------------------------------------------- */
/*      Read the header.                                                */
/* -------------------------------------------------------------------- */
    VSIFSeek( poDS->fp, 0, SEEK_SET );
    VSIFRead( poDS->pabyTBMHeader, 1, TBM_HEADER_SIZE, poDS->fp );

    // Determine processing center where the dataset was created
    if ( EQUALN((const char *) poDS->pabyTBMHeader + 30, "CMS", 3) )
         poDS->iProcCenter = CMS;
    else if ( EQUALN((const char *) poDS->pabyTBMHeader + 30, "DSS", 3) )
         poDS->iProcCenter = DSS;
    else if ( EQUALN((const char *) poDS->pabyTBMHeader + 30, "NSS", 3) )
         poDS->iProcCenter = NSS;
    else if ( EQUALN((const char *) poDS->pabyTBMHeader + 30, "UKM", 3) )
         poDS->iProcCenter = UKM;
    else
	 poDS->iProcCenter = UNKNOWN_CENTER;
    
    // Determine spacecraft type
    if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NA", 2) )
         poDS->iSpacecraftID = NOAA6;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NB", 2) )
	 poDS->iSpacecraftID = NOAAB;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NC", 2) )
	 poDS->iSpacecraftID = NOAA7;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NE", 2) )
	 poDS->iSpacecraftID = NOAA8;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NF", 2) )
	 poDS->iSpacecraftID = NOAA9;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NG", 2) )
	 poDS->iSpacecraftID = NOAA10;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NH", 2) )
	 poDS->iSpacecraftID = NOAA11;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "ND", 2) )
	 poDS->iSpacecraftID = NOAA12;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NI", 2) )
	 poDS->iSpacecraftID = NOAA13;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NJ", 2) )
	 poDS->iSpacecraftID = NOAA14;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NK", 2) )
	 poDS->iSpacecraftID = NOAA15;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NL", 2) )
	 poDS->iSpacecraftID = NOAA16;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 39, "NM", 2) )
	 poDS->iSpacecraftID = NOAA17;
    else
	 goto bad;
	   
    // Determine dataset type
    if ( EQUALN((const char *)poDS->pabyTBMHeader + 34, "HRPT", 4) )
	 poDS->iDataType = HRPT;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 34, "LHRR", 4) )
	 poDS->iDataType = LAC;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 34, "GHRR", 4) )
	 poDS->iDataType = GAC;
    else
	 goto bad;

    // Get revolution number (as string, we don't need this value for processing)
    memcpy(poDS->pszRevolution, poDS->pabyTBMHeader + 62, 5);
    poDS->pszRevolution[5] = 0;

    // Get receiving station name
    if ( EQUALN((const char *)poDS->pabyTBMHeader + 70, "DU", 2) )
	 poDS->iSource = DU;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 70, "GC", 2) )
	 poDS->iSource = GC;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 70, "HO", 2) )
	 poDS->iSource = HO;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 70, "MO", 2) )
	 poDS->iSource = MO;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 70, "WE", 2) )
	 poDS->iSource = WE;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 70, "SO", 2) )
	 poDS->iSource = SO;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 70, "WI", 2) )
	 poDS->iSource = WI;
    else
	 poDS->iSource = UNKNOWN_STATION;

    // Determine number of bands and data format
    // (10-bit packed or 16-bit unpacked)
    for ( i = 97; i < 117; i++ )
        if (poDS->pabyTBMHeader[i] == 1 || poDS->pabyTBMHeader[i] == 'Y')
	{
	    poDS->nBands++;
	    poDS->iChannels |= (1 << (i - 97));
	}
    if (poDS->nBands == 0 || poDS->nBands > 5)
    {
        poDS->nBands = 5;
	poDS->iChannels = 0x1F;
    }
    if ( EQUALN((const char *)poDS->pabyTBMHeader + 117, "10", 2) )
	poDS->iDataFormat = PACKED10BIT;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 117, "16", 2) )
        poDS->iDataFormat = UNPACKED16BIT;
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 117, "08", 2) )
        poDS->iDataFormat = UNPACKED8BIT;
    else
	goto bad;

    switch( poDS->iDataType )
    {
	case HRPT:
	case LAC:
            poDS->nRasterXSize = 2048;
	    poDS->nBufferSize = 20484;
	    poDS->dfGCPStart = 25.5; // GCPs are located at center of pixel
	    poDS->dfGCPStep = 40;
	    poDS->nGCPPerLine = 51;
	    if (poDS->iSpacecraftID <= NOAA14)
	    {
		if (poDS->iDataFormat == PACKED10BIT)
		{
		    poDS->nRecordSize = 14800;
                    poDS->nRecordDataEnd = 14104;
		}
                else if (poDS->iDataFormat == UNPACKED16BIT)
		{
		    switch(poDS->nBands)
		    {
			case 1:
			poDS->nRecordSize = 4544;
                        poDS->nRecordDataEnd = 4544;
			break;
			case 2:
			poDS->nRecordSize = 8640;
                        poDS->nRecordDataEnd = 8640;
			break;
			case 3:
		        poDS->nRecordSize = 12736;
		        poDS->nRecordDataEnd = 12736;
			break;
			case 4:
			poDS->nRecordSize = 16832;
                        poDS->nRecordDataEnd = 16832;
			break;
			case 5:
			poDS->nRecordSize = 20928;
                        poDS->nRecordDataEnd = 20928;
			break;
		    }
		}
		else // UNPACKED8BIT
		{
		    switch(poDS->nBands)
		    {
			case 1:
			poDS->nRecordSize = 2496;
                        poDS->nRecordDataEnd = 2496;
			break;
			case 2:
			poDS->nRecordSize = 4544;
                        poDS->nRecordDataEnd = 4544;
			break;
			case 3:
		        poDS->nRecordSize = 6592;
		        poDS->nRecordDataEnd = 6592;
			break;
			case 4:
			poDS->nRecordSize = 8640;
                        poDS->nRecordDataEnd = 8640;
			break;
			case 5:
			poDS->nRecordSize = 10688;
                        poDS->nRecordDataEnd = 10688;
			break;
		    }
		}
                poDS->nDataStartOffset = poDS->nRecordSize + 122;
	        poDS->nRecordDataStart = 448;
		poDS->iGCPCodeOffset = 53;
	        poDS->iGCPOffset = 104;
	    }
	    else if (poDS->iSpacecraftID <= NOAA17)
	    {
		if (poDS->iDataFormat == PACKED10BIT)
		{
		    poDS->nRecordSize = 15872;
                    poDS->nRecordDataEnd = 14920;
		}
                else if (poDS->iDataFormat == UNPACKED16BIT)
		{
		    switch(poDS->nBands)
		    {
			case 1:
			poDS->nRecordSize = 6144;
                        poDS->nRecordDataEnd = 5360;
			break;
			case 2:
			poDS->nRecordSize = 10240;
                        poDS->nRecordDataEnd = 9456;
			break;
			case 3:
		        poDS->nRecordSize = 14336;
		        poDS->nRecordDataEnd = 13552;
			break;
			case 4:
			poDS->nRecordSize = 18432;
                        poDS->nRecordDataEnd = 17648;
			break;
			case 5:
			poDS->nRecordSize = 22528;
                        poDS->nRecordDataEnd = 21744;
			break;
		    }
		}
		else // UNPACKED8BIT
		{
		    switch(poDS->nBands)
		    {
			case 1:
			poDS->nRecordSize = 4096;
                        poDS->nRecordDataEnd = 3312;
			break;
			case 2:
			poDS->nRecordSize = 6144;
                        poDS->nRecordDataEnd = 5360;
			break;
			case 3:
		        poDS->nRecordSize = 8192;
		        poDS->nRecordDataEnd = 7408;
			break;
			case 4:
			poDS->nRecordSize = 10240;
                        poDS->nRecordDataEnd = 9456;
			break;
			case 5:
			poDS->nRecordSize = 12288;
                        poDS->nRecordDataEnd = 11504;
			break;
		    }
		}
	        poDS->nDataStartOffset = poDS->nRecordSize + 512;
		poDS->nRecordDataStart = 1264;
	        		poDS->iGCPCodeOffset = 0; // XXX: not exists for NOAA15?
                poDS->iGCPOffset = 640;
	    }
	    else
		goto bad;
        break;
	case GAC:
    	    poDS->nRasterXSize = 409;
            poDS->nBufferSize = 4092;
	    poDS->dfGCPStart = 5.5; // FIXME: depends to scan direction
	    poDS->dfGCPStep = 8;
	    poDS->nGCPPerLine = 51;
	    if (poDS->iSpacecraftID <= NOAA14)
	    {
                if (poDS->iDataFormat == PACKED10BIT)
		{
		    poDS->nRecordSize = 3220;
	            poDS->nRecordDataEnd = 3176;
		}
		else if (poDS->iDataFormat == UNPACKED16BIT)
		    switch(poDS->nBands)
		    {
			case 1:
			poDS->nRecordSize = 1268;
                        poDS->nRecordDataEnd = 1266;
			break;
			case 2:
			poDS->nRecordSize = 2084;
                        poDS->nRecordDataEnd = 2084;
			break;
			case 3:
		        poDS->nRecordSize = 2904;
		        poDS->nRecordDataEnd = 2902;
			break;
			case 4:
			poDS->nRecordSize = 3720;
                        poDS->nRecordDataEnd = 3720;
			break;
			case 5:
			poDS->nRecordSize = 4540;
                        poDS->nRecordDataEnd = 4538;
			break;
		    }
		else // UNPACKED8BIT
		{
		    switch(poDS->nBands)
		    {
			case 1:
			poDS->nRecordSize = 860;
                        poDS->nRecordDataEnd = 858;
			break;
			case 2:
			poDS->nRecordSize = 1268;
                        poDS->nRecordDataEnd = 1266;
			break;
			case 3:
		        poDS->nRecordSize = 1676;
		        poDS->nRecordDataEnd = 1676;
			break;
			case 4:
			poDS->nRecordSize = 2084;
                        poDS->nRecordDataEnd = 2084;
			break;
			case 5:
			poDS->nRecordSize = 2496;
                        poDS->nRecordDataEnd = 2494;
			break;
		    }
		}
	        poDS->nDataStartOffset = poDS->nRecordSize * 2 + 122;
	        poDS->nRecordDataStart = 448;
		poDS->iGCPCodeOffset = 53;
		poDS->iGCPOffset = 104;
	    }
	    else if (poDS->iSpacecraftID <= NOAA17)
	    {
		if (poDS->iDataFormat == PACKED10BIT)
		{
		    poDS->nRecordSize = 4608;
                    poDS->nRecordDataEnd = 3992;
		}
                else if (poDS->iDataFormat == UNPACKED16BIT)
		{
		    switch(poDS->nBands)
		    {
			case 1:
			poDS->nRecordSize = 2360;
                        poDS->nRecordDataEnd = 2082;
			break;
			case 2:
			poDS->nRecordSize = 3176;
                        poDS->nRecordDataEnd = 2900;
			break;
			case 3:
		        poDS->nRecordSize = 3992;
		        poDS->nRecordDataEnd = 3718;
			break;
			case 4:
			poDS->nRecordSize = 4816;
                        poDS->nRecordDataEnd = 4536;
			break;
			case 5:
			poDS->nRecordSize = 5632;
                        poDS->nRecordDataEnd = 5354;
			break;
		    }
		}
		else // UNPACKED8BIT
		{
		    switch(poDS->nBands)
		    {
			case 1:
			poDS->nRecordSize = 1952;
                        poDS->nRecordDataEnd = 1673;
			break;
			case 2:
			poDS->nRecordSize = 2360;
                        poDS->nRecordDataEnd = 2082;
			break;
			case 3:
		        poDS->nRecordSize = 2768;
		        poDS->nRecordDataEnd = 2491;
			break;
			case 4:
			poDS->nRecordSize = 3176;
                        poDS->nRecordDataEnd = 2900;
			break;
			case 5:
			poDS->nRecordSize = 3584;
                        poDS->nRecordDataEnd = 3309;
			break;
		    }
		}
		poDS->nDataStartOffset = poDS->nRecordSize + 512;
	        poDS->nRecordDataStart = 1264;
		poDS->iGCPCodeOffset = 0; // XXX: not exists for NOAA15?
                poDS->iGCPOffset = 640;
	    }
	    else
		goto bad;
	break;
	default:
	    goto bad;
    }
    // Compute number of lines dinamycally, so we can read partially
    // downloaded files
    CPLStat(poOpenInfo->pszFilename, &sStat);
    poDS->nRasterYSize =
	(sStat.st_size - poDS->nDataStartOffset) / poDS->nRecordSize;

/* -------------------------------------------------------------------- */
/*      Load some info from header.	                                */
/* -------------------------------------------------------------------- */
    poDS->ProcessDatasetHeader();

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    int iBand;
    
    for( iBand = 1, i = 0; iBand <= poDS->nBands; iBand++ )
    {
        poDS->SetBand( iBand, new L1BRasterBand( poDS, iBand ));
        
	// Channels descriptions
        if ( poDS->iSpacecraftID >= NOAA6 && poDS->iSpacecraftID <= NOAA17 )
        {
            if ( !(i & 0x01) && poDS->iChannels & 0x01 )
	    {
	        poDS->GetRasterBand(iBand)->SetDescription( paszChannelsDesc[0] );
	        i |= 0x01;
		continue;
	    }
	    if ( !(i & 0x02) && poDS->iChannels & 0x02 )
	    {
	        poDS->GetRasterBand(iBand)->SetDescription( paszChannelsDesc[1] );
                i |= 0x02;
		continue;
	    }
	    if ( !(i & 0x04) && poDS->iChannels & 0x04 )
	    {
	        if (poDS->iSpacecraftID >= NOAA15 && poDS->iSpacecraftID <= NOAA17)
                    if (poDS->iInstrumentStatus & 0x0400)
	                poDS->GetRasterBand(iBand)->SetDescription( paszChannelsDesc[7] );
		    else
		        poDS->GetRasterBand(iBand)->SetDescription( paszChannelsDesc[6] );
	        else    
	            poDS->GetRasterBand(iBand)->SetDescription( paszChannelsDesc[2] );
		i |= 0x04;
		continue;
	    }
	    if ( !(i & 0x08) && poDS->iChannels & 0x08 )
	    {
	        poDS->GetRasterBand(iBand)->SetDescription( paszChannelsDesc[3] );
                i |= 0x08;
		continue;
	    }
	    if ( !(i & 0x10) && poDS->iChannels & 0x10 )
	    {
		if (poDS->iSpacecraftID == NOAA13)	 		// 5 NOAA-13
		    poDS->GetRasterBand(iBand)->SetDescription( paszChannelsDesc[5] );
	        else if (poDS->iSpacecraftID == NOAA6 ||
		         poDS->iSpacecraftID == NOAA8 ||
		         poDS->iSpacecraftID == NOAA10)	 	// 4 NOAA-6,-8,-10
		    poDS->GetRasterBand(iBand)->SetDescription( paszChannelsDesc[3] );
		else
	            poDS->GetRasterBand(iBand)->SetDescription( paszChannelsDesc[4] );
		i |= 0x10;
		continue;
	    }
        }
    }

/* -------------------------------------------------------------------- */
/*      Do we have GCPs?		                                */
/* -------------------------------------------------------------------- */
    if ( EQUALN((const char *)poDS->pabyTBMHeader + 96, "Y", 1) )
    {
	poDS->ProcessRecordHeaders();
    }

/* -------------------------------------------------------------------- */
/*      Get and set other important information	as metadata             */
/* -------------------------------------------------------------------- */
    char *pszText;
    switch( poDS->iSpacecraftID )
    {
	case TIROSN:
	    pszText = "TIROS-N";
	break;
	case NOAA6:
	    pszText = "NOAA-6(A)";
	break;
	case NOAAB:
	    pszText = "NOAA-B";
	break;
	case NOAA7:
	    pszText = "NOAA-7(C)";
	break;
	case NOAA8:
	    pszText = "NOAA-8(E)";
	break;
	case NOAA9:
	    pszText = "NOAA-9(F)";
	break;
	case NOAA10:
	    pszText = "NOAA-10(G)";
	break;
	case NOAA11:
	    pszText = "NOAA-11(H)";
	break;
	case NOAA12:
	    pszText = "NOAA-12(D)";
	break;
	case NOAA13:
	    pszText = "NOAA-13(I)";
	break;
	case NOAA14:
	    pszText = "NOAA-14(J)";
	break;
	case NOAA15:
	    pszText = "NOAA-15(K)";
	break;
	case NOAA16:
	    pszText = "NOAA-16(L)";
	break;
	case NOAA17:
	    pszText = "NOAA-17(M)";
	break;
	default:
	    pszText = "Unknown";
    }
    poDS->SetMetadataItem( "SATELLITE",  pszText );
    switch( poDS->iDataType )
    {
        case LAC:
	    pszText = "AVHRR LAC";
	break;
        case HRPT:
	    pszText = "AVHRR HRPT";
	break;
        case GAC:
	    pszText = "AVHRR GAC";
	break;
	default:
	    pszText = "Unknown";
    }
    poDS->SetMetadataItem( "DATA_TYPE",  pszText );
    poDS->SetMetadataItem( "REVOLUTION",  poDS->pszRevolution );
    switch( poDS->iSource )
    {
        case DU:
	    pszText = "Dundee, Scotland, UK";
	break;
        case GC:
	    pszText = "Fairbanks, Alaska, USA (formerly Gilmore Creek)";
	break;
        case HO:
	    pszText = "Honolulu, Hawaii, USA";
	break;
        case MO:
	    pszText = "Monterey, California, USA";
	break;
        case WE:
	    pszText = "Western Europe CDA, Lannion, France";
	break;
        case SO:
	    pszText = "SOCC (Satellite Operations Control Center), Suitland, Maryland, USA";
	break;
        case WI:
	    pszText = "Wallops Island, Virginia, USA";
	break;
	default:
	    pszText = "Unknown receiving station";
    }
    poDS->SetMetadataItem( "SOURCE",  pszText );
    switch( poDS->iProcCenter )
    {
        case CMS:
	    pszText = "Centre de Meteorologie Spatiale - Lannion, France";
	break;
        case DSS:
	    pszText = "Dundee Satellite Receiving Station - Dundee, Scotland, UK";
	break;
        case NSS:
	    pszText = "NOAA/NESDIS - Suitland, Maryland, USA";
	break;
        case UKM:
	    pszText = "United Kingdom Meteorological Office - Bracknell, England, UK";
	break;
	default:
	    pszText = "Unknown processing center";
    }
    poDS->SetMetadataItem( "PROCESSING_CENTER",  pszText );
    // Time of first scanline
    poDS->SetMetadataItem( "START",  poDS->sStartTime.PrintTime() );
    // Time of last scanline
    poDS->SetMetadataItem( "STOP",  poDS->sStopTime.PrintTime() );
    // AVHRR Earth location indication
    switch(poDS->iLocationIndicator)
    {
        case ASCEND:
        poDS->SetMetadataItem( "LOCATION", "Ascending" );
	break;
	case DESCEND:
	default:
	poDS->SetMetadataItem( "LOCATION", "Descending" );
	break;
    }

    return( poDS );
bad:
    delete poDS;
    return NULL;
}

/************************************************************************/
/*                        GDALRegister_L1B()				*/
/************************************************************************/

void GDALRegister_L1B()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "L1B" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "L1B" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "NOAA Polar Orbiter Level 1b Data Set" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_l1b.html" );

        poDriver->pfnOpen = L1BDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

