/******************************************************************************
 * $Id$
 *
 * Project:  High Performance Image Reprojector
 * Purpose:  Implementation of high level convenience APIs for warper.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.9  2004/02/25 19:51:00  warmerda
 * Fixed nodata check in GDT_Int16 case (from Manuel Massing).
 *
 * Revision 1.8  2003/07/26 17:32:16  warmerda
 * Added info on new warp options.
 *
 * Revision 1.7  2003/05/06 18:31:35  warmerda
 * added WRITE_FLUSH comment
 *
 * Revision 1.6  2003/04/22 19:41:24  dron
 * Fixed problem in GDALWarpNoDataMasker(): missed breaks in switch.
 *
 * Revision 1.5  2003/03/02 05:25:26  warmerda
 * added GDALWarpNoDataMasker
 *
 * Revision 1.4  2003/02/22 02:04:11  warmerda
 * added dfMaxError to reproject function
 *
 * Revision 1.3  2003/02/21 15:41:55  warmerda
 * rename data member
 *
 * Revision 1.2  2003/02/20 21:53:06  warmerda
 * partial implementation
 *
 * Revision 1.1  2003/02/18 17:25:50  warmerda
 * New
 *
 */

#include "gdalwarper.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                         GDALReprojectImage()                         */
/************************************************************************/

/**
 * Reproject image.
 *
 * This is a convenience function utilizing the GDALWarpOperation class to
 * reproject an image from a source to a destination.  In particular, this
 * function takes care of establishing the transformation function to
 * implement the reprojection, and will default a variety of other 
 * warp options. 
 *
 * By default all bands are transferred, with no masking or nodata values
 * in effect.  No metadata, projection info, or color tables are transferred 
 * to the output file. 
 *
 * @param hSrcDS the source image file. 
 * @param pszSrcWKT the source projection.  If NULL the source projection
 * is read from from hSrcDS.
 * @param hDstDS the destination image file. 
 * @param pszDstWKT the destination projection.  If NULL the destination
 * projection will be read from hDstDS.
 * @param eResampleAlg the type of resampling to use.  
 * @param dfWarpMemoryLimit the amount of memory (in bytes) that the warp
 * API is allowed to use for caching.  This is in addition to the memory
 * already allocated to the GDAL caching (as per GDALSetCacheMax()).  May be
 * 0.0 to use default memory settings.
 * @param dfMaxError maximum error measured in input pixels that is allowed
 * in approximating the transformation (0.0 for exact calculations).
 * @param pfnProgress a GDALProgressFunc() compatible callback function for
 * reporting progress or NULL. 
 * @param pProgressArg argument to be passed to pfnProgress.  May be NULL.
 * @param psOptions warp options, normally NULL.
 *
 * @return CE_None on success or CE_Failure if something goes wrong.
 */

CPLErr GDALReprojectImage( GDALDatasetH hSrcDS, const char *pszSrcWKT, 
                           GDALDatasetH hDstDS, const char *pszDstWKT,
                           GDALResampleAlg eResampleAlg, 
                           double dfWarpMemoryLimit, 
                           double dfMaxError,
                           GDALProgressFunc pfnProgress, void *pProgressArg, 
                           GDALWarpOptions *psOptions )

{
    GDALWarpOptions *psWOptions;

/* -------------------------------------------------------------------- */
/*      Default a few parameters.                                       */
/* -------------------------------------------------------------------- */
    if( pszSrcWKT == NULL )
        pszSrcWKT = GDALGetProjectionRef( hSrcDS );

    if( pszDstWKT == NULL )
        pszDstWKT = pszSrcWKT;

/* -------------------------------------------------------------------- */
/*      Setup a reprojection based transformer.                         */
/* -------------------------------------------------------------------- */
    void *hTransformArg;

    hTransformArg = 
        GDALCreateGenImgProjTransformer( hSrcDS, pszSrcWKT, hDstDS, pszDstWKT, 
                                         TRUE, 1000.0, 2 );

    if( hTransformArg == NULL )
        return CE_Failure;

/* -------------------------------------------------------------------- */
/*      Create a copy of the user provided options, or a defaulted      */
/*      options structure.                                              */
/* -------------------------------------------------------------------- */
    if( psOptions == NULL )
        psWOptions = GDALCreateWarpOptions();
    else
        psWOptions = GDALCloneWarpOptions( psOptions );

/* -------------------------------------------------------------------- */
/*      Set transform.                                                  */
/* -------------------------------------------------------------------- */
    if( dfMaxError > 0.0 )
    {
        psWOptions->pTransformerArg = 
            GDALCreateApproxTransformer( GDALGenImgProjTransform, 
                                         hTransformArg, dfMaxError );

        psWOptions->pfnTransformer = GDALApproxTransform;
    }
    else
    {
        psWOptions->pfnTransformer = GDALGenImgProjTransform;
        psWOptions->pTransformerArg = hTransformArg;
    }

/* -------------------------------------------------------------------- */
/*      Set file and band mapping.                                      */
/* -------------------------------------------------------------------- */
    int  iBand;

    psWOptions->hSrcDS = hSrcDS;
    psWOptions->hDstDS = hDstDS;

    if( psWOptions->nBandCount == 0 )
    {
        psWOptions->nBandCount = MIN(GDALGetRasterCount(hSrcDS),
                                     GDALGetRasterCount(hDstDS));
        
        psWOptions->panSrcBands = (int *) 
            CPLMalloc(sizeof(int) * psWOptions->nBandCount);
        psWOptions->panDstBands = (int *) 
            CPLMalloc(sizeof(int) * psWOptions->nBandCount);

        for( iBand = 0; iBand < psWOptions->nBandCount; iBand++ )
        {
            psWOptions->panSrcBands[iBand] = iBand+1;
            psWOptions->panDstBands[iBand] = iBand+1;
        }
    }

/* -------------------------------------------------------------------- */
/*      Set source nodata values if the source dataset seems to have    */
/*      any.                                                            */
/* -------------------------------------------------------------------- */
    for( iBand = 0; iBand < psWOptions->nBandCount; iBand++ )
    {
        GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, iBand+1 );
        int             bGotNoData = FALSE;
        double          dfNoDataValue;
        
        dfNoDataValue = GDALGetRasterNoDataValue( hBand, &bGotNoData );
        if( bGotNoData )
        {
            if( psWOptions->padfSrcNoDataReal == NULL )
            {
                int  ii;

                psWOptions->padfSrcNoDataReal = (double *) 
                    CPLMalloc(sizeof(double) * psWOptions->nBandCount);
                psWOptions->padfSrcNoDataImag = (double *) 
                    CPLMalloc(sizeof(double) * psWOptions->nBandCount);

                for( ii = 0; ii < psWOptions->nBandCount; ii++ )
                {
                    psWOptions->padfSrcNoDataReal[ii] = -1.1e20;
                    psWOptions->padfSrcNoDataReal[ii] = 0.0;
                }
            }

            psWOptions->padfSrcNoDataReal[iBand] = dfNoDataValue;
        }
    }

/* -------------------------------------------------------------------- */
/*      Set the progress function.                                      */
/* -------------------------------------------------------------------- */
    if( pfnProgress != NULL )
    {
        psWOptions->pfnProgress = pfnProgress;
        psWOptions->pProgressArg = pProgressArg;
    }

/* -------------------------------------------------------------------- */
/*      Create a warp options based on the options.                     */
/* -------------------------------------------------------------------- */
    GDALWarpOperation  oWarper;
    CPLErr eErr;

    eErr = oWarper.Initialize( psWOptions );

    if( eErr == CE_None )
        eErr = oWarper.ChunkAndWarpImage( 0, 0, 
                                          GDALGetRasterXSize(hDstDS),
                                          GDALGetRasterYSize(hDstDS) );

/* -------------------------------------------------------------------- */
/*      Cleanup.                                                        */
/* -------------------------------------------------------------------- */
    GDALDestroyGenImgProjTransformer( hTransformArg );

    if( dfMaxError > 0.0 )
        GDALDestroyApproxTransformer( psWOptions->pTransformerArg );
        
    GDALDestroyWarpOptions( psWOptions );

    return eErr;
}

/************************************************************************/
/*                    GDALCreateAndReprojectImage()                     */
/*                                                                      */
/*      This is a "quicky" reprojection API.                            */
/************************************************************************/

CPLErr GDALCreateAndReprojectImage( 
    GDALDatasetH hSrcDS, const char *pszSrcWKT, 
    const char *pszDstFilename, const char *pszDstWKT,
    GDALDriverH hDstDriver, char **papszCreateOptions,
    GDALResampleAlg eResampleAlg, double dfWarpMemoryLimit, double dfMaxError,
    GDALProgressFunc pfnProgress, void *pProgressArg, 
    GDALWarpOptions *psOptions )
    
{
/* -------------------------------------------------------------------- */
/*      Default a few parameters.                                       */
/* -------------------------------------------------------------------- */
    if( hDstDriver == NULL )
        hDstDriver = GDALGetDriverByName( "GTiff" );

    if( pszSrcWKT == NULL )
        pszSrcWKT = GDALGetProjectionRef( hSrcDS );

    if( pszDstWKT == NULL )
        pszDstWKT = pszSrcWKT;

/* -------------------------------------------------------------------- */
/*      Create a transformation object from the source to               */
/*      destination coordinate system.                                  */
/* -------------------------------------------------------------------- */
    void *hTransformArg;

    hTransformArg = 
        GDALCreateGenImgProjTransformer( hSrcDS, pszSrcWKT, NULL, pszDstWKT, 
                                         TRUE, 1000.0, 2 );

    if( hTransformArg == NULL )
        return CE_Failure;

/* -------------------------------------------------------------------- */
/*      Get approximate output definition.                              */
/* -------------------------------------------------------------------- */
    double adfDstGeoTransform[6];
    int    nPixels, nLines;

    if( GDALSuggestedWarpOutput( hSrcDS, 
                                 GDALGenImgProjTransform, hTransformArg, 
                                 adfDstGeoTransform, &nPixels, &nLines )
        != CE_None )
        return CE_Failure;

    GDALDestroyGenImgProjTransformer( hTransformArg );

/* -------------------------------------------------------------------- */
/*      Create the output file.                                         */
/* -------------------------------------------------------------------- */
    GDALDatasetH hDstDS;

    hDstDS = GDALCreate( hDstDriver, pszDstFilename, nPixels, nLines, 
                         GDALGetRasterCount(hSrcDS),
                         GDALGetRasterDataType(GDALGetRasterBand(hSrcDS,1)),
                         papszCreateOptions );

    if( hDstDS == NULL )
        return CE_Failure;

/* -------------------------------------------------------------------- */
/*      Write out the projection definition.                            */
/* -------------------------------------------------------------------- */
    GDALSetProjection( hDstDS, pszDstWKT );
    GDALSetGeoTransform( hDstDS, adfDstGeoTransform );

/* -------------------------------------------------------------------- */
/*      Perform the reprojection.                                       */
/* -------------------------------------------------------------------- */
    CPLErr eErr ;

    eErr = 
        GDALReprojectImage( hSrcDS, pszSrcWKT, hDstDS, pszDstWKT, 
                            eResampleAlg, dfWarpMemoryLimit, dfMaxError,
                            pfnProgress, pProgressArg, psOptions );

    GDALClose( hDstDS );

    return eErr;
}

/************************************************************************/
/*                        GDALWarpNoDataMasker()                        */
/*                                                                      */
/*      GDALMaskFunc for establishing a validity mask for a source      */
/*      band based on a provided NODATA value.                          */
/************************************************************************/

CPLErr 
GDALWarpNoDataMasker( void *pMaskFuncArg, int nBandCount, GDALDataType eType, 
                      int /* nXOff */, int /* nYOff */, int nXSize, int nYSize,
                      GByte **ppImageData, 
                      int bMaskIsFloat, void *pValidityMask )

{
    double *padfNoData = (double *) pMaskFuncArg;
    GUInt32 *panValidityMask = (GUInt32 *) pValidityMask;

    if( nBandCount != 1 || bMaskIsFloat )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Invalid nBandCount or bMaskIsFloat argument in SourceNoDataMask" );
        return CE_Failure;
    }

    switch( eType )
    {
      case GDT_Byte:
      {
          int nNoData = (int) padfNoData[0];
          GByte *pabyData = (GByte *) *ppImageData;
          int iOffset;

          // nothing to do if value is out of range.
          if( padfNoData[0] < 0.0 || padfNoData[0] > 255.000001 
              || padfNoData[1] != 0.0 )
              return CE_None;

          for( iOffset = nXSize*nYSize-1; iOffset >= 0; iOffset-- )
          {
              if( pabyData[iOffset] == nNoData )
              {
                  panValidityMask[iOffset>>5] &= ~(0x01 << (iOffset & 0x1f));
              }
          }
      }
      break;
      
      case GDT_Int16:
      {
          int nNoData = (int) padfNoData[0];
          GInt16 *panData = (GInt16 *) *ppImageData;
          int iOffset;

          // nothing to do if value is out of range.
          if( padfNoData[0] < -32768 || padfNoData[0] > 32767
              || padfNoData[1] != 0.0 )
              return CE_None;

          for( iOffset = nXSize*nYSize-1; iOffset >= 0; iOffset-- )
          {
              if( panData[iOffset] == nNoData )
              {
                  panValidityMask[iOffset>>5] &= ~(0x01 << (iOffset & 0x1f));
              }
          }
      }
      break;
      
      case GDT_UInt16:
      {
          int nNoData = (int) padfNoData[0];
          GUInt16 *panData = (GUInt16 *) *ppImageData;
          int iOffset;

          // nothing to do if value is out of range.
          if( padfNoData[0] < 0 || padfNoData[0] > 65535
              || padfNoData[1] != 0.0 )
              return CE_None;

          for( iOffset = nXSize*nYSize-1; iOffset >= 0; iOffset-- )
          {
              if( panData[iOffset] == nNoData )
              {
                  panValidityMask[iOffset>>5] &= ~(0x01 << (iOffset & 0x1f));
              }
          }
      }
      break;
      
      case GDT_Float32:
      {
          float fNoData = (float) padfNoData[0];
          float *pafData = (float *) *ppImageData;
          int iOffset;

          // nothing to do if value is out of range.
          if( padfNoData[1] != 0.0 )
              return CE_None;

          for( iOffset = nXSize*nYSize-1; iOffset >= 0; iOffset-- )
          {
              if( pafData[iOffset] == fNoData )
              {
                  panValidityMask[iOffset>>5] &= ~(0x01 << (iOffset & 0x1f));
              }
          }
      }
      break;
      
      default:
      {
          double  *padfWrk;
          int     iLine, iPixel;
          int     nWordSize = GDALGetDataTypeSize(eType)/8;

          padfWrk = (double *) CPLMalloc(nXSize * sizeof(double) * 2);
          for( iLine = 0; iLine < nYSize; iLine++ )
          {
              GDALCopyWords( ((GByte *) *ppImageData)+nWordSize*iLine*nXSize, 
                             eType, nWordSize,
                             padfWrk, GDT_CFloat64, 16, nXSize );
              
              for( iPixel = 0; iPixel < nXSize; iPixel++ )
              {
                  if( padfWrk[iPixel*2] == padfNoData[0]
                      && padfWrk[iPixel*2+1] == padfNoData[1] )
                  {
                      int iOffset = iPixel + iLine * nXSize;
                      
                      panValidityMask[iOffset>>5] &=
                          ~(0x01 << (iOffset & 0x1f));
                  }
              }
              
          }

          CPLFree( padfWrk );
      }
      break;
    }

    return CE_None;
}


/************************************************************************/
/* ==================================================================== */
/*                           GDALWarpOptions                            */
/* ==================================================================== */
/************************************************************************/

/**
 * \var char **GDALWarpOptions::papszWarpOptions;
 *
 * A string list of additional options controlling the warp operation in
 * name=value format.  A suitable string list can be prepared with 
 * CSLSetNameValue().
 *
 * The following values are currently supported:
 * 
 *  - INIT_DEST=[value] or INIT_DEST=NO_DATA: This option forces the 
 * destination image to be initialized to the indicated value (for all bands)
 * or indicates that it should be initialized to the NO_DATA value in
 * padfDstNoDataReal/padfDstNoDataImag.  If this value isn't set the
 * destination image will be read and overlayed.  
 *
 * - WRITE_FLUSH=YES/NO: This option forces a flush to disk of data after
 * each chunk is processed.  In some cases this helps ensure a serial 
 * writing of the output data otherwise a block of data may be written to disk
 * each time a block of data is read for the input buffer resulting in alot
 * of extra seeking around the disk, and reduced IO throughput.  The default
 * at this time is NO.
 *
 * Normally when computing the source raster data to 
 * load to generate a particular output area, the warper samples transforms
 * 21 points along each edge of the destination region back onto the source
 * file, and uses this to compute a bounding window on the source image that
 * is sufficient.  Depending on the transformation in effect, the source 
 * window may be a bit too small, or even missing large areas.  Problem 
 * situations are those where the transformation is very non-linear or 
 * "inside out".  Examples are transforming from WGS84 to Polar Steregraphic
 * for areas around the pole, or transformations where some of the image is
 * untransformable.  The following options provide some additional control
 * to deal with errors in computing the source window:
 * 
 * - SAMPLE_GRID=YES/NO: Setting this option to YES will force the sampling to 
 * include internal points as well as edge points which can be important if
 * the transformation is esoteric inside out, or if large sections of the
 * destination image are not transformable into the source coordinate system.
 *
 * - SAMPLE_STEPS: Modifies the density of the sampling grid.  The default
 * number of steps is 21.   Increasing this can increase the computational
 * cost, but improves the accuracy with which the source region is computed.
 *
 * - SOURCE_EXTRA: This is a number of extra pixels added around the source
 * window for a given request, and by default it is 1 to take care of rounding
 * error.  Setting this larger will incease the amount of data that needs to
 * be read, but can avoid missing source data.  
 */

/************************************************************************/
/*                       GDALCreateWarpOptions()                        */
/************************************************************************/

GDALWarpOptions *GDALCreateWarpOptions()

{
    GDALWarpOptions *psOptions;

    psOptions = (GDALWarpOptions *) CPLCalloc(sizeof(GDALWarpOptions),1);

    psOptions->eResampleAlg = GRA_NearestNeighbour;
    psOptions->pfnProgress = GDALDummyProgress;
    psOptions->eWorkingDataType = GDT_Unknown;

    return psOptions;
}

/************************************************************************/
/*                       GDALDestroyWarpOptions()                       */
/************************************************************************/

void GDALDestroyWarpOptions( GDALWarpOptions *psOptions )

{
    CSLDestroy( psOptions->papszWarpOptions );
    CPLFree( psOptions->panSrcBands );
    CPLFree( psOptions->panDstBands );
    CPLFree( psOptions->padfSrcNoDataReal );
    CPLFree( psOptions->padfSrcNoDataImag );
    CPLFree( psOptions->padfDstNoDataReal );
    CPLFree( psOptions->padfDstNoDataImag );
    CPLFree( psOptions->papfnSrcPerBandValidityMaskFunc );
    CPLFree( psOptions->papSrcPerBandValidityMaskFuncArg );

    CPLFree( psOptions );
}


#define COPY_MEM(target,type,count)					\
   if( (psSrcOptions->target) != NULL && (count) != 0 ) 		\
   { 									\
       (psDstOptions->target) = (type *) CPLMalloc(sizeof(type)*count); \
       memcpy( (psDstOptions->target), (psSrcOptions->target),		\
 	       sizeof(type) * count ); 	        			\
   }

/************************************************************************/
/*                        GDALCloneWarpOptions()                        */
/************************************************************************/

GDALWarpOptions *GDALCloneWarpOptions( const GDALWarpOptions *psSrcOptions )

{
    GDALWarpOptions *psDstOptions = GDALCreateWarpOptions();

    memcpy( psDstOptions, psSrcOptions, sizeof(GDALWarpOptions) );

    if( psSrcOptions->papszWarpOptions != NULL )
        psDstOptions->papszWarpOptions = 
            CSLDuplicate( psSrcOptions->papszWarpOptions );

    COPY_MEM( panSrcBands, int, psSrcOptions->nBandCount );
    COPY_MEM( panDstBands, int, psSrcOptions->nBandCount );
    COPY_MEM( padfSrcNoDataReal, double, psSrcOptions->nBandCount );
    COPY_MEM( padfSrcNoDataImag, double, psSrcOptions->nBandCount );
    COPY_MEM( papfnSrcPerBandValidityMaskFunc, GDALMaskFunc, 
              psSrcOptions->nBandCount );

    return psDstOptions;
}

