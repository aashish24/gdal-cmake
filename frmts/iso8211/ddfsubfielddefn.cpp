/******************************************************************************
 * $Id$
 *
 * Project:  ISO 8211 Access
 * Purpose:  Implements the DDFSubfieldDefn class.
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
/*                          DDFSubfieldDefn()                           */
/************************************************************************/

DDFSubfieldDefn::DDFSubfieldDefn()

{
    pszName = NULL;
    
    bIsVariable = TRUE;
    nFormatWidth = 0;
    chFormatDelimeter = DDF_UNIT_TERMINATOR;
    eBinaryFormat = NotBinary;
    eType = DDFString;
    
    pszFormatString = CPLStrdup("");

    nMaxBufChars = 0;
    pachBuffer = NULL;
}

/************************************************************************/
/*                          ~DDFSubfieldDefn()                          */
/************************************************************************/

DDFSubfieldDefn::~DDFSubfieldDefn()

{
    CPLFree( pszName );
    CPLFree( pszFormatString );
    CPLFree( pachBuffer );
}

/************************************************************************/
/*                              SetName()                               */
/************************************************************************/

void DDFSubfieldDefn::SetName( const char * pszNewName )

{
    CPLFree( pszName );

    pszName = CPLStrdup( pszNewName );
}

/************************************************************************/
/*                             SetFormat()                              */
/*                                                                      */
/*      While interpreting the format string we don't support:          */
/*                                                                      */
/*       o Passing an explicit terminator for variable length field.    */
/*       o 'X' for unused data ... this should really be filtered       */
/*         out by DDFFieldDefn::ApplyFormats(), but isn't.              */
/*       o 'B' bitstrings that aren't a multiple of eight.              */
/************************************************************************/

int DDFSubfieldDefn::SetFormat( const char * pszFormat )

{
    CPLFree( pszFormatString );
    pszFormatString = CPLStrdup( pszFormat );

/* -------------------------------------------------------------------- */
/*      These values will likely be used.                               */
/* -------------------------------------------------------------------- */
    if( pszFormatString[1] == '(' )
    {
        nFormatWidth = atoi(pszFormatString+2);
        bIsVariable = nFormatWidth == 0;
    }
    else
        bIsVariable = TRUE;
    
/* -------------------------------------------------------------------- */
/*      Interpret the format string.                                    */
/* -------------------------------------------------------------------- */
    switch( pszFormatString[0] )
    {
      case 'A':
      case 'C':		// It isn't clear to me how this is different than 'A'
        eType = DDFString;
        break;

      case 'R':
        eType = DDFFloat;
        break;
        
      case 'I':
      case 'S':
        eType = DDFInt;
        break;

      case 'B':
      case 'b':
        // Is the width expressed in bits? (is it a bitstring)
        bIsVariable = FALSE;
        if( pszFormatString[1] == '(' )
        {
            CPLAssert( atoi(pszFormatString+2) % 8 == 0 );
            
            nFormatWidth = atoi(pszFormatString+2) / 8;
            eBinaryFormat = SInt; // good default, works for SDTS.

            eType = DDFInt;
        }
        
        // or do we have a binary type indicator? (is it binary)
        else
        {
            eBinaryFormat = (DDFBinaryFormat) (pszFormatString[1] - '0');
            nFormatWidth = atoi(pszFormatString+2);

            if( eBinaryFormat == SInt || eBinaryFormat == UInt )
                eType = DDFInt;
            else
                eType = DDFFloat;
        }
        break;

      case 'X':
        // 'X' is extra space, and shouldn't be directly assigned to a
        // subfield ... I haven't encountered it in use yet though.
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Format type of `%c' not supported.\n",
                  pszFormatString[0] );
        
        CPLAssert( FALSE );
        return FALSE;
        break;
        
      default:
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Format type of `%c' not recognised.\n",
                  pszFormatString[0] );
        
        CPLAssert( FALSE );
        return FALSE;
    }
    
    return TRUE;
}

/************************************************************************/
/*                                Dump()                                */
/************************************************************************/

/**
 * Write out subfield definition info to debugging file.
 *
 * A variety of information about this field definition is written to the
 * give debugging file handle.
 *
 * @param fp The standard io file handle to write to.  ie. stderr
 */

void DDFSubfieldDefn::Dump( FILE * fp )

{
    fprintf( fp, "    DDFSubfieldDefn:\n" );
    fprintf( fp, "        Label = `%s'\n", pszName );
    fprintf( fp, "        FormatString = `%s'\n", pszFormatString );
}

/************************************************************************/
/*                           GetDataLength()                            */
/*                                                                      */
/*      This method will scan for the end of a variable field.          */
/************************************************************************/

/**
 * Scan for the end of variable length data.  Given a pointer to the data
 * for this subfield (from within a DDFRecord) this method will return the
 * number of bytes which are data for this subfield.  The number of bytes
 * consumed as part of this field can also be fetched.  This number may
 * be one longer than the length if there is a terminator character
 * used.<p>
 *
 * This method is mainly for internal use, or for applications which
 * want the raw binary data to interpret themselves.  Otherwise use one
 * of ExtractStringData(), ExtractIntData() or ExtractFloatData().
 *
 * @param pachSourceData The pointer to the raw data for this field.  This
 * may have come from DDFRecord::GetData(), taking into account skip factors
 * over previous subfields data.
 * @param nMaxBytes The maximum number of bytes that are accessable after
 * pachSourceData.
 * @param pnConsumedBytes Pointer to an integer into which the number of
 * bytes consumed by this field should be written.  May be NULL to ignore.
 *
 * @return The number of bytes at pachSourceData which are actual data for
 * this record (not including unit, or field terminator).  
 */

int DDFSubfieldDefn::GetDataLength( const char * pachSourceData,
                                    int nMaxBytes, int * pnConsumedBytes )

{
    if( !bIsVariable )
    {
        CPLAssert( nFormatWidth <= nMaxBytes );

        if( pnConsumedBytes != NULL )
            *pnConsumedBytes = nFormatWidth;
        
        return nFormatWidth;
    }
    else
    {
        int	nLength = 0;
        
        while( nLength < nMaxBytes
               && pachSourceData[nLength] != chFormatDelimeter 
               && pachSourceData[nLength] != DDF_FIELD_TERMINATOR )
        {
            nLength++;
        }

        if( pnConsumedBytes != NULL )
        {
            if( nMaxBytes == 0 )
                *pnConsumedBytes = nLength;
            else
                *pnConsumedBytes = nLength+1;
        }
        
        return nLength;
    }
}

/************************************************************************/
/*                         ExtractStringData()                          */
/************************************************************************/

/**
 * Extract a zero terminated string containing the data for this subfield.
 * Given a pointer to the data
 * for this subfield (from within a DDFRecord) this method will return the
 * data for this subfield.  The number of bytes
 * consumed as part of this field can also be fetched.  This number may
 * be one longer than the string length if there is a terminator character
 * used.<p>
 *
 * @param pachSourceData The pointer to the raw data for this field.  This
 * may have come from DDFRecord::GetData(), taking into account skip factors
 * over previous subfields data.
 * @param nMaxBytes The maximum number of bytes that are accessable after
 * pachSourceData.
 * @param pnConsumedBytes Pointer to an integer into which the number of
 * bytes consumed by this field should be written.  May be NULL to ignore.
 * This is used as a skip factor to increment pachSourceData to point to the
 * next subfields data.
 *
 * @return A pointer to a buffer containing the data for this field.  The
 * returned pointer is to an internal buffer which is invalidated on the
 * next ExtractStringData() call on this DDFSubfieldDefn().  It should not
 * be freed by the application.
 *
 * @see ExtractIntData(), ExtractFloatData()
 */

const char *
DDFSubfieldDefn::ExtractStringData( const char * pachSourceData,
                                    int nMaxBytes, int * pnConsumedBytes )

{
    int		nLength = GetDataLength( pachSourceData, nMaxBytes,
                                         pnConsumedBytes );

/* -------------------------------------------------------------------- */
/*      Do we need to grow the buffer.                                  */
/* -------------------------------------------------------------------- */
    if( nMaxBufChars < nLength+1 )
    {
        CPLFree( pachBuffer );
        
        nMaxBufChars = nLength+1;
        pachBuffer = (char *) CPLMalloc(nMaxBufChars);
    }

/* -------------------------------------------------------------------- */
/*      Copy the data to the buffer.  We use memcpy() so that it        */
/*      will work for binary data.                                      */
/* -------------------------------------------------------------------- */
    memcpy( pachBuffer, pachSourceData, nLength );
    pachBuffer[nLength] = '\0';

    return pachBuffer;
}

/************************************************************************/
/*                          ExtractFloatData()                          */
/************************************************************************/

/**
 * Extract a subfield value as a float.  Given a pointer to the data
 * for this subfield (from within a DDFRecord) this method will return the
 * floating point data for this subfield.  The number of bytes
 * consumed as part of this field can also be fetched.  This method may be
 * called for any type of subfield, and will return zero if the subfield is
 * not numeric.
 *
 * @param pachSourceData The pointer to the raw data for this field.  This
 * may have come from DDFRecord::GetData(), taking into account skip factors
 * over previous subfields data.
 * @param nMaxBytes The maximum number of bytes that are accessable after
 * pachSourceData.
 * @param pnConsumedBytes Pointer to an integer into which the number of
 * bytes consumed by this field should be written.  May be NULL to ignore.
 * This is used as a skip factor to increment pachSourceData to point to the
 * next subfields data.
 *
 * @return The subfield's numeric value (or zero if it isn't numeric).
 *
 * @see ExtractIntData(), ExtractStringData()
 */

double
DDFSubfieldDefn::ExtractFloatData( const char * pachSourceData,
                                   int nMaxBytes, int * pnConsumedBytes )

{
    switch( pszFormatString[0] )
    {
      case 'A':
      case 'I':
      case 'R':
      case 'S':
      case 'C':
        return atof(ExtractStringData(pachSourceData, nMaxBytes,
                                      pnConsumedBytes));

      case 'B':
      case 'b':
        unsigned char	abyData[8];

        CPLAssert( nFormatWidth <= nMaxBytes );
        if( pnConsumedBytes != NULL )
            *pnConsumedBytes = nFormatWidth;

        // Byte swap the data if it isn't in machine native format.
        // In any event we copy it into our buffer to ensure it is
        // word aligned.
#ifdef CPL_LSB
        if( pszFormatString[0] == 'B' )
#else            
        if( pszFormatString[0] == 'b' )
#endif            
        {
            for( int i = 0; i < nFormatWidth; i++ )
                abyData[nFormatWidth-i-1] = pachSourceData[i];
        }
        else
        {
            memcpy( abyData, pachSourceData, nFormatWidth );
        }

        // Interpret the bytes of data.
        switch( eBinaryFormat )
        {
          case UInt:
            if( nFormatWidth == 1 )
                return( abyData[0] );
            else if( nFormatWidth == 2 )
                return( *((GUInt16 *) abyData) );
            else if( nFormatWidth == 4 )
                return( *((GUInt32 *) abyData) );
            else
            {
                CPLAssert( FALSE );
                return 0.0;
            }
            
          case SInt:
            if( nFormatWidth == 1 )
                return( *((signed char *) abyData) );
            else if( nFormatWidth == 2 )
                return( *((GInt16 *) abyData) );
            else if( nFormatWidth == 4 )
                return( *((GInt32 *) abyData) );
            else
            {
                CPLAssert( FALSE );
                return 0.0;
            }
            
          case FloatReal:
            if( nFormatWidth == 4 )
                return( *((float *) abyData) );
            else if( nFormatWidth == 8 )
                return( *((double *) abyData) );
            else
            {
                CPLAssert( FALSE );
                return 0.0;
            }

          case NotBinary:            
          case FPReal:
          case FloatComplex:
            CPLAssert( FALSE );
            return 0.0;
        }
        break;
        // end of 'b'/'B' case.

      default:
        CPLAssert( FALSE );
        return 0.0;
    }

    CPLAssert( FALSE );
    return 0.0;
}

/************************************************************************/
/*                           ExtractIntData()                           */
/************************************************************************/

/**
 * Extract a subfield value as an integer.  Given a pointer to the data
 * for this subfield (from within a DDFRecord) this method will return the
 * int data for this subfield.  The number of bytes
 * consumed as part of this field can also be fetched.  This method may be
 * called for any type of subfield, and will return zero if the subfield is
 * not numeric.
 *
 * @param pachSourceData The pointer to the raw data for this field.  This
 * may have come from DDFRecord::GetData(), taking into account skip factors
 * over previous subfields data.
 * @param nMaxBytes The maximum number of bytes that are accessable after
 * pachSourceData.
 * @param pnConsumedBytes Pointer to an integer into which the number of
 * bytes consumed by this field should be written.  May be NULL to ignore.
 * This is used as a skip factor to increment pachSourceData to point to the
 * next subfields data.
 *
 * @return The subfield's numeric value (or zero if it isn't numeric).
 *
 * @see ExtractFloatData(), ExtractStringData()
 */

int
DDFSubfieldDefn::ExtractIntData( const char * pachSourceData,
                                 int nMaxBytes, int * pnConsumedBytes )

{
    switch( pszFormatString[0] )
    {
      case 'A':
      case 'I':
      case 'R':
      case 'S':
      case 'C':
        return atoi(ExtractStringData(pachSourceData, nMaxBytes,
                                      pnConsumedBytes));

      case 'B':
      case 'b':
        unsigned char	abyData[8];

        CPLAssert( nFormatWidth <= nMaxBytes );
        if( pnConsumedBytes != NULL )
            *pnConsumedBytes = nFormatWidth;

        // Byte swap the data if it isn't in machine native format.
        // In any event we copy it into our buffer to ensure it is
        // word aligned.
#ifdef CPL_LSB
        if( pszFormatString[0] == 'B' )
#else            
        if( pszFormatString[0] == 'b' )
#endif            
        {
            for( int i = 0; i < nFormatWidth; i++ )
                abyData[nFormatWidth-i-1] = pachSourceData[i];
        }
        else
        {
            memcpy( abyData, pachSourceData, nFormatWidth );
        }

        // Interpret the bytes of data.
        switch( eBinaryFormat )
        {
          case UInt:
            if( nFormatWidth == 1 )
                return( abyData[0] );
            else if( nFormatWidth == 2 )
                return( *((GUInt16 *) abyData) );
            else if( nFormatWidth == 4 )
                return( (int) *((GUInt32 *) abyData) );
            else
            {
                CPLAssert( FALSE );
                return 0;
            }
            
          case SInt:
            if( nFormatWidth == 1 )
                return( *((signed char *) abyData) );
            else if( nFormatWidth == 2 )
                return( *((GInt16 *) abyData) );
            else if( nFormatWidth == 4 )
                return( *((GInt32 *) abyData) );
            else
            {
                CPLAssert( FALSE );
                return 0;
            }
            
          case FloatReal:
            if( nFormatWidth == 4 )
                return( (int) *((float *) abyData) );
            else if( nFormatWidth == 8 )
                return( (int) *((double *) abyData) );
            else
            {
                CPLAssert( FALSE );
                return 0;
            }

          case NotBinary:            
          case FPReal:
          case FloatComplex:
            CPLAssert( FALSE );
            return 0;
        }
        break;
        // end of 'b'/'B' case.

      default:
        CPLAssert( FALSE );
        return 0;
    }

    CPLAssert( FALSE );
    return 0;
}

/************************************************************************/
/*                              DumpData()                              */
/*                                                                      */
/*      Dump the instance data for this subfield from a data            */
/*      record.  This fits into the output dump stream of a DDFField.   */
/************************************************************************/

/**
 * Dump subfield value to debugging file.
 *
 * @param pachData Pointer to data for this subfield.
 * @param nMaxBytes Maximum number of bytes available in pachData.
 * @param fp File to write report to.
 */

void DDFSubfieldDefn::DumpData( const char * pachData, int nMaxBytes,
                                FILE * fp )

{
    if( eType == DDFFloat )
        fprintf( fp, "      Subfield `%s' = %f\n",
                 pszName,
                 ExtractFloatData( pachData, nMaxBytes, NULL ) );
    else if( eType == DDFInt )
        fprintf( fp, "      Subfield `%s' = %d\n",
                 pszName,
                 ExtractIntData( pachData, nMaxBytes, NULL ) );
    else
        fprintf( fp, "      Subfield `%s' = `%s'\n",
                 pszName,
                 ExtractStringData( pachData, nMaxBytes, NULL ) );
}
