/**********************************************************************
 * $Id$
 *
 * Name:     cpl_string.h
 * Project:  CPL - Common Portability Library
 * Purpose:  String and StringList functions.
 * Author:   Daniel Morissette, danmo@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 1998, Daniel Morissette
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
 * $Log$
 * Revision 1.1  1998/12/03 18:26:02  warmerda
 * New
 *
 **********************************************************************/

#ifndef _CPL_STRING_H_INCLUDED
#define _CPL_STRING_H_INCLUDED

#include "cpl_vsi.h"
#include "cpl_error.h"
#include "cpl_conv.h"

/*=====================================================================
                   Stringlist functions (strlist.c)
 =====================================================================*/
CPL_C_START

char    **CSLAddString(char **papszStrList, const char *pszNewString);
int     CSLCount(char **papszStrList);
void    CSLDestroy(char **papszStrList);
char    **CSLDuplicate(char **papszStrList);

char    **CSLTokenizeString(const char *pszString );
char    **CSLTokenizeStringComplex(const char *pszString,
                                   const char *pszDelimiter,
                                   int bHonourStrings, int bAllowEmptyTokens );

int     CSLPrint(char **papszStrList, FILE *fpOut);
char    **CSLLoad(const char *pszFname);
int     CSLSave(char **papszStrList, const char *pszFname);

char  **CSLInsertStrings(char **papszStrList, int nInsertAtLineNo, 
                         char **papszNewLines);
char  **CSLInsertString(char **papszStrList, int nInsertAtLineNo, 
                        char *pszNewLine);
char  **CSLRemoveStrings(char **papszStrList, int nFirstLineToDelete,
                         int nNumToRemove, char ***ppapszRetStrings);

const char *CPLSPrintf(char *fmt, ...);
char  **CSLAppendPrintf(char **papszStrList, char *fmt, ...);

const char *CSLFetchNameValue(char **papszStrList, const char *pszName);

CPL_C_END

#endif /* _CPL_STRING_H_INCLUDED */
