/******************************************************************************
 * $Id$
 *
 * Project:  Microstation DGN Access Library
 * Purpose:  DGN Access Library file open code.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam (warmerdam@pobox.com)
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
 * Revision 1.4  2001/01/10 16:13:09  warmerda
 * added documentation
 *
 * Revision 1.3  2000/12/28 21:28:59  warmerda
 * added element index support
 *
 * Revision 1.2  2000/12/14 17:10:57  warmerda
 * implemented TCB, Ellipse, TEXT
 *
 * Revision 1.1  2000/11/28 19:03:47  warmerda
 * New
 *
 */

#include "dgnlibp.h"

/************************************************************************/
/*                              DGNOpen()                               */
/************************************************************************/

/**
 * Open a DGN file. 
 *
 * The file is opened, and minimally verified to ensure it is a DGN (ISFF)
 * file.  If the file cannot be opened for read access an error with code
 * CPLE_OpenFailed with be reported via CPLError() and NULL returned.  
 * If the file header does
 * not appear to be a DGN file, an error with code CPLE_AppDefined will be
 * reported via CPLError(), and NULL returned.
 *
 * If successful a handle for further access is returned.  This should be
 * closed with DGNClose() when no longer needed.  
 *
 * DGNOpen() does not scan the file on open, and should be very fast even for
 * large files.  
 *
 * @param pszFilename name of file to try opening.
 *
 * @return handle to use for further access to file using DGN API, or NULL
 * if open fails.
 */

DGNHandle DGNOpen( const char * pszFilename )

{
    DGNInfo	*psDGN;
    FILE	*fp;

/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
    fp = VSIFOpen( pszFilename, "rb" );
    if( fp == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Unable to open `%s' for read access.\n", 
                  pszFilename );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Verify the format ... add later.                                */
/* -------------------------------------------------------------------- */
    GByte	abyHeader[4];

    VSIFRead( abyHeader, 1, 4, fp );
    if( abyHeader[0] != 0x08 || abyHeader[1] != 0x09
        || abyHeader[2] != 0xFE || abyHeader[3] != 0x02 )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "File `%s' does not have expected DGN header.\n", 
                  pszFilename );
        VSIFClose( fp );
        return NULL;
    }

    VSIRewind( fp );

/* -------------------------------------------------------------------- */
/*      Create the info structure.                                      */
/* -------------------------------------------------------------------- */
    psDGN = (DGNInfo *) CPLCalloc(sizeof(DGNInfo),1);
    psDGN->fp = fp;
    psDGN->next_element_id = 0;

    psDGN->got_tcb = FALSE;
    psDGN->dimension = 2;
    psDGN->scale = 1.0;
    psDGN->origin_x = 0.0;
    psDGN->origin_y = 0.0;
    psDGN->origin_z = 0.0;					       

    psDGN->index_built = FALSE;
    psDGN->element_count = 0;
    psDGN->element_index = NULL;

    psDGN->got_bounds = FALSE;

    return (DGNHandle) psDGN;
}

/************************************************************************/
/*                              DGNClose()                              */
/************************************************************************/

/**
 * Close DGN file. 
 *
 * @param hDGN Handle from DGNOpen() for file to close.
 */

void DGNClose( DGNHandle hDGN )

{
    DGNInfo	*psDGN = (DGNInfo *) hDGN;

    VSIFClose( psDGN->fp );
    CPLFree( psDGN );
}

