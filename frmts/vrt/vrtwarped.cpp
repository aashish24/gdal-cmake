/******************************************************************************
 * $Id$
 *
 * Project:  Virtual GDAL Datasets
 * Purpose:  Implementation of VRTWarpedRasterBand *and VRTWarpedDataset.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.1  2004/08/11 18:42:40  warmerda
 * New
 *
 */

#include "vrtdataset.h"
#include "cpl_minixml.h"
#include "cpl_string.h"
#include "../../alg/gdalwarper.h"


CPL_CVSID("$Id$");
/************************************************************************/
/*                      GDALAutoCreateWarpedVRT()                       */
/************************************************************************/

/**
 * Create virtual warped dataset automatically.
 *
 * This function will create a warped virtual file representing the 
 * input image warped into the target coordinate system.  A GenImgProj
 * transformation is created to accomplish any required GCP/Geotransform
 * warp and reprojection to the target coordinate system.  The output virtual
 * dataset will be "northup" in the target coordinate system.   The
 * GDALSuggestedWarpOutput() function is used to determine the bounds and
 * resolution of the output virtual file which should be large enough to 
 * include all the input image 
 *
 * Note that the constructed GDALDatasetH will acquire one or more references 
 * to the passed in hSrcDS.  Reference counting semantics on the source 
 * dataset should be honoured.  That is, don't just GDALClose() it unless it 
 * was opened with GDALOpenShared(). 
 *
 * @param hSrcDS The source dataset. 
 *
 * @param pszSrcWKT The coordinate system of the source image.  If NULL, it 
 * will be read from the source image. 
 *
 * @param pszDstWKT The coordinate system to convert to.  If NULL no change 
 * of coordinate system will take place.  
 *
 * @param eResampleAlg One of GRA_NearestNeighbour, GRA_Bilinear, GRA_Cubic or 
 * GRA_CubicSpline.  Controls the sampling method used. 
 *
 * @param dfMaxError Maximum error measured in input pixels that is allowed in 
 * approximating the transformation (0.0 for exact calculations).
 *
 * @param nOverviewLevels The number of "power of 2" overview levels to be 
 * built.  If zero, no overview levels will be managed.  
 *
 * @param psOptions Additional warp options, normally NULL.
 *
 * @return NULL on failure, or a new virtual dataset handle on success.
 */

GDALDatasetH GDALAutoCreateWarpedVRT( GDALDatasetH hSrcDS, 
                                      const char *pszSrcWKT,
                                      const char *pszDstWKT,
                                      GDALResampleAlg eResampleAlg, 
                                      double dfMaxError, 
                                      const GDALWarpOptions *psOptionsIn )
    
{
    GDALWarpOptions *psWO;
    int i;

/* -------------------------------------------------------------------- */
/*      Populate the warp options.                                      */
/* -------------------------------------------------------------------- */
    if( psOptionsIn != NULL )
        psWO = GDALCloneWarpOptions( psOptionsIn );
    else
        psWO = GDALCreateWarpOptions();

    psWO->eResampleAlg = eResampleAlg;

    psWO->hSrcDS = hSrcDS;
    GDALReferenceDataset( hSrcDS );

    psWO->nBandCount = GDALGetRasterCount( hSrcDS );
    psWO->panSrcBands = (int *) CPLMalloc(sizeof(int) * psWO->nBandCount);
    psWO->panDstBands = (int *) CPLMalloc(sizeof(int) * psWO->nBandCount);

    for( i = 0; i < psWO->nBandCount; i++ )
    {
        psWO->panSrcBands[i] = i+1;
        psWO->panDstBands[i] = i+1;
    }

    /* TODO: should fill in no data where available */

/* -------------------------------------------------------------------- */
/*      Create the transformer.                                         */
/* -------------------------------------------------------------------- */
    psWO->pfnTransformer = GDALGenImgProjTransform;
    psWO->pTransformerArg = 
        GDALCreateGenImgProjTransformer( psWO->hSrcDS, pszSrcWKT, NULL, NULL,
                                         TRUE, 1.0, 0 );

/* -------------------------------------------------------------------- */
/*      Figure out the desired output bounds and resolution.            */
/* -------------------------------------------------------------------- */
    double adfDstGeoTransform[6];
    int    nDstPixels, nDstLines;
    CPLErr eErr;

    eErr = 
        GDALSuggestedWarpOutput( hSrcDS, psWO->pfnTransformer, 
                                 psWO->pTransformerArg, 
                                 adfDstGeoTransform, &nDstPixels, &nDstLines );

/* -------------------------------------------------------------------- */
/*      Update the transformer to include an output geotransform        */
/*      back to pixel/line coordinates.                                 */
/*                                                                      */
/* -------------------------------------------------------------------- */
    GDALSetGenImgProjTransformerDstGeoTransform( 
        psWO->pTransformerArg, adfDstGeoTransform );

/* -------------------------------------------------------------------- */
/*      Create the VRT file.                                            */
/* -------------------------------------------------------------------- */
    GDALDatasetH hDstDS;

    hDstDS = GDALCreateWarpedVRT( hSrcDS, nDstPixels, nDstLines, 
                                  adfDstGeoTransform, psWO );

    GDALDestroyWarpOptions( psWO );

    if( pszDstWKT != NULL )
        GDALSetProjection( hDstDS, pszDstWKT );
    else if( pszSrcWKT != NULL )
        GDALSetProjection( hDstDS, pszDstWKT );
    else if( GDALGetGCPCount( hSrcDS ) > 0 )
        GDALSetProjection( hDstDS, GDALGetGCPProjection( hSrcDS ) );
    else 
        GDALSetProjection( hDstDS, GDALGetProjectionRef( hSrcDS ) );

    return hDstDS;
}

/************************************************************************/
/*                        GDALCreateWarpedVRT()                         */
/************************************************************************/

/**
 * Create virtual warped dataset.
 *
 * This function will create a warped virtual file representing the 
 * input image warped based on a provided transformation.  Output bounds
 * and resolution are provided explicitly.
 *
 * Note that the constructed GDALDatasetH will acquire one or more references 
 * to the passed in hSrcDS.  Reference counting semantics on the source 
 * dataset should be honoured.  That is, don't just GDALClose() it unless it 
 * was opened with GDALOpenShared(). 
 *
 * @param hSrcDS The source dataset. 
 *
 * @param nOverviewLevels The number of "power of 2" overview levels to be 
 * built.  If zero, no overview levels will be managed.  
 *
 * @param psOptions Additional warp options, normally NULL.
 *
 * @return NULL on failure, or a new virtual dataset handle on success.
 */

GDALDatasetH 
GDALCreateWarpedVRT( GDALDatasetH hSrcDS, 
                     int nPixels, int nLines, double *padfGeoTransform,
                     GDALWarpOptions *psOptions )
    
{
/* -------------------------------------------------------------------- */
/*      Create the VRTDataset and populate it with bands.               */
/* -------------------------------------------------------------------- */
    VRTWarpedDataset *poDS = new VRTWarpedDataset( nPixels, nLines );
    int i;

    psOptions->hDstDS = (GDALDatasetH) poDS;

    poDS->SetGeoTransform( padfGeoTransform );

    for( i = 0; i < psOptions->nBandCount; i++ )
    {
        VRTWarpedRasterBand *poBand;
        GDALRasterBand *poSrcBand = (GDALRasterBand *) 
            GDALGetRasterBand( hSrcDS, i+1 );

        poDS->AddBand( poSrcBand->GetRasterDataType(), NULL );

        poBand = (VRTWarpedRasterBand *) poDS->GetRasterBand( i+1 );
        poBand->CopyCommonInfoFrom( poSrcBand );
    }

/* -------------------------------------------------------------------- */
/*      Initialize the warp on the VRTWarpedDataset.                    */
/* -------------------------------------------------------------------- */
    poDS->Initialize( psOptions );
    
    return (GDALDatasetH) poDS;
}

/************************************************************************/
/* ==================================================================== */
/*                          VRTWarpedDataset                            */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                          VRTWarpedDataset()                          */
/************************************************************************/

VRTWarpedDataset::VRTWarpedDataset( int nXSize, int nYSize )
        : VRTDataset( nXSize, nYSize )

{
    poWarper = NULL;
    nBlockXSize = 512;
    nBlockYSize = 128;
    eAccess = GA_Update;
}

/************************************************************************/
/*                         ~VRTWarpedDataset()                          */
/************************************************************************/

VRTWarpedDataset::~VRTWarpedDataset()

{
    FlushCache();
    if( poWarper != NULL )
    {
        const GDALWarpOptions *psWO = poWarper->GetOptions();

        if( psWO->hSrcDS != NULL )
            GDALClose( psWO->hSrcDS );
        delete poWarper;
    }
}

/************************************************************************/
/*                             Initialize()                             */
/*                                                                      */
/*      Initialize a dataset from passed in warp options.               */
/************************************************************************/

CPLErr VRTWarpedDataset::Initialize( void *psWO )

{
    if( poWarper != NULL )
        delete poWarper;

    poWarper = new GDALWarpOperation();

    return poWarper->Initialize( (GDALWarpOptions *) psWO );
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTWarpedDataset::XMLInit( CPLXMLNode *psTree, const char *pszVRTPath )

{
    CPLErr eErr;

/* -------------------------------------------------------------------- */
/*      First initialize all the general VRT stuff.  This will even     */
/*      create the VRTWarpedRasterBands and initialize them.            */
/* -------------------------------------------------------------------- */
    eErr = VRTDataset::XMLInit( psTree, pszVRTPath );

    if( eErr != CE_None )
        return eErr;

/* -------------------------------------------------------------------- */
/*      Now find the block size.                                        */
/* -------------------------------------------------------------------- */
    nBlockXSize = atoi(CPLGetXMLValue(psTree,"BlockXSize","512"));
    nBlockYSize = atoi(CPLGetXMLValue(psTree,"BlockYSize","128"));

/* -------------------------------------------------------------------- */
/*      Find the GDALWarpOptions XML tree.                              */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psOptionsTree;
    psOptionsTree = CPLGetXMLNode( psTree, "GDALWarpOptions" );
    if( psOptionsTree == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Count not find required GDALWarpOptions in XML." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Adjust the SourceDataset in the warp options to take into       */
/*      account that it is relative to the VRT if appropriate.          */
/* -------------------------------------------------------------------- */
    int bRelativeToVRT = 
        atoi(CPLGetXMLValue(psOptionsTree,
                            "SourceDataset.relativeToVRT", "0" ));

    const char *pszRelativePath = CPLGetXMLValue(psOptionsTree,
                                                 "SourceDataset", "" );
    char *pszAbsolutePath;

    if( bRelativeToVRT )
        pszAbsolutePath = 
            CPLStrdup(CPLProjectRelativeFilename( pszVRTPath, 
                                                  pszRelativePath ) );
    else
        pszAbsolutePath = CPLStrdup(pszRelativePath);

    CPLSetXMLValue( psOptionsTree, "SourceDataset", pszAbsolutePath );
    CPLFree( pszAbsolutePath );

/* -------------------------------------------------------------------- */
/*      And instantiate the warp options, and corresponding warp        */
/*      operation.                                                      */
/* -------------------------------------------------------------------- */
    GDALWarpOptions *psWO;

    psWO = GDALDeserializeWarpOptions( psOptionsTree );
    if( psWO == NULL )
        return CE_Failure;

    this->eAccess = GA_Update;
    psWO->hDstDS = this;

/* -------------------------------------------------------------------- */
/*      Instantiate the warp operation.                                 */
/* -------------------------------------------------------------------- */
    poWarper = new GDALWarpOperation();

    eErr = poWarper->Initialize( psWO );

    GDALDestroyWarpOptions( psWO );
    if( eErr != CE_None )
    {
        delete poWarper;
        poWarper = NULL;
    }

    return eErr;
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTWarpedDataset::SerializeToXML( const char *pszVRTPath )

{
    CPLXMLNode *psTree;

    psTree = VRTDataset::SerializeToXML( pszVRTPath );

    if( psTree == NULL )
        return psTree;

/* -------------------------------------------------------------------- */
/*      Set subclass.                                                   */
/* -------------------------------------------------------------------- */
    CPLCreateXMLNode( 
        CPLCreateXMLNode( psTree, CXT_Attribute, "subClass" ), 
        CXT_Text, "VRTWarpedDataset" );

/* -------------------------------------------------------------------- */
/*      Serialize the block size.                                       */
/* -------------------------------------------------------------------- */
    CPLCreateXMLElementAndValue( psTree, "BlockXSize",
                                 CPLSPrintf( "%d", nBlockXSize ) );
    CPLCreateXMLElementAndValue( psTree, "BlockYSize",
                                 CPLSPrintf( "%d", nBlockYSize ) );

/* ==================================================================== */
/*      Serialize the warp options.                                     */
/* ==================================================================== */
    CPLXMLNode *psWOTree;

    if( poWarper != NULL )
    {
/* -------------------------------------------------------------------- */
/*      We reset the destination dataset name so it doesn't get         */
/*      written out in the serialize warp options.                      */
/* -------------------------------------------------------------------- */
        char *pszSavedName = CPLStrdup(GetDescription());
        SetDescription("");

        psWOTree = GDALSerializeWarpOptions( poWarper->GetOptions() );
        CPLAddXMLChild( psTree, psWOTree );

        SetDescription( pszSavedName );
        CPLFree( pszSavedName );

/* -------------------------------------------------------------------- */
/*      We need to consider making the source dataset relative to       */
/*      the VRT file if possible.  Adjust accordingly.                  */
/* -------------------------------------------------------------------- */
        CPLXMLNode *psSDS = CPLGetXMLNode( psWOTree, "SourceDataset" );
        int bRelativeToVRT;
        char *pszRelativePath;

        pszRelativePath = 
            CPLStrdup(
                CPLExtractRelativePath( pszVRTPath, psSDS->psChild->pszValue, 
                                        &bRelativeToVRT ) );

        CPLFree( psSDS->psChild->pszValue );
        psSDS->psChild->pszValue = pszRelativePath;

        CPLCreateXMLNode( 
            CPLCreateXMLNode( psSDS, CXT_Attribute, "relativeToVRT" ), 
            CXT_Text, bRelativeToVRT ? "1" : "0" );
    }

    return psTree;
}

/************************************************************************/
/*                            GetBlockSize()                            */
/************************************************************************/

void VRTWarpedDataset::GetBlockSize( int *pnBlockXSize, int *pnBlockYSize )

{
    *pnBlockXSize = nBlockXSize;
    *pnBlockYSize = nBlockYSize;
}

/************************************************************************/
/*                            ProcessBlock()                            */
/*                                                                      */
/*      Warp a single requested block, and then push each band of       */
/*      the result into the block cache.                                */
/************************************************************************/

CPLErr VRTWarpedDataset::ProcessBlock( int iBlockX, int iBlockY )

{
    if( poWarper == NULL )
        return CE_Failure;

    const GDALWarpOptions *psWO = poWarper->GetOptions();

/* -------------------------------------------------------------------- */
/*      Allocate block of memory large enough to hold all the bands     */
/*      for this block.                                                 */
/* -------------------------------------------------------------------- */
    GByte *pabyDstBuffer;
    int   nDstBufferSize;
    int   nWordSize = (GDALGetDataTypeSize(psWO->eWorkingDataType) / 8);

    nDstBufferSize = nBlockXSize * nBlockYSize * psWO->nBandCount * nWordSize;

    pabyDstBuffer = (GByte *) VSIMalloc(nDstBufferSize);

    if( pabyDstBuffer == NULL )
    {
        CPLError( CE_Failure, CPLE_OutOfMemory,
                  "Out of memory allocating %d byte buffer in VRTWarpedDataset::ProcessBlock()",
                  nDstBufferSize );
        return CE_Failure;
    }				

    memset( pabyDstBuffer, 0, nDstBufferSize );

/* -------------------------------------------------------------------- */
/*      Warp into this buffer.                                          */
/* -------------------------------------------------------------------- */
    CPLErr eErr;

    eErr = 
        poWarper->WarpRegionToBuffer( 
            iBlockX * nBlockXSize, iBlockY * nBlockYSize, 
            nBlockXSize, nBlockYSize,
            pabyDstBuffer, psWO->eWorkingDataType );

    if( eErr != CE_None )
    {
        VSIFree( pabyDstBuffer );
        return eErr;
    }
                        
/* -------------------------------------------------------------------- */
/*      Copy out into cache blocks for each band.                       */
/* -------------------------------------------------------------------- */
    int iBand;

    for( iBand = 0; iBand < psWO->nBandCount; iBand++ )
    {
        GDALRasterBand *poBand;
        GDALRasterBlock *poBlock;

        poBand = GetRasterBand(iBand+1);
        poBlock = poBand->GetBlockRef( iBlockX, iBlockY, TRUE );

        GDALCopyWords( pabyDstBuffer + iBand*nBlockXSize*nBlockYSize*nWordSize,
                       psWO->eWorkingDataType, nWordSize, 
                       poBlock->GetDataRef(), 
                       poBlock->GetDataType(), nWordSize,
                       nBlockXSize * nBlockYSize );
    }
    
    return CE_None;
}

/************************************************************************/
/*                              AddBand()                               */
/************************************************************************/

CPLErr VRTWarpedDataset::AddBand( GDALDataType eType, char **papszOptions )

{
    SetBand( GetRasterCount() + 1,
             new VRTWarpedRasterBand( this, GetRasterCount() + 1, eType ) );

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                        VRTWarpedRasterBand                           */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                        VRTWarpedRasterBand()                         */
/************************************************************************/

VRTWarpedRasterBand::VRTWarpedRasterBand( GDALDataset *poDS, int nBand,
                                          GDALDataType eType )

{
    Initialize( poDS->GetRasterXSize(), poDS->GetRasterYSize() );

    this->poDS = poDS;
    this->nBand = nBand;
    this->eAccess = GA_Update;

    ((VRTWarpedDataset *) poDS)->GetBlockSize( &nBlockXSize, 
                                               &nBlockYSize );

    if( eType != GDT_Unknown )
        this->eDataType = eType;
}

/************************************************************************/
/*                        ~VRTWarpedRasterBand()                        */
/************************************************************************/

VRTWarpedRasterBand::~VRTWarpedRasterBand()

{
    FlushCache();
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr VRTWarpedRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                     void * pImage )

{
    CPLErr eErr;
    VRTWarpedDataset *poWDS = (VRTWarpedDataset *) poDS;
    GDALRasterBlock *poBlock;

    poBlock = GetBlockRef( nBlockXOff, nBlockYOff, TRUE );
    poBlock->AddLock();

    eErr = poWDS->ProcessBlock( nBlockXOff, nBlockYOff );

    if( eErr == CE_None )
    {
        int nDataBytes;
        nDataBytes = (GDALGetDataTypeSize(poBlock->GetDataType()) / 8)
            * poBlock->GetXSize() * poBlock->GetYSize();
        memcpy( pImage, poBlock->GetDataRef(), nDataBytes );
    }

    poBlock->DropLock();

    return eErr;
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTWarpedRasterBand::XMLInit( CPLXMLNode * psTree, 
                                  const char *pszVRTPath )

{
    return VRTRasterBand::XMLInit( psTree, pszVRTPath );
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTWarpedRasterBand::SerializeToXML( const char *pszVRTPath )

{
    CPLXMLNode *psTree = VRTRasterBand::SerializeToXML( pszVRTPath );

/* -------------------------------------------------------------------- */
/*      Set subclass.                                                   */
/* -------------------------------------------------------------------- */
    CPLCreateXMLNode( 
        CPLCreateXMLNode( psTree, CXT_Attribute, "subClass" ), 
        CXT_Text, "VRTWarpedRasterBand" );

    return psTree;
}
