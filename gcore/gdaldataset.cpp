/******************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
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
 * gdaldataset.cpp
 *
 * The GDALDataset class.
 * 
 * $Log$
 * Revision 1.6  1999/05/16 20:04:58  warmerda
 * Don't emit an error message when SetProjection() is called for datasets
 * that don't implement the call.
 *
 * Revision 1.5  1999/04/21 04:16:51  warmerda
 * experimental docs
 *
 * Revision 1.4  1999/01/11 15:37:55  warmerda
 * fixed log keyword
 * 
 */

#include "gdal_priv.h"

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
}

/************************************************************************/
/*                            ~GDALDataset()                            */
/************************************************************************/

GDALDataset::~GDALDataset()

{
    int		i;

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
/*                             GDALClose()                              */
/************************************************************************/

void GDALClose( GDALDatasetH hDS )

{
    delete ((GDALDataset *) hDS);
}

/************************************************************************/
/*                             FlushCache()                             */
/*                                                                      */
/*      Flush all write caches to disk.                                 */
/************************************************************************/

void GDALDataset::FlushCache()

{
    int		i;

    for( i = 0; i < nBands; i++ )
    {
        if( papoBands[i] != NULL )
            papoBands[i]->FlushCache();
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
    if( nBands < nNewBand ) {
        int		i;
        
        papoBands = (GDALRasterBand **)
            VSIRealloc(papoBands, sizeof(GDALRasterBand*) * nNewBand);

        for( i = nBands; i < nNewBand; i++ )
            papoBands[i] = NULL;
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
}

/************************************************************************/
/*                           GetRasterXSize()                           */
/************************************************************************/

//! Fetch raster width in pixels.

/*!

Returns the raster width of all GDALBands associated with this
GDALDataset.  The C function GDALGetRasterXSize() does the same.

*/

int GDALDataset::GetRasterXSize()

{
    return nRasterXSize;
}

/************************************************************************/
/*                         GDALGetRasterXSize()                         */
/************************************************************************/

int GDALGetRasterXSize( GDALDatasetH hDataset )

{
    return ((GDALDataset *) hDataset)->GetRasterXSize();
}


/************************************************************************/
/*                           GetRasterYSize()                           */
/************************************************************************/

//! Fetch raster height in pixels.

int GDALDataset::GetRasterYSize()

{
    return nRasterYSize;
}

/************************************************************************/
/*                         GDALGetRasterYSize()                         */
/************************************************************************/

int GDALGetRasterYSize( GDALDatasetH hDataset )

{
    return ((GDALDataset *) hDataset)->GetRasterYSize();
}

/************************************************************************/
/*                           GetRasterBand()                            */
/************************************************************************/

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

/*! \relates GDALRasterBand

  \param hDS Dataset handle
  \param nBandId The band number to fetch from 1 to GDALGetRasterCount().
  \return a GDALRasterBand handle (GDALRasterBandH).

*/

GDALRasterBandH GDALGetRasterBand( GDALDatasetH hDS, int nBandId )

{
    return( (GDALRasterBandH) ((GDALDataset *) hDS)->GetRasterBand(nBandId) );
}

/************************************************************************/
/*                           GetRasterCount()                           */
/************************************************************************/

int GDALDataset::GetRasterCount()

{
    return( nBands );
}

/************************************************************************/
/*                         GDALGetRasterCount()                         */
/************************************************************************/

int GDALGetRasterCount( GDALDatasetH hDS )

{
    return( ((GDALDataset *) hDS)->GetRasterCount() );
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *GDALDataset::GetProjectionRef()

{
    return( "" );
}

/************************************************************************/
/*                        GDALGetProjectionRef()                        */
/************************************************************************/

const char *GDALGetProjectionRef( GDALDatasetH hDS )

{
    return( ((GDALDataset *) hDS)->GetProjectionRef() );
}

/************************************************************************/
/*                           SetProjection()                            */
/************************************************************************/

CPLErr GDALDataset::SetProjection( const char * )

{
    return CE_Failure;
}

/************************************************************************/
/*                         GDALSetProjection()                          */
/************************************************************************/

CPLErr GDALSetProjection( GDALDatasetH hDS, const char * pszProjection )

{
    return( ((GDALDataset *) hDS)->SetProjection(pszProjection) );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr GDALDataset::GetGeoTransform( double * padfTransform )

{
    CPLAssert( padfTransform != NULL );
        
    padfTransform[0] = 0.0;	/* X Origin (top left corner) */
    padfTransform[1] = 1.0;	/* X Pixel size */
    padfTransform[2] = 0.0;

    padfTransform[3] = 0.0;	/* Y Origin (top left corner) */
    padfTransform[4] = 0.0;	
    padfTransform[5] = -1.0;	/* Y Pixel Size */

    return( CE_Failure );
}

/************************************************************************/
/*                        GDALGetGeoTransform()                         */
/************************************************************************/

CPLErr GDALGetGeoTransform( GDALDatasetH hDS, double * padfTransform )

{
    return( ((GDALDataset *) hDS)->GetGeoTransform(padfTransform) );
}

/************************************************************************/
/*                          SetGeoTransform()                           */
/************************************************************************/

CPLErr GDALDataset::SetGeoTransform( double * )

{
    CPLError( CE_Failure, CPLE_NotSupported,
              "SetGeoTransform() not supported for this dataset." );
    
    return( CE_Failure );
}

/************************************************************************/
/*                        GDALSetGeoTransform()                         */
/************************************************************************/

CPLErr GDALSetGeoTransform( GDALDatasetH hDS, double * padfTransform )

{
    return( ((GDALDataset *) hDS)->GetGeoTransform(padfTransform) );
}

/************************************************************************/
/*                         GetInternalHandle()                          */
/************************************************************************/

void *GDALDataset::GetInternalHandle( const char * )

{
    return( NULL );
}

/************************************************************************/
/*                       GDALGetInternalHandle()                        */
/************************************************************************/

void *GDALGetInternalHandle( GDALDatasetH hDS, const char * pszRequest )

{
    return( ((GDALDataset *) hDS)->GetInternalHandle(pszRequest) );
}
