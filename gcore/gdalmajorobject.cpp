/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Base class for objects with metadata, etc.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
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
 * Revision 1.4  2001/09/25 15:56:37  warmerda
 * implemented rest of C entry points for metadata, document C++ methods
 *
 * Revision 1.3  2001/07/18 04:04:30  warmerda
 * added CPL_CVSID
 *
 * Revision 1.2  2000/06/26 15:26:21  warmerda
 * added GDALGetDescription
 *
 * Revision 1.1  2000/04/20 20:52:03  warmerda
 * New
 *
 */

#include "gdal_priv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                          GDALMajorObject()                           */
/************************************************************************/

GDALMajorObject::GDALMajorObject()

{
    pszDescription = NULL;
    papszMetadata = NULL;
}

/************************************************************************/
/*                          ~GDALMajorObject()                          */
/************************************************************************/

GDALMajorObject::~GDALMajorObject()

{
    CPLFree( pszDescription );
    CSLDestroy( papszMetadata );
}

/************************************************************************/
/*                           GetDescription()                           */
/************************************************************************/

/**
 * Fetch object description. 
 *
 * The semantics of the returned description are specific to the derived
 * type.  For GDALDatasets it is the dataset name, as could be (or perhaps
 * was) used with GDALOpen(). 
 * 
 * @return pointer to internal description string.
 */

const char *GDALMajorObject::GetDescription() const

{
    if( pszDescription == NULL )
        return "";
    else
        return pszDescription;
}

/************************************************************************/
/*                         GDALGetDescription()                         */
/************************************************************************/

const char *GDALGetDescription( GDALMajorObjectH hObject )

{
    return ((GDALMajorObject *) hObject)->GetDescription();
}

/************************************************************************/
/*                           SetDescription()                           */
/************************************************************************/

void GDALMajorObject::SetDescription( const char * pszNewDesc ) 

{
    CPLFree( pszDescription );
    pszDescription = CPLStrdup( pszNewDesc );
}

/************************************************************************/
/*                            GetMetadata()                             */
/************************************************************************/

/**
 * Fetch metadata.
 *
 * The returned string list is owned by the object, and may change at
 * any time.  It is formated as a "Name=value" list with the last pointer
 * value being NULL.  Use the the CPL StringList functions such as 
 * CSLFetchNameValue() to manipulate it. 
 *
 * Note that relatively few formats return any metadata at this time. 
 *
 * This method does the same thing as the C function GDALGetMetadata().
 *
 * @param pszDomain the domain of interest.  Use "" or NULL for the default
 * domain.
 * 
 * @return NULL or a string list. 
 */

char **GDALMajorObject::GetMetadata( const char * pszDomain )

{
    if( pszDomain == NULL || EQUAL(pszDomain,"") )
        return papszMetadata;
    else
        return NULL;
}

/************************************************************************/
/*                          GDALGetMetadata()                           */
/************************************************************************/

char **GDALGetMetadata( GDALMajorObjectH hObject, const char * pszDomain )

{
    return ((GDALMajorObject *) hObject)->GetMetadata(pszDomain);
}

/************************************************************************/
/*                            SetMetadata()                             */
/************************************************************************/

/** 
 * Set metadata. 
 *
 * The C function GDALSetMetadata() does the same thing as this method.
 *
 * @param papszMetadata the metadata in name=value string list format to 
 * apply.  
 * @param pszDomain the domain of interest.  Use "" or NULL for the default
 * domain. 
 * @return CE_None on success, CE_Failure on failure and CE_Warning if the
 * metadata has been accepted, but is likely not maintained persistently 
 * by the underlying object between sessions.
 */

CPLErr GDALMajorObject::SetMetadata( char ** papszMetadataIn, 
                                     const char * pszDomain )

{
    if( pszDomain != NULL && !EQUAL(pszDomain,"") )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "Non-default domain not supported for this object." );
        return CE_Failure;
    }

    CSLDestroy( papszMetadata );
    papszMetadata = CSLDuplicate( papszMetadataIn );
    
    return CE_None;
}

/************************************************************************/
/*                          GDALSetMetadata()                           */
/************************************************************************/

CPLErr GDALSetMetadata( GDALMajorObjectH hObject, char **papszMD, 
                        const char *pszDomain )

{
    return ((GDALMajorObject *) hObject)->SetMetadata( papszMD, pszDomain );
}


/************************************************************************/
/*                          GetMetadataItem()                           */
/************************************************************************/

/**
 * Fetch single metadata item.
 *
 * The C function GDALGetMetadataItem() does the same thing as this method.
 *
 * @param pszName the key for the metadata item to fetch.
 * @param pszDomain the domain to fetch for, use NULL for the default domain.
 *
 * @return NULL on failure to find the key, or a pointer to an internal
 * copy of the value string on success.
 */

const char *GDALMajorObject::GetMetadataItem( const char * pszName, 
                                              const char * pszDomain )

{
    char  **papszMD = GetMetadata( pszDomain );
    
    return CSLFetchNameValue( papszMD, pszName );
}

/************************************************************************/
/*                        GDALGetMetadataItem()                         */
/************************************************************************/

const char *GDALGetMetadataItem( GDALMajorObjectH hObject, 
                                 const char *pszName, 
                                 const char *pszDomain )

{
    return ((GDALMajorObject *) hObject)->GetMetadataItem( pszName, pszDomain);
}

/************************************************************************/
/*                          SetMetadataItem()                           */
/************************************************************************/

/**
 * Set single metadata item.
 *
 * The C function GDALSetMetadataItem() does the same thing as this method.
 *
 * @param pszName the key for the metadata item to fetch.
 * @param pszValue the value to assign to the key.
 * @param pszDomain the domain to set within, use NULL for the default domain.
 *
 * @return CE_None on success, or an error code on failure.
 */

CPLErr GDALMajorObject::SetMetadataItem( const char * pszName, 
                                         const char * pszValue, 
                                         const char * pszDomain )

{
    if( pszDomain != NULL && !EQUAL(pszDomain,"") )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "Non-default domain not supported for this object." );
        return CE_Failure;
    }

    papszMetadata = CSLSetNameValue( papszMetadata, pszName, pszValue );

    return CE_None;
}

/************************************************************************/
/*                        GDALSetMetadataItem()                         */
/************************************************************************/

CPLErr GDALSetMetadataItem( GDALMajorObjectH hObject, 
                            const char *pszName, const char *pszValue, 
                            const char *pszDomain )

{
    return ((GDALMajorObject *) hObject)->SetMetadataItem( pszName, pszValue,
                                                           pszDomain );
}
