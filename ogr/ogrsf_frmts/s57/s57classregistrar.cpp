/******************************************************************************
 * $Id$
 *
 * Project:  S-57 Translator
 * Purpose:  Implements S57ClassRegistrar class for keeping track of
 *           information on S57 object classes.
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
 * Revision 1.2  1999/11/18 19:01:25  warmerda
 * expanded tabs
 *
 * Revision 1.1  1999/11/08 22:22:55  warmerda
 * New
 *
 */

#include "s57.h"
#include "cpl_conv.h"
#include "cpl_string.h"

/************************************************************************/
/*                         S57ClassRegistrar()                          */
/************************************************************************/

S57ClassRegistrar::S57ClassRegistrar()

{
    nClasses = 0;
    papszClassesInfo = NULL;
    
    iCurrentClass = -1;

    papszCurrentFields = NULL;
    papszTempResult = NULL;
}

/************************************************************************/
/*                         ~S57ClassRegistrar()                         */
/************************************************************************/

S57ClassRegistrar::~S57ClassRegistrar()

{
    CSLDestroy( papszClassesInfo );
    CSLDestroy( papszCurrentFields );
    CSLDestroy( papszTempResult );
}

/************************************************************************/
/*                              LoadInfo()                              */
/************************************************************************/

int S57ClassRegistrar::LoadInfo( const char * pszDirectory, int bReportErr )

{
    const char  *pszFilename;
    FILE        *fp;

/* ==================================================================== */
/*      Read the s57objectclasses file.                                 */
/* ==================================================================== */
/* -------------------------------------------------------------------- */
/*      Try to open the csv file.                                       */
/* -------------------------------------------------------------------- */
    pszFilename = CPLFormFilename( pszDirectory, "s57objectclasses.csv", NULL);

    fp = VSIFOpen( pszFilename, "rt" );
    if( fp == NULL )
    {
        if( bReportErr )
            CPLError( CE_Failure, CPLE_OpenFailed,
                      "Failed to open %s.\n",
                      pszFilename );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Skip the line defining the column titles.                       */
/* -------------------------------------------------------------------- */
    const char * pszLine = CPLReadLine( fp );

    if( !EQUAL(pszLine,
               "\"Code\",\"ObjectClass\",\"Acronym\",\"Attribute_A\","
               "\"Attribute_B\",\"Attribute_C\",\"Class\",\"Primitives\"" ) )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "s57objectclasses columns don't match expected format!\n" );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Read and form string list.                                      */
/* -------------------------------------------------------------------- */
    
    CSLDestroy( papszClassesInfo );
    papszClassesInfo = (char **) CPLCalloc(sizeof(char *),MAX_CLASSES);

    nClasses = 0;

    while( nClasses < MAX_CLASSES
           && (pszLine = CPLReadLine(fp)) != NULL )
    {
        papszClassesInfo[nClasses] = CPLStrdup(pszLine);
        if( papszClassesInfo[nClasses] == NULL )
            break;

        nClasses++;
    }

    if( nClasses == MAX_CLASSES )
        CPLError( CE_Warning, CPLE_AppDefined,
                  "MAX_CLASSES exceeded in S57ClassRegistrar::LoadInfo().\n" );

/* -------------------------------------------------------------------- */
/*      Cleanup, and establish state.                                   */
/* -------------------------------------------------------------------- */
    VSIFClose( fp );
    iCurrentClass = -1;

    if( nClasses == 0 )
        return FALSE;

/* ==================================================================== */
/*      Read the attributes list.                                       */
/* ==================================================================== */
    pszFilename = CPLFormFilename( pszDirectory, "s57attributes.csv", NULL);

    fp = VSIFOpen( pszFilename, "rt" );
    if( fp == NULL )
    {
        if( bReportErr )
            CPLError( CE_Failure, CPLE_OpenFailed,
                      "Failed to open %s.\n",
                      pszFilename );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Skip the line defining the column titles.                       */
/* -------------------------------------------------------------------- */
    pszLine = CPLReadLine( fp );

    if( !EQUAL(pszLine,
          "\"Code\",\"Attribute\",\"Acronym\",\"Attributetype\",\"Class\"") )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "s57attributes columns don't match expected format!\n" );
        return FALSE;
    }
    
/* -------------------------------------------------------------------- */
/*      Prepare arrays for the per-attribute information.               */
/* -------------------------------------------------------------------- */
    nAttrMax = MAX_ATTRIBUTES-1;
    papszAttrNames = (char **) CPLCalloc(sizeof(char *),nAttrMax);
    papszAttrAcronym = (char **) CPLCalloc(sizeof(char *),nAttrMax);
    papapszAttrValues = (char ***) CPLCalloc(sizeof(char **),nAttrMax);
    pachAttrType = (char *) CPLCalloc(sizeof(char),nAttrMax);
    pachAttrClass = (char *) CPLCalloc(sizeof(char),nAttrMax);
    panAttrIndex = (int *) CPLCalloc(sizeof(int),nAttrMax);
    
/* -------------------------------------------------------------------- */
/*      Read and form string list.                                      */
/* -------------------------------------------------------------------- */
    int         iAttr;
    
    while( (pszLine = CPLReadLine(fp)) != NULL )
    {
        char    **papszTokens = CSLTokenizeStringComplex( pszLine, ",",
                                                          TRUE, TRUE );

        if( CSLCount(papszTokens) < 5 )
        {
            CPLAssert( FALSE );
            continue;
        }
        
        iAttr = atoi(papszTokens[0]);
        if( iAttr < 0 || iAttr >= nAttrMax
            || papszAttrNames[iAttr] != NULL )
        {
            CPLAssert( FALSE );
            continue;
        }
        
        papszAttrNames[iAttr] = CPLStrdup(papszTokens[1]);
        papszAttrAcronym[iAttr] = CPLStrdup(papszTokens[2]);
        pachAttrType[iAttr] = papszTokens[3][0];
        pachAttrClass[iAttr] = papszTokens[4][0];

        CSLDestroy( papszTokens );
    }

    VSIFClose( fp );
    
/* -------------------------------------------------------------------- */
/*      Build unsorted index of attributes.                             */
/* -------------------------------------------------------------------- */
    nAttrCount = 0;
    for( iAttr = 0; iAttr < nAttrMax; iAttr++ )
    {
        if( papszAttrAcronym[iAttr] != NULL )
            panAttrIndex[nAttrCount++] = iAttr;
    }

/* -------------------------------------------------------------------- */
/*      Sort index by acronym.                                          */
/* -------------------------------------------------------------------- */
    int         bModified;

    do
    {
        bModified = FALSE;
        for( iAttr = 0; iAttr < nAttrCount-1; iAttr++ )
        {
            if( strcmp(papszAttrAcronym[panAttrIndex[iAttr]],
                       papszAttrAcronym[panAttrIndex[iAttr+1]]) > 0 )
            {
                int     nTemp;

                nTemp = panAttrIndex[iAttr];
                panAttrIndex[iAttr] = panAttrIndex[iAttr+1];
                panAttrIndex[iAttr+1] = nTemp;

                bModified = TRUE;
            }
        }
    } while( bModified );
    
    return TRUE;
}

/************************************************************************/
/*                         SelectClassByIndex()                         */
/************************************************************************/

int S57ClassRegistrar::SelectClassByIndex( int nNewIndex )

{
    if( nNewIndex < 0 || nNewIndex >= nClasses )
        return FALSE;

    CSLDestroy( papszCurrentFields );
    papszCurrentFields = CSLTokenizeStringComplex( papszClassesInfo[nNewIndex],
                                                   ",", TRUE, TRUE );

    iCurrentClass = nNewIndex;

    return TRUE;
}

/************************************************************************/
/*                             SelectClass()                            */
/************************************************************************/

int S57ClassRegistrar::SelectClass( int nOBJL )

{
    for( int i = 0; i < nClasses; i++ )
    {
        if( atoi(papszClassesInfo[i]) == nOBJL )
            return SelectClassByIndex( i );
    }

    return FALSE;
}

/************************************************************************/
/*                            SelectClass()                             */
/************************************************************************/

int S57ClassRegistrar::SelectClass( const char *pszAcronym )

{
    for( int i = 0; i < nClasses; i++ )
    {
        if( !SelectClassByIndex( i ) )
            continue;

        if( EQUAL(GetAcronym(),pszAcronym) )
            return TRUE;
    }

    return FALSE;
}

/************************************************************************/
/*                              GetOBJL()                               */
/************************************************************************/

int S57ClassRegistrar::GetOBJL()

{
    if( iCurrentClass >= 0 )
        return atoi(papszClassesInfo[iCurrentClass]);
    else
        return -1;
}

/************************************************************************/
/*                           GetDescription()                           */
/************************************************************************/

const char * S57ClassRegistrar::GetDescription()

{
    if( iCurrentClass >= 0
        && CSLCount(papszCurrentFields) > 1 )
        return papszCurrentFields[1];
    else
        return NULL;
}

/************************************************************************/
/*                             GetAcronym()                             */
/************************************************************************/

const char * S57ClassRegistrar::GetAcronym()

{
    if( iCurrentClass >= 0
        && CSLCount(papszCurrentFields) > 2 )
        return papszCurrentFields[2];
    else
        return NULL;
}

/************************************************************************/
/*                          GetAttributeList()                          */
/*                                                                      */
/*      The passed string can be "a", "b", "c" or NULL for all.  The    */
/*      returned list remained owned by this object, not the caller.    */
/************************************************************************/

char **S57ClassRegistrar::GetAttributeList( const char * pszType )

{
    if( iCurrentClass < 0 )
        return NULL;
    
    CSLDestroy( papszTempResult );
    papszTempResult = NULL;
    
    for( int iColumn = 3; iColumn < 6; iColumn++ )
    {
        if( pszType != NULL && iColumn == 3 && !EQUAL(pszType,"a") )
            continue;
        
        if( pszType != NULL && iColumn == 4 && !EQUAL(pszType,"b") )
            continue;
        
        if( pszType != NULL && iColumn == 5 && !EQUAL(pszType,"c") )
            continue;

        char    **papszTokens;

        papszTokens =
            CSLTokenizeStringComplex( papszCurrentFields[iColumn], ";",
                                      TRUE, FALSE );

        papszTempResult = CSLInsertStrings( papszTempResult, 0,
                                            papszTokens );

        CSLDestroy( papszTokens );
    }

    return papszTempResult;
}

/************************************************************************/
/*                            GetClassCode()                            */
/************************************************************************/

char S57ClassRegistrar::GetClassCode()

{
    if( iCurrentClass >= 0
        && CSLCount(papszCurrentFields) > 6 )
        return papszCurrentFields[6][0];
    else
        return '\0';
}

/************************************************************************/
/*                           GetPrimitives()                            */
/************************************************************************/

char **S57ClassRegistrar::GetPrimitives()

{
    if( iCurrentClass >= 0
        && CSLCount(papszCurrentFields) > 7 )
    {
        CSLDestroy( papszTempResult );
        papszTempResult = 
            CSLTokenizeStringComplex( papszCurrentFields[7], ";",
                                      TRUE, FALSE );
        return papszTempResult;
    }
    else
        return NULL;
}

/************************************************************************/
/*                         FindAttrByAcronym()                          */
/************************************************************************/

int     S57ClassRegistrar::FindAttrByAcronym( const char * pszName )

{
    int         iStart, iEnd, iCandidate;

    iStart = 0;
    iEnd = nAttrCount-1;

    while( iStart <= iEnd )
    {
        int     nCompareValue;
        
        iCandidate = (iStart + iEnd)/2;
        nCompareValue =
            strcmp( pszName, papszAttrAcronym[panAttrIndex[iCandidate]] );

        if( nCompareValue < 0 )
        {
            iEnd = iCandidate-1;
        }
        else if( nCompareValue > 0 )
        {
            iStart = iCandidate+1;
        }
        else
            return panAttrIndex[iCandidate];
    }

    return -1;
}
