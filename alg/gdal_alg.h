/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Image Processing Algorithms
 * Purpose:  Prototypes, and definitions for various GDAL based algorithms.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam
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
 * Revision 1.1  2001/01/22 22:30:59  warmerda
 * New
 *
 */

#ifndef GDAL_ALG_H_INCLUDED
#define GDAL_ALG_H_INCLUDED

/**
 * \file gdal_alg.h
 *
 * Public (C callable) GDAL algorithm entry points, and definitions.
 */

#include "gdal.h"

CPL_C_START

int GDALComputeMedianCutPCT( GDALRasterBandH hRed, 
                             GDALRasterBandH hGreen, 
                             GDALRasterBandH hBlue, 
                             int (*pfnIncludePixel)(int,int,void*),
                             int nColors, 
                             GDALColorTableH hColorTable,
                             GDALProgressFunc pfnProgress, 
                             void * pProgressArg );

int GDALDitherRGB2PCT( GDALRasterBandH hRed, 
                       GDALRasterBandH hGreen, 
                       GDALRasterBandH hBlue, 
                       GDALRasterBandH hTarget, 
                       GDALColorTableH hColorTable, 
                       GDALProgressFunc pfnProgress, 
                       void * pProgressArg );

CPL_C_END

#endif /* ndef GDAL_ALG_H_INCLUDED */
