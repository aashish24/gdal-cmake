/******************************************************************************
 * $Id$
 *
 * Project:  ISO 8211 Access
 * Purpose:  Implements the DDFModule class.
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
 * Revision 1.1  1999/04/27 18:45:05  warmerda
 * New
 *
 */

#include "iso8211.h"
#include "cpl_conv.h"

/************************************************************************/
/*                             DDFModule()                              */
/************************************************************************/

/**
 * The constructor.
 */

DDFModule::DDFModule()

{
    nFieldDefnCount = 0;
    paoFieldDefns = NULL;
    poRecord = NULL;
}

/************************************************************************/
/*                             ~DDFModule()                             */
/************************************************************************/

/**
 * The destructor.
 */

DDFModule::~DDFModule()

{
    if( fpDDF != NULL )
        VSIFClose( fpDDF );
    
    if( paoFieldDefns != NULL )
        delete[] paoFieldDefns;

    if( poRecord != NULL )
        delete poRecord;
}

/************************************************************************/
/*                                Open()                                */
/*                                                                      */
/*      Open an ISO 8211 file, and read the DDR record to build the     */
/*      field definitions.                                              */
/************************************************************************/

/**
 * Open a ISO 8211 (DDF) file for reading.
 *
 * If the open succeeds the data descriptive record (DDR) will have been
 * read, and all the field and subfield definitions will be available.
 *
 * @param pszFilename   The name of the file to open.
 *
 * @return FALSE if the open fails or TRUE if it succeeds.  Errors messages
 * are issued internally with CPLError().
 */

int DDFModule::Open( const char * pszFilename )

{
    static const size_t nLeaderSize = 24;
    
/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
    fpDDF = VSIFOpen( pszFilename, "rb" );

    if( fpDDF == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Unable to open DDF file `%s'.",
                  pszFilename );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Read the 24 byte leader.                                        */
/* -------------------------------------------------------------------- */
    char	achLeader[nLeaderSize];
    
    if( VSIFRead( achLeader, 1, nLeaderSize, fpDDF ) != nLeaderSize )
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Leader is short on DDF file `%s'.",
                  pszFilename );
        
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Extract information from leader.                                */
/* -------------------------------------------------------------------- */
    _recLength			  = DDFScanInt( achLeader+0, 5 );
    _interchangeLevel 		  = achLeader[5];
    _leaderIden                   = achLeader[6];
    _inlineCodeExtensionIndicator = achLeader[7];
    _versionNumber                = achLeader[8];
    _appIndicator                 = achLeader[9];
    _fieldControlLength           = DDFScanInt(achLeader+10,2);
    _fieldAreaStart               = DDFScanInt(achLeader+12,5);
    _sizeFieldLength              = DDFScanInt(achLeader+20,1);
    _sizeFieldPos                 = DDFScanInt(achLeader+21,1);
    _sizeFieldTag                 = DDFScanInt(achLeader+23,1);

/* -------------------------------------------------------------------- */
/*	Read the whole record info memory.				*/
/* -------------------------------------------------------------------- */
    char	*pachRecord;

    pachRecord = (char *) CPLMalloc(_recLength);
    memcpy( pachRecord, achLeader, nLeaderSize );

    if( VSIFRead( pachRecord+nLeaderSize, 1, _recLength-nLeaderSize, fpDDF )
        != _recLength - nLeaderSize )
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Header record is short on DDF file `%s'.",
                  pszFilename );
        
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      First make a pass counting the directory entries.               */
/* -------------------------------------------------------------------- */
    int		i;
    int		nFieldEntryWidth;

    nFieldEntryWidth = _sizeFieldLength + _sizeFieldPos + _sizeFieldTag;

    nFieldDefnCount = 0;
    for( i = nLeaderSize; i < _recLength; i += nFieldEntryWidth )
    {
        if( pachRecord[i] == DDF_FIELD_TERMINATOR )
            break;

        nFieldDefnCount++;
    }

/* -------------------------------------------------------------------- */
/*      Allocate, and read field definitions.                           */
/* -------------------------------------------------------------------- */
    paoFieldDefns = new DDFFieldDefn[nFieldDefnCount];
    
    for( i = 0; i < nFieldDefnCount; i++ )
    {
        char	szTag[128];
        int	nEntryOffset = nLeaderSize + i*nFieldEntryWidth;
        int	nFieldLength, nFieldPos;
        
        strncpy( szTag, pachRecord+nEntryOffset, _sizeFieldTag );
        szTag[_sizeFieldTag] = '\0';

        nEntryOffset += _sizeFieldTag;
        nFieldLength = DDFScanInt( pachRecord+nEntryOffset, _sizeFieldLength );
        
        nEntryOffset += _sizeFieldLength;
        nFieldPos = DDFScanInt( pachRecord+nEntryOffset, _sizeFieldPos );
        
        paoFieldDefns[i].Initialize( this, szTag, nFieldLength,
                                     pachRecord+_fieldAreaStart+nFieldPos );
    }

    CPLFree( pachRecord );
    
    return TRUE;
}

/************************************************************************/
/*                                Dump()                                */
/************************************************************************/

/**
 * Write out module info to debugging file.
 *
 * A variety of information about the module is written to the debugging
 * file.  This includes all the field and subfield definitions read from
 * the header. 
 *
 * @param fp The standard io file handle to write to.  ie. stderr.
 */

void DDFModule::Dump( FILE * fp )

{
    fprintf( fp, "DDFModule:\n" );
    fprintf( fp, "    _recLength = %ld\n", _recLength );
    fprintf( fp, "    _interchangeLevel = %c\n", _interchangeLevel );
    fprintf( fp, "    _leaderIden = %c\n", _leaderIden );
    fprintf( fp, "    _inlineCodeExtensionIndicator = %c\n",
             _inlineCodeExtensionIndicator );
    fprintf( fp, "    _versionNumber = %c\n", _versionNumber );
    fprintf( fp, "    _appIndicator = %c\n", _appIndicator );
    fprintf( fp, "    _fieldControlLength = %d\n", _fieldControlLength );
    fprintf( fp, "    _fieldAreaStart = %ld\n", _fieldAreaStart );
    fprintf( fp, "    _sizeFieldLength = %ld\n", _sizeFieldLength );
    fprintf( fp, "    _sizeFieldPos = %ld\n", _sizeFieldPos );
    fprintf( fp, "    _sizeFieldTag = %ld\n", _sizeFieldTag );

    for( int i = 0; i < nFieldDefnCount; i++ )
    {
        paoFieldDefns[i].Dump( fp );
    }
}

/************************************************************************/
/*                           FindFieldDefn()                            */
/************************************************************************/

/**
 * Fetch the definition of the named field.
 *
 * This function will scan the DDFFieldDefn's on this module, to find
 * one with the indicated field name.
 *
 * @param pszFieldName The name of the field to search for.  The comparison is
 *                     case insensitive.
 *
 * @return A pointer to the request DDFFieldDefn object is returned, or NULL
 * if none matching the name are found.  The return object remains owned by
 * the DDFModule, and should not be deleted by application code.
 */

DDFFieldDefn *DDFModule::FindFieldDefn( const char *pszFieldName )

{
    for( int i = 0; i < nFieldDefnCount; i++ )
    {
        if( EQUAL( pszFieldName, paoFieldDefns[i].GetName()) )
            return paoFieldDefns + i;
    }

    return NULL;
}

/************************************************************************/
/*                             ReadRecord()                             */
/*                                                                      */
/*      Read one record from the file, and return to the                */
/*      application.  The returned record is owned by the module,       */
/*      and is reused from call to call in order to preserve headers    */
/*      when they aren't being re-read from record to record.           */
/************************************************************************/

/**
 * Read one record from the file.
 *
 * @return A pointer to a DDFRecord object is returned, or NULL if a read
 * error, or end of file occurs.  The returned record is owned by the
 * module, and should not be deleted by the application.  The record is
 * only valid untill the next ReadRecord() at which point it is overwritten.
 */

DDFRecord *DDFModule::ReadRecord()

{
    if( poRecord == NULL )
        poRecord = new DDFRecord( this );

    if( poRecord->Read() )
        return poRecord;
    else
        return NULL;
}

/************************************************************************/
/*                              GetField()                              */
/************************************************************************/

/**
 * Fetch a field definition by index.
 *
 * @param i (from 0 to GetFieldCount() - 1.
 * @return the returned field pointer or NULL if the index is out of range.
 */

DDFFieldDefn *DDFModule::GetField(int i)

{
    if( i < 0 || i >= nFieldDefnCount )
        return NULL;
    else
        return paoFieldDefns + i;
}
    


