/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Format registration functions..
 * Author:   Ken Shih, kshih@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Les Technologies SoftMap Inc.
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
 * Revision 1.3  1999/11/19 02:50:30  danmo
 * Added TAB + MIF registration
 *
 * Revision 1.2  1999/09/07 14:11:21  warmerda
 * Added registration of NTF format support.
 *
 * Revision 1.1  1999/07/23 19:20:27  kshih
 * Modifications for errors etc...
 *
 */

#include "ogrsf_frmts.h"


/************************************************************************/
/*                        SFRegisterOGRFormats()                        */
/*                                                                      */
/*      Register all the OGR formats we wish to expose through the      */
/*      SFCOM provider.                                                 */
/************************************************************************/
void	SFRegisterOGRFormats()
{
    static bool	bRegistered = false;
    if (bRegistered)
        return;
    bRegistered = true;

    // Add formats to be registered here.
    RegisterOGRShape();
    RegisterOGRNTF();
    RegisterOGRTAB();
    RegisterOGRMIF();
}
