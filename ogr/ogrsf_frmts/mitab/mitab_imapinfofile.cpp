/**********************************************************************
 * $Id: mitab_imapinfofile.cpp,v 1.6 2000/01/26 18:17:35 warmerda Exp $
 *
 * Name:     mitab_imapinfo
 * Project:  MapInfo mid/mif Tab Read/Write library
 * Language: C++
 * Purpose:  Implementation of the IMapInfoFile class, super class of
 *           of MIFFile and TABFile
 * Author:   Daniel Morissette, danmo@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 1999, 2000, Daniel Morissette
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log: mitab_imapinfofile.cpp,v $
 * Revision 1.6  2000/01/26 18:17:35  warmerda
 * added CreateField method
 *
 * Revision 1.5  2000/01/15 22:30:44  daniel
 * Switch to MIT/X-Consortium OpenSource license
 *
 * Revision 1.4  2000/01/11 19:06:25  daniel
 * Added support for conversion of collections in CreateFeature()
 *
 * Revision 1.3  1999/12/14 02:14:50  daniel
 * Added static SmartOpen() method + TABView support
 *
 * Revision 1.2  1999/11/08 19:15:44  stephane
 * Add headers method
 *
 * Revision 1.1  1999/11/08 04:17:27  stephane
 * First Revision
 *
 **********************************************************************/

#include "mitab.h"
#include "mitab_utils.h"

#include <ctype.h>      /* isspace() */

/**********************************************************************
 *                   IMapInfoFile::IMapInfoFile()
 *
 * Constructor.
 **********************************************************************/
IMapInfoFile::IMapInfoFile()
{
    m_poFilterGeom = NULL;    
}


/**********************************************************************
 *                   IMapInfoFile::~IMapInfoFile()
 *
 * Destructor.
 **********************************************************************/
IMapInfoFile::~IMapInfoFile()
{
    if( m_poFilterGeom != NULL )
    {
	delete m_poFilterGeom;
	m_poFilterGeom = NULL;
    }
}

/**********************************************************************
 *                   IMapInfoFile::SmartOpen()
 *
 * Use this static method to automatically open any flavour of MapInfo
 * dataset.  This method will detect the file type, create an object
 * of the right type, and open the file.
 *
 * Call GetFileClass() on the returned object if you need to find out
 * its exact type.  (To access format-specific methods for instance)
 *
 * Returns the new object ptr. , or NULL if the open failed.
 **********************************************************************/
IMapInfoFile *IMapInfoFile::SmartOpen(const char *pszFname,
                                      GBool bTestOpenNoError /*=FALSE*/)
{
    IMapInfoFile *poFile = NULL;
    int nLen = 0;

    if (pszFname)
        nLen = strlen(pszFname);

    if (nLen > 4 && (EQUAL(pszFname + nLen-4, ".MIF") ||
                     EQUAL(pszFname + nLen-4, ".MID") ) )
    {
        /*-------------------------------------------------------------
         * MIF/MID file
         *------------------------------------------------------------*/
        poFile = new MIFFile;
    }
    else if (nLen > 4 && EQUAL(pszFname + nLen-4, ".TAB"))
    {
        /*-------------------------------------------------------------
         * .TAB file ... is it a TABFileView or a TABFile?
         * We have to read the .tab header to find out.
         *------------------------------------------------------------*/
        FILE *fp;
        const char *pszLine;
        char *pszAdjFname = CPLStrdup(pszFname);

        TABAdjustFilenameExtension(pszAdjFname);
        fp = VSIFOpen(pszAdjFname, "r");
        while(poFile == NULL && fp && (pszLine = CPLReadLine(fp)) != NULL)
        {
            while (isspace(*pszLine))  pszLine++;
            if (EQUALN(pszLine, "Fields", 6))
                poFile = new TABFile;
            else if (EQUALN(pszLine, "create view", 11))
                poFile = new TABView;
        }

        if (fp)
            VSIFClose(fp);

        CPLFree(pszAdjFname);
    }

    /*-----------------------------------------------------------------
     * Perform the open() call
     *----------------------------------------------------------------*/
    if (poFile && poFile->Open(pszFname, "r", bTestOpenNoError) != 0)
    {
        delete poFile;
        poFile = NULL;
    }

    if (!bTestOpenNoError && poFile == NULL)
    {
        CPLError(CE_Failure, CPLE_FileIO,
                 "%s could not be opened as a MapInfo dataset.", pszFname);
    }

    return poFile;
}



/**********************************************************************
 *                   IMapInfoFile::GetNextFeature()
 *
 * Standard OGR GetNextFeature implementation.  This methode is used
 * to retreive the next OGRFeature.
 **********************************************************************/
OGRFeature *IMapInfoFile::GetNextFeature()
{
    OGRFeature *poFeature, *poFeatureRef;
      
    poFeatureRef = GetFeatureRef(m_nCurFeatureId+1);
    if (poFeatureRef)
    {
	poFeature = poFeatureRef->Clone();
	poFeature->SetFID(poFeatureRef->GetFID());
	return poFeature;
    }
    else
      return NULL;
}

/**********************************************************************
 *                   IMapInfoFile::CreateFeature()
 *
 * Standard OGR CreateFeature implementation.  This methode is used
 * to create a new feature in current dataset 
 **********************************************************************/
OGRErr     IMapInfoFile::CreateFeature(OGRFeature *poFeature)
{
    TABFeature *poTABFeature;
    OGRGeometry   *poGeom;
    OGRwkbGeometryType eGType;

    /*-----------------------------------------------------------------
     * MITAB won't accept new features unless they are in a type derived
     * from TABFeature... so we have to do our best to map to the right
     * feature type based on the geometry type.
     *----------------------------------------------------------------*/
    poGeom = poFeature->GetGeometryRef();
    if( poGeom != NULL )
        eGType = poGeom->getGeometryType();
    else
        eGType = wkbNone;

    switch (eGType)
    {
      /*-------------------------------------------------------------
       * POINT
       *------------------------------------------------------------*/
      case wkbPoint:
	poTABFeature = new TABPoint(poFeature->GetDefnRef());
	break;
      /*-------------------------------------------------------------
       * REGION
       *------------------------------------------------------------*/
      case wkbPolygon:
      case wkbMultiPolygon:
	poTABFeature = new TABRegion(poFeature->GetDefnRef());
	break;
      /*-------------------------------------------------------------
       * LINE/PLINE/MULTIPLINE
       *------------------------------------------------------------*/
      case wkbLineString:
      case wkbMultiLineString:
	poTABFeature = new TABPolyline(poFeature->GetDefnRef());
	break;
      /*-------------------------------------------------------------
       * Collection types that are not directly supported... convert
       * to multiple features in output file through recursive calls.
       *------------------------------------------------------------*/
      case wkbGeometryCollection:
      case wkbMultiPoint:
      {
          OGRErr eStatus = OGRERR_NONE;
          int i;
          OGRGeometryCollection *poColl = (OGRGeometryCollection*)poGeom;
          OGRFeature *poTmpFeature = poFeature->Clone();

          for (i=0; eStatus==OGRERR_NONE && i<poColl->getNumGeometries(); i++)
          {
              poTmpFeature->SetGeometry(poColl->getGeometryRef(i));
              eStatus = CreateFeature(poTmpFeature);
          }
          delete poTmpFeature;
          return eStatus;
        }
        break;
      /*-------------------------------------------------------------
       * Unsupported type.... convert to MapInfo geometry NONE
       *------------------------------------------------------------*/
      case wkbUnknown:
      default:
         poTABFeature = new TABFeature(poFeature->GetDefnRef()); 
        break;
    }

    if( poGeom != NULL )
        poTABFeature->SetGeometryDirectly(poGeom->clone());
    
    for (int i=0; i< poFeature->GetDefnRef()->GetFieldCount();i++)
    {
	poTABFeature->SetField(i,poFeature->GetRawFieldRef( i ));
    }
    

    if (SetFeature(poTABFeature) > -1)
      return OGRERR_NONE;
    else
      return OGRERR_FAILURE;
}

/**********************************************************************
 *                   IMapInfoFile::GetFeature()
 *
 * Standard OGR GetFeature implementation.  This methode is used
 * to get the wanted (nFeatureId) feature, a NULL value will be 
 * returned on error.
 **********************************************************************/
OGRFeature *IMapInfoFile::GetFeature(long nFeatureId)
{
    OGRFeature *poFeature, *poFeatureRef;

    
    poFeatureRef = GetFeatureRef(nFeatureId);
    if (poFeatureRef)
    {
	poFeature = poFeatureRef->Clone();
	poFeature->SetFID(poFeatureRef->GetFID());
	
	return poFeature;
    }
    else
      return NULL;
}

/**********************************************************************
 *                   IMapInfoFile::GetSpatialFilter()
 *
 * Standard OGR GetSpatialFilter implementation.  This methode is used
 * to retreive the SpacialFilter object.
 **********************************************************************/
OGRGeometry *IMapInfoFile::GetSpatialFilter()
{
    return m_poFilterGeom;
}


/**********************************************************************
 *                   IMapInfoFile::SetSpatialFilter()
 *
 * Standard OGR SetSpatialFiltere implementation.  This methode is used
 * to set a SpatialFilter for this OGRLayer
 **********************************************************************/
void IMapInfoFile::SetSpatialFilter (OGRGeometry * poGeomIn )

{
    if( m_poFilterGeom != NULL )
    {
        delete m_poFilterGeom;
        m_poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
        m_poFilterGeom = poGeomIn->clone();
}

/************************************************************************/
/*                            CreateField()                             */
/*                                                                      */
/*      Create a native field based on a generic OGR definition.        */
/************************************************************************/

OGRErr IMapInfoFile::CreateField( OGRFieldDefn *poField, int bApproxOK )

{
    TABFieldType	eTABType;

    if( poField->GetType() == OFTInteger )
        eTABType = TABFInteger;
    else if( poField->GetType() == OFTReal )
        eTABType = TABFFloat;
    else if( poField->GetType() == OFTString )
        eTABType = TABFChar;
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "IMapInfoFile::CreateField() called with unsupported field"
                  " type %d.\n"
                  "Note that Mapinfo files don't support list field types.\n",
                  poField->GetType() );

        return OGRERR_FAILURE;
    }

    if( AddFieldNative( poField->GetNameRef(), eTABType,
                        poField->GetWidth(), poField->GetPrecision() ) > -1 )
        return OGRERR_NONE;
    else
        return OGRERR_FAILURE;
}
