/******************************************************************************
 * $Id$
 *
 * Project:  netCDF read/write Driver
 * Purpose:  GDAL bindings over netCDF library.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam
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
 * Revision 1.12  2005/08/19 20:07:55  dnadeau
 * add netcdf band metadata info
 *
 * Revision 1.11  2005/08/18 17:00:31  dnadeau
 * fix latitude/longitude variable name identification
 *
 * Revision 1.9  2005/08/17 21:43:16  dnadeau
 * support CF convention and lat/long proj.
 *
 * Revision 1.8  2005/07/29 16:21:37  fwarmerdam
 * use CPLIsNan, and float.h
 *
 * Revision 1.7  2005/07/29 02:33:32  dnadeau
 * enhance netcdf to conform to CF convention.  
 * Allow multidimensionnal array and read metatdata
 *
 * Revision 1.6  2005/05/05 15:54:49  fwarmerdam
 * PAM Enabled
 *
 * Revision 1.5  2004/10/16 14:57:31  fwarmerdam
 * Substantial rewrite by Radim to sometimes handle COARDS style datasets
 * but really only under some circumstances.   CreateCopy() removed since
 * no COARDS implementation is available.
 *
 * Revision 1.4  2004/10/14 14:51:31  fwarmerdam
 * Fixed last fix.
 *
 * Revision 1.3  2004/10/14 13:59:13  fwarmerdam
 * Added error for non-GMT netCDF files.
 *
 * Revision 1.2  2004/01/07 21:02:19  warmerda
 * fix up driver metadata
 *
 * Revision 1.1  2004/01/07 20:05:53  warmerda
 * New
 *
 */

#include <float.h>
#include "gdal_pam.h"
#include "gdal_frmts.h"
#include "cpl_string.h"
#include "netcdf.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*			     netCDFDataset				*/
/* ==================================================================== */
/************************************************************************/

class netCDFRasterBand;

class netCDFDataset : public GDALPamDataset
{
    double      adfGeoTransform[6];
    char        **papszMetadata;
    int          *panBandDimPos;         // X, Y, Z postion in array
    int          *panBandZLev;

  public:
    int         cdfid;
    char          papszDimName[NC_MAX_NAME][1024];
    int          *paDimIds;

		netCDFDataset( );
		~netCDFDataset( );
    
    static GDALDataset *Open( GDALOpenInfo * );

    CPLErr      ReadAttributes( int, int );

    CPLErr 	GetGeoTransform( double * padfTransform );    

};

/************************************************************************/
/* ==================================================================== */
/*                         netCDFRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class netCDFRasterBand : public GDALPamRasterBand
{
    nc_type nc_datatype;
    int         nZId;
    int         nZDim;
    int		nLevel;
    int         nBandXPos;
    int         nBandYPos;
    int         *panBandZPos;
    int         *panBandZLev;
    int         bNoDataSet;
    double      dfNoDataValue;
    CPLErr	    CreateBandMetadata( ); 
    
  public:

    netCDFRasterBand( netCDFDataset *poDS, 
		      int nZId, 
		      int nZDim,
		      int nLevel, 
		      int *panBandZLen,
		      int *panBandPos, 
		      int nBand );
    ~netCDFRasterBand( );
    virtual double          GetNoDataValue( int * );
    virtual CPLErr          SetNoDataValue( double );
    virtual CPLErr IReadBlock( int, int, void * );


};

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/

double netCDFRasterBand::GetNoDataValue( int * pbSuccess )

{
    if( pbSuccess )
        *pbSuccess = bNoDataSet;

    return dfNoDataValue;
}

/************************************************************************/
/*                           SetNoDataValue()                           */
/************************************************************************/

CPLErr netCDFRasterBand::SetNoDataValue( double dfNoData )

{
    bNoDataSet = TRUE;
    dfNoDataValue = dfNoData;

    return CE_None;
}

/************************************************************************/
/*                         ~netCDFRasterBand()                          */
/************************************************************************/

netCDFRasterBand::~netCDFRasterBand()
{
    if( panBandZPos ) 
	CPLFree( panBandZPos );
    if( panBandZLev )
	CPLFree( panBandZLev );
}

/************************************************************************/
/*                         CreateBandMetadata()                         */
/************************************************************************/

CPLErr netCDFRasterBand::CreateBandMetadata( ) 
{
    char     szVarName[NC_MAX_NAME];
    char     szMetaName[NC_MAX_NAME];
    char     szMetaTemp[8192];
    int      nd;
    int      i,j;
    int      Sum  = 1;
    int      Taken = 0;
    int      result = 0;
    int      status;
    int      nVarID;
    int      nDims;
    size_t   start[1];
    size_t   count[1];

    nc_type nVarType;
    netCDFDataset *poDS;

    poDS = (netCDFDataset *) this->poDS;
/* -------------------------------------------------------------------- */
/*      Compute all dimensions from Band number and save in Metadata    */
/* -------------------------------------------------------------------- */
    nc_inq_varname( poDS->cdfid, nZId, szVarName );
    nc_inq_varndims( poDS->cdfid, nZId, &nd );
/* -------------------------------------------------------------------- */
/*      Compute multidimention band position                            */
/*                                                                      */
/* BandPosition = (Total - sum(PastBandLevels) - 1)/sum(remainingLevels)*/
/* if Data[2,3,4,x,y]                                                   */
/*                                                                      */
/*  BandPos0 = (nBand ) / (3*4)                                         */
/*  BandPos1 = (nBand - BandPos0*(3*4) ) / (4)                          */
/*  BandPos2 = (nBand - BandPos0*(3*4) ) % (4)                          */
/* -------------------------------------------------------------------- */

    sprintf( szMetaName,"NETCDF_VARNAME");
    sprintf( szMetaTemp,"%s",szVarName);
    papszMetadata = CSLSetNameValue(papszMetadata, 
				    szMetaName, 
				    szMetaTemp);
    if( nd == 3 ) {
	Sum *= panBandZLev[0];
    }

    for( i=0; i < nd-2 ; i++ ) {
	if( i != nd - 2 -1 ) {
	    for( j=i+1; j < nd-2; j++ ) {
		Sum *= panBandZLev[j];
	    }
	    result = (int) ( ( nLevel-Taken ) / Sum );
	}
	else {
	    result = (int) ( ( nLevel-Taken ) % Sum );
	}

	strcpy( szVarName,poDS->papszDimName[poDS->paDimIds[panBandZPos[i]]] );

	sprintf( szMetaName,"NETCDF_DIMENSION_%s",  szVarName );

	status=nc_inq_varid(poDS->cdfid,  
			    szVarName,
			    &nVarID );

/* -------------------------------------------------------------------- */
/*      Try to uppercase the first letter of the variable               */
/* -------------------------------------------------------------------- */

	if( status != NC_NOERR ) {
	    szVarName[0]=toupper(szVarName[0]);
	    status=nc_inq_varid(poDS->cdfid,  
				szVarName,
				&nVarID );
	}

	status = nc_inq_vartype( poDS->cdfid, nVarID, &nVarType );

	status = nc_inq_varndims( poDS->cdfid, nVarID, &nDims );

	if( nDims == 1 ) {
	    count[0]=1;
	    start[0]=result;
	    switch( nVarType ) {
	    case NC_SHORT:
		short sData;
		status =  nc_get_vara_short( poDS->cdfid, nVarID, 
					     start,
					     count, &sData );
		sprintf( szMetaTemp,"%d", sData );
		break;
	    case NC_INT:
		int nData;
		status =  nc_get_vara_int( poDS->cdfid, nVarID, 
					   start,
					   count, &nData );
		sprintf( szMetaTemp,"%d", nData );
		break;
	    case NC_FLOAT:
		float fData;
		status =  nc_get_vara_float( poDS->cdfid, nVarID, 
					     start,
					     count, &fData );
		sprintf( szMetaTemp,"%f", fData );
		break;
	    case NC_DOUBLE:
		double dfData;
		status =  nc_get_vara_double( poDS->cdfid, nVarID, 
					      start,
					      count, &dfData);
		sprintf( szMetaTemp,"%g", dfData );
		break;
	    default:
		break;
	    }
	}
	else
	    sprintf( szMetaTemp,"%d", result+1);
	
	papszMetadata = CSLSetNameValue(papszMetadata, 
					szMetaName, 
					szMetaTemp);
	Taken += result * Sum;
    }
    
    return CE_None;
}

/************************************************************************/
/*                          netCDFRasterBand()                          */
/************************************************************************/

netCDFRasterBand::netCDFRasterBand( netCDFDataset *poDS, 
				    int nZId, 
				    int nZDim,
				    int nLevel, 
				    int *panBandZLev, 
				    int *panBandDimPos, 
				    int nBand)

{
    double   dfNoData;
    int      bNoDataSet = FALSE;
    nc_type  vartype=NC_NAT;
    nc_type  atttype=NC_NAT;
    size_t   attlen;
    int      status;
    char     szNoValueName[8192];


    this->panBandZPos = NULL;
    this->panBandZLev = NULL;
    this->poDS = poDS;
    this->nBand = nBand;
    this->nZId = nZId;
    this->nZDim = nZDim;
    this->nLevel = nLevel;
    this->nBandXPos = panBandDimPos[0];
    this->nBandYPos = panBandDimPos[1];

/* -------------------------------------------------------------------- */
/*      Take care of all other dimmensions                              */
/* ------------------------------------------------------------------ */
    if( nZDim > 2 ) {
	this->panBandZPos = 
	    (int *) CPLCalloc( nZDim-2, sizeof( int ) );
	this->panBandZLev = 
	    (int *) CPLCalloc( nZDim-2, sizeof( int ) );

	for ( int i=0; i < nZDim - 2; i++ ){
	    this->panBandZPos[i] = panBandDimPos[i+2];
	    this->panBandZLev[i] = panBandZLev[i];
	}

	CreateBandMetadata();
    }
    bNoDataSet    = FALSE;
    dfNoDataValue = -9999.0;

    nBlockXSize   = poDS->GetRasterXSize( );
    nBlockYSize   = 1;

/* -------------------------------------------------------------------- */
/*      Get the type of the "z" variable, our target raster array.      */
/* -------------------------------------------------------------------- */
    if( nc_inq_var( poDS->cdfid, nZId, NULL, &nc_datatype, NULL, NULL,
                   NULL ) != NC_NOERR ){
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Error in nc_var_inq() on 'z'." );
        return;
    }

    if( nc_datatype == NC_BYTE )
        eDataType = GDT_Byte;
    else if( nc_datatype == NC_SHORT )
        eDataType = GDT_Int16;
    else if( nc_datatype == NC_INT )
        eDataType = GDT_Int32;
    else if( nc_datatype == NC_FLOAT )
        eDataType = GDT_Float32;
    else if( nc_datatype == NC_DOUBLE )
        eDataType = GDT_Float64;
    else
    {
        if( nBand == 1 )
            CPLError( CE_Warning, CPLE_AppDefined, 
                      "Unsupported netCDF datatype (%d), treat as Float32.", 
                      (int) nc_datatype );
        eDataType = GDT_Float32;
    }
/* -------------------------------------------------------------------- */
/*      Find out what is No Data for this variable                      */
/* -------------------------------------------------------------------- */

    status = nc_inq_att( poDS->cdfid, nZId, 
			 _FillValue, &atttype, &attlen);

/* -------------------------------------------------------------------- */
/*      Look for either Missing_Value or _FillValue attributes          */
/* -------------------------------------------------------------------- */

    if( status == NC_NOERR ) {
	strcpy(szNoValueName, _FillValue );
    }
    else {
	status = nc_inq_att( poDS->cdfid, nZId, 
			     "missing_value", &atttype, &attlen );
	if( status == NC_NOERR ) {

	    strcpy( szNoValueName, "missing_value" );
	}
    }

    nc_inq_vartype( poDS->cdfid, nZId, &vartype );

    if( status == NC_NOERR ) {
	switch( atttype ) {
	case NC_CHAR:
	    char *fillc;
	    fillc = (char *) CPLCalloc( attlen+1, sizeof(char) );
	    status=nc_get_att_text( poDS->cdfid, nZId,
				    szNoValueName, fillc );
	    dfNoData = atof( fillc );
	    CPLFree(fillc);
	    break;
	case NC_SHORT:
	    short sNoData;
	    status = nc_get_att_short( poDS->cdfid, nZId,
				       szNoValueName, &sNoData );
	    dfNoData = (double) sNoData;
	    break;
	case NC_INT:
	    int nNoData;
	    status = nc_get_att_int( poDS->cdfid, nZId,
				     szNoValueName, &nNoData );
	    dfNoData = (double) nNoData;
	    break;
	case NC_FLOAT:
	    float fNoData;
	    status = nc_get_att_float( poDS->cdfid, nZId,
				       szNoValueName, &fNoData );
	    dfNoData = (double) fNoData;
	    break;
	case NC_DOUBLE:
	    status = nc_get_att_double( poDS->cdfid, nZId,
					 szNoValueName, &dfNoData );
	    break;
	default:
	    break;
	}
	status = nc_get_att_double( poDS->cdfid, nZId, 
				    szNoValueName, &dfNoData );
	
    } else {
	switch( vartype ) {
	case NC_BYTE:
	    /* don't do default fill-values for bytes, too risky */
	    dfNoData = 0.0;
	    break;
	case NC_CHAR:
	    dfNoData = NC_FILL_CHAR;
	    break;
	case NC_SHORT:
	    dfNoData = NC_FILL_SHORT;
	    break;
	case NC_INT:
	    dfNoData = NC_FILL_INT;
	    break;
	case NC_FLOAT:
	    dfNoData = NC_FILL_FLOAT;
	    break;
	case NC_DOUBLE:
	    dfNoData = NC_FILL_DOUBLE;
	    break;
	default:
	    dfNoData = 0.0;
	    break;
	}
	    bNoDataSet = TRUE;
    }
    SetNoDataValue( dfNoData );
    
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr netCDFRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )

{
    int    nErr;
    int    cdfid = ( ( netCDFDataset * ) poDS )->cdfid;
    size_t start[MAX_NC_DIMS], edge[MAX_NC_DIMS];
    char   pszName[1024];
    int    i,j;
    int    Sum,Taken;
    int    nd;

    *pszName='\0';
    memset( start, 0, sizeof( start ) );
    memset( edge,  0, sizeof( edge )  );
    nc_inq_varndims ( cdfid, nZId, &nd );

/* -------------------------------------------------------------------- */
/*      Locate X, Y and Z position in the array                         */
/* -------------------------------------------------------------------- */
	
    start[nBandXPos] = 0;          // x dim can move arround in array
    start[nBandYPos] = nBlockYOff; // y
        
    edge[nBandXPos] = nBlockXSize; 
    edge[nBandYPos] = 1;

    if( nd == 3 ) {
        start[panBandZPos[0]]  = nLevel;     // z
        edge [panBandZPos[0]]  = 1;
    }

/* -------------------------------------------------------------------- */
/*      Compute multidimention band position                            */
/*                                                                      */
/* BandPosition = (Total - sum(PastBandLevels) - 1)/sum(remainingLevels)*/
/* if Data[2,3,4,x,y]                                                   */
/*                                                                      */
/*  BandPos0 = (nBand ) / (3*4)                                         */
/*  BandPos1 = (nBand - (3*4) ) / (4)                                   */
/*  BandPos2 = (nBand - (3*4) ) % (4)                                   */
/* -------------------------------------------------------------------- */
    if( nd > 3 ) {
	for( i=0; i < nd-2-1 ; i++ ) {
	    Sum  = 1;
	    Taken = 0;
	    for( j=i+1; j < nd-2; j++ ) {
		Sum *= panBandZLev[j];
	    }
	    start[panBandZPos[i]] = ( nLevel-Taken ) / Sum;
	    edge[panBandZPos[i]] = 1;
	    Taken += Sum;
	}
	start[panBandZPos[i]] = ( nLevel-( Taken-Sum ) ) % Sum;
	edge[panBandZPos[i]] = 1;
    }



    if( eDataType == GDT_Byte )
        nErr = nc_get_vara_uchar( cdfid, nZId, start, edge, 
                                  (unsigned char *) pImage );
    else if( eDataType == GDT_Int16 )
        nErr = nc_get_vara_short( cdfid, nZId, start, edge, 
                                  (short int *) pImage );
    else if( eDataType == GDT_Int32 )
    {
        if( sizeof(long) == 4 )
            nErr = nc_get_vara_long( cdfid, nZId, start, edge, 
                                     (long *) pImage );
        else
            nErr = nc_get_vara_int( cdfid, nZId, start, edge, 
                                    (int *) pImage );
    }
    else if( eDataType == GDT_Float32 ){
        nErr = nc_get_vara_float( cdfid, nZId, start, edge, 
                                  (float *) pImage );
	for( i=0; i<nBlockXSize; i++ ){
	    if( CPLIsNan( ( (float *) pImage )[i] ) )
		( (float *)pImage )[i] = dfNoDataValue;
	}
    }
    else if( eDataType == GDT_Float64 ){
        nErr = nc_get_vara_double( cdfid, nZId, start, edge, 
                                   (double *) pImage );
	for( i=0; i<nBlockXSize; i++ ){
	    if( CPLIsNan( ( (double *) pImage)[i] ) ) 
		( (double *)pImage )[i] = dfNoDataValue;
	}

    }

    if( nErr != NC_NOERR )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "netCDF scanline fetch failed: %s", 
                  nc_strerror( nErr ) );
        return CE_Failure;
    }
    else
        return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*				netCDFDataset				*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           netCDFDataset()                            */
/************************************************************************/

netCDFDataset::netCDFDataset()

{
    papszMetadata   = NULL;						
}


/************************************************************************/
/*                           ~netCDFDataset()                           */
/************************************************************************/

netCDFDataset::~netCDFDataset()

{

    FlushCache();
    if( papszMetadata != NULL )
      CSLDestroy( papszMetadata );
    nc_close( cdfid );
}


/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr netCDFDataset::GetGeoTransform( double * padfTransform )

{
    memcpy( padfTransform, adfGeoTransform, sizeof(double) * 6 );
    return CE_None;
}

/************************************************************************/
/*                        ReadAttributes()                              */
/************************************************************************/

CPLErr netCDFDataset::ReadAttributes( int cdfid, int var)

{
    char    szAttrName[NC_MAX_NAME];
    char    szVarName [NC_MAX_NAME];
    char    szMetaName[NC_MAX_NAME];
    char    szMetaTemp[8192];
    nc_type nAttrType;
    size_t  nAttrLen,m;
    int     nbAttr;
    char    szTemp[NC_MAX_NAME];

    nc_inq_varnatts( cdfid, var, &nbAttr );
    if( var == NC_GLOBAL ) {
	strcpy( szVarName,"NC_GLOBAL" );
    }
    else {
	nc_inq_varname(  cdfid, var, szVarName );
    }

    for( int l=0; l < nbAttr; l++){
	
	nc_inq_attname( cdfid, var, l, szAttrName);
	sprintf( szMetaName, "%s:%s", szVarName, szAttrName  );
	*szMetaTemp='\0';
	nc_inq_att( cdfid, var, szAttrName, &nAttrType, &nAttrLen );
	
	
	switch (nAttrType) {
	case NC_CHAR:
	    char *pszTemp;
	    pszTemp = (char *) CPLCalloc( nAttrLen+1, sizeof( char ) );
	    nc_get_att_text( cdfid, var, szAttrName,pszTemp );
	    pszTemp[nAttrLen]='\0';
	    strcpy(szMetaTemp,pszTemp);
	    CPLFree(pszTemp);
	    break;
	case NC_SHORT:
	    short *psTemp;
	    
	    psTemp = (short *) CPLCalloc( nAttrLen, sizeof( short ) );
	    nc_get_att_short( cdfid, var, szAttrName, psTemp );
	    for(m=0; m < nAttrLen-1; m++) {
		sprintf( szTemp, "%d, ",psTemp[m] );
		strcat(szMetaTemp,szTemp);
	    }
	    sprintf( szTemp, "%d",psTemp[m] );
	    CPLFree(psTemp);
	    strcat(szMetaTemp,szTemp);
	    
	    break;
	case NC_INT:
	    int *pnTemp;
	    
	    pnTemp = (int *) CPLCalloc( nAttrLen, sizeof( int ) );
	    nc_get_att_int( cdfid, var, szAttrName, pnTemp );
	    for(m=0; m < nAttrLen-1; m++) {
		sprintf( szTemp, "%d",pnTemp[m] );
		strcat(szMetaTemp,szTemp);
	    }
	    sprintf( szTemp, "%d",pnTemp[m] );
	    CPLFree(pnTemp);
	    strcat(szMetaTemp,szTemp);
	    break;
	case NC_FLOAT:
	    float *pfTemp;
	    pfTemp = (float *) CPLCalloc( nAttrLen, sizeof( float ) );
	    nc_get_att_float( cdfid, var, szAttrName, pfTemp );
	    for(m=0; m < nAttrLen-1; m++) {
		sprintf( szTemp, "%e",pfTemp[m] );
		strcat(szMetaTemp,szTemp);
	    }
	    sprintf( szTemp, "%e",pfTemp[m] );
	    CPLFree(pfTemp);
	    strcat(szMetaTemp,szTemp);
	    
	    break;
	case NC_DOUBLE:
	    double *pdfTemp;
	    pdfTemp = (double *) CPLCalloc(nAttrLen, sizeof(double));
	    nc_get_att_double( cdfid, var, szAttrName, pdfTemp );
	    for(m=0; m < nAttrLen-1; m++) {
		sprintf( szTemp, "%g",pdfTemp[m] );
		strcat(szMetaTemp,szTemp);
	    }
	    sprintf( szTemp, "%g",pdfTemp[m] );
	    CPLFree(pdfTemp);
	    strcat(szMetaTemp,szTemp);
	    
	    break;
	default:
	    break;
	}

	papszMetadata = CSLSetNameValue(papszMetadata, 
					szMetaName, 
					szMetaTemp);
	
    }
	
    SetMetadata( papszMetadata );
    return CE_None;

}
/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *netCDFDataset::Open( GDALOpenInfo * poOpenInfo )

{
    size_t       xdim, ydim;
    int          j;
    unsigned int k;
    int          nDimXid=-1;
    int          nDimYid=-1;
    int          nVarLatID=-1;
    int          nVarLonID=-1;
    int          cdfid, dim_count, var_count;
    int          i = 0;
    size_t       lev_count;
    size_t       nTotLevCount = 1;
    int          nDim = 2;
    size_t       start[2], edge[2];
    int          x_range_id, y_range_id;
    int          nDimsX, nDimsY;
    double       *pdfLat, *pdfLon;
    int          status;
    int          nLatSizeArray;
    int          nLonSizeArray;
    int          nDimID;
    char attname[NC_MAX_NAME];


/* -------------------------------------------------------------------- */
/*      Does this file have the netCDF magic number?                    */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->fp == NULL || poOpenInfo->nHeaderBytes < 50 )
        return NULL;

    if( poOpenInfo->pabyHeader[0] != 'C' 
        || poOpenInfo->pabyHeader[1] != 'D' 
        || poOpenInfo->pabyHeader[2] != 'F' 
        || poOpenInfo->pabyHeader[3] != 1 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Try opening the dataset.                                        */
/* -------------------------------------------------------------------- */

    if( nc_open( poOpenInfo->pszFilename, NC_NOWRITE, &cdfid ) != NC_NOERR )
        return NULL;

    if( nc_inq_ndims( cdfid, &dim_count ) != NC_NOERR || dim_count < 2 )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "%s is a netCDF file, but not in GMT configuration.",
                  poOpenInfo->pszFilename );

        nc_close( cdfid );
        return NULL;
    }
    CPLDebug( "GDAL_netCDF", "dim_count = %d\n", dim_count );

    if( (status = nc_get_att_text( cdfid, NC_GLOBAL, "Conventions",
				   attname )) != NC_NOERR ) {
	CPLError( CE_Warning, CPLE_AppDefined, 
		  "No UNIDATA NC_GLOBAL:Conventions attribute");
        /* note that 'Conventions' is always capital 'C' in CF spec*/
    }

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    netCDFDataset 	*poDS;

    poDS = new netCDFDataset();

    poDS->cdfid = cdfid;
    
/* -------------------------------------------------------------------- */
/*      Get dimensions.  If we can't find this, then this is a          */
/*      netCDF file, but not a normal grid product.                     */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*      Find out which dimension to use for X and Y axis.               */
/* -------------------------------------------------------------------- */
    for( j=0; j < dim_count; j++ ){
	nc_inq_dimname( cdfid, j, poDS->papszDimName[j] );
/* -------------------------------------------------------------------- */
/*      Make sure variable all in lowercase before comparison           */
/* -------------------------------------------------------------------- */
	for( k=0; k < strlen(poDS->papszDimName[j]); k++ )
	    if( isupper( poDS->papszDimName[j][k] ) ) 
		poDS->papszDimName[j][k] = tolower(poDS->papszDimName[j][k]);
	
	if ( EQUAL( poDS->papszDimName[j],"lat" ) ||
	     EQUAL( poDS->papszDimName[j], "latitude" ) ||
	     EQUAL( poDS->papszDimName[j],"y" ) ){
	    nc_inq_dimlen ( cdfid, j, &ydim );
	    poDS->nRasterYSize = ydim;
	    nDimYid=j;
	    nc_inq_varid( cdfid, poDS->papszDimName[j], &nVarLatID );
	    
	}
	if ( EQUAL( poDS->papszDimName[j],"lon" ) ||
	     EQUAL( poDS->papszDimName[j], "longitude" ) ||
	     EQUAL( poDS->papszDimName[j],"x" ) ){
	    nc_inq_dimlen ( cdfid, j, &xdim );
	    poDS->nRasterXSize = xdim;
	    nDimXid=j;
	    nc_inq_varid( cdfid, poDS->papszDimName[j], &nVarLonID );
	}
    }
    
    
    if( (nDimYid == -1) || (nDimXid == -1 ) ){
	CPLError( CE_Warning, CPLE_AppDefined, 
		  "x/lon/longitude or y/lat/latitude "
		  "variable(s) not found!");
	//	return NULL;
    }
/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    if ( nc_inq_nvars ( cdfid, &var_count) != NC_NOERR )
	return NULL;    
    
    CPLDebug( "GDAL_netCDF", "var_count = %d\n", var_count );
    
    // Add new band for each variable - 3. dimension level
    
    poDS->ReadAttributes( cdfid, NC_GLOBAL );	
    for ( int var = 0; var < var_count; var++ ) {
	int nd;
	
	nc_inq_varndims ( cdfid, var, &nd );
	poDS->paDimIds = (int *)CPLCalloc(nd, sizeof( int ) );
	poDS->panBandDimPos = ( int * ) CPLCalloc( nd, sizeof( int ) );
	nc_inq_vardimid( cdfid, var, poDS->paDimIds );
	
	if ( ( nd < 2 ) || ( nd > 4 ) ) {
	    CPLFree( poDS->paDimIds );
	    CPLFree( poDS->panBandDimPos );
	    continue;
	}
/* -------------------------------------------------------------------- */
/*      Assume first dimension as Y and second as X if                  */
/*      file does not follow UNIDATA Conventions                        */
/* -------------------------------------------------------------------- */
	if( (nDimYid == -1) || (nDimXid == -1 ) ){
	    nDimYid = 0;
	    nc_inq_dimlen ( cdfid, nDimYid, &ydim );
	    poDS->nRasterYSize = ydim;
	    nDimXid = 1;
	    nc_inq_dimlen ( cdfid, nDimXid, &xdim );
	    poDS->nRasterXSize = xdim;
	}
/* -------------------------------------------------------------------- */
/*      Verify that this variable dimensions have X name and Y          */
/*      name. If it does not, I reject that variable.                   */
/* -------------------------------------------------------------------- */
	for( j=0,k=0; j < nd; j++ ){
	    if( poDS->paDimIds[j] == nDimXid ){ 
		poDS->panBandDimPos[0] = j;         // Save Position of XDim
		k++;
	    }
	    if( poDS->paDimIds[j] == nDimYid ){
		poDS->panBandDimPos[1] = j;         // Save Position of YDim
		k++;
	    }
	}
/* -------------------------------------------------------------------- */
/*      X and Y Dimension Ids were not found!                           */
/* -------------------------------------------------------------------- */
	if( k != 2 ) {
	    CPLFree( poDS->paDimIds );
	    CPLFree( poDS->panBandDimPos );
	    continue;
	}
	    
/* -------------------------------------------------------------------- */
/*      Read Metadata for each variable                                 */
/* -------------------------------------------------------------------- */
	poDS->ReadAttributes( cdfid, var );
	
/* -------------------------------------------------------------------- */
/*      Look for third dimension ID                                     */
/* -------------------------------------------------------------------- */
	poDS->panBandZLev = (int *)CPLCalloc( sizeof( nd ) - 2, 
					sizeof( int ) );
	
	nTotLevCount = 1;
	if ( dim_count > 2 ) {
	    nDim=2;
	    for( j=0; j < nd; j++ ){
		if( ( poDS->paDimIds[j] != nDimXid ) && 
		    ( poDS->paDimIds[j] != nDimYid ) ){
		    nc_inq_dimlen ( cdfid, poDS->paDimIds[j], &lev_count );
		    nTotLevCount *= lev_count;
		    poDS->panBandZLev[ nDim-2 ] = lev_count;
		    poDS->panBandDimPos[ nDim++ ] = j;  // Save Position of ZDim
		    
		}
	    }
	}
	for ( unsigned int lev = 0; lev < nTotLevCount ; lev++ ) {
	    
	    netCDFRasterBand *poBand = new netCDFRasterBand( poDS, 
							     var, 
							     nDim,
							     lev,
							     poDS->panBandZLev,
							     poDS->panBandDimPos,
							     i+1 );
	    poDS->SetBand( i+1, poBand );
	    i++;
	}
	CPLFree( poDS->paDimIds );
	CPLFree( poDS->panBandDimPos );
	CPLFree( poDS->panBandZLev );
    }
    poDS->nBands = i;

    for( j=0; j < dim_count; j++ ){
	nc_inq_dimname( cdfid, j, poDS->papszDimName[j] );
	status = nc_inq_varid( cdfid, poDS->papszDimName[j], &nDimID );
	if( status == NC_NOERR ) {
	    poDS->ReadAttributes( cdfid, nDimID );
	}
    }
/* -------------------------------------------------------------------- */
/*      Get x/y range information.                                      */
/* -------------------------------------------------------------------- */

    poDS->adfGeoTransform[0] = 0.0;
    poDS->adfGeoTransform[1] = 1.0;
    poDS->adfGeoTransform[2] = 0.0;
    poDS->adfGeoTransform[3] = 0.0;
    poDS->adfGeoTransform[4] = 0.0;
    poDS->adfGeoTransform[5] = 1.0;
    
    
    status = nc_inq_varid( cdfid, "lat", &nVarLatID );
    if ( status ==  NC_ENOTVAR ) {
	status = nc_inq_varid( cdfid, "Lat", &nVarLatID );
    }
    if ( status ==  NC_ENOTVAR ) {
	status = nc_inq_varid( cdfid, "latitude", &nVarLatID );
    } 
    if ( status ==  NC_ENOTVAR ) {
	status = nc_inq_varid( cdfid, "Latitude", &nVarLatID );
    }
    
    status = nc_inq_varid( cdfid, "lon", &nVarLonID );
    if ( status ==  NC_ENOTVAR ) {
	status = nc_inq_varid( cdfid, "Lon", &nVarLonID );
    } 
    if( status == NC_ENOTVAR ) {
	status = nc_inq_varid( cdfid, "longitude", &nVarLonID );
    } 
    if( status == NC_ENOTVAR ) {
	status = nc_inq_varid( cdfid, "Longitude", &nVarLonID );
    }
    
    nc_inq_varndims ( cdfid, nVarLatID, &nDimsY );

/* -------------------------------------------------------------------- */
/*      No dimensional arrays are not supported                         */
/* -------------------------------------------------------------------- */
    if( nDimsY == 0 ) nVarLatID = -1;

    if( (nVarLatID != -1) && (nVarLonID != -1) ) {
	nc_inq_varndims ( cdfid, nVarLatID, &nDimsY );
	nc_inq_varndims ( cdfid, nVarLonID, &nDimsX );
	if( nDimsY == 2 ) {
	    nLatSizeArray = xdim*ydim;
	    nLonSizeArray = xdim*ydim;
	    pdfLat = (double *) CPLCalloc( nLatSizeArray, sizeof(double) );
	    pdfLon = (double *) CPLCalloc( nLonSizeArray, sizeof(double) );
	    start[0] = 0;
	    start[1] = 0;
	    edge[0]  = ydim;
	    edge[1]  = xdim;
	    nc_get_vara_double( cdfid, nVarLatID, start, edge, pdfLat);
	    nc_get_vara_double( cdfid, nVarLonID, start, edge, pdfLon);
	}
	else {   // Assume 1 dimensionnal array
	    nLatSizeArray = ydim;
	    nLonSizeArray = xdim;
	    pdfLat = (double *) CPLCalloc( ydim, sizeof(double) );
	    pdfLon = (double *) CPLCalloc( xdim, sizeof(double) );
	    start[0] = 0;
	    edge [0] = ydim;
	    nc_get_vara_double( cdfid, nVarLatID, start, edge, pdfLat);
	    edge [0] = xdim;
	    nc_get_vara_double( cdfid, nVarLonID, start, edge, pdfLon);
	}

/* -------------------------------------------------------------------- */
/*      If longitude contains negative values, add 360 degrees to       */
/*      set it to eastern values.                                       */
/* -------------------------------------------------------------------- */
	
	for( int i=0; i < nLonSizeArray; i++) {
	    if( pdfLon[i] < 0 ) {
		pdfLon[i] += 360;
	    }
	}

/* -------------------------------------------------------------------- */
/*      Set transformation array                                        */
/* -------------------------------------------------------------------- */
	poDS->adfGeoTransform[0] = pdfLon[0];
	poDS->adfGeoTransform[3] = pdfLat[0];
	
	poDS->adfGeoTransform[2] = 0;
	poDS->adfGeoTransform[4] = 0;
	poDS->adfGeoTransform[1] = 
	    ( pdfLon[nLonSizeArray-1] - pdfLon[0] ) / 
	    poDS->nRasterXSize;
	poDS->adfGeoTransform[5] = 
	    ( pdfLat[nLatSizeArray-1] - pdfLat[0] ) / poDS->nRasterYSize;
	CPLFree( pdfLat );
	CPLFree( pdfLon );
	
    }

    
    if( nc_inq_varid( cdfid, "x_range", &x_range_id ) == NC_NOERR 
	&& nc_inq_varid( cdfid, "y_range", &y_range_id ) == NC_NOERR )
	{
	    double x_range[2], y_range[2];
	    
	    nc_get_vara_double( cdfid, x_range_id, start, edge, x_range );
	    nc_get_vara_double( cdfid, y_range_id, start, edge, y_range );
	    
	    poDS->adfGeoTransform[0] = x_range[0];
	    poDS->adfGeoTransform[1] = 
		( x_range[1] - x_range[0] ) / poDS->nRasterXSize;
	    poDS->adfGeoTransform[2] = 0.0;
	    poDS->adfGeoTransform[3] = y_range[1];
	    poDS->adfGeoTransform[4] = 0.0;
	    poDS->adfGeoTransform[5] = 
		( y_range[0] - y_range[1] ) / poDS->nRasterYSize;
	}



/* -------------------------------------------------------------------- */
/*      Initialize any PAM information.                                 */
/* -------------------------------------------------------------------- */
    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->TryLoadXML( );

    return( poDS );
}
/************************************************************************/
/*                          GDALRegister_netCDF()                          */
/************************************************************************/

void GDALRegister_netCDF()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "netCDF" ) == NULL )
    {
        poDriver = new GDALDriver( );
        
        poDriver->SetDescription( "netCDF" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "network Common Data Format" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#netCDF" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "nc" );

        poDriver->pfnOpen = netCDFDataset::Open;

        GetGDALDriverManager( )->RegisterDriver( poDriver );
    }
}



/* -------------------------------------------------------------------- */
/*      Set Lambert Conformal Conic Projection                          */
/* -------------------------------------------------------------------- */

//    char **papszVarRefNames;
//    const char *pszValue;
//    int nVarProjectionID;
//    int nAttrType,nAttrLen;
//    char szAttrName[MAX_NC_NAME];
//    char szProjectionName[MAX_NC_NAME];
//    char szVarName[MAX_NC_NAME];
//    char szTemp[MAX_NC_NAME];
//    char szGridMappingName[MAX_NC_NAME]="";
//    char szGridMappingValue[MAX_NC_NAME];
//    char szCoordinatesVarName[MAX_NC_NAME]="";
//    char szCoordinatesValue[MAX_NC_NAME];
//    double dfStdP1, dfStdP2;
//    double dfCenterLat, dfCenterLong;
//    for ( int var = 0; var < var_count; var++ ) {
//	nc_inq_varname(  cdfid, var, szVarName );
//	strcpy(szTemp,szVarName);
//	strcat(szTemp,":coordinates");
//	pszValue = CSLFetchNameValue(poDS->papszMetadata, szTemp);
//	if( pszValue ) {
//	    printf("coordinates = %s\n",pszValue);
//	    strcpy(szCoordinatesVarName,szTemp);
//	    strcpy(szCoordinatesValue,pszValue);
//	    papszVarRefNames = CSLTokenizeString2( pszValue,
//						   " ", CSLT_HONOURSTRINGS );
//
//	    break;
//	}
//    }
//
//
//    for ( int var = 0; var < var_count; var++ ) {
//	nc_inq_varname(  cdfid, var, szVarName );
//	strcpy(szTemp,szVarName);
//	strcat(szTemp,":grid_mapping");
//	pszValue = CSLFetchNameValue(poDS->papszMetadata, szTemp);
//	if( pszValue ) {
//	    printf("grid_mapping = %s\n",pszValue);
//	    strcpy(szGridMappingName,szTemp);
//	    strcpy(szGridMappingValue,pszValue);
//	    break;
//	}
//    }
//
//#define GRD_MAPPING_NAME                ":grid_mapping_name"
//#define L_C_CONIC                       "lambert_conformal_conic"
//#define L_C_CONIC_STD_PARALLEL          ":standard_parallel"
//#define L_C_CONIC_LONG_CENTRAL_MERIDIAN ":longitude_of_central_meridian"
//#define L_C_CONIC_LAT_PROJ_ORIGIN ":latitude_of_projection_origin"
//#define L_C_CONIC_EARTH_SHAPE           ":GRIB_earth_shape"
//#define L_C_CONIC_EARTH_SHAPE_CODE      ":GRIB_earth_shape_code"
//
//
//    if( !( EQUAL(szGridMappingName,"" ) ) ) {
//	nc_inq_varid( cdfid, szGridMappingValue, &nVarProjectionID );
//	poDS->ReadAttributes( cdfid, nVarProjectionID, 9999 );
//    
//	strcpy(szTemp,szGridMappingValue);
//	strcat(szTemp,GRD_MAPPING_NAME);
//	pszValue = CSLFetchNameValue(poDS->papszMetadata, szTemp);
///* -------------------------------------------------------------------- */
///*      Lambert conformal conic                                         */
///* -------------------------------------------------------------------- */
//	if( EQUAL( pszValue, L_C_CONIC ) ) {
//	    strcpy(szTemp,szGridMappingValue);
//	    strcat( szTemp, L_C_CONIC_STD_PARALLEL );
//	    pszValue = CSLFetchNameValue(poDS->papszMetadata, szTemp);
//	    if( pszValue )
//		dfStdP1 = atof(pszValue);
//
//	    strcpy(szTemp,szGridMappingValue);
//	    strcat( szTemp, L_C_CONIC_LONG_CENTRAL_MERIDIAN );
//	    pszValue = CSLFetchNameValue(poDS->papszMetadata, szTemp);
//	    if( pszValue )
//		dfCenterLat = atof(pszValue);
//
//	    strcpy(szTemp,szGridMappingValue);
//	    strcat( szTemp, L_C_CONIC_LAT_PROJ_ORIGIN );
//	    pszValue = CSLFetchNameValue(poDS->papszMetadata, szTemp);
//	    if( pszValue )
//		dfCenterLong = atof(pszValue);
//
//	    strcpy(szTemp,szGridMappingValue);
//	    strcat( szTemp, L_C_CONIC_EARTH_SHAPE );
//	    pszValue = CSLFetchNameValue(poDS->papszMetadata, szTemp);
//	    if( pszValue )
//		printf("%s = %s\n",szTemp, pszValue);
//
//	    strcpy(szTemp,szGridMappingValue);
//	    strcat( szTemp, L_C_CONIC_EARTH_SHAPE_CODE );
//	    pszValue = CSLFetchNameValue(poDS->papszMetadata, szTemp);
//	    if( pszValue )
//		printf("%s = %s\n",szTemp, pszValue);
//	}
//    }

    
