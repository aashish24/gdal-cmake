/******************************************************************************
 * $Id$
 *
 * Project:  Mapinfo Image Warper
 * Purpose:  Implementation of one or more GDALTrasformerFunc types, including
 *           the GenImgProj (general image reprojector) transformer.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, i3 - information integration and imaging 
 *                          Fort Collin, CO
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
 * Revision 1.11  2003/07/08 15:29:14  warmerda
 * avoid warnings
 *
 * Revision 1.10  2003/06/03 19:42:54  warmerda
 * added partial support for RPC in GenImgProjTransformer
 *
 * Revision 1.9  2003/02/06 04:56:35  warmerda
 * added documentation
 *
 * Revision 1.8  2002/12/13 15:55:27  warmerda
 * fix suggested output to preserve orig size exact on null transform
 *
 * Revision 1.7  2002/12/13 04:57:49  warmerda
 * remove approx transformer debug output
 *
 * Revision 1.6  2002/12/09 16:08:32  warmerda
 * added approximating transformer
 *
 * Revision 1.5  2002/12/07 17:09:38  warmerda
 * added order flag to GenImgProjTransformer
 *
 * Revision 1.4  2002/12/06 21:43:56  warmerda
 * added GCP support to general transformer
 *
 * Revision 1.3  2002/12/05 21:45:01  warmerda
 * fixed a few GenImgProj bugs
 *
 * Revision 1.2  2002/12/05 16:37:46  warmerda
 * fixed method name
 *
 * Revision 1.1  2002/12/05 05:43:23  warmerda
 * New
 *
 */

#include "gdal_priv.h"
#include "gdal_alg.h"
#include "ogr_spatialref.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                          GDALTransformFunc                           */
/*                                                                      */
/*      Documentation for GDALTransformFunc typedef.                    */
/************************************************************************/

/*!

\typedef int GDALTransformerFunc

Generic signature for spatial point transformers.

This function signature is used for a variety of functions that accept 
passed in functions used to transform point locations between two coordinate
spaces.  

The GDALCreateGenImgProjTransformer(), GDALCreateReprojectionTransformer(), 
GDALCreateGCPTransformer() and GDALCreateApproxTransformer() functions can
be used to prepare argument data for some built-in transformers.  As well,
applications can implement their own transformers to the following signature.

\code
typedef int 
(*GDALTransformerFunc)( void *pTransformerArg, 
                        int bDstToSrc, int nPointCount, 
                        double *x, double *y, double *z, int *panSuccess );
\endcode

@param pTransformerArg application supplied callback data used by the
transformer.  

@param bDstToSrc if TRUE the transformation will be from the destination
coordinate space to the source coordinate system, otherwise the transformation
will be from the source coordinate system to the destination coordinate system.

@param nPointCount number of points in the x, y and z arrays.

@param x input X coordinates.  Results returned in same array.

@param y input Y coordinates.  Results returned in same array.

@param z input Z coordinates.  Results returned in same array.

@param panSuccess array of ints in which success (TRUE) or failure (FALSE)
flags are returned for the translation of each point.

@return TRUE if the overall transformation succeeds (though some individual
points may have failed) or FALSE if the overall transformation fails.

*/

/************************************************************************/
/*                        GDALInvGeoTransform()                         */
/*                                                                      */
/*      Invert a standard 3x2 "GeoTransform" style matrix with an       */
/*      implicit [1 0 0] final row.                                     */
/************************************************************************/

int GDALInvGeoTransform( double *gt_in, double *gt_out )

{
    double	det, inv_det;

    /* we assume a 3rd row that is [1 0 0] */

    /* Compute determinate */

    det = gt_in[1] * gt_in[5] - gt_in[2] * gt_in[4];

    if( fabs(det) < 0.000000000000001 )
        return 0;

    inv_det = 1.0 / det;

    /* compute adjoint, and devide by determinate */

    gt_out[1] =  gt_in[5] * inv_det;
    gt_out[4] = -gt_in[4] * inv_det;

    gt_out[2] = -gt_in[2] * inv_det;
    gt_out[5] =  gt_in[1] * inv_det;

    gt_out[0] = ( gt_in[2] * gt_in[3] - gt_in[0] * gt_in[5]) * inv_det;
    gt_out[3] = (-gt_in[1] * gt_in[3] + gt_in[0] * gt_in[4]) * inv_det;

    return 1;
}

/************************************************************************/
/*                      GDALSuggestedWarpOutput()                       */
/************************************************************************/

/**
 * Suggest output file size.
 *
 * This function is used to suggest the size, and georeferenced extents
 * appropriate given the indicated transformation and input file.  It walks
 * the edges of the input file (approximately 20 sample points along each
 * edge) transforming into output coordinates in order to get an extents box.
 *
 * Then a resolution is computed with the intent that the length of the
 * distance from the top left corner of the output imagery to the bottom right
 * corner would represent the same number of pixels as in the source image. 
 * Note that if the image is somewhat rotated the diagonal taken isnt of the
 * whole output bounding rectangle, but instead of the locations where the
 * top/left and bottom/right corners transform.  The output pixel size is 
 * always square.  This is intended to approximately preserve the resolution
 * of the input data in the output file. 
 * 
 * The values returned in padfGeoTransformOut, pnPixels and pnLines are
 * the suggested number of pixels and lines for the output file, and the
 * geotransform relating those pixels to the output georeferenced coordinates.
 *
 * The trickiest part of using the function is ensuring that the 
 * transformer created is from source file pixel/line coordinates to 
 * output file georeferenced coordinates.  This can be accomplished with 
 * GDALCreateGenImProjTransformer() by passing a NULL for the hDstDS.  
 *
 * @param hSrcDS the input image (it is assumed the whole input images is
 * being transformed). 
 * @param pfnTransformer the transformer function.
 * @param pTransformArg the callback data for the transformer function.
 * @param padfGeoTransformOut the array of six doubles in which the suggested
 * geotransform is returned. 
 * @param pnPixels int in which the suggest pixel width of output is returned.
 * @param pnLines int in which the suggest pixel height of output is returned.
 *
 * @return CE_None if successful or CE_Failure otherwise. 
 */


CPLErr GDALSuggestedWarpOutput( GDALDatasetH hSrcDS, 
                                GDALTransformerFunc pfnTransformer, 
                                void *pTransformArg, 
                                double *padfGeoTransformOut, 
                                int *pnPixels, int *pnLines )

{
/* -------------------------------------------------------------------- */
/*      Setup sample points all around the edge of the input raster.    */
/* -------------------------------------------------------------------- */
    int    nSamplePoints=0, abSuccess[84];
    double adfX[84], adfY[84], adfZ[84], dfRatio;
    int    nInXSize = GDALGetRasterXSize( hSrcDS );
    int    nInYSize = GDALGetRasterYSize( hSrcDS );

    // Take 20 steps 
    for( dfRatio = 0.0; dfRatio <= 1.01; dfRatio += 0.05 )
    {
        
        // Ensure we end exactly at the end.
        if( dfRatio > 0.99 )
            dfRatio = 1.0;

        // Along top 
        adfX[nSamplePoints]   = dfRatio * nInXSize;
        adfY[nSamplePoints]   = 0.0;
        adfZ[nSamplePoints++] = 0.0;

        // Along bottom 
        adfX[nSamplePoints]   = dfRatio * nInXSize;
        adfY[nSamplePoints]   = nInYSize;
        adfZ[nSamplePoints++] = 0.0;

        // Along left
        adfX[nSamplePoints]   = 0.0;
        adfY[nSamplePoints] = dfRatio * nInYSize;
        adfZ[nSamplePoints++] = 0.0;

        // Along right
        adfX[nSamplePoints]   = nInXSize;
        adfY[nSamplePoints] = dfRatio * nInYSize;
        adfZ[nSamplePoints++] = 0.0;
    }

    CPLAssert( nSamplePoints == 84 );

/* -------------------------------------------------------------------- */
/*      Transform them to the output coordinate system.                 */
/* -------------------------------------------------------------------- */
    if( !pfnTransformer( pTransformArg, FALSE, nSamplePoints, 
                         adfX, adfY, adfZ, abSuccess ) )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "GDALSuggestedWarpOutput() failed because the passed\n"
                  "transformer failed." );
        return CE_Failure;
    }
        
/* -------------------------------------------------------------------- */
/*      Collect the bounds, ignoring any failed points.                 */
/* -------------------------------------------------------------------- */
    double dfMinXOut=0, dfMinYOut=0, dfMaxXOut=0, dfMaxYOut=0;
    int    bGotInitialPoint = FALSE;
    int    nFailedCount = 0, i;

    for( i = 0; i < nSamplePoints; i++ )
    {
        if( !abSuccess[i] )
        {
            nFailedCount++;
            continue;
        }

        if( !bGotInitialPoint )
        {
            bGotInitialPoint = TRUE;
            dfMinXOut = dfMaxXOut = adfX[i];
            dfMinYOut = dfMaxYOut = adfY[i];
        }
        else
        {
            dfMinXOut = MIN(dfMinXOut,adfX[i]);
            dfMinYOut = MIN(dfMinYOut,adfY[i]);
            dfMaxXOut = MAX(dfMaxXOut,adfX[i]);
            dfMaxYOut = MAX(dfMaxYOut,adfY[i]);
        }
    }

    if( nFailedCount > nSamplePoints - 10 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Too many points (%d out of %d) failed to transform,\n"
                  "unable to compute output bounds.",
                  nFailedCount, nSamplePoints );
        return CE_Failure;
    }

    if( nFailedCount > 0 )
        CPLDebug( "GDAL", 
                  "GDALSuggestedWarpOutput(): %d out of %d points failed to transform.", 
                  nFailedCount, nSamplePoints );

/* -------------------------------------------------------------------- */
/*      Compute the distance in "georeferenced" units from the top      */
/*      corner of the transformed input image to the bottom left        */
/*      corner of the transformed input.  Use this distance to          */
/*      compute an approximate pixel size in the output                 */
/*      georeferenced coordinates.                                      */
/* -------------------------------------------------------------------- */
    double dfDiagonalDist, dfDeltaX, dfDeltaY;
    
    dfDeltaX = adfX[nSamplePoints-1] - adfX[0];
    dfDeltaY = adfY[nSamplePoints-1] - adfY[0];

    dfDiagonalDist = sqrt( dfDeltaX * dfDeltaX + dfDeltaY * dfDeltaY );
    
/* -------------------------------------------------------------------- */
/*      Compute a pixel size from this.                                 */
/* -------------------------------------------------------------------- */
    double dfPixelSize;

    dfPixelSize = dfDiagonalDist 
        / sqrt(((double)nInXSize)*nInXSize + ((double)nInYSize)*nInYSize);

    *pnPixels = (int) ((dfMaxXOut - dfMinXOut) / dfPixelSize + 0.5);
    *pnLines = (int) ((dfMaxYOut - dfMinYOut) / dfPixelSize + 0.5);

/* -------------------------------------------------------------------- */
/*      Set the output geotransform.                                    */
/* -------------------------------------------------------------------- */
    padfGeoTransformOut[0] = dfMinXOut;
    padfGeoTransformOut[1] = dfPixelSize;
    padfGeoTransformOut[2] = 0.0;
    padfGeoTransformOut[3] = dfMaxYOut;
    padfGeoTransformOut[4] = 0.0;
    padfGeoTransformOut[5] = - dfPixelSize;
    
    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*			 GDALGenImgProjTransformer                      */
/* ==================================================================== */
/************************************************************************/

typedef struct {

    double   adfSrcGeoTransform[6];
    double   adfSrcInvGeoTransform[6];

    void     *pSrcGCPTransformArg;
    void     *pSrcRPCTransformArg;

    void     *pReprojectArg;

    double   adfDstGeoTransform[6];
    double   adfDstInvGeoTransform[6];
    
    void     *pDstGCPTransformArg;

} GDALGenImgProjTransformInfo;

/************************************************************************/
/*                  GDALCreateGenImgProjTransformer()                   */
/************************************************************************/

/**
 * Create image to image transformer.
 *
 * This function creates a transformation object that maps from pixel/line
 * coordinates on one image to pixel/line coordinates on another image.  The
 * images may potentially be georeferenced in different coordinate systems, 
 * and may used GCPs to map between their pixel/line coordinates and 
 * georeferenced coordinates (as opposed to the default assumption that their
 * geotransform should be used). 
 *
 * This transformer potentially performs three concatenated transformations.
 *
 * The first stage is from source image pixel/line coordinates to source
 * image georeferenced coordinates, and may be done using the geotransform, 
 * or if not defined using a polynomial model derived from GCPs.  If GCPs
 * are used this stage is accomplished using GDALGCPTransform(). 
 *
 * The second stage is to change projections from the source coordinate system
 * to the destination coordinate system, assuming they differ.  This is 
 * accomplished internally using GDALReprojectionTransform().
 *
 * The third stage is converting from destination image georeferenced
 * coordinates to destination image coordinates.  This is done using the
 * destination image geotransform, or if not available, using a polynomial 
 * model derived from GCPs. If GCPs are used this stage is accomplished using 
 * GDALGCPTransform().  This stage is skipped if hDstDS is NULL when the
 * transformation is created. 
 * 
 * @param hSrcDS source dataset.  
 * @param pszSrcWKT the coordinate system for the source dataset.  If NULL, 
 * it will be read from the dataset itself. 
 * @param hDstDS destination dataset (or NULL). 
 * @param pszDstWKT the coordinate system for the destination dataset.  If
 * NULL, and hDstDS not NULL, it will be read from the destination dataset.
 * @param bGCPUseOK TRUE if GCPs should be used if the geotransform is not
 * available. 
 * @param dfGCPErrorThreshold the maximum error allowed for the GCP model
 * to be considered valid.  Exact semantics not yet defined. 
 * @param nOrder the maximum order to use for GCP derived polynomials if 
 * possible. 
 * 
 * @return handle suitable for use GDALGenImgProjTransform(), and to be
 * deallocated with GDALDestroyGenImgProjTransformer().
 */

void *
GDALCreateGenImgProjTransformer( GDALDatasetH hSrcDS, const char *pszSrcWKT,
                                 GDALDatasetH hDstDS, const char *pszDstWKT,
                                 int bGCPUseOK, double dfGCPErrorThreshold,
                                 int nOrder )

{
    GDALGenImgProjTransformInfo *psInfo;
    char **papszMD;
    GDALRPCInfo sRPCInfo;

/* -------------------------------------------------------------------- */
/*      Initialize the transform info.                                  */
/* -------------------------------------------------------------------- */
    psInfo = (GDALGenImgProjTransformInfo *) 
        CPLCalloc(sizeof(GDALGenImgProjTransformInfo),1);

/* -------------------------------------------------------------------- */
/*      Get forward and inverse geotransform for the source image.      */
/* -------------------------------------------------------------------- */
    if( GDALGetGeoTransform( hSrcDS, psInfo->adfSrcGeoTransform ) == CE_None
        && (psInfo->adfSrcGeoTransform[0] != 0.0
            || psInfo->adfSrcGeoTransform[1] != 1.0
            || psInfo->adfSrcGeoTransform[2] != 0.0
            || psInfo->adfSrcGeoTransform[3] != 0.0
            || psInfo->adfSrcGeoTransform[4] != 0.0
            || ABS(psInfo->adfSrcGeoTransform[5]) != 1.0) )
    {
        GDALInvGeoTransform( psInfo->adfSrcGeoTransform, 
                             psInfo->adfSrcInvGeoTransform );
    }

    else if( bGCPUseOK && GDALGetGCPCount( hSrcDS ) > 0 )
    {
        psInfo->pSrcGCPTransformArg = 
            GDALCreateGCPTransformer( GDALGetGCPCount( hSrcDS ),
                                      GDALGetGCPs( hSrcDS ), nOrder, FALSE );
        if( psInfo->pSrcGCPTransformArg == NULL )
        {
            GDALDestroyGenImgProjTransformer( psInfo );
            return NULL;
        }
    }

    else if( bGCPUseOK && (papszMD = GDALGetMetadata( hSrcDS, "" )) != NULL
             && GDALExtractRPCInfo( papszMD, &sRPCInfo ) )
    {
        psInfo->pSrcRPCTransformArg = 
            GDALCreateRPCTransformer( &sRPCInfo, FALSE, 0.1 );
        if( psInfo->pSrcRPCTransformArg == NULL )
        {
            GDALDestroyGenImgProjTransformer( psInfo );
            return NULL;
        }
    }

    else
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Unable to compute a transformation between pixel/line\n"
                  "and georeferenced coordinates for %s.\n"
                  "There is no affine transformation and no GCPs.", 
                  GDALGetDescription( hSrcDS ) );

        GDALDestroyGenImgProjTransformer( psInfo );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Setup reprojection.                                             */
/* -------------------------------------------------------------------- */
    if( pszSrcWKT != NULL && pszDstWKT != NULL
        && !EQUAL(pszSrcWKT,pszDstWKT) )
    {
        psInfo->pReprojectArg = 
            GDALCreateReprojectionTransformer( pszSrcWKT, pszDstWKT );
    }
        
/* -------------------------------------------------------------------- */
/*      Get forward and inverse geotransform for destination image.     */
/*      If we have no destination use a unit transform.                 */
/* -------------------------------------------------------------------- */
    if( hDstDS )
    {
        GDALGetGeoTransform( hDstDS, psInfo->adfDstGeoTransform );
        GDALInvGeoTransform( psInfo->adfDstGeoTransform, 
                             psInfo->adfDstInvGeoTransform );
    }
    else
    {
        psInfo->adfDstGeoTransform[0] = 0.0;
        psInfo->adfDstGeoTransform[1] = 1.0;
        psInfo->adfDstGeoTransform[2] = 0.0;
        psInfo->adfDstGeoTransform[3] = 0.0;
        psInfo->adfDstGeoTransform[4] = 0.0;
        psInfo->adfDstGeoTransform[5] = 1.0;
        memcpy( psInfo->adfDstInvGeoTransform, psInfo->adfDstGeoTransform,
                sizeof(double) * 6 );
    }
    
    return psInfo;
}
    

/************************************************************************/
/*                  GDALDestroyGenImgProjTransformer()                  */
/************************************************************************/

/**
 * GenImgProjTransformer deallocator.
 *
 * This function is used to deallocate the handle created with
 * GDALCreateGenImgProjTransformer().
 *
 * @param hTransformArg the handle to deallocate. 
 */

void GDALDestroyGenImgProjTransformer( void *hTransformArg )

{
    GDALGenImgProjTransformInfo *psInfo = 
        (GDALGenImgProjTransformInfo *) hTransformArg;

    if( psInfo->pSrcGCPTransformArg != NULL )
        GDALDestroyGCPTransformer( psInfo->pSrcGCPTransformArg );

    if( psInfo->pDstGCPTransformArg != NULL )
        GDALDestroyGCPTransformer( psInfo->pDstGCPTransformArg );

    if( psInfo->pReprojectArg != NULL )
        GDALDestroyReprojectionTransformer( psInfo->pReprojectArg );

    CPLFree( psInfo );
}

/************************************************************************/
/*                      GDALGenImgProjTransform()                       */
/************************************************************************/

/**
 * Perform general image reprojection transformation.
 *
 * Actually performs the transformation setup in 
 * GDALCreateGenImgProjTransformer().  This function matches the signature
 * required by the GDALTransformerFunc(), and more details on the arguments
 * can be found in that topic. 
 */

int GDALGenImgProjTransform( void *pTransformArg, int bDstToSrc, 
                             int nPointCount, 
                             double *padfX, double *padfY, double *padfZ,
                             int *panSuccess )
{
    GDALGenImgProjTransformInfo *psInfo = 
        (GDALGenImgProjTransformInfo *) pTransformArg;
    int   i;
    double *padfGeoTransform;
    void *pGCPTransformArg;
    void *pRPCTransformArg;

/* -------------------------------------------------------------------- */
/*      Convert from src (dst) pixel/line to src (dst)                  */
/*      georeferenced coordinates.                                      */
/* -------------------------------------------------------------------- */
    if( bDstToSrc )
    {
        padfGeoTransform = psInfo->adfDstGeoTransform;
        pGCPTransformArg = psInfo->pDstGCPTransformArg;
        pRPCTransformArg = NULL;
    }
    else
    {
        padfGeoTransform = psInfo->adfSrcGeoTransform;
        pGCPTransformArg = psInfo->pSrcGCPTransformArg;
        pRPCTransformArg = psInfo->pSrcRPCTransformArg;
    }

    if( pGCPTransformArg == NULL && pRPCTransformArg == NULL )
    {
        for( i = 0; i < nPointCount; i++ )
        {
            double dfNewX, dfNewY;
            
            dfNewX = padfGeoTransform[0]
                + padfX[i] * padfGeoTransform[1]
                + padfY[i] * padfGeoTransform[2];
            dfNewY = padfGeoTransform[3]
                + padfX[i] * padfGeoTransform[4]
                + padfY[i] * padfGeoTransform[5];
            
            padfX[i] = dfNewX;
            padfY[i] = dfNewY;
        }
    }
    else if( pGCPTransformArg != NULL )
    {
        if( !GDALGCPTransform( pGCPTransformArg, FALSE, 
                               nPointCount, padfX, padfY, padfZ,
                               panSuccess ) )
            return FALSE;
    }
    else if( pRPCTransformArg != NULL )
    {
        if( !GDALRPCTransform( pRPCTransformArg, FALSE, 
                               nPointCount, padfX, padfY, padfZ,
                               panSuccess ) )
            return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Reproject if needed.                                            */
/* -------------------------------------------------------------------- */
    if( psInfo->pReprojectArg )
    {
        if( !GDALReprojectionTransform( psInfo->pReprojectArg, bDstToSrc, 
                                        nPointCount, padfX, padfY, padfZ,
                                        panSuccess ) )
            return FALSE;
    }
    else
    {
        for( i = 0; i < nPointCount; i++ )
            panSuccess[i] = 1;
    }

/* -------------------------------------------------------------------- */
/*      Convert dst (src) georef coordinates back to pixel/line.        */
/* -------------------------------------------------------------------- */
    if( bDstToSrc )
    {
        padfGeoTransform = psInfo->adfSrcInvGeoTransform;
        pGCPTransformArg = psInfo->pSrcGCPTransformArg;
        pRPCTransformArg = psInfo->pSrcRPCTransformArg;
    }
    else
    {
        padfGeoTransform = psInfo->adfDstInvGeoTransform;
        pGCPTransformArg = psInfo->pDstGCPTransformArg;
        pRPCTransformArg = NULL;
    }
        
    if( pGCPTransformArg == NULL && pRPCTransformArg == NULL )
    {
        for( i = 0; i < nPointCount; i++ )
        {
            double dfNewX, dfNewY;
            
            dfNewX = padfGeoTransform[0]
                + padfX[i] * padfGeoTransform[1]
                + padfY[i] * padfGeoTransform[2];
            dfNewY = padfGeoTransform[3]
                + padfX[i] * padfGeoTransform[4]
                + padfY[i] * padfGeoTransform[5];
            
            padfX[i] = dfNewX;
            padfY[i] = dfNewY;
        }
    }
    else if( pGCPTransformArg != NULL )
    {
        if( !GDALGCPTransform( pGCPTransformArg, TRUE,
                               nPointCount, padfX, padfY, padfZ,
                               panSuccess ) )
            return FALSE;
    }
    else if( pRPCTransformArg != NULL )
    {
        if( !GDALRPCTransform( pRPCTransformArg, TRUE,
                               nPointCount, padfX, padfY, padfZ,
                               panSuccess ) )
            return FALSE;
    }
        
    return TRUE;
}


/************************************************************************/
/* ==================================================================== */
/*			 GDALReprojectionTransformer                    */
/* ==================================================================== */
/************************************************************************/

typedef struct {
    OGRCoordinateTransformation *poForwardTransform;
    OGRCoordinateTransformation *poReverseTransform;
} GDALReprojectionTransformInfo;

/************************************************************************/
/*                 GDALCreateReprojectionTransformer()                  */
/************************************************************************/

/**
 * Create reprojection transformer.
 *
 * Creates a callback data structure suitable for use with 
 * GDALReprojectionTransformation() to represent a transformation from
 * one geographic or projected coordinate system to another.  On input
 * the coordinate systems are described in OpenGIS WKT format. 
 *
 * Internally the OGRCoordinateTransformation object is used to implement
 * the reprojection.
 *
 * @param pszSrcWKT the coordinate system for the source coordinate system.
 * @param pszDstWKT the coordinate system for the destination coordinate 
 * system.
 *
 * @return Handle for use with GDALReprojectionTransform(), or NULL if the 
 * system fails to initialize the reprojection. 
 **/

void *GDALCreateReprojectionTransformer( const char *pszSrcWKT, 
                                         const char *pszDstWKT )

{
    OGRSpatialReference oSrcSRS, oDstSRS;
    OGRCoordinateTransformation *poForwardTransform;

/* -------------------------------------------------------------------- */
/*      Ingest the SRS definitions.                                     */
/* -------------------------------------------------------------------- */
    if( oSrcSRS.importFromWkt( (char **) &pszSrcWKT ) != OGRERR_NONE )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Failed to import coordinate system `%s'.", 
                  pszSrcWKT );
        return NULL;
    }
    if( oDstSRS.importFromWkt( (char **) &pszDstWKT ) != OGRERR_NONE )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Failed to import coordinate system `%s'.", 
                  pszSrcWKT );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Build the forward coordinate transformation.                    */
/* -------------------------------------------------------------------- */
    poForwardTransform = OGRCreateCoordinateTransformation(&oSrcSRS,&oDstSRS);

    if( poForwardTransform == NULL )
        // OGRCreateCoordinateTransformation() will report errors on its own.
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create a structure to hold the transform info, and also         */
/*      build reverse transform.  We assume that if the forward         */
/*      transform can be created, then so can the reverse one.          */
/* -------------------------------------------------------------------- */
    GDALReprojectionTransformInfo *psInfo;

    psInfo = (GDALReprojectionTransformInfo *) 
        CPLCalloc(sizeof(GDALReprojectionTransformInfo),1);

    psInfo->poForwardTransform = poForwardTransform;
    psInfo->poReverseTransform = 
        OGRCreateCoordinateTransformation(&oDstSRS,&oSrcSRS);

    return psInfo;
}

/************************************************************************/
/*                 GDALDestroyReprojectionTransformer()                 */
/************************************************************************/

/**
 * Destroy reprojection transformation.
 *
 * @param pTransformArg the transformation handle returned by
 * GDALCreateReprojectionTransformer().
 */

void GDALDestroyReprojectionTransformer( void *pTransformAlg )

{
    GDALReprojectionTransformInfo *psInfo = 
        (GDALReprojectionTransformInfo *) pTransformAlg;		

    if( psInfo->poForwardTransform )
        delete psInfo->poForwardTransform;

    if( psInfo->poReverseTransform )
    delete psInfo->poReverseTransform;

    CPLFree( psInfo );
}

/************************************************************************/
/*                     GDALReprojectionTransform()                      */
/************************************************************************/

/**
 * Perform reprojection transformation.
 *
 * Actually performs the reprojection transformation described in 
 * GDALCreateReprojectionTransformer().  This function matches the 
 * GDALTransformerFunc() signature.  Details of the arguments are described
 * there. 
 */

int GDALReprojectionTransform( void *pTransformArg, int bDstToSrc, 
                                int nPointCount, 
                                double *padfX, double *padfY, double *padfZ,
                                int *panSuccess )

{
    GDALReprojectionTransformInfo *psInfo = 
        (GDALReprojectionTransformInfo *) pTransformArg;		
    int bSuccess;

    if( bDstToSrc )
        bSuccess = psInfo->poReverseTransform->Transform( 
            nPointCount, padfX, padfY, padfZ );
    else
        bSuccess = psInfo->poForwardTransform->Transform( 
            nPointCount, padfX, padfY, padfZ );

    if( bSuccess )
        memset( panSuccess, 1, sizeof(int) * nPointCount );
    else
        memset( panSuccess, 0, sizeof(int) * nPointCount );

    return bSuccess;
}


/************************************************************************/
/* ==================================================================== */
/*      Approximate transformer.                                        */
/* ==================================================================== */
/************************************************************************/

typedef struct 
{
    GDALTransformerFunc pfnBaseTransformer;
    void             *pBaseCBData;
    double	      dfMaxError;
} ApproxTransformInfo;

/************************************************************************/
/*                    GDALCreateApproxTransformer()                     */
/************************************************************************/

/**
 * Create an approximating transformer.
 *
 * This function creates a context for an approximated transformer.  Basically
 * a high precision transformer is supplied as input and internally linear
 * approximations are computed to generate results to within a defined
 * precision. 
 *
 * The approximation is actually done at the point where GDALApproxTransform()
 * calls are made, and depend on the assumption that the roughly linear.  The
 * first and last point passed in must be the extreme values and the 
 * intermediate values should describe a curve between the end points.  The
 * approximator transforms and center using the approximate transformer, and
 * then compares the true middle transformed value to a linear approximation
 * based on the end points.  If the error is within the supplied threshold
 * then the end points are used to linearly approximate all the values 
 * otherwise the inputs points are split into two smaller sets, and the
 * function recursively called till a sufficiently small set of points if found
 * that the linear approximation is OK, or that all the points are exactly
 * computed. 
 *
 * This function is very suitable for approximating transformation results
 * from output pixel/line space to input coordinates for warpers that operate
 * on one input scanline at a time.  Care should be taken using it in other
 * circumstances as little internal validation is done, in order to keep things
 * fast. 
 *
 * @param pfnBaseTransformer the high precision transformer which should be
 * approximated. 
 * @param pBaseTransformArg the callback argument for the high precision 
 * transformer. 
 * @param dfMaxError the maximum cartesian error in the "output" space that
 * is to be accepted in the linear approximation.
 * 
 * @return callback pointer suitable for use with GDALApproxTransform().  It
 * should be deallocated with GDALDestroyApproxTransformer().
 */

void *GDALCreateApproxTransformer( GDALTransformerFunc pfnBaseTransformer,
                                   void *pBaseTransformArg, double dfMaxError)

{
    ApproxTransformInfo	*psATInfo;

    psATInfo = (ApproxTransformInfo*) CPLMalloc(sizeof(ApproxTransformInfo));
    psATInfo->pfnBaseTransformer = pfnBaseTransformer;
    psATInfo->pBaseCBData = pBaseTransformArg;
    psATInfo->dfMaxError = dfMaxError;

    return psATInfo;
}

/************************************************************************/
/*                    GDALDestroyApproxTransformer()                    */
/************************************************************************/

/**
 * Cleanup approximate transformer.
 *
 * Deallocates the resources allocated by GDALCreateApproxTransformer().
 * 
 * @param pCBData callback data originally returned by 
 * GDALCreateApproxTransformer().
 */

void GDALDestroyApproxTransformer( void * pCBData )

{
    CPLFree( pCBData );
}

/************************************************************************/
/*                        GDALApproxTransform()                         */
/************************************************************************/

/**
 * Perform approximate transformation.
 *
 * Actually performs the approximate transformation described in 
 * GDALCreateApproxTransformer().  This function matches the 
 * GDALTransformerFunc() signature.  Details of the arguments are described
 * there. 
 */

int GDALApproxTransform( void *pCBData, int bDstToSrc, int nPoints, 
                         double *x, double *y, double *z, int *panSuccess )

{
    ApproxTransformInfo *psATInfo = (ApproxTransformInfo *) pCBData;
    double x2[3], y2[3], z2[3], dfDeltaX, dfDeltaY, dfError, dfDist, dfDeltaZ;
    int nMiddle, anSuccess2[3], i, bSuccess;

    nMiddle = (nPoints-1)/2;

/* -------------------------------------------------------------------- */
/*      Bail if our preconditions are not met, or if error is not       */
/*      acceptable.                                                     */
/* -------------------------------------------------------------------- */
    if( y[0] != y[nPoints-1] || y[0] != y[nMiddle]
        || x[0] == x[nPoints-1] || x[0] == x[nMiddle]
        || psATInfo->dfMaxError == 0.0 || nPoints <= 5 )
    {
        return psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, bDstToSrc,
                                             nPoints, x, y, z, panSuccess );
    }

/* -------------------------------------------------------------------- */
/*      Transform first, last and middle point.                         */
/* -------------------------------------------------------------------- */
    x2[0] = x[0];
    y2[0] = y[0];
    z2[0] = z[0];
    x2[1] = x[nMiddle];
    y2[1] = y[nMiddle];
    z2[1] = z[nMiddle];
    x2[2] = x[nPoints-1];
    y2[2] = y[nPoints-1];
    z2[2] = z[nPoints-1];

    bSuccess = 
        psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, bDstToSrc, 3, 
                                      x2, y2, z2, anSuccess2 );
    if( !bSuccess )
        return psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, bDstToSrc,
                                             nPoints, x, y, z, panSuccess );
    
/* -------------------------------------------------------------------- */
/*      Is the error at the middle acceptable relative to an            */
/*      interpolation of the middle position?                           */
/* -------------------------------------------------------------------- */
    dfDeltaX = (x2[2] - x2[0]) / (x[nPoints-1] - x[0]);
    dfDeltaY = (y2[2] - y2[0]) / (x[nPoints-1] - x[0]);
    dfDeltaZ = (z2[2] - z2[0]) / (x[nPoints-1] - x[0]);

    dfError = fabs((x2[0] + dfDeltaX * (x[nMiddle] - x[0])) - x2[1])
        + fabs((y2[0] + dfDeltaY * (x[nMiddle] - x[0])) - y2[1]);

    if( dfError > psATInfo->dfMaxError )
    {
#ifdef notdef
        CPLDebug( "GDAL", "ApproxTransformer - "
                  "error %g over threshold %g, subdivide %d points.",
                  dfError, psATInfo->dfMaxError, nPoints );
#endif

        bSuccess = 
            GDALApproxTransform( psATInfo, bDstToSrc, nMiddle, 
                                 x, y, z, panSuccess );
            
        if( !bSuccess )
            return FALSE;

        bSuccess = 
            GDALApproxTransform( psATInfo, bDstToSrc, nPoints - nMiddle,
                                 x+nMiddle, y+nMiddle, z+nMiddle,
                                 panSuccess+nMiddle );

        if( !bSuccess )
            return FALSE;

        return TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Error is OK since this is just used to compute output bounds    */
/*      of newly created file for gdalwarper.  So just use affine       */
/*      approximation of the reverse transform.  Eventually we          */
/*      should implement iterative searching to find a result within    */
/*      our error threshold.                                            */
/* -------------------------------------------------------------------- */
    for( i = nPoints-1; i >= 0; i-- )
    {
        dfDist = (x[i] - x[0]);
        y[i] = y2[0] + dfDeltaY * dfDist;
        x[i] = x2[0] + dfDeltaX * dfDist;
        z[i] = z2[0] + dfDeltaZ * dfDist;
        panSuccess[i] = TRUE;
    }
    
    return TRUE;
}

