/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  The OGRFeature class implementation.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999,  Les Technologies SoftMap Inc.
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
 * Revision 1.7  1999/08/30 16:33:51  warmerda
 * Use provided formatting in GetAsString() for real fields.
 *
 * Revision 1.6  1999/08/30 14:52:33  warmerda
 * added support for StringList fields
 *
 * Revision 1.5  1999/08/26 17:38:01  warmerda
 * added support for real and integer lists
 *
 * Revision 1.4  1999/07/27 01:52:31  warmerda
 * added fid to readable dump of feature
 *
 * Revision 1.3  1999/07/27 00:48:34  warmerda
 * added fid, and Equal() support
 *
 * Revision 1.2  1999/07/05 17:19:52  warmerda
 * added docs
 *
 * Revision 1.1  1999/06/11 19:21:02  warmerda
 * New
 *
 */

#include "ogr_feature.h"
#include "ogr_p.h"

/************************************************************************/
/*                             OGRFeature()                             */
/************************************************************************/

/**
 * Constructor
 *
 * Note that the OGRFeature will increment the reference count of it's
 * defining OGRFeatureDefn.  Destruction of the OGRFeatureDefn before
 * destruction of all OGRFeatures that depend on it is likely to result in
 * a crash. 
 *
 * @param poDefnIn feature class (layer) definition to which the feature will
 * adhere.
 */

OGRFeature::OGRFeature( OGRFeatureDefn * poDefnIn )

{
    poDefnIn->Reference();
    poDefn = poDefnIn;

    nFID = OGRNullFID;
    
    poGeometry = NULL;

    // we should likely be initializing from the defaults, but this will
    // usually be a waste. 
    pauFields = (OGRField *) CPLCalloc( poDefn->GetFieldCount(),
                                        sizeof(OGRField) );
}

/************************************************************************/
/*                            ~OGRFeature()                             */
/************************************************************************/

OGRFeature::~OGRFeature()

{
    poDefn->Dereference();

    if( poGeometry != NULL )
        delete poGeometry;

    for( int i = 0; i < poDefn->GetFieldCount(); i++ )
    {
        OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn(i);

        switch( poFDefn->GetType() )
        {
          case OFTString:
            if( pauFields[i].String != NULL )
                CPLFree( pauFields[i].String );
            break;

          case OFTStringList:
            CSLDestroy( pauFields[i].StringList.paList );
            break;

          case OFTIntegerList:
          case OFTRealList:
            CPLFree( pauFields[i].IntegerList.paList );
            break;

          default:
            // should add support for string lists, and wide strings.
            break;
        }
    }
    
    CPLFree( pauFields );
}

/************************************************************************/
/*                             GetDefnRef()                             */
/************************************************************************/

/**
 * \fn OGRFeatureDefn *OGRFeature::GetDefnRef();
 *
 * Fetch feature definition.
 *
 * @return a reference to the feature definition object.
 */

/************************************************************************/
/*                        SetGeometryDirectly()                         */
/************************************************************************/

/**
 * Set feature geometry.
 *
 * This method updates the features geometry, and operate exactly as
 * SetGeometry(), except that this method assumes ownership of the
 * passed geometry.
 *
 * @param poGeomIn new geometry to apply to feature.
 *
 * @return OGRERR_NONE if successful, or OGR_UNSUPPORTED_GEOMETRY_TYPE if
 * the geometry type is illegal for the OGRFeatureDefn (checking not yet
 * implemented). 
 */ 

OGRErr OGRFeature::SetGeometryDirectly( OGRGeometry * poGeomIn )

{
    if( poGeometry != NULL )
        delete poGeometry;

    poGeometry = poGeomIn;

    // I should be verifying that the geometry matches the defn's type.
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                            SetGeometry()                             */
/************************************************************************/

/**
 * Set feature geometry.
 *
 * This method updates the features geometry, and operate exactly as
 * SetGeometryDirectly(), except that this method does not assume ownership
 * of the passed geometry, but instead makes a copy of it. 
 *
 * @param poGeomIn new geometry to apply to feature.
 *
 * @return OGRERR_NONE if successful, or OGR_UNSUPPORTED_GEOMETRY_TYPE if
 * the geometry type is illegal for the OGRFeatureDefn (checking not yet
 * implemented). 
 */ 

OGRErr OGRFeature::SetGeometry( OGRGeometry * poGeomIn )

{
    if( poGeometry != NULL )
        delete poGeometry;

    poGeometry = poGeomIn->clone();

    // I should be verifying that the geometry matches the defn's type.
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                           GetGeometryRef()                           */
/************************************************************************/

/**
 * \fn OGRGeometry *OGRFeature::GetGeometryRef();
 *
 * Fetch pointer to feature geometry.
 *
 * @return pointer to internal feature geometry.  This object should
 * not be modified.
 */

/************************************************************************/
/*                               Clone()                                */
/************************************************************************/

/**
 * Duplicate feature.
 *
 * The newly created feature is owned by the caller, and will have it's own
 * reference to the OGRFeatureDefn.
 *
 * @return new feature, exactly matching this feature.
 */

OGRFeature *OGRFeature::Clone()

{
    OGRFeature	*poNew = new OGRFeature( poDefn );

    poNew->SetGeometry( poGeometry );

    for( int i = 0; i < poDefn->GetFieldCount(); i++ )
    {
        poNew->SetField( i, pauFields + i );
    }

    return poNew;
}

/************************************************************************/
/*                           GetFieldCount()                            */
/************************************************************************/

/**
 * \fn int OGRFeature::GetFieldCount();
 *
 * Fetch number of fields on this feature.  This will always be the same
 * as the field count for the OGRFeatureDefn.
 *
 * @return count of fields.
 */

/************************************************************************/
/*                          GetFieldDefnRef()                           */
/************************************************************************/

/**
 * \fn OGRFieldDefn *OGRFeature::GetFieldDefnRef( int iField );
 *
 * Fetch definition for this field.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field definition (from the OGRFeatureDefn).  This is an
 * internal reference, and should not be deleted or modified.
 */

/************************************************************************/
/*                           GetFieldIndex()                            */
/************************************************************************/

/**
 * \fn int OGRFeature::GetFieldIndex( const char * pszName );
 * 
 * Fetch the field index given field name.
 *
 * This is a cover for the OGRFeatureDefn::GetFieldIndex() method. 
 *
 * @param pszName the name of the field to search for. 
 *
 * @return the field index, or -1 if no matching field is found.
 */

/************************************************************************/
/*                           GetRawFieldRef()                           */
/************************************************************************/

/**
 * \fn OGRField *OGRFeature::GetRawFieldRef( int iField );
 *
 * Fetch a pointer to the internal field value given the index.  
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the returned pointer is to an internal data structure, and should
 * not be freed, or modified. 
 */

/************************************************************************/
/*                         GetFieldAsInteger()                          */
/************************************************************************/

/**
 * Fetch field value as integer.
 *
 * OFTString features will be translated using atoi().  OFTReal fields
 * will be cast to integer.   Other field types, or errors will result in
 * a return value of zero.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.
 */

int OGRFeature::GetFieldAsInteger( int iField )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );
    
    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTInteger )
        return pauFields[iField].Integer;
    else if( poFDefn->GetType() == OFTReal )
        return (int) pauFields[iField].Real;
    else if( poFDefn->GetType() == OFTString )
    {
        if( pauFields[iField].String == NULL )
            return 0;
        else
            return atoi(pauFields[iField].String);
    }
    else
        return 0;
}

/************************************************************************/
/*                          GetFieldAsDouble()                          */
/************************************************************************/

/**
 * Fetch field value as a double.
 *
 * OFTString features will be translated using atof().  OFTInteger fields
 * will be cast to double.   Other field types, or errors will result in
 * a return value of zero.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.
 */

double OGRFeature::GetFieldAsDouble( int iField )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );
    
    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTReal )
        return pauFields[iField].Real;
    else if( poFDefn->GetType() == OFTInteger )
        return pauFields[iField].Integer;
    else if( poFDefn->GetType() == OFTString )
    {
        if( pauFields[iField].String == NULL )
            return 0;
        else
            return atof(pauFields[iField].String);
    }
    else
        return 0.0;
}

/************************************************************************/
/*                          GetFieldAsString()                          */
/************************************************************************/

/**
 * Fetch field value as a string.
 *
 * OFTReal and OFTInteger fields will be translated to string using
 * sprintf(), but not necessarily using the established formatting rules.
 * Other field types, or errors will result in a return value of zero.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.  This string is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief. 
 */

const char *OGRFeature::GetFieldAsString( int iField )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );
    static char		szTempBuffer[80];

    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTString )
    {
        if( pauFields[iField].String == NULL )
            return "";
        else
            return pauFields[iField].String;
    }
    else if( poFDefn->GetType() == OFTInteger )
    {
        sprintf( szTempBuffer, "%d", pauFields[iField].Integer );
        return szTempBuffer;
    }
    else if( poFDefn->GetType() == OFTReal )
    {
        char	szFormat[64];

        if( poFDefn->GetWidth() != 0 )
        {
            sprintf( szFormat, "%%%d.%df",
                     poFDefn->GetWidth(), poFDefn->GetPrecision() );
        }
        else
            strcpy( szFormat, "%g" );
        
        sprintf( szTempBuffer, szFormat, pauFields[iField].Real );
        
        return szTempBuffer;
    }
    else if( poFDefn->GetType() == OFTIntegerList )
    {
        char	szItem[32];
        int	i, nCount = pauFields[iField].IntegerList.nCount;

        sprintf( szTempBuffer, "(%d:", nCount );
        for( i = 0; i < nCount; i++ )
        {
            sprintf( szItem, "%d", pauFields[iField].IntegerList.paList[i] );
            if( strlen(szTempBuffer) + strlen(szItem) + 6
                > sizeof(szTempBuffer) )
            {
                break;
            }
            
            if( i > 0 )
                strcat( szTempBuffer, "," );
            
            strcat( szTempBuffer, szItem );
        }

        if( i < nCount )
            strcat( szTempBuffer, ",...)" );
        else
            strcat( szTempBuffer, ")" );
        
        return szTempBuffer;
    }
    else if( poFDefn->GetType() == OFTRealList )
    {
        char	szItem[40];
        char	szFormat[64];
        int	i, nCount = pauFields[iField].RealList.nCount;

        if( poFDefn->GetWidth() != 0 )
        {
            sprintf( szFormat, "%%%d.%df",
                     poFDefn->GetWidth(), poFDefn->GetPrecision() );
        }
        else
            strcpy( szFormat, "%g" );
        
        sprintf( szTempBuffer, "(%d:", nCount );
        for( i = 0; i < nCount; i++ )
        {
            sprintf( szItem, szFormat, pauFields[iField].RealList.paList[i] );
            if( strlen(szTempBuffer) + strlen(szItem) + 6
                > sizeof(szTempBuffer) )
            {
                break;
            }
            
            if( i > 0 )
                strcat( szTempBuffer, "," );
            
            strcat( szTempBuffer, szItem );
        }

        if( i < nCount )
            strcat( szTempBuffer, ",...)" );
        else
            strcat( szTempBuffer, ")" );
        
        return szTempBuffer;
    }
    else if( poFDefn->GetType() == OFTStringList )
    {
        int	i, nCount = pauFields[iField].StringList.nCount;

        sprintf( szTempBuffer, "(%d:", nCount );
        for( i = 0; i < nCount; i++ )
        {
            const char	*pszItem = pauFields[iField].StringList.paList[i];
            
            if( strlen(szTempBuffer) + strlen(pszItem)  + 6
                > sizeof(szTempBuffer) )
            {
                break;
            }

            if( i > 0 )
                strcat( szTempBuffer, "," );
            
            strcat( szTempBuffer, pszItem );
        }

        if( i < nCount )
            strcat( szTempBuffer, ",...)" );
        else
            strcat( szTempBuffer, ")" );
        
        return szTempBuffer;
    }
    else
        return "";
}

/************************************************************************/
/*                       GetFieldAsIntegerList()                        */
/************************************************************************/

/**
 * Fetch field value as a list of integers.
 *
 * Currently this method only works for OFTIntegerList fields.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param pnCount an integer to put the list count (number of integers) into.
 *
 * @return the field value.  This list is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.  If *pnCount is zero
 * on return the returned pointer may be NULL or non-NULL.
 */

const int *OGRFeature::GetFieldAsIntegerList( int iField, int *pnCount )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTIntegerList )
    {
        if( pnCount != NULL )
            *pnCount = pauFields[iField].IntegerList.nCount;

        return pauFields[iField].IntegerList.paList;
    }
    else
    {
        if( pnCount != NULL )
            *pnCount = 0;
        
        return NULL;
    }
}

/************************************************************************/
/*                        GetFieldAsDoubleList()                        */
/************************************************************************/

/**
 * Fetch field value as a list of doubles.
 *
 * Currently this method only works for OFTRealList fields.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param pnCount an integer to put the list count (number of doubles) into.
 *
 * @return the field value.  This list is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.  If *pnCount is zero
 * on return the returned pointer may be NULL or non-NULL.
 */

const double *OGRFeature::GetFieldAsDoubleList( int iField, int *pnCount )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTRealList )
    {
        if( pnCount != NULL )
            *pnCount = pauFields[iField].RealList.nCount;

        return pauFields[iField].RealList.paList;
    }
    else
    {
        if( pnCount != NULL )
            *pnCount = 0;
        
        return NULL;
    }
}

/************************************************************************/
/*                        GetFieldAsStringList()                        */
/************************************************************************/

/**
 * Fetch field value as a list of strings.
 *
 * Currently this method only works for OFTStringList fields.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.  This list is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.
 */

char **OGRFeature::GetFieldAsStringList( int iField )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTStringList )
    {
        return pauFields[iField].StringList.paList;
    }
    else
    {
        return NULL;
    }
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to integer value. 
 *
 * OFTInteger and OFTReal fields will be set directly.  OFTString fields
 * will be assigned a string representation of the value, but not necessarily
 * taking into account formatting constraints on this field.  Other field
 * types may be unaffected.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param nValue the value to assign.
 */

void OGRFeature::SetField( int iField, int nValue )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTInteger )
    {
        pauFields[iField].Integer = nValue;
    }
    else if( poFDefn->GetType() == OFTReal )
    {
        pauFields[iField].Real = nValue;
    }
    else if( poFDefn->GetType() == OFTString )
    {
        char	szTempBuffer[64];

        sprintf( szTempBuffer, "%d", nValue );

        CPLFree( pauFields[iField].String );
        pauFields[iField].String = CPLStrdup( szTempBuffer );
    }
    else
        /* do nothing for other field types */;
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to double value. 
 *
 * OFTInteger and OFTReal fields will be set directly.  OFTString fields
 * will be assigned a string representation of the value, but not necessarily
 * taking into account formatting constraints on this field.  Other field
 * types may be unaffected.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param dfValue the value to assign.
 */

void OGRFeature::SetField( int iField, double dfValue )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTReal )
    {
        pauFields[iField].Real = dfValue;
    }
    else if( poFDefn->GetType() == OFTInteger )
    {
        pauFields[iField].Integer = (int) dfValue;
    }
    else if( poFDefn->GetType() == OFTString )
    {
        char	szTempBuffer[128];

        sprintf( szTempBuffer, "%g", dfValue );

        CPLFree( pauFields[iField].String );
        pauFields[iField].String = CPLStrdup( szTempBuffer );
    }
    else
        /* do nothing for other field types */;
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to string value. 
 *
 * OFTInteger fields will be set based on an atoi() conversion of the string.
 * OFTReal fields will be set based on an atof() conversion of the string.
 * Other field types may be unaffected.
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param pszValue the value to assign.
 */

void OGRFeature::SetField( int iField, const char * pszValue )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTString )
    {
        CPLFree( pauFields[iField].String );
        pauFields[iField].String = CPLStrdup( pszValue );
    }
    else if( poFDefn->GetType() == OFTInteger )
    {
        pauFields[iField].Integer = atoi(pszValue);
    }
    else if( poFDefn->GetType() == OFTReal )
    {
        pauFields[iField].Real = atof(pszValue);
    }
    else
        /* do nothing for other field types */;
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to list of integers value. 
 *
 * This method currently on has an effect of OFTIntegerList fields.
 *
 * @param iField the field to set, from 0 to GetFieldCount()-1.
 * @param nCount the number of values in the list being assigned.
 * @param panValues the values to assign.
 */

void OGRFeature::SetField( int iField, int nCount, int *panValues )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    if( poFDefn->GetType() == OFTIntegerList )
    {
        OGRField	uField;

        uField.IntegerList.nCount = nCount;
        uField.IntegerList.paList = panValues;

        SetField( iField, &uField );
    }
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to list of doubles value. 
 *
 * This method currently on has an effect of OFTRealList fields.
 *
 * @param iField the field to set, from 0 to GetFieldCount()-1.
 * @param nCount the number of values in the list being assigned.
 * @param padfValues the values to assign.
 */

void OGRFeature::SetField( int iField, int nCount, double * padfValues )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    if( poFDefn->GetType() == OFTRealList )
    {
        OGRField	uField;
        
        uField.RealList.nCount = nCount;
        uField.RealList.paList = padfValues;
        
        SetField( iField, &uField );
    }
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to list of strings value. 
 *
 * This method currently on has an effect of OFTStringList fields.
 *
 * @param iField the field to set, from 0 to GetFieldCount()-1.
 * @param papszValues the values to assign.
 */

void OGRFeature::SetField( int iField, char ** papszValues )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    if( poFDefn->GetType() == OFTStringList )
    {
        OGRField	uField;
        
        uField.StringList.nCount = CSLCount(papszValues);
        uField.StringList.paList = papszValues;
        
        SetField( iField, &uField );
    }
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field.
 *
 * The passed value OGRField must be of exactly the same type as the
 * target field, or an application crash may occur.  The passed value
 * is copied, and will not be affected.  It remains the responsibility of
 * the caller. 
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param puValue the value to assign.
 */

void OGRFeature::SetField( int iField, OGRField * puValue )

{
    OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL );
    
    if( poFDefn->GetType() == OFTInteger )
    {
        pauFields[iField].Integer = puValue->Integer;
    }
    else if( poFDefn->GetType() == OFTReal )
    {
        pauFields[iField].Real = puValue->Real;
    }
    else if( poFDefn->GetType() == OFTString )
    {
        CPLFree( pauFields[iField].String );
        pauFields[iField].String = CPLStrdup( puValue->String );
    }
    else if( poFDefn->GetType() == OFTIntegerList )
    {
        int	nCount = puValue->IntegerList.nCount;
        
        CPLFree( pauFields[iField].IntegerList.paList );
        pauFields[iField].IntegerList.paList =
            (int *) CPLMalloc(sizeof(int) * nCount);
        memcpy( pauFields[iField].IntegerList.paList,
                puValue->IntegerList.paList,
                sizeof(int) * nCount );
        pauFields[iField].IntegerList.nCount = nCount;
    }
    else if( poFDefn->GetType() == OFTRealList )
    {
        int	nCount = puValue->RealList.nCount;
        
        CPLFree( pauFields[iField].RealList.paList );
        pauFields[iField].RealList.paList =
            (double *) CPLMalloc(sizeof(double) * nCount);
        memcpy( pauFields[iField].RealList.paList,
                puValue->RealList.paList,
                sizeof(double) * nCount );
        pauFields[iField].RealList.nCount = nCount;
    }
    else if( poFDefn->GetType() == OFTStringList )
    {
        CSLDestroy( pauFields[iField].StringList.paList );
        pauFields[iField].StringList.paList =
            CSLDuplicate( puValue->StringList.paList );

        pauFields[iField].StringList.nCount = puValue->StringList.nCount;
        CPLAssert( CSLCount(puValue->StringList.paList)
                   == puValue->StringList.nCount );
    }
    else
        /* do nothing for other field types */;
}

/************************************************************************/
/*                            DumpReadable()                            */
/************************************************************************/

/**
 * Dump this feature in a human readable form.
 *
 * This dumps the attributes, and geometry; however, it doesn't definition
 * information (other than field types and names), nor does it report the
 * geometry spatial reference system.
 *
 * @param fpOut the stream to write to, such as strout.
 */

void OGRFeature::DumpReadable( FILE * fpOut )

{
    fprintf( fpOut, "OGRFeature(%s):%ld\n", poDefn->GetName(), GetFID() );
    for( int iField = 0; iField < GetFieldCount(); iField++ )
    {
        OGRFieldDefn	*poFDefn = poDefn->GetFieldDefn(iField);
        
        fprintf( fpOut, "  %s (%s) = %s\n",
                 poFDefn->GetNameRef(),
                 OGRFieldDefn::GetFieldTypeName(poFDefn->GetType()),
                 GetFieldAsString( iField ) );
    }
    
    if( poGeometry != NULL )
        poGeometry->dumpReadable( fpOut, "  " );

    fprintf( fpOut, "\n" );
}

/************************************************************************/
/*                               GetFID()                               */
/************************************************************************/

/**
 * \fn long OGRFeature::GetFID();
 *
 * Get feature identifier.
 *
 * @return feature id or OGRNullFID if none has been assigned.
 */

/************************************************************************/
/*                               SetFID()                               */
/************************************************************************/

/**
 * Set the feature identifier.
 *
 * For specific types of features this operation may fail on illegal
 * features ids.  Generally it always succeeds.  Feature ids should be
 * greater than or equal to zero, with the exception of OGRNullFID (-1)
 * indicating that the feature id is unknown.
 *
 * @param nFID the new feature identifier value to assign.
 *
 * @return On success OGRERR_NONE, or on failure some other value. 
 */

OGRErr OGRFeature::SetFID( long nFIDIn )

{
    nFID = nFIDIn;

    return OGRERR_NONE;
}

/************************************************************************/
/*                               Equal()                                */
/************************************************************************/

/**
 * Test if two features are the same.
 *
 * Two features are considered equal if the share them (pointer equality)
 * same OGRFeatureDefn, have the same field values, and the same geometry
 * (as tested by OGRGeometry::Equal()) as well as the same feature id.
 *
 * @param poFeature the other feature to test this one against.
 *
 * @return TRUE if they are equal, otherwise FALSE.
 */

OGRBoolean OGRFeature::Equal( OGRFeature * poFeature )

{
    if( poFeature == this )
        return TRUE;

    if( GetFID() != poFeature->GetFID() )
        return FALSE;
    
    if( GetDefnRef() != poFeature->GetDefnRef() )
        return FALSE;

    //notdef: add testing of attributes at a later date.

    if( GetGeometryRef() != NULL
        && (!GetGeometryRef()->Equal( poFeature->GetGeometryRef() ) ) )
        return FALSE;

    return TRUE;
}
