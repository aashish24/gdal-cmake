/******************************************************************************
 * $Id$
 *
 * Project:  NOAA Polar Orbiter Level 1b Dataset Reader (AVHRR)
 * Purpose:  Partial implementation, can read NOAA-9(F)-NOAA-17(M) AVHRR data
 * Author:   Andrey Kiselev, a_kissel@eudoramail.com
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

CPL_CVSID("$Id$");

static GDALDriver	*poL1BDriver = NULL;

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
    WE,		// Westarn Europe CDA, Lannion, France
    SO,		// SOCC (Satellite Operations Control Center), Suitland, Maryland, USA
    WI,		// Wallops Island, Virginia, USA
    UNKNOWN_STATION
};

enum {		// Data processing center:
    CMS,	// Centre de Meteorologie Spatiale - Lannion, France
    DSS,	// Dundee Satellite Receiving Station - Dundee, Scotland, UK
    NSS,	// NOAA/NESDIS - Suitland, Maryland, USA
    UKM,	// United Kingdom Meteorological Office - Bracknell, England, UK
    UNKNOWN_CENTER
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
//    GByte	 pabyDataHeader[DATASET_HEADER_SIZE];
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
    int		nGCPStart;
    int		nGCPStep;
    int		nGCPPerLine;
    double	dfTLDist, dfTRDist, dfBLDist, dfBRDist;

    int		nBufferSize;
    int		iSpacecraftID;
    int		iDataType;	// LAC, GAC, HRPT
    int		iDataFormat;	// 10-bit packed or 16-bit unpacked
    int		nRecordDataStart;
    int		nRecordDataEnd;
    int		nDataStartOffset;
    int		nRecordSize;

    double      adfGeoTransform[6];
    char        *pszProjection; 

    FILE	*fp;

    void        ProcessRecordHeaders();
    void	FetchNOAA9GCPs(GDAL_GCP *pasGCPList, GInt16 *piRecordHeader, int iLine);
    void	FetchNOAA15GCPs(GDAL_GCP *pasGCPList, GInt32 *piRecordHeader, int iLine);
    void	UpdateCorners(GDAL_GCP *psGCP);
    void	FetchNOAA9TimeCode(TimeCode *psTime, GByte *piRecordHeader);
    void	FetchNOAA15TimeCode(TimeCode *psTime, GUInt16 *piRecordHeader);
    void	ComputeGeoref();

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
    GUInt32 *iscan;		// Packed scanline buffer
    GUInt32 iword, jword;
    int i, j;
    GUInt16 *scan;
	    
/* -------------------------------------------------------------------- */
/*      Seek to data.                                                   */
/* -------------------------------------------------------------------- */
    VSIFSeek(poGDS->fp,
	poGDS->nDataStartOffset + nBlockYOff * poGDS->nRecordSize, SEEK_SET);

/* -------------------------------------------------------------------- */
/*      Read data into the buffer.					*/
/* -------------------------------------------------------------------- */
    // Read packed scanline
    int nBlockSize = nBlockXSize * nBlockYSize;
    switch (poGDS->iDataFormat)
    {
        case PACKED10BIT:
            iscan = (GUInt32 *)CPLMalloc(poGDS->nRecordSize);
            scan = (GUInt16 *)CPLMalloc(poGDS->nBufferSize);
            VSIFRead(iscan, 1, poGDS->nRecordSize, poGDS->fp);
            j = 0;
            for(i = poGDS->nRecordDataStart / (int)sizeof(iscan[0]);
		i < poGDS->nRecordDataEnd / (int)sizeof(iscan[0]); i++)
            {
                iword = iscan[i];
#ifdef CPL_LSB
                CPL_SWAP32PTR(&iword);
#endif
                jword = iword & 0x3FF00000;
                scan[j++] = jword >> 20;
                jword = iword & 0x000FFC00;
                scan[j++] = jword >> 10;
                scan[j++] = iword & 0x000003FF;
             }
    
             for( i = 0, j = 0; i < nBlockSize; i++ )
             {
                 ((GUInt16 *) pImage)[i] = scan[j + nBand - 1];
	         j += poGDS->nBands;
             }
    
             CPLFree(iscan);
             CPLFree(scan);
	break;
	case UNPACKED16BIT: // FIXME: Not implemented yet, need a sample image
	default:
	break;
    }
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
    pszProjection = "GEOGCS[\"WGS 72\",DATUM[\"WGS_1972\",SPHEROID[\"WGS 72\",6378135,298.26,AUTHORITY[\"EPSG\",7043]],TOWGS84[0,0,4.5,0,0,0.554,0.2263],AUTHORITY[\"EPSG\",6322]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",8901]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",9108]],AXIS[\"Lat\",\"NORTH\"],AXIS[\"Long\",\"EAST\"],AUTHORITY[\"EPSG\",4322]";
    nBands = 0;
}

/************************************************************************/
/*                            ~L1BDataset()                         */
/************************************************************************/

L1BDataset::~L1BDataset()

{
    if ( pasGCPList != NULL )
        CPLFree( pasGCPList );
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
    return pszProjection;
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

void L1BDataset::ComputeGeoref()
{
    int bApproxOK = TRUE;
    GDALGCPsToGeoTransform( 4, pasCorners, adfGeoTransform, bApproxOK );
/*	adfGeoTransform[0]; //lon
	adfGeoTransform[3]; //lat*/
	;
}

/************************************************************************/
/*	Fetch timecode from the record header (NOAA9-NOAA14 version)	*/
/************************************************************************/
void L1BDataset::FetchNOAA9TimeCode(TimeCode *psTime, GByte *piRecordHeader)
{
    GUInt32 lTemp;

    lTemp = ((piRecordHeader[2] >> 1) & 0x7F);
    psTime->SetYear((lTemp > 77)?(lTemp + 1900):(lTemp + 2000)); // Avoid `Year 2000' bug
    psTime->SetDay((GUInt32)(piRecordHeader[2] & 0x01) << 8 | (GUInt32)piRecordHeader[3]);
    psTime->SetMillisecond(
	((GUInt32)(piRecordHeader[4] & 0x07) << 24) | ((GUInt32)piRecordHeader[5] << 16) |
	((GUInt32)piRecordHeader[6] << 8) | (GUInt32)piRecordHeader[7]);
}

/************************************************************************/
/*	Fetch timecode from the record header (NOAA15-NOAA17 version)	*/
/************************************************************************/
void L1BDataset::FetchNOAA15TimeCode(TimeCode *psTime, GUInt16 *piRecordHeader)
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
#else
    psTime->SetYear(piRecordHeader[1]);
    psTime->SetDay(piRecordHeader[2]);
    psTime->SetMillisecond((GUInt32)piRecordHeader[4] << 16 | (GUInt32)piRecordHeader[5]);
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
/* Fetch the GCP from the individual scanlines (NOAA9-NOAA14 version)	*/
/************************************************************************/

void L1BDataset::FetchNOAA9GCPs(GDAL_GCP *pasGCPList, GInt16 *piRecordHeader, int iLine)
{
    int		nGoodGCPs, iPixel;
    
    nGoodGCPs =(piRecordHeader[iGCPCodeOffset] <= nGCPPerLine)?
	    piRecordHeader[iGCPCodeOffset]:nGCPPerLine;
    iPixel = nGCPStart;
    int j = iGCPOffset / (int)sizeof(piRecordHeader[0]);
    while ( j < iGCPOffset / (int)sizeof(piRecordHeader[0]) + 2 * nGoodGCPs )
    {
#ifdef CPL_LSB
        pasGCPList[nGCPCount].dfGCPY = CPL_SWAP16(piRecordHeader[j]) / 128.0; j++;
        pasGCPList[nGCPCount].dfGCPX = CPL_SWAP16(piRecordHeader[j]) / 128.0; j++;
#else
        pasGCPList[nGCPCount].dfGCPY = piRecordHeader[j++] / 128.0;
        pasGCPList[nGCPCount].dfGCPX = piRecordHeader[j++] / 128.0;
#endif
//	pasGCPList[nGCPCount].pszId;
	pasGCPList[nGCPCount].dfGCPZ = 0.0;
	pasGCPList[nGCPCount].dfGCPPixel = (double)iPixel;
	iPixel += nGCPStep;
	pasGCPList[nGCPCount].dfGCPLine = (double)iLine;
        UpdateCorners(&pasGCPList[nGCPCount]);
        nGCPCount++;
    }
}

/************************************************************************/
/* Fetch the GCP from the individual scanlines (NOAA15-NOAA17 version)	*/
/************************************************************************/

void L1BDataset::FetchNOAA15GCPs(GDAL_GCP *pasGCPList, GInt32 *piRecordHeader, int iLine)
{
    GUInt32	lTemp;
    int		nGoodGCPs, iPixel;
    
    nGoodGCPs = nGCPPerLine;
    iPixel = nGCPStart;
    int j = iGCPOffset / (int)sizeof(piRecordHeader[0]);
    while ( j < iGCPOffset / (int)sizeof(piRecordHeader[0]) + 2 * nGoodGCPs )
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
//	pasGCPList[nGCPCount].pszId;
	pasGCPList[nGCPCount].dfGCPZ = 0.0;
	pasGCPList[nGCPCount].dfGCPPixel = (double)iPixel;
	iPixel += nGCPStep;
	pasGCPList[nGCPCount].dfGCPLine = (double)iLine;
        UpdateCorners(&pasGCPList[nGCPCount]);
        nGCPCount++;
    }
}

/************************************************************************/
/*			ProcessRecordHeaders()					*/
/************************************************************************/

void L1BDataset::ProcessRecordHeaders()
{
    int		iLine;
    void	*piRecordHeader;

    piRecordHeader = CPLMalloc(nRecordDataStart);
    pasGCPList = (GDAL_GCP *) CPLCalloc( GetRasterYSize() * nGCPPerLine, sizeof(GDAL_GCP) );
    GDALInitGCPs( GetRasterYSize() * nGCPPerLine, pasGCPList );
    dfTLDist = dfTRDist = dfBLDist = dfBRDist = GetRasterXSize() * GetRasterXSize() +
	    				GetRasterYSize() * GetRasterYSize();
    
    for ( iLine = 0; iLine < GetRasterYSize(); iLine++ )
    {
	VSIFSeek(fp, nDataStartOffset + iLine * nRecordSize, SEEK_SET);
	VSIFRead(piRecordHeader, 1, nRecordDataStart, fp);
	if (iSpacecraftID <= NOAA14)
	{
	    if (iLine == 0 )
	        FetchNOAA9TimeCode(&sStartTime, (GByte *) piRecordHeader);
	    else if (iLine == GetRasterYSize() - 1)
		    FetchNOAA9TimeCode(&sStopTime, (GByte *) piRecordHeader);
            FetchNOAA9GCPs(pasGCPList, (GInt16 *)piRecordHeader, iLine);
	}
	else
	{
	    if (iLine == 0)
	        FetchNOAA15TimeCode(&sStartTime, (GUInt16 *) piRecordHeader);
	    else if (iLine == GetRasterYSize() - 1)
	        FetchNOAA15TimeCode(&sStopTime, (GUInt16 *) piRecordHeader);
	    FetchNOAA15GCPs(pasGCPList, (GInt32 *)piRecordHeader, iLine);
	}
    }
    ComputeGeoref();
    CPLFree(piRecordHeader);
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
    if( !EQUALN((const char *) poOpenInfo->pabyHeader + 33, ".", 1) &&
	!EQUALN((const char *) poOpenInfo->pabyHeader + 38, ".", 1) && 
	!EQUALN((const char *) poOpenInfo->pabyHeader + 41, ".", 1) && 
	!EQUALN((const char *) poOpenInfo->pabyHeader + 48, ".", 1) && 
	!EQUALN((const char *) poOpenInfo->pabyHeader + 54, ".", 1) &&
	!EQUALN((const char *) poOpenInfo->pabyHeader + 60, ".", 1) &&
	!EQUALN((const char *) poOpenInfo->pabyHeader + 69, ".", 1) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    L1BDataset 	*poDS;
    VSIStatBuf  sStat;

    poDS = new L1BDataset();

    poDS->poDriver = poL1BDriver;
    poDS->fp = poOpenInfo->fp;
    poOpenInfo->fp = NULL;
    
/* -------------------------------------------------------------------- */
/*      Read the header.                                                */
/* -------------------------------------------------------------------- */
    VSIFSeek( poDS->fp, 0, SEEK_SET );
    // Skip TBM (Terabit memory) Header record
//    VSIFSeek( poDS->fp, TBM_HEADER_SIZE, SEEK_SET );
    VSIFRead( poDS->pabyTBMHeader, 1, TBM_HEADER_SIZE, poDS->fp );
//    VSIFRead( poDS->abyDataHeader, 1, DATASET_HEADER_SIZE, poDS->fp );

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

    // Determine number of channels and data format
    // (10-bit packed or 16-bit unpacked)
    /*if ( EQUALN((const char *)poDS->pabyTBMHeader + 74, "T", 1) )
    {*/
	 poDS->nBands = 5;
	 poDS->iDataFormat = PACKED10BIT;
    /*}
    else if ( EQUALN((const char *)poDS->pabyTBMHeader + 74, "S", 1) )
    {
        for ( int i = 97; i < 117; i++ ) // FIXME: should report type of the band
	    if (poDS->pabyTBMHeader[i] == 1)
	        poDS->nBands++;
	poDS->iDataFormat = UNPACKED16BIT;
    }
    else
	 goto bad;*/

    switch( poDS->iDataType )
    {
	case HRPT:
	case LAC:
            poDS->nRasterXSize = 2048;
	    poDS->nBufferSize = 20484;
	    poDS->nGCPStart = 25;
	    poDS->nGCPStep = 40;
	    poDS->nGCPPerLine = 51;
	    if (poDS->iSpacecraftID <= NOAA14)
	    {
                poDS->nDataStartOffset = 14922;
                poDS->nRecordSize = 14800;
	        poDS->nRecordDataStart = 448;
	        poDS->nRecordDataEnd = 14104;
		poDS->iGCPCodeOffset = 53;
	poDS->iGCPOffset = 104;
	    }
	    else if (poDS->iSpacecraftID <= NOAA17)
	    {
		poDS->nDataStartOffset = 16384;
                poDS->nRecordSize = 15872;
	        poDS->nRecordDataStart = 1264;
	        poDS->nRecordDataEnd = 14920;
		poDS->iGCPCodeOffset = 0; // XXX: not exist for NOAA15?
                poDS->iGCPOffset = 640;
	    }
	    else
		goto bad;
        break;
	case GAC:
    	    poDS->nRasterXSize = 409;
	    poDS->nBufferSize = 4092;
	    poDS->nGCPStart = 5;
	    poDS->nGCPStep = 8;
	    poDS->nGCPPerLine = 51;
	    if (poDS->iSpacecraftID <= NOAA14)
	    {
	        poDS->nDataStartOffset = 6562;
                poDS->nRecordSize = 3220;
	        poDS->nRecordDataStart = 448;
	        poDS->nRecordDataEnd = 3176;
		poDS->iGCPCodeOffset = 53;
		poDS->iGCPOffset = 104;
	    }
	    else if (poDS->iSpacecraftID <= NOAA17)
	    {
		poDS->nDataStartOffset = 9728;
                poDS->nRecordSize = 4608;
	        poDS->nRecordDataStart = 1264;
	        poDS->nRecordDataEnd = 3992;
		poDS->iGCPCodeOffset = 0; // XXX: not exist for NOAA15?
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
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( i = 1; i <= poDS->nBands; i++ )
        poDS->SetBand( i, new L1BRasterBand( poDS, i ));

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
	    pszText = "Westarn Europe CDA, Lannion, France";
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

    if( poL1BDriver == NULL )
    {
        poL1BDriver = poDriver = new GDALDriver();
        
        poDriver->pszShortName = "L1B";
        poDriver->pszLongName = "NOAA Polar Orbiter Level 1b Data Set (AVHRR)";
        poDriver->pszHelpTopic = "frmt_l1b.html";
        
        poDriver->pfnOpen = L1BDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

