/**********************************************************************
 * $Id: mitab_middatafile.cpp,v 1.2 1999/11/11 01:22:05 stephane Exp $
 *
 * Name:     mitab_datfile.cpp
 * Project:  MapInfo TAB Read/Write library
 * Language: C++
 * Purpose:  Implementation of the MIDDATAFile class used to handle
 *           reading/writing of the MID/MIF files
 * Author:   Stephane Villeneuve, s.villeneuve@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 1999, Daniel Morissette
 *
 * All rights reserved.  This software may be copied or reproduced, in
 * all or in part, without the prior written consent of its author,
 * Daniel Morissette (danmo@videotron.ca).  However, any material copied
 * or reproduced must bear the original copyright notice (above), this 
 * original paragraph, and the original disclaimer (below).
 * 
 * The entire risk as to the results and performance of the software,
 * supporting text and other information contained in this file
 * (collectively called the "Software") is with the user.  Although 
 * considerable efforts have been used in preparing the Software, the 
 * author does not warrant the accuracy or completeness of the Software.
 * In no event will the author be liable for damages, including loss of
 * profits or consequential damages, arising out of the use of the 
 * Software.
 * 
 **********************************************************************
 *
 * $Log: mitab_middatafile.cpp,v $
 * Revision 1.2  1999/11/11 01:22:05  stephane
 * Remove DebugFeature call, Point Reading error, add IsValidFeature() to test correctly if we are on a feature
 *
 * Revision 1.1  1999/11/08 04:16:07  stephane
 * First Revision
 *
 *
 **********************************************************************/

#include "mitab.h"

/*=====================================================================
 *                      class MIDDATAFile
 *
 *====================================================================*/

MIDDATAFile::MIDDATAFile()
{
    m_fp = NULL;
    m_szLastRead[0] = '\0';
    m_szSavedLine[0] = '\0';
    m_pszDelimiter = NULL;
    
    m_dfXMultiplier = 1.0;
    m_dfYMultiplier = 1.0;
    m_dfXDisplacement = 0.0;
    m_dfYDisplacement = 0.0;

}

MIDDATAFile::~MIDDATAFile()
{
    Close();
}

void MIDDATAFile::SaveLine(const char *pszLine)
{
    if (pszLine == NULL)
    {
	m_szSavedLine[0] = '\0';
    }
    else
    {
	strncpy(m_szSavedLine,pszLine,MIDMAXCHAR);
    }
}

const char *MIDDATAFile::GetSavedLine()
{
    return m_szSavedLine;
}

int MIDDATAFile::Open(const char *pszFname, const char *pszAccess)
{
   if (m_fp)
   {
       CPLError(CE_Failure, CPLE_FileIO,
                 "Open() failed: object already contains an open file");
       return -1;
   }

    /*-----------------------------------------------------------------
     * Validate access mode and make sure we use Text access.
     *----------------------------------------------------------------*/
    if (EQUALN(pszAccess, "r", 1))
    {
        m_eAccessMode = TABRead;
        pszAccess = "rt";
    }
    else if (EQUALN(pszAccess, "w", 1))
    {
        m_eAccessMode = TABWrite;
        pszAccess = "wt";
    }
    else
    {
        CPLError(CE_Failure, CPLE_FileIO,
                 "Open() failed: access mode \"%s\" not supported", pszAccess);
        return -1;
    }

    /*-----------------------------------------------------------------
     * Open file for reading
     *----------------------------------------------------------------*/
    m_pszFname = CPLStrdup(pszFname);
    m_fp = VSIFOpen(m_pszFname, pszAccess);

    if (m_fp == NULL)
    {
        CPLError(CE_Failure, CPLE_FileIO,
                 "Open() failed for %s", m_pszFname);
        CPLFree(m_pszFname);
        m_pszFname = NULL;
        return -1;
    }

    return 0;
}

int MIDDATAFile::Rewind()
{
    if (m_fp == NULL || m_eAccessMode == TABWrite) 
      return -1;

    else
      rewind(m_fp);
    return 0;
}

int MIDDATAFile::Close()
{
    if (m_fp == NULL)
        return 0;
   
    // Close file
    VSIFClose(m_fp);
    m_fp = NULL;

    CPLFree(m_pszFname);
    m_pszFname = NULL;

    return 0;

}

const char *MIDDATAFile::GetLine()
{
    const char *pszLine;
    
    if (m_eAccessMode == TABRead)
    {
	
	pszLine = CPLReadLine(m_fp);

	if (pszLine == NULL)
	{
	    m_szLastRead[0] = '\0';
	}
	else
	{
	    strncpy(m_szLastRead,pszLine,MIDMAXCHAR);
	}
	//if (pszLine)
	//  printf("%s\n",pszLine);
	return pszLine;
    }
    else
      CPLAssert(FALSE);
    
    return NULL;
}

const char *MIDDATAFile::GetLastLine()
{
    if (m_eAccessMode == TABRead)
    {
	// printf("%s\n",m_szLastRead);
	return m_szLastRead;
    }
    else
    {
	CPLAssert(FALSE);
    }
    // Never return NULL, don't need to test the string
    return "";
}

void MIDDATAFile::WriteLine(const char *pszFormat,...)
{
    va_list args;

    if (m_eAccessMode == TABWrite  && m_fp)
    {
	va_start(args, pszFormat);
	 vfprintf( m_fp, pszFormat, args );
	va_end(args);
    } 
    else
    {
	CPLAssert(FALSE);
    }
}


void MIDDATAFile::SetTranslation(double dfXMul,double dfYMul, 
				 double dfXTran,
				 double dfYTran)
{
    m_dfXMultiplier = dfXMul;
    m_dfYMultiplier = dfYMul;
    m_dfXDisplacement = dfXTran;
    m_dfYDisplacement = dfYTran;
}

double MIDDATAFile::GetXTrans(double dfX)
{
    return (dfX * m_dfXMultiplier) + m_dfXDisplacement;
}

double MIDDATAFile::GetYTrans(double dfY)
{
    return (dfY * m_dfYMultiplier) + m_dfYDisplacement;
}


GBool MIDDATAFile::IsValidFeature(const char *pszString)
{
    char **papszToken ;

    papszToken = CSLTokenizeString(pszString);
    
    //   printf("%s\n",pszString);

    if (CSLCount(papszToken) == 0)
    {
	CSLDestroy(papszToken);
	return FALSE;
    }

    if (EQUAL(papszToken[0],"NONE")||EQUAL(papszToken[0],"POINT")||
	EQUAL(papszToken[0],"LINE")||EQUAL(papszToken[0],"PLINE")||
	EQUAL(papszToken[0],"REGION")||EQUAL(papszToken[0],"ARC")||
	EQUAL(papszToken[0],"TEXT")||EQUAL(papszToken[0],"RECT")||
	EQUAL(papszToken[0],"ROUNDRECT")||EQUAL(papszToken[0],"ELLIPSE"))
    {
	CSLDestroy(papszToken);
	return TRUE;
    }

    return FALSE;

}
