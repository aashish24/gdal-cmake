/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Base class for raster file formats.  
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1998, 2003, Frank Warmerdam
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
 * Revision 1.39  2003/05/14 08:53:10  dron
 * Fixed mistake in RasterIO() description.
 *
 * Revision 1.38  2003/05/08 14:38:40  warmerda
 * added BlockBasedFlushCache
 *
 * Revision 1.37  2003/04/30 17:13:48  warmerda
 * added docs for many C functions
 *
 * Revision 1.36  2003/04/25 19:49:51  warmerda
 * first crack at RasterIO implementation
 *
 * Revision 1.35  2003/03/18 05:58:52  warmerda
 * Added GDALFlushCache().
 *
 * Revision 1.34  2003/02/13 16:18:58  warmerda
 * Fixed typo.
 *
 * Revision 1.33  2003/02/13 16:10:23  warmerda
 * Cleaned up GetProjectionRef() info.
 *
 * Revision 1.32  2003/01/28 16:54:49  warmerda
 * indicate required include file
 *
 * Revision 1.31  2003/01/28 16:07:31  warmerda
 * improved documentation
 *
 * Revision 1.30  2002/12/18 15:17:05  warmerda
 * added errors in some unimplemented methods
 *
 * Revision 1.29  2002/09/06 01:29:55  warmerda
 * added C entry points for GetAccess() and GetOpenDatasets()
 *
 * Revision 1.28  2002/07/09 20:33:12  warmerda
 * expand tabs
 *
 * Revision 1.27  2002/05/28 18:56:22  warmerda
 * added shared dataset concept
 *
 * Revision 1.26  2001/11/16 21:36:01  warmerda
 * added the AddBand() method on GDALDataset
 *
 * Revision 1.25  2001/10/18 14:35:22  warmerda
 * avoid conflicts between parameters and member data
 *
 * Revision 1.24  2001/10/17 21:47:02  warmerda
 * added SetGCPs() on GDALDataset
 *
 * Revision 1.23  2001/07/18 04:04:30  warmerda
 * added CPL_CVSID
 *
 * Revision 1.22  2001/01/10 22:24:37  warmerda
 * Patched GDALDataset::FlushCache() to recover gracefully if papoBands
 * doesn't exist yet matching nBands.
 *
 * Revision 1.21  2000/10/06 15:27:13  warmerda
 * default bands to same access as dataset in SetBand()
 *
 * Revision 1.20  2000/08/09 16:26:00  warmerda
 * debug message on dataset cleanup
 *
 * Revision 1.19  2000/07/11 14:35:43  warmerda
 * added documentation
 *
 * Revision 1.18  2000/06/27 16:46:56  warmerda
 * default to using dummy progress func
 *
 * Revision 1.17  2000/06/26 21:44:50  warmerda
 * make progress func save for overviews
 *
 * Revision 1.16  2000/06/26 18:47:31  warmerda
 * added GDALBuildOverviews
 *
 * Revision 1.15  2000/04/21 21:56:23  warmerda
 * move metadata to GDALMajorObject, added BuildOverviews
 *
 * Revision 1.14  2000/03/31 13:42:06  warmerda
 * added gcp support methods
 *
 * Revision 1.13  2000/03/23 16:53:55  warmerda
 * default geotransform is 0,1,0,0,0,1
 *
 * Revision 1.12  2000/03/06 21:50:10  warmerda
 * fixed bug with setting nBands
 *
 * Revision 1.11  2000/03/06 02:20:56  warmerda
 * added reference counting
 *
 * Revision 1.10  2000/02/28 16:34:49  warmerda
 * set the nRasterX/YSize in bands
 */

#include "gdal_priv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

static int nGDALDatasetCount = 0;
static GDALDataset **papoGDALDatasetList = NULL;

/************************************************************************/
/* ==================================================================== */
/*                             GDALDataset                              */
/* ==================================================================== */
/************************************************************************/

/**
 * \class GDALDataset "gdal_priv.h"
 *
 * A dataset encapsulating one or more raster bands.  Details are
 * further discussed in the <a href="gdal_datamodel.html#GDALDataset">GDAL
 * Data Model</a>.
 *
 * Use GDALOpen() or GDALOpenShared() to create a GDALDataset for a named file,
 * or GDALDriver::Create() or GDALDriver::CreateCopy() to create a new 
 * dataset.
 */

/************************************************************************/
/*                            GDALDataset()                             */
/************************************************************************/

GDALDataset::GDALDataset()

{
    poDriver = NULL;
    eAccess = GA_ReadOnly;
    nRasterXSize = 512;
    nRasterYSize = 512;
    nBands = 0;
    papoBands = NULL;
    nRefCount = 1;
    bShared = FALSE;

/* -------------------------------------------------------------------- */
/*      Add this dataset to the open dataset list.                      */
/* -------------------------------------------------------------------- */
    nGDALDatasetCount++;
    papoGDALDatasetList = (GDALDataset **) 
        CPLRealloc( papoGDALDatasetList, sizeof(void *) * nGDALDatasetCount );
    papoGDALDatasetList[nGDALDatasetCount-1] = this;
}

/************************************************************************/
/*                            ~GDALDataset()                            */
/************************************************************************/

/**
 * Destroy an open GDALDataset.
 *
 * This is the accepted method of closing a GDAL dataset and deallocating
 * all resources associated with it.
 *
 * Equivelent of the C callable GDALClose().  Except that GDALClose() first
 * decrements the reference count, and then closes only if it has dropped to
 * zero.
 */

GDALDataset::~GDALDataset()

{
    int         i;

    CPLDebug( "GDAL", "GDALClose(%s)\n", GetDescription() );

/* -------------------------------------------------------------------- */
/*      Remove dataset from the "open" dataset list.                    */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nGDALDatasetCount; i++ )
    {
        if( papoGDALDatasetList[i] == this )
        {
            papoGDALDatasetList[i] = papoGDALDatasetList[nGDALDatasetCount-1];
            nGDALDatasetCount--;
            if( nGDALDatasetCount == 0 )
            {
                CPLFree( papoGDALDatasetList );
                papoGDALDatasetList = NULL;
            }
            break;
        }
    }

/* -------------------------------------------------------------------- */
/*      Destroy the raster bands if they exist.                         */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nBands && papoBands != NULL; i++ )
    {
        if( papoBands[i] != NULL )
            delete papoBands[i];
    }

    CPLFree( papoBands );
}

/************************************************************************/
/*                             FlushCache()                             */
/************************************************************************/

/**
 * Flush all write cached data to disk.
 *
 * Any raster (or other GDAL) data written via GDAL calls, but buffered
 * internally will be written to disk.
 *
 * This method is the same as the C function GDALFlushCache().
 */

void GDALDataset::FlushCache()

{
    int         i;

    // This sometimes happens if a dataset is destroyed before completely
    // built. 

    if( papoBands == NULL )
        return;

    for( i = 0; i < nBands; i++ )
    {
        if( papoBands[i] != NULL )
            papoBands[i]->FlushCache();
    }
}

/************************************************************************/
/*                           GDALFlushCache()                           */
/************************************************************************/

/**
 * @see GDALDataset::FlushCache().
 */

void GDALFlushCache( GDALDatasetH hDS )

{
    ((GDALDataset *) hDS)->FlushCache();
}

/************************************************************************/
/*                        BlockBasedFlushCache()                        */
/*                                                                      */
/*      This helper method can be called by the                         */
/*      GDALDataset::FlushCache() for particular drivers to ensure      */
/*      that buffers will be flushed in a manner suitable for pixel     */
/*      interleaved (by block) IO.  That is, if all the bands have      */
/*      the same size blocks then a given block will be flushed for     */
/*      all bands before proceeding to the next block.                  */
/************************************************************************/

void GDALDataset::BlockBasedFlushCache()

{
    GDALRasterBand *poBand1, *poBand;
    int  nBlockXSize, nBlockYSize, iBand;
    
    poBand1 = GetRasterBand( 1 );
    if( poBand1 == NULL )
    {
        GDALDataset::FlushCache();
        return;
    }

    poBand1->GetBlockSize( &nBlockXSize, &nBlockYSize );
    
/* -------------------------------------------------------------------- */
/*      Verify that all bands match.                                    */
/* -------------------------------------------------------------------- */
    for( iBand = 1; iBand < nBands; iBand++ )
    {
        int nThisBlockXSize, nThisBlockYSize;
        GDALRasterBand *poBand = GetRasterBand( iBand+1 );
        
        poBand->GetBlockSize( &nThisBlockXSize, &nThisBlockYSize );
        if( nThisBlockXSize != nBlockXSize && nThisBlockYSize != nBlockYSize )
        {
            GDALDataset::FlushCache();
            return;
        }
    }

/* -------------------------------------------------------------------- */
/*      Now flush writable data.                                        */
/* -------------------------------------------------------------------- */
    for( int iY = 0; iY < poBand1->nBlocksPerColumn; iY++ )
    {
        for( int iX = 0; iX < poBand1->nBlocksPerRow; iX++ )
        {
            for( iBand = 0; iBand < nBands; iBand++ )
            {
                GDALRasterBand *poBand = GetRasterBand( iBand+1 );
                
                if( poBand->papoBlocks[iX + iY*poBand1->nBlocksPerRow] != NULL)
                {
                    CPLErr    eErr;
                    
                    eErr = poBand->FlushBlock( iX, iY );
                    
                    if( eErr != CE_None )
                        return;
                }
            }
        }
    }
}

/************************************************************************/
/*                          RasterInitialize()                          */
/*                                                                      */
/*      Initialize raster size                                          */
/************************************************************************/

void GDALDataset::RasterInitialize( int nXSize, int nYSize )

{
    CPLAssert( nXSize > 0 && nYSize > 0 );
    
    nRasterXSize = nXSize;
    nRasterYSize = nYSize;
}

/************************************************************************/
/*                              AddBand()                               */
/************************************************************************/

/**
 * Add a band to a dataset.
 *
 * This method will add a new band to the dataset if the underlying format
 * supports this action.  Most formats do not. 
 *
 * Note that the new GDALRasterBand is not returned.  It may be fetched
 * after successful completion of the method by calling 
 * GDALDataset::GetRasterBand(GDALDataset::GetRasterCount()-1) as the newest
 * band will always be the last band.
 *
 * @param eType the data type of the pixels in the new band. 
 *
 * @param papszOptions a list of NAME=VALUE option strings.  The supported
 * options are format specific.  NULL may be passed by default.
 *
 * @return CE_None on success or CE_Failure on failure.  
 */

CPLErr GDALDataset::AddBand( GDALDataType eType, char **papszOptions )

{
    CPLError( CE_Failure, CPLE_NotSupported, 
              "Dataset does not support the AddBand() method." );

    return CE_Failure;
}

/************************************************************************/
/*                            GDALAddBand()                             */
/************************************************************************/

/**
 * @see GDALDataset::AddBand().
 */

CPLErr GDALAddBand( GDALDatasetH hDataset, 
                    GDALDataType eType, char **papszOptions )

{
    return ((GDALDataset *) hDataset)->AddBand( eType, papszOptions );
}

/************************************************************************/
/*                              SetBand()                               */
/*                                                                      */
/*      Set a band in the band array, updating the band count, and      */
/*      array size appropriately.                                       */
/************************************************************************/

void GDALDataset::SetBand( int nNewBand, GDALRasterBand * poBand )

{
/* -------------------------------------------------------------------- */
/*      Do we need to grow the bands list?                              */
/* -------------------------------------------------------------------- */
    if( nBands < nNewBand || papoBands == NULL ) {
        int             i;

        if( papoBands == NULL )
            papoBands = (GDALRasterBand **)
                VSICalloc(sizeof(GDALRasterBand*), MAX(nNewBand,nBands));
        else
            papoBands = (GDALRasterBand **)
                VSIRealloc(papoBands, sizeof(GDALRasterBand*) *
                           MAX(nNewBand,nBands));

        for( i = nBands; i < nNewBand; i++ )
            papoBands[i] = NULL;

        nBands = MAX(nBands,nNewBand);
    }

/* -------------------------------------------------------------------- */
/*      Set the band.  Resetting the band is currently not permitted.   */
/* -------------------------------------------------------------------- */
    CPLAssert( papoBands[nNewBand-1] == NULL );

    papoBands[nNewBand-1] = poBand;

/* -------------------------------------------------------------------- */
/*      Set back reference information on the raster band.  Note        */
/*      that the GDALDataset is a friend of the GDALRasterBand          */
/*      specifically to allow this.                                     */
/* -------------------------------------------------------------------- */
    poBand->nBand = nNewBand;
    poBand->poDS = this;
    poBand->nRasterXSize = nRasterXSize;
    poBand->nRasterYSize = nRasterYSize;
    poBand->eAccess = eAccess; /* default access to be same as dataset */
}

/************************************************************************/
/*                           GetRasterXSize()                           */
/************************************************************************/

/**

 Fetch raster width in pixels.

 Equivelent of the C function GDALGetRasterXSize().

 @return the width in pixels of raster bands in this GDALDataset.

*/

int GDALDataset::GetRasterXSize()

{
    return nRasterXSize;
}

/************************************************************************/
/*                         GDALGetRasterXSize()                         */
/************************************************************************/

/**
 * @see GDALDataset::GetRasterXSize().
 */

int GDALGetRasterXSize( GDALDatasetH hDataset )

{
    return ((GDALDataset *) hDataset)->GetRasterXSize();
}


/************************************************************************/
/*                           GetRasterYSize()                           */
/************************************************************************/

/**

 Fetch raster height in pixels.

 Equivelent of the C function GDALGetRasterYSize().

 @return the height in pixels of raster bands in this GDALDataset.

*/

int GDALDataset::GetRasterYSize()

{
    return nRasterYSize;
}

/************************************************************************/
/*                         GDALGetRasterYSize()                         */
/************************************************************************/

/**
 * @see GDALDataset::GetRasterYSize().
 */

int GDALGetRasterYSize( GDALDatasetH hDataset )

{
    return ((GDALDataset *) hDataset)->GetRasterYSize();
}

/************************************************************************/
/*                           GetRasterBand()                            */
/************************************************************************/

/**

 Fetch a band object for a dataset.

 Equivelent of the C function GDALGetRasterBand().

 @param nBandId the index number of the band to fetch, from 1 to
                GetRasterCount().

 @return the height in pixels of raster bands in this GDALDataset.

*/

GDALRasterBand * GDALDataset::GetRasterBand( int nBandId )

{
    if( nBandId < 1 || nBandId > nBands )
    {
        CPLError( CE_Fatal, CPLE_IllegalArg,
                  "GDALDataset::GetRasterBand(%d) - Illegal band #\n",
                  nBandId );
    }

    return( papoBands[nBandId-1] );
}

/************************************************************************/
/*                         GDALGetRasterBand()                          */
/************************************************************************/

/**
 * @see GDALDataset::GetRasterBand().
 */

GDALRasterBandH GDALGetRasterBand( GDALDatasetH hDS, int nBandId )

{
    return( (GDALRasterBandH) ((GDALDataset *) hDS)->GetRasterBand(nBandId) );
}

/************************************************************************/
/*                           GetRasterCount()                           */
/************************************************************************/

/**
 * Fetch the number of raster bands on this dataset.
 *
 * Same as the C function GDALGetRasterCount().
 *
 * @return the number of raster bands.
 */

int GDALDataset::GetRasterCount()

{
    return( nBands );
}

/************************************************************************/
/*                         GDALGetRasterCount()                         */
/************************************************************************/

/**
 * @see GDALDataset::GetRasterCount().
 */

int GDALGetRasterCount( GDALDatasetH hDS )

{
    return( ((GDALDataset *) hDS)->GetRasterCount() );
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

/**
 * Fetch the projection definition string for this dataset.
 *
 * Same as the C function GDALGetProjectionRef().
 *
 * The returned string defines the projection coordinate system of the
 * image in OpenGIS WKT format.  It should be suitable for use with the 
 * OGRSpatialReference class.
 *
 * When a projection definition is not available an empty (but not NULL)
 * string is returned.
 *
 * @return a pointer to an internal projection reference string.  It should
 * not be altered, freed or expected to last for long. 
 *
 * @see http://gdal.velocet.ca/projects/opengis/ogrhtml/osr_tutorial.html
 */

const char *GDALDataset::GetProjectionRef()

{
    return( "" );
}

/************************************************************************/
/*                        GDALGetProjectionRef()                        */
/************************************************************************/

/**
 * @see GDALDataset::GetProjectionRef()
 */

const char *GDALGetProjectionRef( GDALDatasetH hDS )

{
    return( ((GDALDataset *) hDS)->GetProjectionRef() );
}

/************************************************************************/
/*                           SetProjection()                            */
/************************************************************************/

/**
 * Set the projection reference string for this dataset.
 *
 * The string should be in OGC WKT or PROJ.4 format.  An error may occur
 * because of incorrectly specified projection strings, because the dataset
 * is not writable, or because the dataset does not support the indicated
 * projection.  Many formats do not support writing projections.
 *
 * This method is the same as the C GDALSetProjection() function. 
 *
 * @param pszProjection projection reference string.
 *
 * @return CE_Failure if an error occurs, otherwise CE_None.
 */

CPLErr GDALDataset::SetProjection( const char * )

{
    CPLError( CE_Failure, CPLE_NotSupported, 
              "Dataset does not support the SetProjection() method." );
    return CE_Failure;
}

/************************************************************************/
/*                         GDALSetProjection()                          */
/************************************************************************/

/**
 * @see GDALDataset::SetProjection()
 */

CPLErr GDALSetProjection( GDALDatasetH hDS, const char * pszProjection )

{
    return( ((GDALDataset *) hDS)->SetProjection(pszProjection) );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

/**
 * Fetch the affine transformation coefficients.
 *
 * Fetches the coefficients for transforming between pixel/line (P,L) raster
 * space, and projection coordinates (Xp,Yp) space.
 *
 *  Xp = padfTransform[0] + P*padfTransform[1] + L*padfTransform[2];
 *  Yp = padfTransform[3] + P*padfTransform[4] + L*padfTransform[5];
 *
 * In a north up image, padfTransform[1] is the pixel width, and
 * padfTransform[5] is the pixel height.  The upper left corner of the
 * upper left pixel is at position (padfTransform[0],padfTransform[3]).
 *
 * The default transform is (0,1,0,0,0,1) and should be returned even when
 * a CE_Failure error is returned, such as for formats that don't support
 * transformation to projection coordinates.
 *
 * NOTE: GetGeoTransform() isn't expressive enough to handle the variety of
 * OGC Grid Coverages pixel/line to projection transformation schemes.
 * Eventually this method will be depreciated in favour of a more general
 * scheme.
 *
 * This method does the same thing as the C GDALGetGeoTransform() function.
 *
 * @param padfTransform an existing six double buffer into which the
 * transformation will be placed.
 *
 * @return CE_None on success, or CE_Failure if no transform can be fetched.
 */

CPLErr GDALDataset::GetGeoTransform( double * padfTransform )

{
    CPLAssert( padfTransform != NULL );
        
    padfTransform[0] = 0.0;     /* X Origin (top left corner) */
    padfTransform[1] = 1.0;     /* X Pixel size */
    padfTransform[2] = 0.0;

    padfTransform[3] = 0.0;     /* Y Origin (top left corner) */
    padfTransform[4] = 0.0;     
    padfTransform[5] = 1.0;     /* Y Pixel Size */

    return( CE_Failure );
}

/************************************************************************/
/*                        GDALGetGeoTransform()                         */
/************************************************************************/

/**
 * @see GDALDataset::GetGeoTransform()
 */

CPLErr GDALGetGeoTransform( GDALDatasetH hDS, double * padfTransform )

{
    return( ((GDALDataset *) hDS)->GetGeoTransform(padfTransform) );
}

/************************************************************************/
/*                          SetGeoTransform()                           */
/************************************************************************/

/**
 * Set the affine transformation coefficients.
 *
 * See GetGeoTransform() for details on the meaning of the padfTransform
 * coefficients.
 *
 * This method does the same thing as the C GDALSetGeoTransform() function.
 *
 * @param padfTransform a six double buffer containing the transformation
 * coefficients to be written with the dataset.
 *
 * @return CE_None on success, or CE_Failure if this transform cannot be
 * written.
 */

CPLErr GDALDataset::SetGeoTransform( double * )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "SetGeoTransform() not supported for this dataset." );
    
    return( CE_Failure );
}

/************************************************************************/
/*                        GDALSetGeoTransform()                         */
/************************************************************************/

/**
 * @see GDALDataset::SetGeoTransform()
 */

CPLErr GDALSetGeoTransform( GDALDatasetH hDS, double * padfTransform )

{
    return( ((GDALDataset *) hDS)->SetGeoTransform(padfTransform) );
}

/************************************************************************/
/*                         GetInternalHandle()                          */
/************************************************************************/

/**
 * Fetch a format specific internally meaningful handle.
 *
 * This method is the same as the C GDALGetInternalHandle() method. 
 *
 * @param pszHandleName the handle name desired.  The meaningful names
 * will be specific to the file format.
 *
 * @return the desired handle value, or NULL if not recognised/supported.
 */

void *GDALDataset::GetInternalHandle( const char * )

{
    return( NULL );
}

/************************************************************************/
/*                       GDALGetInternalHandle()                        */
/************************************************************************/

/**
 * @see GDALDataset::GetInternalHandle()
 */

void *GDALGetInternalHandle( GDALDatasetH hDS, const char * pszRequest )

{
    return( ((GDALDataset *) hDS)->GetInternalHandle(pszRequest) );
}

/************************************************************************/
/*                             GetDriver()                              */
/************************************************************************/

/**
 * Fetch the driver to which this dataset relates.
 *
 * This method is the same as the C GDALGetDatasetDriver() function.
 *
 * @return the driver on which the dataset was created with GDALOpen() or
 * GDALCreate().
 */

GDALDriver * GDALDataset::GetDriver()

{
    return poDriver;
}

/************************************************************************/
/*                        GDALGetDatasetDriver()                        */
/************************************************************************/

/**
 * @see GDALDataset::GetDriver()
 */

GDALDriverH GDALGetDatasetDriver( GDALDatasetH hDataset )

{
    return (GDALDriverH) ((GDALDataset *) hDataset)->GetDriver();
}

/************************************************************************/
/*                             Reference()                              */
/************************************************************************/

/**
 * Add one to dataset reference count.
 *
 * The reference is one after instantiation.
 *
 * This method is the same as the C GDALReferenceDataset() function.
 *
 * @return the post-increment reference count.
 */

int GDALDataset::Reference()

{
    return ++nRefCount;
}

/************************************************************************/
/*                        GDALReferenceDataset()                        */
/************************************************************************/

/**
 * @see GDALDataset::Reference()
 */

int GDALReferenceDataset( GDALDatasetH hDataset )

{
    return ((GDALDataset *) hDataset)->Reference();
}

/************************************************************************/
/*                            Dereference()                             */
/************************************************************************/

/**
 * Subtract one from dataset reference count.
 *
 * The reference is one after instantiation.  Generally when the reference
 * count has dropped to zero the dataset may be safely deleted (closed).
 *
 * This method is the same as the C GDALDereferenceDataset() function.
 *
 * @return the post-decrement reference count.
 */

int GDALDataset::Dereference()

{
    return --nRefCount;
}

/************************************************************************/
/*                       GDALDereferenceDataset()                       */
/************************************************************************/

/**
 * @see GDALDataset::Dereference()
 */

int GDALDereferenceDataset( GDALDatasetH hDataset )

{
    return ((GDALDataset *) hDataset)->Dereference();
}

/************************************************************************/
/*                             GetShared()                              */
/************************************************************************/

/**
 * Returns shared flag.
 *
 * @return TRUE if the GDALDataset is available for sharing, or FALSE if not.
 */

int GDALDataset::GetShared()

{
    return bShared;
}

/************************************************************************/
/*                            MarkAsShared()                            */
/************************************************************************/

/**
 * Mark this dataset as available for sharing.
 */

void GDALDataset::MarkAsShared()

{
    CPLAssert( !bShared );

    bShared = TRUE;
}

/************************************************************************/
/*                            GetGCPCount()                             */
/************************************************************************/

/**
 * Get number of GCPs. 
 *
 * This method is the same as the C function GDALGetGCPCount(). 
 *
 * @return number of GCPs for this dataset.  Zero if there are none.
 */

int GDALDataset::GetGCPCount()

{
    return 0;
}

/************************************************************************/
/*                          GDALGetGCPCount()                           */
/************************************************************************/

/**
 * @see GDALDataset::GetGCPCount()
 */

int GDALGetGCPCount( GDALDatasetH hDS )

{
    return ((GDALDataset *) hDS)->GetGCPCount();
}

/************************************************************************/
/*                          GetGCPProjection()                          */
/************************************************************************/

/**
 * Get output projection for GCPs. 
 *
 * This method is the same as the C function GDALGetGCPProjection(). 
 *
 * The projection string follows the normal rules from GetProjectionRef().
 * 
 * @return internal projection string or "" if there are no GCPs. 
 */

const char *GDALDataset::GetGCPProjection()

{
    return "";
}

/************************************************************************/
/*                        GDALGetGCPProjection()                        */
/************************************************************************/

/**
 * @see GDALDataset::GetGCPProjection()
 */

const char *GDALGetGCPProjection( GDALDatasetH hDS )

{
    return ((GDALDataset *) hDS)->GetGCPProjection();
}

/************************************************************************/
/*                               GetGCPs()                              */
/************************************************************************/

/**
 * Fetch GCPs.
 *
 * This method is the same as the C function GDALGetGCPs(). 
 *
 * @return pointer to internal GCP structure list.  It should not be modified, 
 * and may change on the next GDAL call. 
 */ 

const GDAL_GCP *GDALDataset::GetGCPs()

{
    return NULL;
}

/************************************************************************/
/*                            GDALGetGCPs()                             */
/************************************************************************/

/**
 * @see GDALDataset::GetGCPs()
 */

const GDAL_GCP *GDALGetGCPs( GDALDatasetH hDS )

{
    return ((GDALDataset *) hDS)->GetGCPs();
}


/************************************************************************/
/*                              SetGCPs()                               */
/************************************************************************/

/**
 * Assign GCPs.
 *
 * This method is the same as the C function GDALSetGCPs(). 
 *
 * This method assigns the passed set of GCPs to this dataset, as well as
 * setting their coordinate system.  Internally copies are made of the
 * coordinate system and list of points, so the caller remains resposible for
 * deallocating these arguments if appropriate. 
 *
 * Most formats do not support setting of GCPs, even foramts that can 
 * handle GCPs.  These formats will return CE_Failure. 
 *
 * @param nGCPCount number of GCPs being assigned. 
 *
 * @param pasGCPList array of GCP structures being assign (nGCPCount in array).
 *
 * @param pszGCPProjection the new OGC WKT coordinate system to assign for the 
 * GCP output coordinates.  This parameter should be "" if no output coordinate
 * system is known.
 *
 * @return CE_None on success, CE_Failure on failure (including if action is
 * not supported for this format). 
 */ 

CPLErr GDALDataset::SetGCPs( int nGCPCount, const GDAL_GCP *pasGCPList,
                             const char *pszGCPProjection )

{
    CPLError( CE_Failure, CPLE_NotSupported, 
              "Dataset does not support the SetGCPs() method." );

    return CE_Failure;
}

/************************************************************************/
/*                            GDALSetGCPs()                             */
/************************************************************************/

/**
 * @see GDALDataset::SetGCPs()
 */

CPLErr GDALSetGCPs( GDALDatasetH hDS, int nGCPCount, 
                    const GDAL_GCP *pasGCPList, 
                    const char *pszGCPProjection )

{
    return ((GDALDataset *) hDS)->SetGCPs( nGCPCount, pasGCPList, 
                                           pszGCPProjection );
}

/************************************************************************/
/*                           BuildOverviews()                           */
/************************************************************************/

/**
 * Build raster overview(s)
 *
 * If the operation is unsupported for the indicated dataset, then 
 * CE_Failure is returned, and CPLGetLastError() will return CPLE_NonSupported.
 *
 * This method is the same as the C function GDALBuildOverviews().
 *
 * @param pszResampling one of "NEAREST", "AVERAGE" or "MODE" controlling
 * the downsampling method applied.
 * @param nOverviews number of overviews to build. 
 * @param panOverviewList the list of overview decimation factors to build. 
 * @param nBand number of bands to build overviews for in panBandList.  Build
 * for all bands if this is 0. 
 * @param panBandList list of band numbers. 
 * @param pfnProgress a function to call to report progress, or NULL.
 * @param pProgressData application data to pass to the progress function.
 *
 * @return CE_None on success or CE_Failure if the operation doesn't work.
 *
 * For example, to build overview level 2, 4 and 8 on all bands the following
 * call could be made:
 * <pre>
 *   int       anOverviewList[3] = { 2, 4, 8 };
 *   
 *   poDataset->BuildOverviews( "NEAREST", 3, anOverviewList, 0, NULL, 
 *                              GDALDummyProgress, NULL );
 * </pre>
 */

CPLErr GDALDataset::BuildOverviews( const char *pszResampling, 
                                    int nOverviews, int *panOverviewList, 
                                    int nListBands, int *panBandList,
                                    GDALProgressFunc pfnProgress, 
                                    void * pProgressData )
    
{
    CPLErr   eErr;
    int      *panAllBandList = NULL;

    if( nListBands == 0 )
    {
        nListBands = GetRasterCount();
        panAllBandList = (int *) CPLMalloc(sizeof(int) * nListBands);
        for( int i = 0; i < nListBands; i++ )
            panAllBandList[i] = i+1;

        panBandList = panAllBandList;
    }

    if( pfnProgress == NULL )
        pfnProgress = GDALDummyProgress;

    eErr = IBuildOverviews( pszResampling, nOverviews, panOverviewList, 
                            nListBands, panBandList, pfnProgress, pProgressData );

    if( panAllBandList != NULL )
        CPLFree( panAllBandList );

    return eErr;
}

/************************************************************************/
/*                         GDALBuildOverviews()                         */
/************************************************************************/

/**
 * @see GDALDataset::BuildOverviews()
 */

CPLErr GDALBuildOverviews( GDALDatasetH hDataset,
                           const char *pszResampling, 
                           int nOverviews, int *panOverviewList, 
                           int nListBands, int *panBandList,
                           GDALProgressFunc pfnProgress, 
                           void * pProgressData )

{
    return ((GDALDataset *) hDataset)->BuildOverviews(
        pszResampling, nOverviews, panOverviewList, nListBands, panBandList, 
        pfnProgress, pProgressData );
}
    
/************************************************************************/
/*                          IBuildOverviews()                           */
/*                                                                      */
/*      Default implementation.                                         */
/************************************************************************/

CPLErr GDALDataset::IBuildOverviews( const char *pszResampling, 
                                     int nOverviews, int *panOverviewList, 
                                     int nListBands, int *panBandList,
                                     GDALProgressFunc pfnProgress, 
                                     void * pProgressData )
    
{
    if( pfnProgress == NULL )
        pfnProgress = GDALDummyProgress;

    if( oOvManager.IsInitialized() )
        return oOvManager.BuildOverviews( NULL, pszResampling, 
                                          nOverviews, panOverviewList,
                                          nListBands, panBandList,
                                          pfnProgress, pProgressData );
    else
    {
        CPLError( CE_Failure, CPLE_NotSupported,
                  "BuildOverviews() not supported for this dataset." );
        
        return( CE_Failure );
    }
}

/************************************************************************/
/*                             IRasterIO()                              */
/*                                                                      */
/*      The default implementation of IRasterIO() is to pass the        */
/*      request off to each band objects rasterio methods with          */
/*      appropriate arguments.                                          */
/************************************************************************/

CPLErr GDALDataset::IRasterIO( GDALRWFlag eRWFlag,
                               int nXOff, int nYOff, int nXSize, int nYSize,
                               void * pData, int nBufXSize, int nBufYSize,
                               GDALDataType eBufType, 
                               int nBandCount, int *panBandMap,
                               int nPixelSpace, int nLineSpace, int nBandSpace)
    
{
    int iBandIndex; 
    CPLErr eErr = CE_None;

    for( iBandIndex = 0; 
         iBandIndex < nBandCount && eErr == CE_None; 
         iBandIndex++ )
    {
        GDALRasterBand *poBand = GetRasterBand(panBandMap[iBandIndex]);
        GByte *pabyBandData;

        pabyBandData = ((GByte *) pData) + iBandIndex * nBandSpace;
        
        eErr = poBand->IRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize, 
                                  (void *) pabyBandData, nBufXSize, nBufYSize,
                                  eBufType, nPixelSpace, nLineSpace );
    }

    return eErr;
}


/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

/**
 * Read/write a region of image data from multiple bands.
 *
 * This method allows reading a region of one or more GDALRasterBands from
 * this dataset into a buffer,  or writing data from a buffer into a region 
 * of the GDALRasterBands.  It automatically takes care of data type
 * translation if the data type (eBufType) of the buffer is different than
 * that of the GDALRasterBand.
 * The method also takes care of image decimation / replication if the
 * buffer size (nBufXSize x nBufYSize) is different than the size of the
 * region being accessed (nXSize x nYSize).
 *
 * The nPixelSpace, nLineSpace and nBandSpace parameters allow reading into or
 * writing from various organization of buffers. 
 *
 * For highest performance full resolution data access, read and write
 * on "block boundaries" as returned by GetBlockSize(), or use the
 * ReadBlock() and WriteBlock() methods.
 *
 * This method is the same as the C GDALDatasetRasterIO() function.
 *
 * @param eRWFlag Either GF_Read to read a region of data, or GT_Write to
 * write a region of data.
 *
 * @param nXOff The pixel offset to the top left corner of the region
 * of the band to be accessed.  This would be zero to start from the left side.
 *
 * @param nYOff The line offset to the top left corner of the region
 * of the band to be accessed.  This would be zero to start from the top.
 *
 * @param nXSize The width of the region of the band to be accessed in pixels.
 *
 * @param nYSize The height of the region of the band to be accessed in lines.
 *
 * @param pData The buffer into which the data should be read, or from which
 * it should be written.  This buffer must contain at least
 * nBufXSize * nBufYSize * nBandCount words of type eBufType.  It is organized
 * in left to right,top to bottom pixel order.  Spacing is controlled by the
 * nPixelSpace, and nLineSpace parameters.
 *
 * @param nBufXSize the width of the buffer image into which the desired region
 * is to be read, or from which it is to be written.
 *
 * @param nBufYSize the height of the buffer image into which the desired
 * region is to be read, or from which it is to be written.
 *
 * @param eBufType the type of the pixel values in the pData data buffer.  The
 * pixel values will automatically be translated to/from the GDALRasterBand
 * data type as needed.
 *
 * @param nBandCount the number of bands being read or written. 
 *
 * @param panBandMap the list of nBandCount band numbers being read/written.
 * Note band numbers are 1 based.   This may be NULL to select the first 
 * nBandCount bands.
 *
 * @param nPixelSpace The byte offset from the start of one pixel value in
 * pData to the start of the next pixel value within a scanline.  If defaulted
 * (0) the size of the datatype eBufType is used.
 *
 * @param nLineSpace The byte offset from the start of one scanline in
 * pData to the start of the next.  If defaulted the size of the datatype
 * eBufType * nBufXSize is used.
 *
 * @param nBandSpace the byte offset from the start of one bands data to the
 * start of the next.  If defaulted (zero) the value will be 
 * nLineSpace * nBufYSize implying band sequential organization
 * of the data buffer. 
 *
 * @return CE_Failure if the access fails, otherwise CE_None.
 */

CPLErr GDALDataset::RasterIO( GDALRWFlag eRWFlag,
                              int nXOff, int nYOff, int nXSize, int nYSize,
                              void * pData, int nBufXSize, int nBufYSize,
                              GDALDataType eBufType, 
                              int nBandCount, int *panBandMap,
                              int nPixelSpace, int nLineSpace, int nBandSpace )

{
    int i;
    int bNeedToFreeBandMap = FALSE;
    CPLErr eErr = CE_None;

/* -------------------------------------------------------------------- */
/*      If pixel and line spaceing are defaulted assign reasonable      */
/*      value assuming a packed buffer.                                 */
/* -------------------------------------------------------------------- */
    if( nPixelSpace == 0 )
        nPixelSpace = GDALGetDataTypeSize( eBufType ) / 8;
    
    if( nLineSpace == 0 )
        nLineSpace = nPixelSpace * nBufXSize;
    
    if( nBandSpace == 0 )
        nBandSpace = nLineSpace * nBufYSize;

    if( panBandMap == NULL )
    {
        panBandMap = (int *) CPLMalloc(sizeof(int) * nBandCount);
        for( i = 0; i < nBandCount; i++ )
            panBandMap[i] = i+1;

        bNeedToFreeBandMap = TRUE;
    }
    
/* -------------------------------------------------------------------- */
/*      Do some validation of parameters.                               */
/* -------------------------------------------------------------------- */
    if( nXOff < 0 || nXOff + nXSize > nRasterXSize
        || nYOff < 0 || nYOff + nYSize > nRasterYSize )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "Access window out of range in RasterIO().  Requested\n"
                  "(%d,%d) of size %dx%d on raster of %dx%d.",
                  nXOff, nYOff, nXSize, nYSize, nRasterXSize, nRasterYSize );
        eErr = CE_Failure;
    }

    if( eRWFlag != GF_Read && eRWFlag != GF_Write )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "eRWFlag = %d, only GF_Read (0) and GF_Write (1) are legal.",
                  eRWFlag );
        eErr = CE_Failure;
    }

    for( i = 0; i < nBandCount && eErr == CE_None; i++ )
    {
        if( panBandMap[i] < 1 || panBandMap[i] > GetRasterCount() )
        {
            CPLError( CE_Failure, CPLE_IllegalArg, 
                      "panBandMap[%d] = %d, this band does not exist on dataset.",
                      i, panBandMap[i] );
            eErr = CE_Failure;
        }

        if( eErr == CE_None && GetRasterBand( panBandMap[i] ) == NULL )
        {
            CPLError( CE_Failure, CPLE_IllegalArg, 
                      "panBandMap[%d]=%d, this band should exist but is NULL!",
                      i, panBandMap[i] );
            eErr = CE_Failure;
        }
    }

/* -------------------------------------------------------------------- */
/*      Some size values are "noop".  Lets just return to avoid         */
/*      stressing lower level functions.                                */
/* -------------------------------------------------------------------- */
    if( nXSize < 1 || nYSize < 1 || nBufXSize < 1 || nBufYSize < 1 )
    {
        CPLDebug( "GDAL", 
                  "RasterIO() skipped for odd window or buffer size.\n"
                  "  Window = (%d,%d)x%dx%d\n"
                  "  Buffer = %dx%d\n",
                  nXOff, nYOff, nXSize, nYSize, 
                  nBufXSize, nBufYSize );
    }
/* -------------------------------------------------------------------- */
/*      Call the format specific function.                              */
/* -------------------------------------------------------------------- */
    else if( eErr == CE_None )
    {
        eErr = 
            IRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize,
                       pData, nBufXSize, nBufYSize, eBufType,
                       nBandCount, panBandMap,
                       nPixelSpace, nLineSpace, nBandSpace );
    }

/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
    if( bNeedToFreeBandMap )
        CPLFree( panBandMap );

    return eErr;
}

/************************************************************************/
/*                        GDALDatasetRasterIO()                         */
/************************************************************************/

/**
 * @see GDALDataset::RasterIO()
 */

CPLErr GDALDatasetRasterIO( GDALDatasetH hDS, GDALRWFlag eRWFlag,
                            int nXOff, int nYOff, int nXSize, int nYSize,
                            void * pData, int nBufXSize, int nBufYSize,
                            GDALDataType eBufType,
                            int nBandCount, int *panBandMap,
                            int nPixelSpace, int nLineSpace, int nBandSpace )

{
    GDALDataset    *poDS = (GDALDataset *) hDS;
    
    return( poDS->RasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize,
                            pData, nBufXSize, nBufYSize, eBufType,
                            nBandCount, panBandMap, 
                            nPixelSpace, nLineSpace, nBandSpace ) );
}
                     
/************************************************************************/
/*                          GetOpenDatasets()                           */
/************************************************************************/

/**
 * Fetch all open GDAL dataset handles.
 *
 * This method is the same as the C function GDALGetOpenDatasets().
 *
 * @param pnCount integer into which to place the count of dataset pointers
 * being returned.
 *
 * @return a pointer to an array of dataset handles. 
 */

GDALDataset **GDALDataset::GetOpenDatasets( int *pnCount )

{
    *pnCount = nGDALDatasetCount;
    return papoGDALDatasetList;
}

/************************************************************************/
/*                        GDALGetOpenDatasets()                         */
/************************************************************************/

/**
 * @see GDALDataset::GetOpenDatasets()
 */

void GDALGetOpenDatasets( GDALDatasetH ***hDS, int *pnCount )

{
    *hDS = (GDALDatasetH **) GDALDataset::GetOpenDatasets( pnCount);
}

/************************************************************************/
/*                             GDALGetAccess()                          */
/************************************************************************/

/**
 * @see GDALDataset::GetAccess()
 */

int GDALGetAccess( GDALDatasetH hDS )
{
    return ((GDALDataset *) hDS)->GetAccess();
}
