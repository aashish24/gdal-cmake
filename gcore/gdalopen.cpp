/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Implementation of GDALOpen(), GDALOpenShared(), GDALOpenInfo and
 *           related services.
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
 * Revision 1.14  2002/06/12 21:13:27  warmerda
 * use metadata based driver info
 *
 * Revision 1.13  2002/05/28 18:56:22  warmerda
 * added shared dataset concept
 *
 * Revision 1.12  2001/12/12 17:21:21  warmerda
 * Use CPLStat instead of VSIStat().
 *
 * Revision 1.11  2001/08/20 13:40:28  warmerda
 * modified message on failure to open if not a file
 *
 * Revision 1.10  2001/07/18 04:04:30  warmerda
 * added CPL_CVSID
 *
 * Revision 1.9  2001/01/03 05:32:20  warmerda
 * avoid depending on VSIStatBuf file size for 2GB issues
 *
 * Revision 1.8  2000/04/21 21:55:53  warmerda
 * set filename as description of GDALDatasets.
 *
 * Revision 1.7  2000/01/10 15:43:06  warmerda
 * Fixed debug statement.
 *
 * Revision 1.6  2000/01/10 15:31:02  warmerda
 * Added debug statement in GDALOpen.
 *
 * Revision 1.5  1999/11/11 21:59:07  warmerda
 * added GetDriver() for datasets
 *
 * Revision 1.4  1999/10/01 14:44:02  warmerda
 * added documentation
 *
 * Revision 1.3  1999/04/21 04:00:34  warmerda
 * Initialize fp to NULL.
 *
 * Revision 1.2  1998/12/31 18:52:45  warmerda
 * Use CPL memory functions (they are safe), and fixed up header reading.
 *
 * Revision 1.1  1998/12/03 18:31:45  warmerda
 * New
 *
 */

#include "gdal_priv.h"
#include "cpl_conv.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*                             GDALOpenInfo                             */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                            GDALOpenInfo()                            */
/************************************************************************/

GDALOpenInfo::GDALOpenInfo( const char * pszFilenameIn, GDALAccess eAccessIn )

{
    pszFilename = CPLStrdup( pszFilenameIn );

    nHeaderBytes = 0;
    pabyHeader = NULL;
    bStatOK = FALSE;
    eAccess = eAccessIn;
    fp = NULL;
    
/* -------------------------------------------------------------------- */
/*      Collect information about the file.                             */
/* -------------------------------------------------------------------- */
    if( CPLStat( pszFilename, &sStat ) == 0 )
    {
        bStatOK = TRUE;

        if( VSI_ISREG( sStat.st_mode ) )
        {
            pabyHeader = (GByte *) CPLCalloc(1025,1);

            fp = VSIFOpen( pszFilename, "rb" );

            if( fp != NULL )
            {
                nHeaderBytes = VSIFRead( pabyHeader, 1, 1024, fp );

                VSIRewind( fp );
            }
        }
    }
}

/************************************************************************/
/*                           ~GDALOpenInfo()                            */
/************************************************************************/

GDALOpenInfo::~GDALOpenInfo()

{
    VSIFree( pabyHeader );
    CPLFree( pszFilename );

    if( fp != NULL )
        VSIFClose( fp );
}

/************************************************************************/
/*                              GDALOpen()                              */
/************************************************************************/

/**
 * Open a raster file as a GDALDataset.
 *
 * See Also: GDALOpenShared()
 *
 * @param pszFilename the name of the file to access.  In the case of
 * exotic drivers this may not refer to a physical file, but instead contain
 * information for the driver on how to access a dataset.
 *
 * @param eAccess the desired access, either GA_Update or GA_ReadOnly.  Many
 * drivers support only read only access.
 *
 * @return A GDALDatasetH handle or NULL on failure.  For C++ applications
 * this handle can be cast to a GDALDataset *. 
 */
 

GDALDatasetH GDALOpen( const char * pszFilename, GDALAccess eAccess )

{
    int		iDriver;
    GDALDriverManager *poDM = GetGDALDriverManager();
    GDALOpenInfo oOpenInfo( pszFilename, eAccess );

    CPLErrorReset();
    
    for( iDriver = 0; iDriver < poDM->GetDriverCount(); iDriver++ )
    {
        GDALDriver	*poDriver = poDM->GetDriver( iDriver );
        GDALDataset	*poDS;

        poDS = poDriver->pfnOpen( &oOpenInfo );
        if( poDS != NULL )
        {
            poDS->SetDescription( pszFilename );

            if( poDS->poDriver == NULL )
                poDS->poDriver = poDriver;

            
            CPLDebug( "GDAL", "GDALOpen(%s) succeeds as %s.\n",
                      pszFilename, poDriver->GetDescription() );

            return (GDALDatasetH) poDS;
        }

        if( CPLGetLastErrorNo() != 0 )
            return NULL;
    }

    if( oOpenInfo.bStatOK )
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "`%s' not recognised as a supported file format.\n",
                  pszFilename );
    else
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "`%s' does not exist in the file system,\n"
                  "and is not recognised as a supported dataset name.\n",
                  pszFilename );
              
    return NULL;
}

/************************************************************************/
/*                           GDALOpenShared()                           */
/************************************************************************/

/**
 * Open a raster file as a GDALDataset.
 *
 * This function works the same as GDALOpen(), but allows the sharing of
 * GDALDataset handles for a dataset with other callers to GDALOpenShared().
 * 
 * In particular, GDALOpenShared() will first consult it's list of currently
 * open and shared GDALDataset's, and if the GetDescription() name for one
 * exactly matches the pszFilename passed to GDALOpenShared() it 

 * @param pszFilename the name of the file to access.  In the case of
 * exotic drivers this may not refer to a physical file, but instead contain
 * information for the driver on how to access a dataset.
 *
 * @param eAccess the desired access, either GA_Update or GA_ReadOnly.  Many
 * drivers support only read only access.
 *
 * @return A GDALDatasetH handle or NULL on failure.  For C++ applications
 * this handle can be cast to a GDALDataset *. 
 */
 
GDALDatasetH GDALOpenShared( const char *pszFilename, GDALAccess eAccess )

{
/* -------------------------------------------------------------------- */
/*      First scan the existing list to see if it could already         */
/*      contain the requested dataset.                                  */
/* -------------------------------------------------------------------- */
    int		i, nSharedDatasetCount;
    GDALDataset **papoSharedDatasets 
        		= GDALDataset::GetOpenDatasets(&nSharedDatasetCount);
    
    for( i = 0; i < nSharedDatasetCount; i++ )
    {
        if( strcmp(pszFilename,papoSharedDatasets[i]->GetDescription()) == 0 
            && (eAccess == GA_ReadOnly 
                || papoSharedDatasets[i]->GetAccess() == eAccess ) )
            
        {
            papoSharedDatasets[i]->Reference();
            return papoSharedDatasets[i];
        }
    }

/* -------------------------------------------------------------------- */
/*      Try opening the the requested dataset.                          */
/* -------------------------------------------------------------------- */
    GDALDataset *poDataset;

    poDataset = (GDALDataset *) GDALOpen( pszFilename, eAccess );
    if( poDataset != NULL )
        poDataset->MarkAsShared();
    
    return (GDALDatasetH) poDataset;
}

/************************************************************************/
/*                             GDALClose()                              */
/************************************************************************/

void GDALClose( GDALDatasetH hDS )

{
    GDALDataset *poDS = (GDALDataset *) hDS;
    int		i, nSharedDatasetCount;
    GDALDataset **papoSharedDatasets 
        		= GDALDataset::GetOpenDatasets(&nSharedDatasetCount);

/* -------------------------------------------------------------------- */
/*      If this file is in the shared dataset list then dereference     */
/*      it, and only delete/remote it if the reference count has        */
/*      dropped to zero.                                                */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nSharedDatasetCount; i++ )
    {
        if( papoSharedDatasets[i] == poDS )
        {
            if( poDS->Dereference() > 0 )
                return;

            delete poDS;
            return;
        }
    }

/* -------------------------------------------------------------------- */
/*      This is not shared dataset, so directly delete it.              */
/* -------------------------------------------------------------------- */
    delete poDS;
}

/************************************************************************/
/*                        GDALDumpOpenDataset()                         */
/************************************************************************/

int GDALDumpOpenDatasets( FILE *fp )
   
{
    int		i, nSharedDatasetCount;
    GDALDataset **papoSharedDatasets 
        		= GDALDataset::GetOpenDatasets(&nSharedDatasetCount);

    if( nSharedDatasetCount > 0 )
        VSIFPrintf( fp, "Open GDAL Datasets:\n" );
    
    for( i = 0; i < nSharedDatasetCount; i++ )
    {
        const char *pszDriverName;
        GDALDataset *poDS = papoSharedDatasets[i];
        
        if( poDS->GetDriver() == NULL )
            pszDriverName = "DriverIsNULL";
        else
            pszDriverName = poDS->GetDriver()->GetDescription();

        poDS->Reference();
        VSIFPrintf( fp, "  %d %c %-6s %dx%dx%d %s\n", 
                    poDS->Dereference(), 
                    poDS->GetShared() ? 'S' : 'N',
                    pszDriverName, 
                    poDS->GetRasterXSize(),
                    poDS->GetRasterYSize(),
                    poDS->GetRasterCount(),
                    poDS->GetDescription() );
    }
    
    return nSharedDatasetCount;
}
