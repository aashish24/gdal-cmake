/******************************************************************************
 * $Id$
 *
 * Project:  Multi-resolution Seamless Image Database (MrSID)
 * Purpose:  Read LizardTech's MrSID file format
 * Author:   Andrey Kiselev, dron@remotesensing.org
 *
 ******************************************************************************
 * Copyright (c) 2003, Andrey Kiselev <dron@remotesensing.org>
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
 * Revision 1.8  2004/03/19 12:19:48  dron
 * Few improvements; several memory leaks fixed.
 *
 * Revision 1.7  2004/02/08 14:29:43  dron
 * Support for DSDK v.3.2.x.
 *
 * Revision 1.6  2004/02/03 20:56:45  dron
 * Fixes in coordinate handling.
 *
 * Revision 1.5  2003/07/24 09:33:07  dron
 * Added MrSIDRasterBand::IRasterIO().
 *
 * Revision 1.4  2003/05/27 21:08:38  warmerda
 * avoid int/short casting warnings
 *
 * Revision 1.3  2003/05/25 15:24:49  dron
 * Added georeferencing.
 *
 * Revision 1.2  2003/05/07 10:26:02  dron
 * Added multidimensional metadata handling.
 *
 * Revision 1.1  2003/04/23 12:21:56  dron
 * New.
 *
 */

#include "gdal_priv.h"
#include "cpl_string.h"

#include "lt_fileUtils.h"
#include "lt_xTrans.h"
#include "lt_imageBufferInfo.h"
# include "lt_imageBuffer.h"
#include "lt_pixel.h"

#ifdef MRSID_DSDK_VERSION_31
# include "lt_colorSpace.h"
#endif

#include "MrSIDImageFile.h"
#include "MrSIDNavigator.h"
#include "MetadataReader.h"
#include "MetadataElement.h"

#include <geo_normalize.h>
#include <geovalues.h>

LT_USE_NAMESPACE(LizardTech)

CPL_CVSID("$Id$");

CPL_C_START
void    GDALRegister_MrSID(void);
char *  GTIFGetOGISDefn( GTIFDefn * );
double GTIFAngleToDD( double dfAngle, int nUOMAngle );
CPL_C_END

/************************************************************************/
/* ==================================================================== */
/*                              MrSIDDataset                            */
/* ==================================================================== */
/************************************************************************/

class MrSIDDataset : public GDALDataset
{
    friend class MrSIDRasterBand;

    MrSIDImageFile      *poMrSidFile;
    MrSIDNavigator      *poMrSidNav;
    Pixel               *poDefaultPixel;
    MetadataReader      *poMrSidMetadata;

    ImageBufferInfo::SampleType eSampleType;
    GDALDataType        eDataType;
#ifdef MRSID_DSDK_VERSION_31
    ColorSpace          *poColorSpace;
#else
    MrSIDColorSpace     *poColorSpace;
#endif

    int                 nCurrentZoomLevel;

    int                 bHasGeoTransfom;
    double              adfGeoTransform[6];
    char                *pszProjection;
    GTIFDefn            *psDefn;

    int                 bIsOverview;
    int		        nOverviewCount;
    MrSIDDataset        **papoOverviewDS;

    CPLErr              OpenZoomLevel( int iZoom );
    char                *SerializeMetadataElement( const MetadataElement *poElement );
    int                 GetMetadataElement( const char *pszKey, void *pValue );
    void                FetchProjParms();
    void                GetGTIFDefn();

  public:
                MrSIDDataset();
                ~MrSIDDataset();

    static GDALDataset  *Open( GDALOpenInfo * );

    virtual CPLErr      GetGeoTransform( double * padfTransform );
    const char          *GetProjectionRef();
};

/************************************************************************/
/* ==================================================================== */
/*                           MrSIDRasterBand                            */
/* ==================================================================== */
/************************************************************************/

class MrSIDRasterBand : public GDALRasterBand
{
    friend class MrSIDDataset;

    ImageBufferInfo *poImageBufInfo;
    ImageBuffer     *poImageBuf;

    int             nBlockSize;

    virtual CPLErr IRasterIO( GDALRWFlag, int, int, int, int,
                              void *, int, int, GDALDataType,
                              int, int );

  public:

                MrSIDRasterBand( MrSIDDataset *, int );
                ~MrSIDRasterBand();

    virtual CPLErr          IReadBlock( int, int, void * );
    virtual GDALColorInterp GetColorInterpretation();
    virtual int             GetOverviewCount();
    virtual GDALRasterBand  *GetOverview( int );
};

/************************************************************************/
/*                           MrSIDRasterBand()                          */
/************************************************************************/

MrSIDRasterBand::MrSIDRasterBand( MrSIDDataset *poDS, int nBand )
{
    this->poDS = poDS;
    this->nBand = nBand;
    this->eDataType = poDS->eDataType;

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = ( nBlockXSize * poDS->GetRasterYSize() < 1000000 )?
        poDS->GetRasterYSize():1000000/nBlockXSize + 1;
    nBlockSize = nBlockXSize * nBlockYSize;
    poDS->poMrSidNav->zoomTo( poDS->nCurrentZoomLevel ); 
    poDS->poMrSidNav->resize( nBlockXSize, nBlockYSize, IntRect::TOP_LEFT );

    poImageBufInfo = new ImageBufferInfo( ImageBufferInfo::BIP,
                                          *poDS->poColorSpace,
                                          poDS->eSampleType );
}

/************************************************************************/
/*                            ~MrSIDRasterBand()                        */
/************************************************************************/

MrSIDRasterBand::~MrSIDRasterBand()
{
    if ( poImageBufInfo )
        delete poImageBufInfo;
}

/************************************************************************/
/*                             IRasterIO()                              */
/************************************************************************/

CPLErr MrSIDRasterBand::IRasterIO( GDALRWFlag eRWFlag,
                                   int nXOff, int nYOff, int nXSize, int nYSize,
                                   void * pData, int nBufXSize, int nBufYSize,
                                   GDALDataType eBufType,
                                   int nPixelSpace, int nLineSpace )
    
{
    MrSIDDataset *poGDS = (MrSIDDataset *) poDS;

/* -------------------------------------------------------------------- */
/*      Fallback to default implementation if the whole scanline        */
/*      without subsampling requested.                                  */
/* -------------------------------------------------------------------- */
    if ( nXSize == poGDS->GetRasterXSize()
         && nXSize == nBufXSize
         && nYSize == nBufYSize )
    {
        return GDALRasterBand::IRasterIO( eRWFlag, nXOff, nYOff,
                                          nXSize, nYSize, pData,
                                          nBufXSize, nBufYSize, eBufType,
                                          nPixelSpace, nLineSpace );
    }

/* -------------------------------------------------------------------- */
/*      Use MrSID zooming/panning abilities otherwise.                  */
/* -------------------------------------------------------------------- */
    int         nBufDataSize = GDALGetDataTypeSize( eBufType ) / 8;
    int         iLine;
    int         nNewXSize, nNewYSize, iSrcOffset;

    ImgRect     imageSupport( nXOff, nYOff, nXOff + nXSize, nYOff + nYSize );
    IntDimension targetDims( nBufXSize, nBufYSize );

    /* Again, fallback to default if we can't zoom/pan */
    if ( !poGDS->poMrSidNav->fitWithin( imageSupport, targetDims ) )
    {
        return GDALRasterBand::IRasterIO( eRWFlag, nXOff, nYOff,
                                          nXSize, nYSize, pData,
                                          nBufXSize, nBufYSize, eBufType,
                                          nPixelSpace, nLineSpace );
    }

    poImageBuf = new ImageBuffer( *poImageBufInfo );
    try
    {
        poGDS->poMrSidNav->loadImage( *poImageBuf );
    }
    catch ( ... )
    {
        delete poImageBuf;
        CPLError( CE_Failure, CPLE_AppDefined,
                  "MrSIDRasterBand::IRasterIO(): Failed to load image." );
        return CE_Failure;
    }

    nNewXSize = poImageBuf->getBounds().width();
    nNewYSize = poImageBuf->getBounds().height();
    iSrcOffset = (nBand - 1) * poImageBufInfo->bytesPerSample();

    for ( iLine = 0; iLine < nBufYSize; iLine++ )
    {
        int     iDstLineOff = iLine * nLineSpace;

        if ( nNewXSize == nBufXSize && nNewYSize == nBufYSize )
        {
            GDALCopyWords( (GByte *)poImageBuf->getData()
                           + iSrcOffset + iLine * poImageBuf->getRowBytes(),
                           eDataType, poImageBufInfo->pixelIncrement(),
                           (GByte *)pData + iDstLineOff, eBufType, nPixelSpace,
                           nBufXSize );
        }
        else
        {
            double  dfSrcXInc = (double)nNewXSize / nBufXSize;
            double  dfSrcYInc = (double)nNewYSize / nBufYSize;

            int     iSrcLineOff = iSrcOffset
                + (int)(iLine * dfSrcYInc) * poImageBuf->getRowBytes();
            int     iPixel;

            for ( iPixel = 0; iPixel < nBufXSize; iPixel++ )
            {
                GDALCopyWords( (GByte *)poImageBuf->getData() + iSrcLineOff
                               + (int)(iPixel * dfSrcXInc)
                                      * poImageBufInfo->pixelIncrement(),
                               eDataType, poImageBufInfo->pixelIncrement(),
                               (GByte *)pData + iDstLineOff +
                               iPixel * nBufDataSize,
                               eBufType, nPixelSpace, 1 );
            }
        }
    }

    delete poImageBuf;

    return CE_None;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr MrSIDRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                    void * pImage )
{
    MrSIDDataset    *poGDS = (MrSIDDataset *)poDS;
    int             i, j;

    poImageBuf = new ImageBuffer( *poImageBufInfo );
    poGDS->poMrSidNav->panTo( nBlockXOff * nBlockXSize,
                              nBlockYOff * nBlockYSize,
                              IntRect::TOP_LEFT );

    try
    {
        poGDS->poMrSidNav->loadImage( *poImageBuf );
    }
    catch ( ... )
    {
        delete poImageBuf;
        CPLError( CE_Failure, CPLE_AppDefined,
                  "MrSIDRasterBand::IReadBlock(): Failed to load image." );
        return CE_Failure;
    }

    for ( i = 0, j = nBand - 1; i < nBlockSize; i++, j+=poGDS->nBands )
    {
        switch( eDataType )
        {
            case GDT_UInt16:
            {
                ((GUInt16*)pImage)[i] = ((GUInt16*)poImageBuf->getData())[j];
            }
            break;
            case GDT_Float32:
            {
                ((float*)pImage)[i] = ((float*)poImageBuf->getData())[j];
            }
            break;
            case GDT_Byte:
            default:
            {
                ((GByte*)pImage)[i] = ((GByte*)poImageBuf->getData())[j];
            }
            break;
            }
    }
    delete poImageBuf;

    return CE_None;
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

GDALColorInterp MrSIDRasterBand::GetColorInterpretation()
{
    MrSIDDataset      *poGDS = (MrSIDDataset *) poDS;

#ifdef MRSID_DSDK_VERSION_31

# define GDAL_MRSID_COLOR_SCHEME poGDS->poColorSpace->scheme()
# define GDAL_MRSID_COLORSPACE_RGB ColorSpace::RGB
# define GDAL_MRSID_COLORSPACE_CMYK ColorSpace::CMYK
# define GDAL_MRSID_COLORSPACE_GRAYSCALE ColorSpace::GRAYSCALE
# define GDAL_MRSID_COLORSPACE_RGBK ColorSpace::RGBK

#else

# define GDAL_MRSID_COLOR_SCHEME (int)poGDS->poColorSpace
# define GDAL_MRSID_COLORSPACE_RGB MRSID_COLORSPACE_RGB
# define GDAL_MRSID_COLORSPACE_CMYK MRSID_COLORSPACE_CMYK
# define GDAL_MRSID_COLORSPACE_GRAYSCALE MRSID_COLORSPACE_GRAYSCALE
# define GDAL_MRSID_COLORSPACE_RGBK MRSID_COLORSPACE_RGBK

#endif

    switch( GDAL_MRSID_COLOR_SCHEME )
    {
        case GDAL_MRSID_COLORSPACE_RGB:
            if( nBand == 1 )
                return GCI_RedBand;
            else if( nBand == 2 )
                return GCI_GreenBand;
            else if( nBand == 3 )
                return GCI_BlueBand;
            else
                return GCI_Undefined;
        case GDAL_MRSID_COLORSPACE_CMYK:
            if( nBand == 1 )
                return GCI_CyanBand;
            else if( nBand == 2 )
                return GCI_MagentaBand;
            else if( nBand == 3 )
                return GCI_YellowBand;
            else if( nBand == 4 )
                return GCI_BlackBand;
            else
                return GCI_Undefined;
        case GDAL_MRSID_COLORSPACE_GRAYSCALE:
            return GCI_GrayIndex;
        case GDAL_MRSID_COLORSPACE_RGBK:
            if( nBand == 1 )
                return GCI_RedBand;
            else if( nBand == 2 )
                return GCI_GreenBand;
            else if( nBand == 3 )
                return GCI_BlueBand;
            else if( nBand == 4 )
                return GCI_AlphaBand;
            else
                return GCI_Undefined;
        default:
            return GCI_Undefined;
    }

#undef GDAL_MRSID_COLORSPACE_RGB
#undef GDAL_MRSID_COLORSPACE_CMYK
#undef GDAL_MRSID_COLORSPACE_GRAYSCALE
#undef GDAL_MRSID_COLORSPACE_RGBK

}

/************************************************************************/
/*                          GetOverviewCount()                          */
/************************************************************************/

int MrSIDRasterBand::GetOverviewCount()

{
    MrSIDDataset        *poGDS = (MrSIDDataset *) poDS;

    return poGDS->nOverviewCount;
}

/************************************************************************/
/*                            GetOverview()                             */
/************************************************************************/

GDALRasterBand *MrSIDRasterBand::GetOverview( int i )

{
    MrSIDDataset        *poGDS = (MrSIDDataset *) poDS;

    if( i < 0 || i >= poGDS->nOverviewCount )
        return NULL;
    else
        return poGDS->papoOverviewDS[i]->GetRasterBand( nBand );
}

/************************************************************************/
/*                           MrSIDDataset()                             */
/************************************************************************/

MrSIDDataset::MrSIDDataset()
{
    poMrSidFile = NULL;
    poMrSidNav = NULL;
    poDefaultPixel = NULL;
    poMrSidMetadata = NULL;
    poColorSpace = NULL;
    nBands = 0;
    eSampleType = ImageBufferInfo::UINT8;
    eDataType = GDT_Byte;
    pszProjection = CPLStrdup( "" );
    bHasGeoTransfom = FALSE;
    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;
    psDefn = NULL;
    nCurrentZoomLevel = 0;
    bIsOverview = FALSE;
    nOverviewCount = 0;
    papoOverviewDS = NULL;

}

/************************************************************************/
/*                            ~MrSIDDataset()                           */
/************************************************************************/

MrSIDDataset::~MrSIDDataset()
{
    // Delete MrSID file object only in base dataset object and don't delete
    // it in overviews.
    if ( poMrSidFile && !bIsOverview )
        delete poMrSidFile;
    if ( poMrSidNav )
        delete poMrSidNav;
    if ( poDefaultPixel )
        delete poDefaultPixel;
    if ( poMrSidMetadata )
        delete poMrSidMetadata;
    if ( poColorSpace )
        delete poColorSpace;
    if ( pszProjection )
        CPLFree( pszProjection );
    if ( psDefn )
        delete psDefn;
    if ( papoOverviewDS )
    {
        for( int i = 0; i < nOverviewCount; i++ )
            delete papoOverviewDS[i];
        CPLFree( papoOverviewDS );
    }
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr MrSIDDataset::GetGeoTransform( double * padfTransform )
{
    memcpy( padfTransform, adfGeoTransform, sizeof(adfGeoTransform[0]) * 6 );

    if ( !bHasGeoTransfom )
        return CE_Failure;

    return CE_None;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *MrSIDDataset::GetProjectionRef()
{
    if( pszProjection )
        return pszProjection;
    else
        return "";
}

/************************************************************************/
/*                      SerializeMetadataElement()                      */
/************************************************************************/

char *MrSIDDataset::SerializeMetadataElement( const MetadataElement *poElement )
{
#ifdef MRSID_DSDK_VERSION_31
    IntDimension oDim = poElement->getDimensions();
#else
    IntDimension oDim( poElement->getDimensionWidth(),
                       poElement->getDimensionHeight() );
#endif
    int         i, j, iLength;
    const char  *pszTemp = NULL;
    char        *pszMetadata = CPLStrdup( "" );

    for ( i = 0; i < oDim.height; i++ )
        for ( j = 0; j < oDim.width; j++ )
        {
            switch( poElement->type() )
            {
                case MetadataValue::BYTE:
                case MetadataValue::SHORT:
                case MetadataValue::LONG:
                    pszTemp = CPLSPrintf( "%lu", (unsigned long)(*poElement)[i][j] );
                break;
                case MetadataValue::SBYTE:
                case MetadataValue::SSHORT:
                case MetadataValue::SLONG:
                    pszTemp = CPLSPrintf( "%ld", (long)(*poElement)[i][j] );
                break;
                case MetadataValue::FLOAT:
                    pszTemp = CPLSPrintf( "%f", (float)(*poElement)[i][j] );
                break;
                case MetadataValue::DOUBLE:
                    pszTemp = CPLSPrintf( "%lf", (double)(*poElement)[i][j] );
                break;
                case MetadataValue::ASCII:
                    pszTemp = (const char*)poElement->getMetadataValue();
                break;
                default:
                    pszTemp = "";
                break;
            }

            iLength = strlen(pszMetadata) + strlen(pszTemp) + 2;

            pszMetadata = (char *)CPLRealloc( pszMetadata, iLength );
            if ( !EQUAL( pszMetadata, "" ) )
                strncat( pszMetadata, ",", 1 );
            strncat( pszMetadata, pszTemp, iLength );
        }

    return pszMetadata;
}

/************************************************************************/
/*                          GetMetadataElement()                        */
/************************************************************************/

int MrSIDDataset::GetMetadataElement( const char *pszKey, void *pValue )
{
    if ( !poMrSidMetadata->keyExists( pszKey ) )
        return FALSE;

    MetadataElement oElement( poMrSidMetadata->getValue( pszKey ) );

    /* XXX: return FALSE if we have more than one element in metadata record */
    if ( oElement.getDimensionality() != MetadataElement::SINGLE_VALUE )
        return FALSE;

    switch( oElement.type() )
    {
        case MetadataValue::BYTE:
        {
            unsigned char iValue = oElement[0][0];
            memcpy( pValue, &iValue, sizeof( iValue ) );
        }
        break;
        case MetadataValue::SHORT:
        {
            unsigned short iValue = oElement[0][0];
            memcpy( pValue, &iValue, sizeof( iValue ) );
        }
        break;
        case MetadataValue::LONG:
        {
            unsigned long iValue = oElement[0][0];
            memcpy( pValue, &iValue, sizeof( iValue ) );
        }
        break;
        case MetadataValue::SBYTE:
        {
            char iValue = oElement[0][0];
            memcpy( pValue, &iValue, sizeof( iValue ) );
        }
        break;
        case MetadataValue::SSHORT:
        {
            signed short iValue = oElement[0][0];
            memcpy( pValue, &iValue, sizeof( iValue ) );
        }
        break;
        case MetadataValue::SLONG:
        {
            signed long iValue = oElement[0][0];
            memcpy( pValue, &iValue, sizeof( iValue ) );
        }
        break;
        case MetadataValue::FLOAT:
        {
            float fValue = oElement[0][0];
            memcpy( pValue, &fValue, sizeof( fValue ) );
        }
        break;
        case MetadataValue::DOUBLE:
        {
            double dfValue = oElement[0][0];
            memcpy( pValue, &dfValue, sizeof( dfValue ) );
        }
        break;
        default:
        break;
    }

    return TRUE;
}

/************************************************************************/
/*                    EPSGProjMethodToCTProjMethod()                    */
/*                                                                      */
/*      Convert between the EPSG enumeration for projection methods,    */
/*      and the GeoTIFF CT codes.                                       */
/*      Explicitly copied from geo_normalize.c of the GeoTIFF package   */
/************************************************************************/

static int EPSGProjMethodToCTProjMethod( int nEPSG )

{
    /* see trf_method.csv for list of EPSG codes */
    
    switch( nEPSG )
    {
      case 9801:
        return( CT_LambertConfConic_1SP );

      case 9802:
        return( CT_LambertConfConic_2SP );

      case 9803:
        return( CT_LambertConfConic_2SP ); /* Belgian variant not supported */

      case 9804:
        return( CT_Mercator );  /* 1SP and 2SP not differentiated */

      case 9805:
        return( CT_Mercator );  /* 1SP and 2SP not differentiated */

      case 9806:
        return( CT_CassiniSoldner );

      case 9807:
        return( CT_TransverseMercator );

      case 9808:
        return( CT_TransvMercator_SouthOriented );

      case 9809:
        return( CT_ObliqueStereographic );

      case 9810:
        return( CT_PolarStereographic );

      case 9811:
        return( CT_NewZealandMapGrid );

      case 9812:
        return( CT_ObliqueMercator ); /* is hotine actually different? */

      case 9813:
        return( CT_ObliqueMercator_Laborde );

      case 9814:
        return( CT_ObliqueMercator_Rosenmund ); /* swiss  */

      case 9815:
        return( CT_ObliqueMercator );

      case 9816: /* tunesia mining grid has no counterpart */
        return( KvUserDefined );
    }

    return( KvUserDefined );
}

/* EPSG Codes for projection parameters.  Unfortunately, these bear no
   relationship to the GeoTIFF codes even though the names are so similar. */

#define EPSGNatOriginLat         8801
#define EPSGNatOriginLong        8802
#define EPSGNatOriginScaleFactor 8805
#define EPSGFalseEasting         8806
#define EPSGFalseNorthing        8807
#define EPSGProjCenterLat        8811
#define EPSGProjCenterLong       8812
#define EPSGAzimuth              8813
#define EPSGAngleRectifiedToSkewedGrid 8814
#define EPSGInitialLineScaleFactor 8815
#define EPSGProjCenterEasting    8816
#define EPSGProjCenterNorthing   8817
#define EPSGPseudoStdParallelLat 8818
#define EPSGPseudoStdParallelScaleFactor 8819
#define EPSGFalseOriginLat       8821
#define EPSGFalseOriginLong      8822
#define EPSGStdParallel1Lat      8823
#define EPSGStdParallel2Lat      8824
#define EPSGFalseOriginEasting   8826
#define EPSGFalseOriginNorthing  8827
#define EPSGSphericalOriginLat   8828
#define EPSGSphericalOriginLong  8829
#define EPSGInitialLongitude     8830
#define EPSGZoneWidth            8831

/************************************************************************/
/*                            SetGTParmIds()                            */
/*                                                                      */
/*      This is hardcoded logic to set the GeoTIFF parameter            */
/*      identifiers for all the EPSG supported projections.  As the     */
/*      trf_method.csv table grows with new projections, this code      */
/*      will need to be updated.                                        */
/*      Explicitly copied from geo_normalize.c of the GeoTIFF package.  */
/************************************************************************/

static int SetGTParmIds( int nCTProjection, 
                         int *panProjParmId, 
                         int *panEPSGCodes )

{
    int anWorkingDummy[7];

    if( panEPSGCodes == NULL )
        panEPSGCodes = anWorkingDummy;
    if( panProjParmId == NULL )
        panProjParmId = anWorkingDummy;

    memset( panEPSGCodes, 0, sizeof(int) * 7 );

    /* psDefn->nParms = 7; */
    
    switch( nCTProjection )
    {
      case CT_CassiniSoldner:
      case CT_NewZealandMapGrid:
        panProjParmId[0] = ProjNatOriginLatGeoKey;
        panProjParmId[1] = ProjNatOriginLongGeoKey;
        panProjParmId[5] = ProjFalseEastingGeoKey;
        panProjParmId[6] = ProjFalseNorthingGeoKey;

        panEPSGCodes[0] = EPSGNatOriginLat;
        panEPSGCodes[1] = EPSGNatOriginLong;
        panEPSGCodes[5] = EPSGFalseEasting;
        panEPSGCodes[6] = EPSGFalseNorthing;
        return TRUE;

      case CT_ObliqueMercator:
        panProjParmId[0] = ProjCenterLatGeoKey;
        panProjParmId[1] = ProjCenterLongGeoKey;
        panProjParmId[2] = ProjAzimuthAngleGeoKey;
        panProjParmId[3] = ProjRectifiedGridAngleGeoKey;
        panProjParmId[4] = ProjScaleAtCenterGeoKey;
        panProjParmId[5] = ProjFalseEastingGeoKey;
        panProjParmId[6] = ProjFalseNorthingGeoKey;

        panEPSGCodes[0] = EPSGProjCenterLat;
        panEPSGCodes[1] = EPSGProjCenterLong;
        panEPSGCodes[2] = EPSGAzimuth;
        panEPSGCodes[3] = EPSGAngleRectifiedToSkewedGrid;
        panEPSGCodes[4] = EPSGInitialLineScaleFactor;
        panEPSGCodes[5] = EPSGProjCenterEasting;
        panEPSGCodes[6] = EPSGProjCenterNorthing;
        return TRUE;

      case CT_ObliqueMercator_Laborde:
        panProjParmId[0] = ProjCenterLatGeoKey;
        panProjParmId[1] = ProjCenterLongGeoKey;
        panProjParmId[2] = ProjAzimuthAngleGeoKey;
        panProjParmId[4] = ProjScaleAtCenterGeoKey;
        panProjParmId[5] = ProjFalseEastingGeoKey;
        panProjParmId[6] = ProjFalseNorthingGeoKey;

        panEPSGCodes[0] = EPSGProjCenterLat;
        panEPSGCodes[1] = EPSGProjCenterLong;
        panEPSGCodes[2] = EPSGAzimuth;
        panEPSGCodes[4] = EPSGInitialLineScaleFactor;
        panEPSGCodes[5] = EPSGProjCenterEasting;
        panEPSGCodes[6] = EPSGProjCenterNorthing;
        return TRUE;
        
      case CT_LambertConfConic_1SP:
      case CT_Mercator:
      case CT_ObliqueStereographic:
      case CT_PolarStereographic:
      case CT_TransverseMercator:
      case CT_TransvMercator_SouthOriented:
        panProjParmId[0] = ProjNatOriginLatGeoKey;
        panProjParmId[1] = ProjNatOriginLongGeoKey;
        panProjParmId[4] = ProjScaleAtNatOriginGeoKey;
        panProjParmId[5] = ProjFalseEastingGeoKey;
        panProjParmId[6] = ProjFalseNorthingGeoKey;

        panEPSGCodes[0] = EPSGNatOriginLat;
        panEPSGCodes[1] = EPSGNatOriginLong;
        panEPSGCodes[4] = EPSGNatOriginScaleFactor;
        panEPSGCodes[5] = EPSGFalseEasting;
        panEPSGCodes[6] = EPSGFalseNorthing;
        return TRUE;

      case CT_LambertConfConic_2SP:
        panProjParmId[0] = ProjFalseOriginLatGeoKey;
        panProjParmId[1] = ProjFalseOriginLongGeoKey;
        panProjParmId[2] = ProjStdParallel1GeoKey;
        panProjParmId[3] = ProjStdParallel2GeoKey;
        panProjParmId[5] = ProjFalseEastingGeoKey;
        panProjParmId[6] = ProjFalseNorthingGeoKey;

        panEPSGCodes[0] = EPSGFalseOriginLat;
        panEPSGCodes[1] = EPSGFalseOriginLong;
        panEPSGCodes[2] = EPSGStdParallel1Lat;
        panEPSGCodes[3] = EPSGStdParallel2Lat;
        panEPSGCodes[5] = EPSGFalseOriginEasting;
        panEPSGCodes[6] = EPSGFalseOriginNorthing;
        return TRUE;

      case CT_SwissObliqueCylindrical:
        panProjParmId[0] = ProjCenterLatGeoKey;
        panProjParmId[1] = ProjCenterLongGeoKey;
        panProjParmId[5] = ProjFalseEastingGeoKey;
        panProjParmId[6] = ProjFalseNorthingGeoKey;

        /* EPSG codes? */
        return TRUE;

      default:
        return( FALSE );
    }
}

/************************************************************************/
/*                           FetchProjParms()                           */
/*                                                                      */
/*      Fetch the projection parameters for a particular projection     */
/*      from MrSID metadata, and fill the GTIFDefn structure out        */
/*      with them.                                                      */
/*      Explicitly copied from geo_normalize.c of the GeoTIFF package.  */
/************************************************************************/

void MrSIDDataset::FetchProjParms()
{
    double dfNatOriginLong = 0.0, dfNatOriginLat = 0.0, dfRectGridAngle = 0.0;
    double dfFalseEasting = 0.0, dfFalseNorthing = 0.0, dfNatOriginScale = 1.0;
    double dfStdParallel1 = 0.0, dfStdParallel2 = 0.0, dfAzimuth = 0.0;

/* -------------------------------------------------------------------- */
/*      Get the false easting, and northing if available.               */
/* -------------------------------------------------------------------- */
    if( !GetMetadataElement( "GEOTIFF_NUM::3082::ProjFalseEastingGeoKey",
                             &dfFalseEasting )
        && !GetMetadataElement( "GEOTIFF_NUM::3090:ProjCenterEastingGeoKey",
                                &dfFalseEasting ) )
        dfFalseEasting = 0.0;
        
    if( !GetMetadataElement( "GEOTIFF_NUM::3083::ProjFalseNorthingGeoKey",
                             &dfFalseNorthing )
        && !GetMetadataElement( "GEOTIFF_NUM::3091::ProjCenterNorthingGeoKey",
                                &dfFalseNorthing ) )
        dfFalseNorthing = 0.0;

    switch( psDefn->CTProjection )
    {
/* -------------------------------------------------------------------- */
      case CT_Stereographic:
/* -------------------------------------------------------------------- */
        if( GetMetadataElement( "GEOTIFF_NUM::3080::ProjNatOriginLongGeoKey",
                                &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3084::ProjFalseOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3088::ProjCenterLongGeoKey",
                                   &dfNatOriginLong ) == 0 )
            dfNatOriginLong = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3081::ProjNatOriginLatGeoKey",
                                &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3085::ProjFalseOriginLatGeoKey",
                                   &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3089::ProjCenterLatGeoKey",
                                   &dfNatOriginLat ) == 0 )
            dfNatOriginLat = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3092::ProjScaleAtNatOriginGeoKey",
                                &dfNatOriginScale ) == 0 )
            dfNatOriginScale = 1.0;
            
        /* notdef: should transform to decimal degrees at this point */

        psDefn->ProjParm[0] = dfNatOriginLat;
        psDefn->ProjParmId[0] = ProjCenterLatGeoKey;
        psDefn->ProjParm[1] = dfNatOriginLong;
        psDefn->ProjParmId[1] = ProjCenterLongGeoKey;
        psDefn->ProjParm[4] = dfNatOriginScale;
        psDefn->ProjParmId[4] = ProjScaleAtNatOriginGeoKey;
        psDefn->ProjParm[5] = dfFalseEasting;
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[6] = dfFalseNorthing;
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        psDefn->nParms = 7;
        break;

/* -------------------------------------------------------------------- */
      case CT_LambertConfConic_1SP:
      case CT_Mercator:
      case CT_ObliqueStereographic:
      case CT_TransverseMercator:
      case CT_TransvMercator_SouthOriented:
/* -------------------------------------------------------------------- */
        if( GetMetadataElement( "GEOTIFF_NUM::3080::ProjNatOriginLongGeoKey",
                                &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3084::ProjFalseOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3088::ProjCenterLongGeoKey", 
                                   &dfNatOriginLong ) == 0 )
            dfNatOriginLong = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3081::ProjNatOriginLatGeoKey",
                                &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3085::ProjFalseOriginLatGeoKey",
                                   &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3089::ProjCenterLatGeoKey",
                                   &dfNatOriginLat ) == 0 )
            dfNatOriginLat = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3092::ProjScaleAtNatOriginGeoKey",
                                &dfNatOriginScale ) == 0 )
            dfNatOriginScale = 1.0;

        /* notdef: should transform to decimal degrees at this point */

        psDefn->ProjParm[0] = dfNatOriginLat;
        psDefn->ProjParmId[0] = ProjNatOriginLatGeoKey;
        psDefn->ProjParm[1] = dfNatOriginLong;
        psDefn->ProjParmId[1] = ProjNatOriginLongGeoKey;
        psDefn->ProjParm[4] = dfNatOriginScale;
        psDefn->ProjParmId[4] = ProjScaleAtNatOriginGeoKey;
        psDefn->ProjParm[5] = dfFalseEasting;
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[6] = dfFalseNorthing;
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        psDefn->nParms = 7;
        break;

/* -------------------------------------------------------------------- */
      case CT_ObliqueMercator: /* hotine */
/* -------------------------------------------------------------------- */
        if( GetMetadataElement( "GEOTIFF_NUM::3080::ProjNatOriginLongGeoKey",
                                &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3084::ProjFalseOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3088::ProjCenterLongGeoKey", 
                                   &dfNatOriginLong ) == 0 )
            dfNatOriginLong = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3081::ProjNatOriginLatGeoKey",
                                &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3085::ProjFalseOriginLatGeoKey",
                                   &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3089::ProjCenterLatGeoKey",
                                   &dfNatOriginLat ) == 0 )
            dfNatOriginLat = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3094::ProjAzimuthAngleGeoKey",
                                &dfAzimuth ) == 0 )
            dfAzimuth = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3096::ProjRectifiedGridAngleGeoKey",
                                &dfRectGridAngle ) == 0 )
            dfRectGridAngle = 90.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3092::ProjScaleAtNatOriginGeoKey",
                                &dfNatOriginScale ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3093::ProjScaleAtCenterGeoKey",
                                   &dfNatOriginScale ) == 0 )
            dfNatOriginScale = 1.0;
            
        /* notdef: should transform to decimal degrees at this point */

        psDefn->ProjParm[0] = dfNatOriginLat;
        psDefn->ProjParmId[0] = ProjCenterLatGeoKey;
        psDefn->ProjParm[1] = dfNatOriginLong;
        psDefn->ProjParmId[1] = ProjCenterLongGeoKey;
        psDefn->ProjParm[2] = dfAzimuth;
        psDefn->ProjParmId[2] = ProjAzimuthAngleGeoKey;
        psDefn->ProjParm[3] = dfRectGridAngle;
        psDefn->ProjParmId[3] = ProjRectifiedGridAngleGeoKey;
        psDefn->ProjParm[4] = dfNatOriginScale;
        psDefn->ProjParmId[4] = ProjScaleAtCenterGeoKey;
        psDefn->ProjParm[5] = dfFalseEasting;
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[6] = dfFalseNorthing;
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        psDefn->nParms = 7;
        break;

/* -------------------------------------------------------------------- */
      case CT_CassiniSoldner:
      case CT_Polyconic:
/* -------------------------------------------------------------------- */
        if( GetMetadataElement( "GEOTIFF_NUM::3080::ProjNatOriginLongGeoKey",
                                &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3084::ProjFalseOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3088::ProjCenterLongGeoKey", 
                                   &dfNatOriginLong ) == 0 )
            dfNatOriginLong = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3081::ProjNatOriginLatGeoKey",
                                &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3085::ProjFalseOriginLatGeoKey",
                                   &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3089::ProjCenterLatGeoKey",
                                   &dfNatOriginLat ) == 0 )
            dfNatOriginLat = 0.0;


        if( GetMetadataElement( "GEOTIFF_NUM::3092::ProjScaleAtNatOriginGeoKey",
                                &dfNatOriginScale ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3093::ProjScaleAtCenterGeoKey",
                                   &dfNatOriginScale ) == 0 )
            dfNatOriginScale = 1.0;
            
        /* notdef: should transform to decimal degrees at this point */

        psDefn->ProjParm[0] = dfNatOriginLat;
        psDefn->ProjParmId[0] = ProjNatOriginLatGeoKey;
        psDefn->ProjParm[1] = dfNatOriginLong;
        psDefn->ProjParmId[1] = ProjNatOriginLongGeoKey;
        psDefn->ProjParm[4] = dfNatOriginScale;
        psDefn->ProjParmId[4] = ProjScaleAtNatOriginGeoKey;
        psDefn->ProjParm[5] = dfFalseEasting;
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[6] = dfFalseNorthing;
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        psDefn->nParms = 7;
        break;

/* -------------------------------------------------------------------- */
      case CT_AzimuthalEquidistant:
      case CT_MillerCylindrical:
      case CT_Equirectangular:
      case CT_Gnomonic:
      case CT_LambertAzimEqualArea:
      case CT_Orthographic:
/* -------------------------------------------------------------------- */
        if( GetMetadataElement( "GEOTIFF_NUM::3080::ProjNatOriginLongGeoKey",
                                &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3084::ProjFalseOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3088::ProjCenterLongGeoKey", 
                                   &dfNatOriginLong ) == 0 )
            dfNatOriginLong = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3081::ProjNatOriginLatGeoKey",
                                &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3085::ProjFalseOriginLatGeoKey",
                                   &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3089::ProjCenterLatGeoKey",
                                   &dfNatOriginLat ) == 0 )
            dfNatOriginLat = 0.0;

        /* notdef: should transform to decimal degrees at this point */

        psDefn->ProjParm[0] = dfNatOriginLat;
        psDefn->ProjParmId[0] = ProjCenterLatGeoKey;
        psDefn->ProjParm[1] = dfNatOriginLong;
        psDefn->ProjParmId[1] = ProjCenterLongGeoKey;
        psDefn->ProjParm[5] = dfFalseEasting;
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[6] = dfFalseNorthing;
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        psDefn->nParms = 7;
        break;

/* -------------------------------------------------------------------- */
      case CT_Robinson:
      case CT_Sinusoidal:
      case CT_VanDerGrinten:
/* -------------------------------------------------------------------- */
        if( GetMetadataElement( "GEOTIFF_NUM::3080::ProjNatOriginLongGeoKey",
                                &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3084::ProjFalseOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3088::ProjCenterLongGeoKey", 
                                   &dfNatOriginLong ) == 0 )
            dfNatOriginLong = 0.0;

        /* notdef: should transform to decimal degrees at this point */

        psDefn->ProjParm[1] = dfNatOriginLong;
        psDefn->ProjParmId[1] = ProjCenterLongGeoKey;
        psDefn->ProjParm[5] = dfFalseEasting;
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[6] = dfFalseNorthing;
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        psDefn->nParms = 7;
        break;

/* -------------------------------------------------------------------- */
      case CT_PolarStereographic:
/* -------------------------------------------------------------------- */
        if( GetMetadataElement( "GEOTIFF_NUM::3095::ProjStraightVertPoleLongGeoKey",
                                &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3080::ProjNatOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3084::ProjFalseOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3088::ProjCenterLongGeoKey",
                                   &dfNatOriginLong ) == 0 )
            dfNatOriginLong = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3081::ProjNatOriginLatGeoKey",
                                &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3085::ProjFalseOriginLatGeoKey",
                                   &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3089::ProjCenterLatGeoKey",
                                   &dfNatOriginLat ) == 0 )
            dfNatOriginLat = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3092::ProjScaleAtNatOriginGeoKey",
                                &dfNatOriginScale ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3093::ProjScaleAtCenterGeoKey",
                                   &dfNatOriginScale ) == 0 )
            dfNatOriginScale = 1.0;
            
        /* notdef: should transform to decimal degrees at this point */

        psDefn->ProjParm[0] = dfNatOriginLat;
        psDefn->ProjParmId[0] = ProjNatOriginLatGeoKey;;
        psDefn->ProjParm[1] = dfNatOriginLong;
        psDefn->ProjParmId[1] = ProjStraightVertPoleLongGeoKey;
        psDefn->ProjParm[4] = dfNatOriginScale;
        psDefn->ProjParmId[4] = ProjScaleAtNatOriginGeoKey;
        psDefn->ProjParm[5] = dfFalseEasting;
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[6] = dfFalseNorthing;
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        psDefn->nParms = 7;
        break;

/* -------------------------------------------------------------------- */
      case CT_LambertConfConic_2SP:
/* -------------------------------------------------------------------- */
        if( GetMetadataElement( "GEOTIFF_NUM::3078::ProjStdParallel1GeoKey",
                                &dfStdParallel1 ) == 0 )
            dfStdParallel1 = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3079::ProjStdParallel2GeoKey",
                                &dfStdParallel2 ) == 0 )
            dfStdParallel1 = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3080::ProjNatOriginLongGeoKey",
                                &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3084::ProjFalseOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3088::ProjCenterLongGeoKey", 
                                   &dfNatOriginLong ) == 0 )
            dfNatOriginLong = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3081::ProjNatOriginLatGeoKey",
                                &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3085::ProjFalseOriginLatGeoKey",
                                   &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3089::ProjCenterLatGeoKey",
                                   &dfNatOriginLat ) == 0 )
            dfNatOriginLat = 0.0;

        /* notdef: should transform to decimal degrees at this point */

        psDefn->ProjParm[0] = dfNatOriginLat;
        psDefn->ProjParmId[0] = ProjFalseOriginLatGeoKey;
        psDefn->ProjParm[1] = dfNatOriginLong;
        psDefn->ProjParmId[1] = ProjFalseOriginLongGeoKey;
        psDefn->ProjParm[2] = dfStdParallel1;
        psDefn->ProjParmId[2] = ProjStdParallel1GeoKey;
        psDefn->ProjParm[3] = dfStdParallel2;
        psDefn->ProjParmId[3] = ProjStdParallel2GeoKey;
        psDefn->ProjParm[5] = dfFalseEasting;
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[6] = dfFalseNorthing;
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        psDefn->nParms = 7;
        break;

/* -------------------------------------------------------------------- */
      case CT_AlbersEqualArea:
      case CT_EquidistantConic:
/* -------------------------------------------------------------------- */
        if( GetMetadataElement( "GEOTIFF_NUM::3078::ProjStdParallel1GeoKey",
                                &dfStdParallel1 ) == 0 )
            dfStdParallel1 = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3079::ProjStdParallel2GeoKey",
                                &dfStdParallel2 ) == 0 )
            dfStdParallel1 = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3080::ProjNatOriginLongGeoKey",
                                &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3084::ProjFalseOriginLongGeoKey",
                                   &dfNatOriginLong ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3088::ProjCenterLongGeoKey", 
                                   &dfNatOriginLong ) == 0 )
            dfNatOriginLong = 0.0;

        if( GetMetadataElement( "GEOTIFF_NUM::3081::ProjNatOriginLatGeoKey",
                                &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3085::ProjFalseOriginLatGeoKey",
                                   &dfNatOriginLat ) == 0
            && GetMetadataElement( "GEOTIFF_NUM::3089::ProjCenterLatGeoKey",
                                   &dfNatOriginLat ) == 0 )
            dfNatOriginLat = 0.0;

        /* notdef: should transform to decimal degrees at this point */

        psDefn->ProjParm[0] = dfStdParallel1;
        psDefn->ProjParmId[0] = ProjStdParallel1GeoKey;
        psDefn->ProjParm[1] = dfStdParallel2;
        psDefn->ProjParmId[1] = ProjStdParallel2GeoKey;
        psDefn->ProjParm[2] = dfNatOriginLat;
        psDefn->ProjParmId[2] = ProjNatOriginLatGeoKey;
        psDefn->ProjParm[3] = dfNatOriginLong;
        psDefn->ProjParmId[3] = ProjNatOriginLongGeoKey;
        psDefn->ProjParm[5] = dfFalseEasting;
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[6] = dfFalseNorthing;
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        psDefn->nParms = 7;
        break;
    }
}

/************************************************************************/
/*                            GetGTIFDefn()                             */
/*      This function borrowed from the GTIFGetDefn() function.         */
/*      See geo_normalize.c from the GeoTIFF package.                   */
/************************************************************************/

void MrSIDDataset::GetGTIFDefn()
{
    double	dfInvFlattening;

/* -------------------------------------------------------------------- */
/*      Initially we default all the information we can.                */
/* -------------------------------------------------------------------- */
    psDefn = new( GTIFDefn );
    psDefn->Model = KvUserDefined;
    psDefn->PCS = KvUserDefined;
    psDefn->GCS = KvUserDefined;
    psDefn->UOMLength = KvUserDefined;
    psDefn->UOMLengthInMeters = 1.0;
    psDefn->UOMAngle = KvUserDefined;
    psDefn->UOMAngleInDegrees = 1.0;
    psDefn->Datum = KvUserDefined;
    psDefn->Ellipsoid = KvUserDefined;
    psDefn->SemiMajor = 0.0;
    psDefn->SemiMinor = 0.0;
    psDefn->PM = KvUserDefined;
    psDefn->PMLongToGreenwich = 0.0;

    psDefn->ProjCode = KvUserDefined;
    psDefn->Projection = KvUserDefined;
    psDefn->CTProjection = KvUserDefined;

    psDefn->nParms = 0;
    for( int i = 0; i < MAX_GTIF_PROJPARMS; i++ )
    {
        psDefn->ProjParm[i] = 0.0;
        psDefn->ProjParmId[i] = 0;
    }

    psDefn->MapSys = KvUserDefined;
    psDefn->Zone = 0;

/* -------------------------------------------------------------------- */
/*	Try to get the overall model type.				*/
/* -------------------------------------------------------------------- */
    GetMetadataElement( "GEOTIFF_NUM::1024::GTModelTypeGeoKey",
                        &(psDefn->Model) );

/* -------------------------------------------------------------------- */
/*      Try to get a PCS.                                               */
/* -------------------------------------------------------------------- */
    if( GetMetadataElement( "GEOTIFF_NUM::3072::ProjectedCSTypeGeoKey",
                            &(psDefn->PCS) ) == 1
        && psDefn->PCS != KvUserDefined )
    {
        /*
         * Translate this into useful information.
         */
        GTIFGetPCSInfo( psDefn->PCS, NULL, &(psDefn->ProjCode),
                        &(psDefn->UOMLength), &(psDefn->GCS) );
    }

/* -------------------------------------------------------------------- */
/*       If we have the PCS code, but didn't find it in the CSV files   */
/*      (likely because we can't find them) we will try some ``jiffy    */
/*      rules'' for UTM and state plane.                                */
/* -------------------------------------------------------------------- */
    if( psDefn->PCS != KvUserDefined && psDefn->ProjCode == KvUserDefined )
    {
        int	nMapSys, nZone;
        int	nGCS = psDefn->GCS;

        nMapSys = GTIFPCSToMapSys( psDefn->PCS, &nGCS, &nZone );
        if( nMapSys != KvUserDefined )
        {
            psDefn->ProjCode = (short) GTIFMapSysToProj( nMapSys, nZone );
            psDefn->GCS = (short) nGCS;
        }
    }
   
/* -------------------------------------------------------------------- */
/*      If the Proj_ code is specified directly, use that.              */
/* -------------------------------------------------------------------- */
    if( psDefn->ProjCode == KvUserDefined )
        GetMetadataElement( "GEOTIFF_NUM::3074::ProjectionGeoKey",
                            &(psDefn->ProjCode) );
    
    if( psDefn->ProjCode != KvUserDefined )
    {
        /*
         * We have an underlying projection transformation value.  Look
         * this up.  For a PCS of ``WGS 84 / UTM 11'' the transformation
         * would be Transverse Mercator, with a particular set of options.
         * The nProjTRFCode itself would correspond to the name
         * ``UTM zone 11N'', and doesn't include datum info.
         */
        GTIFGetProjTRFInfo( psDefn->ProjCode, NULL, &(psDefn->Projection),
                            psDefn->ProjParm );
        
        /*
         * Set the GeoTIFF identity of the parameters.
         */
        psDefn->CTProjection = (short)
            EPSGProjMethodToCTProjMethod( psDefn->Projection );

        SetGTParmIds( psDefn->CTProjection, psDefn->ProjParmId, NULL);
        psDefn->nParms = 7;
    }

/* -------------------------------------------------------------------- */
/*      Try to get a GCS.  If found, it will override any implied by    */
/*      the PCS.                                                        */
/* -------------------------------------------------------------------- */
    GetMetadataElement( "GEOTIFF_NUM::2048::GeographicTypeGeoKey",
                        &(psDefn->GCS) );

/* -------------------------------------------------------------------- */
/*      Derive the datum, and prime meridian from the GCS.              */
/* -------------------------------------------------------------------- */
    if( psDefn->GCS != KvUserDefined )
    {
        GTIFGetGCSInfo( psDefn->GCS, NULL, &(psDefn->Datum), &(psDefn->PM),
                        &(psDefn->UOMAngle) );
    }
    
/* -------------------------------------------------------------------- */
/*      Handle the GCS angular units.  GeogAngularUnitsGeoKey           */
/*      overrides the GCS or PCS setting.                               */
/* -------------------------------------------------------------------- */
    GetMetadataElement( "GEOTIFF_NUM::2054::GeogAngularUnitsGeoKey",
                        &(psDefn->UOMAngle) );
    if( psDefn->UOMAngle != KvUserDefined )
    {
        GTIFGetUOMAngleInfo( psDefn->UOMAngle, NULL,
                             &(psDefn->UOMAngleInDegrees) );
    }

/* -------------------------------------------------------------------- */
/*      Check for a datum setting, and then use the datum to derive     */
/*      an ellipsoid.                                                   */
/* -------------------------------------------------------------------- */
    GetMetadataElement( "GEOTIFF_NUM::2050::GeogGeodeticDatumGeoKey",
                        &(psDefn->Datum) );

    if( psDefn->Datum != KvUserDefined )
    {
        GTIFGetDatumInfo( psDefn->Datum, NULL, &(psDefn->Ellipsoid) );
    }

/* -------------------------------------------------------------------- */
/*      Check for an explicit ellipsoid.  Use the ellipsoid to          */
/*      derive the ellipsoid characteristics, if possible.              */
/* -------------------------------------------------------------------- */
    GetMetadataElement( "GEOTIFF_NUM::2056::GeogEllipsoidGeoKey",
                        &(psDefn->Ellipsoid) );

    if( psDefn->Ellipsoid != KvUserDefined )
    {
        GTIFGetEllipsoidInfo( psDefn->Ellipsoid, NULL,
                              &(psDefn->SemiMajor), &(psDefn->SemiMinor) );
    }

/* -------------------------------------------------------------------- */
/*      Check for overridden ellipsoid parameters.  It would be nice    */
/*      to warn if they conflict with provided information, but for     */
/*      now we just override.                                           */
/* -------------------------------------------------------------------- */
    GetMetadataElement( "GEOTIFF_NUM::2057::GeogSemiMajorAxisGeoKey",
                        &(psDefn->SemiMajor) );
    GetMetadataElement( "GEOTIFF_NUM::2058::GeogSemiMinorAxisGeoKey",
                        &(psDefn->SemiMinor) );
    
    if( GetMetadataElement( "GEOTIFF_NUM::2059::GeogInvFlatteningGeoKey",
                            &dfInvFlattening ) == 1 )
    {
        if( dfInvFlattening != 0.0 )
            psDefn->SemiMinor = 
                psDefn->SemiMajor * (1 - 1.0/dfInvFlattening);
    }

/* -------------------------------------------------------------------- */
/*      Get the prime meridian info.                                    */
/* -------------------------------------------------------------------- */
    GetMetadataElement( "GEOTIFF_NUM::2051::GeogPrimeMeridianGeoKey",
                        &(psDefn->PM) );

    if( psDefn->PM != KvUserDefined )
    {
        GTIFGetPMInfo( psDefn->PM, NULL, &(psDefn->PMLongToGreenwich) );
    }
    else
    {
        GetMetadataElement( "GEOTIFF_NUM::2061::GeogPrimeMeridianLongGeoKey",
                            &(psDefn->PMLongToGreenwich) );

        psDefn->PMLongToGreenwich =
            GTIFAngleToDD( psDefn->PMLongToGreenwich,
                           psDefn->UOMAngle );
    }

/* -------------------------------------------------------------------- */
/*      Have the projection units of measure been overridden?  We       */
/*      should likely be doing something about angular units too,       */
/*      but these are very rarely not decimal degrees for actual        */
/*      file coordinates.                                               */
/* -------------------------------------------------------------------- */
    GetMetadataElement( "GEOTIFF_NUM::3076::ProjLinearUnitsGeoKey",
                        &(psDefn->UOMLength) );

    if( psDefn->UOMLength != KvUserDefined )
    {
        GTIFGetUOMLengthInfo( psDefn->UOMLength, NULL,
                              &(psDefn->UOMLengthInMeters) );
    }

/* -------------------------------------------------------------------- */
/*      Handle a variety of user defined transform types.               */
/* -------------------------------------------------------------------- */
    if( GetMetadataElement( "GEOTIFF_NUM::3075::ProjCoordTransGeoKey",
                            &(psDefn->CTProjection) ) == 1)
    {
        FetchProjParms();
    }

/* -------------------------------------------------------------------- */
/*      Try to set the zoned map system information.                    */
/* -------------------------------------------------------------------- */
    psDefn->MapSys = GTIFProjToMapSys( psDefn->ProjCode, &(psDefn->Zone) );

/* -------------------------------------------------------------------- */
/*      If this is UTM, and we were unable to extract the projection    */
/*      parameters from the CSV file, just set them directly now,       */
/*      since it's pretty easy, and a common case.                      */
/* -------------------------------------------------------------------- */
    if( (psDefn->MapSys == MapSys_UTM_North
         || psDefn->MapSys == MapSys_UTM_South)
        && psDefn->CTProjection == KvUserDefined )
    {
        psDefn->CTProjection = CT_TransverseMercator;
        psDefn->nParms = 7;
        psDefn->ProjParmId[0] = ProjNatOriginLatGeoKey;
        psDefn->ProjParm[0] = 0.0;
            
        psDefn->ProjParmId[1] = ProjNatOriginLongGeoKey;
        psDefn->ProjParm[1] = psDefn->Zone*6 - 183.0;
        
        psDefn->ProjParmId[4] = ProjScaleAtNatOriginGeoKey;
        psDefn->ProjParm[4] = 0.9996;
        
        psDefn->ProjParmId[5] = ProjFalseEastingGeoKey;
        psDefn->ProjParm[5] = 500000.0;
        
        psDefn->ProjParmId[6] = ProjFalseNorthingGeoKey;

        if( psDefn->MapSys == MapSys_UTM_North )
            psDefn->ProjParm[6] = 0.0;
        else
            psDefn->ProjParm[6] = 10000000.0;
    }

    if ( pszProjection )
        CPLFree( pszProjection );
    pszProjection = GTIFGetOGISDefn( psDefn );
}

/************************************************************************/
/*                             OpenZoomLevel()                          */
/************************************************************************/

CPLErr MrSIDDataset::OpenZoomLevel( int iZoom )
{
    try
    {
        poMrSidNav = new MrSIDNavigator( *poMrSidFile );
    }
    catch ( ... )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "MrSIDDataset::OpenZoomLevel(): "
                  "Failed to create MrSIDNavigator object." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Handle sample type and color space.                             */
/* -------------------------------------------------------------------- */
    poDefaultPixel = new Pixel( poMrSidFile->getDefaultPixelValue() );
    eSampleType = poMrSidFile->getSampleType();
#ifdef MRSID_DSDK_VERSION_31
    poColorSpace = new ColorSpace( poMrSidFile->colorSpace() );
#else
    poColorSpace = new MrSIDColorSpace( poMrSidFile->colorSpace() );
#endif
    switch ( eSampleType )
    {
        case ImageBufferInfo::UINT16:
        eDataType = GDT_UInt16;
        break;
        case ImageBufferInfo::UINT32:
        eDataType = GDT_UInt32;
        break;
        case ImageBufferInfo::FLOAT32:
        eDataType = GDT_Float32;
        break;
        case ImageBufferInfo::UINT8:
        default:
        eDataType = GDT_Byte;
        break;
    }

/* -------------------------------------------------------------------- */
/*      Take image geometry.                                            */
/* -------------------------------------------------------------------- */
    nRasterXSize = poMrSidFile->width();
    nRasterYSize = poMrSidFile->height();
    nBands = poMrSidFile->nband();

#ifdef MRSID_DSDK_VERSION_31
    CPLAssert( poColorSpace->samplesPerPixel () == nBands);
#endif

/* -------------------------------------------------------------------- */
/*      Take georeferencing.                                            */
/* -------------------------------------------------------------------- */
    if ( poMrSidFile->hasWorldInfo()
         && poMrSidFile->xu(adfGeoTransform[0])
         && poMrSidFile->yu(adfGeoTransform[3])
         && poMrSidFile->xres(adfGeoTransform[1])
         && poMrSidFile->yres(adfGeoTransform[5])
         && poMrSidFile->xrot(adfGeoTransform[2])
         && poMrSidFile->yrot(adfGeoTransform[4]) )
    {
        adfGeoTransform[5] = - adfGeoTransform[5];
        adfGeoTransform[0] = adfGeoTransform[0] - adfGeoTransform[1] / 2;
        adfGeoTransform[3] = adfGeoTransform[3] - adfGeoTransform[5] / 2;
        bHasGeoTransfom = TRUE;
    }

    if ( iZoom != 0 )
    {
        nCurrentZoomLevel = iZoom;
        nRasterXSize =
            poMrSidFile->getDimensionsAtLevel( nCurrentZoomLevel ).width;
        nRasterYSize =
            poMrSidFile->getDimensionsAtLevel( nCurrentZoomLevel ).height;
    }
    else
    {
        nCurrentZoomLevel = 0;
    }

    CPLDebug( "MrSID", "Opened zoom level %d with size %dx%d.\n",
              iZoom, nRasterXSize, nRasterYSize );

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    int             iBand;

    for( iBand = 1; iBand <= nBands; iBand++ )
        SetBand( iBand, new MrSIDRasterBand( this, iBand ) );

    return CE_None;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *MrSIDDataset::Open( GDALOpenInfo * poOpenInfo )
{
    if( poOpenInfo->fp == NULL )
        return NULL;

    if( !EQUALN((const char *) poOpenInfo->pabyHeader, "msid", 4) )
        return NULL;

    VSIFCloseL( poOpenInfo->fp );
    poOpenInfo->fp = NULL;

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    MrSIDDataset      *poDS;

    poDS = new MrSIDDataset();
    try
    {
#ifdef MRSID_DSDK_VERSION_31
        FileSpecification oFilename( poOpenInfo->pszFilename );
        poDS->poMrSidFile = new MrSIDImageFile( oFilename );
#else
        LTFileSpec      oFilename( poOpenInfo->pszFilename );
        poDS->poMrSidFile = new MrSIDImageFile( oFilename, (LTCallbacks *)0 );
#endif
    }
    catch ( ... )
    {
        delete poDS;
        CPLError( CE_Failure, CPLE_AppDefined,
                  "MrSIDDataset::Open(): Failed to open file %s",
                  poOpenInfo->pszFilename );
        return NULL;
    }

    XTrans::initialize();

/* -------------------------------------------------------------------- */
/*      Take metadata.                                                  */
/* -------------------------------------------------------------------- */
    poDS->poMrSidMetadata = new MetadataReader( poDS->poMrSidFile->metadata() );
    if ( !poDS->poMrSidMetadata->empty() )
    {
        MetadataReader::const_iterator i = poDS->poMrSidMetadata->begin();

        while( i != poDS->poMrSidMetadata->end() )
        {
            char    *pszElement = poDS->SerializeMetadataElement( &(*i) );
            char    *pszKey = CPLStrdup( (*i).getKey().c_str() );
            char    *pszTemp = pszKey;

            // GDAL metadata keys should not contain ':' and '=' characters.
            // We will replace them with '_'.
            do
            {
                if ( *pszTemp == ':' || *pszTemp == '=' )
                    *pszTemp = '_';
            }
            while ( *++pszTemp );

            poDS->SetMetadataItem( pszKey, pszElement );

            CPLFree( pszElement );
            CPLFree( pszKey );
            i++;
        }
    }

    poDS->GetGTIFDefn();

/* -------------------------------------------------------------------- */
/*   Take number of resolution levels (we will use them as overviews).  */
/* -------------------------------------------------------------------- */
    poDS->nOverviewCount = poDS->poMrSidFile->nlev() - 1;

    if ( poDS->nOverviewCount > 0 )
    {
        int         i;

        poDS->papoOverviewDS = (MrSIDDataset **)
            CPLMalloc( poDS->nOverviewCount * (sizeof(void*)) );

        for ( i = 0; i < poDS->nOverviewCount; i++ )
        {
            poDS->papoOverviewDS[i] = new MrSIDDataset();
            poDS->papoOverviewDS[i]->poMrSidFile = poDS->poMrSidFile;
            poDS->papoOverviewDS[i]->OpenZoomLevel( i + 1 );
            poDS->papoOverviewDS[i]->bIsOverview = TRUE;
            
        }
    }

/* -------------------------------------------------------------------- */
/*      Band objects will be created in separate function.              */
/* -------------------------------------------------------------------- */
    poDS->OpenZoomLevel( 0 );

    CPLDebug( "MrSID",
              "Opened image: width %d, height %d, bands %d, overviews %d",
              poDS->nRasterXSize, poDS->nRasterYSize, poDS->nBands,
              poDS->nOverviewCount );

    return( poDS );
}

/************************************************************************/
/*                        GDALRegister_MrSID()                          */
/************************************************************************/

void GDALRegister_MrSID()

{
    GDALDriver  *poDriver;

    if( GDALGetDriverByName( "MrSID" ) == NULL )
    {
        poDriver = new GDALDriver();

        poDriver->SetDescription( "MrSID" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME,
                                   "Multi-resolution Seamless Image Database (MrSID)" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC,
                                   "frmt_mrsid.html" );

        poDriver->pfnOpen = MrSIDDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

