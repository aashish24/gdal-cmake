/******************************************************************************
 * $Id$
 *
 * Project:  SDTS Translator
 * Purpose:  Implementation of SDTSRasterReader class.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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
 * Revision 1.1  1999/06/03 14:02:28  warmerda
 * New
 *
 */

#include "sdts_al.h"

/************************************************************************/
/*                          SDTSRasterReader()                          */
/************************************************************************/

SDTSRasterReader::SDTSRasterReader()

{
    nXSize = 0;
    nYSize = 0;
    nXBlockSize = 0;
    nYBlockSize = 0;
    nXStart = 0;
    nYStart = 0;
    
    strcpy( szINTR, "CE" );
}

/************************************************************************/
/*                             ~SDTSRasterReader()                     */
/************************************************************************/

SDTSRasterReader::~SDTSRasterReader()
{
}

/************************************************************************/
/*                               Close()                                */
/************************************************************************/

void SDTSRasterReader::Close()

{
    oDDFModule.Close();
}

/************************************************************************/
/*                                Open()                                */
/*                                                                      */
/*      Open the requested cell file, and collect required              */
/*      information.                                                    */
/************************************************************************/

int SDTSRasterReader::Open( SDTS_CATD * poCATD, const char * pszModule )

{
    strncpy( szModule, pszModule, sizeof(szModule) );
    
/* ==================================================================== */
/*      Search the LDEF module for the requested cell module.           */
/* ==================================================================== */
    DDFModule	oLDEF;
    DDFRecord	*poRecord;

/* -------------------------------------------------------------------- */
/*      Open the LDEF module, and report failure if it is missing.      */
/* -------------------------------------------------------------------- */
    if( poCATD->GetModuleFilePath("LDEF") == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Can't find LDEF entry in CATD module ... "
                  "can't treat as raster.\n" );
        return FALSE;
    }
    
    if( !oLDEF.Open( poCATD->GetModuleFilePath("LDEF") ) )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Read each record, till we find what we want.                    */
/* -------------------------------------------------------------------- */
    while( (poRecord = oLDEF.ReadRecord() ) != NULL )
    {
        if( EQUAL(poRecord->GetStringSubfield("LDEF",0,"CMNM",0), pszModule) )
            break;
    }

    if( poRecord == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Can't find module `%s' in LDEF file.\n",
                  pszModule );
        return FALSE;
    }
    
/* -------------------------------------------------------------------- */
/*      Extract raster dimensions, and origin offset (0/1).             */
/* -------------------------------------------------------------------- */
    nXSize = poRecord->GetIntSubfield( "LDEF", 0, "NCOL", 0 );
    nYSize = poRecord->GetIntSubfield( "LDEF", 0, "NROW", 0 );

    nXStart = poRecord->GetIntSubfield( "LDEF", 0, "SOCI", 0 );
    nYStart = poRecord->GetIntSubfield( "LDEF", 0, "SORI", 0 );

/* -------------------------------------------------------------------- */
/*      Get the point in the pixel that the origin defines.  We only    */
/*      support top left and center.                                    */
/* -------------------------------------------------------------------- */
    strcpy( szINTR, poRecord->GetStringSubfield(  "LDEF", 0, "INTR", 0 ) );
    if( EQUAL(szINTR,"") )
        strcpy( szINTR, "CE" );
    
    if( !EQUAL(szINTR,"CE") && !EQUAL(szINTR,"TL") )
    {
        CPLError( CE_Warning, CPLE_AppDefined,
                  "Unsupported INTR value of `%s', assume CE.\n"
                  "Positions may be off by one pixel.\n",
                  szINTR );
        strcpy( szINTR, "CE" );
    }

    oLDEF.Close();

/* -------------------------------------------------------------------- */
/*      For now we will assume that the block size is one scanline.     */
/*      We will blow a gasket later while reading the cell file if      */
/*      this isn't the case.                                            */
/*                                                                      */
/*      This isn't a very flexible raster implementation!               */
/* -------------------------------------------------------------------- */
    nXBlockSize = nXSize;
    nYBlockSize = 1;

/* -------------------------------------------------------------------- */
/*      Open the cell file.                                             */
/* -------------------------------------------------------------------- */
    return( oDDFModule.Open( poCATD->GetModuleFilePath(pszModule) ) );
}

/************************************************************************/
/*                              GetBlock()                              */
/*                                                                      */
/*      Read a requested block of raster data from the file.            */
/*                                                                      */
/*      Currently we will always use sequential access.  In the         */
/*      future we should modify the iso8211 library to support          */
/*      seeking, and modify this to seek directly to the right          */
/*      record once it's location is known.                             */
/************************************************************************/

int SDTSRasterReader::GetBlock( int nXOffset, int nYOffset, void * pData )

{
    DDFRecord   *poRecord;
    
    CPLAssert( nXOffset == 0 );
    CPLAssert( GetRasterType() == 1 ); /* int16 */

/* -------------------------------------------------------------------- */
/*      Read through till we find the desired record.                   */
/* -------------------------------------------------------------------- */
    while( (poRecord = oDDFModule.ReadRecord()) != NULL )
    {
        if( poRecord->GetIntSubfield( "CELL", 0, "ROWI", 0 )
            == nYOffset + nYStart )
        {
            break;
        }
    }

/* -------------------------------------------------------------------- */
/*      If we didn't get what we needed just give up.  We should        */
/*      really rewind, and search from the top or better yet index      */
/*      directly to the request.                                        */
/* -------------------------------------------------------------------- */
    if( poRecord == NULL )
    {
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Validate the records size.  Does it represent exactly one       */
/*      scanline?                                                       */
/* -------------------------------------------------------------------- */
    DDFField	*poCVLS;

    poCVLS = poRecord->FindField( "CVLS" );
    if( poCVLS == NULL )
        return FALSE;

    if( poCVLS->GetRepeatCount() != nXSize )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Cell record is %d long, but we expected %d, the number\n"
                  "of pixels in a scanline.  Raster access failed.\n",
                  poCVLS->GetRepeatCount(), nXSize );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Does the CVLS field consist of exactly 1 B(16) field?           */
/* -------------------------------------------------------------------- */
    if( poCVLS->GetDataSize() < 2 * nXSize
        || poCVLS->GetDataSize() > 2 * nXSize + 1 )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Cell record is not of expected format.  Raster access "
                  "failed.\n" );

        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Copy the data to the application buffer, and byte swap if       */
/*      required.                                                       */
/* -------------------------------------------------------------------- */
#ifdef CPL_LSB
    {
        GByte	*pabySrc = (GByte *) poCVLS->GetData();
        GByte	*pabyDst = (GByte *) pData;
        int	i;

        for( i = 0; i < nXSize; i++ )
        {
            pabyDst[i*2] = pabySrc[i*2+1];
            pabyDst[i*2+1] = pabySrc[i*2];
        }
    }
#else    
    memcpy( pData, poCVLS->GetData(), nXSize * 2 );
#endif    

    return TRUE;
}
