#!/usr/bin/env python
###############################################################################
# $Id: idrisi.py,v 1.3 2006/10/27 04:27:12 fwarmerdam Exp $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read/write functionality for INGR.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2007, Frank Warmerdam <warmerdam@pobox.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
###############################################################################

import os
import sys
import gdal

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Read test of byte file.

def ingr_1():

    tst = gdaltest.GDALTest( 'INGR', '8bit_rgb.cot', 2, 4855 )
    return tst.testOpen()

###############################################################################
# Read uint32 file.

def ingr_2():

    tst = gdaltest.GDALTest( 'INGR', 'uint32.cot', 1, 4672 )
    return tst.testOpen()

###############################################################################
# Test paletted file, including checking the palette (format 02 I think).

def ingr_3():

    tst = gdaltest.GDALTest( 'INGR', '8bit_pal.cot', 1, 4855 )
    result = tst.testOpen()
    if result != 'success':
        return result

    ds = gdal.Open( 'data/8bit_pal.cot' )
    ct = ds.GetRasterBand(1).GetRasterColorTable()
    if ct.GetCount() != 256 or ct.GetColorEntry(8) != (8,8,8,255):
        gdaltest.post_reason( 'Wrong color table entry.' )
        return 'fail'

    return 'success'

###############################################################################
# frmt02 is a plain byte format

def ingr_4():

    tst = gdaltest.GDALTest( 'INGR', 'frmt02.cot', 1, 26968 )
    return tst.testOpen()

###############################################################################
# Test creation.

def ingr_5():

    tst = gdaltest.GDALTest( 'INGR', 'frmt02.cot', 1, 26968 )
    return tst.testCreate()

###############################################################################
# Test createcopy.

def ingr_6():

    tst = gdaltest.GDALTest( 'INGR', 'frmt02.cot', 1, 26968 )
    return tst.testCreate()

###############################################################################
# JPEG 8bit

def ingr_7():

    tst = gdaltest.GDALTest( 'INGR', 'frmt30.cot', 1, 29718 )
    return tst.testOpen()

###############################################################################
# Read simple RLE

def ingr_8():

    tst = gdaltest.GDALTest( 'INGR', 'frmt09.cot', 1, 2480 )
    return tst.testOpen()

###############################################################################
# Read Simple RLE Variable

def ingr_9():

    tst = gdaltest.GDALTest( 'INGR', 'frmt10.cot', 1, 47031 )
    return tst.testOpen()

###############################################################################
# CCITT bitonal

def ingr_10():

    tst = gdaltest.GDALTest( 'INGR', 'frmt24.cit', 1, 23035 )
    return tst.testOpen()

###############################################################################
# Adaptive RLE - 24 bit.

def ingr_11():

    tst = gdaltest.GDALTest( 'INGR', 'frmt27.cot', 2, 45616 )
    return tst.testOpen()

###############################################################################
# Uncompressed RGB

def ingr_12():

    tst = gdaltest.GDALTest( 'INGR', 'frmt28.cot', 2, 45616 )
    return tst.testOpen()

###############################################################################
# Adaptive RLE 8bit.

def ingr_13():

    tst = gdaltest.GDALTest( 'INGR', 'frmt29.cot', 1, 26968 )
    return tst.testOpen()

###############################################################################
# JPEG RGB

def ingr_14():

    tst = gdaltest.GDALTest( 'INGR', 'frmt31.cot', 1, 11466 )
    return tst.testOpen()

###############################################################################
# Test writing CCITT Group 4 compressed.

def ingr_15():

    tst = gdaltest.GDALTest( 'INGR', 'frmt24.cit', 1, 23035,
                             options = ['FORMAT=CCITT Group 4'] )
    return tst.testCreateCopy()

###############################################################################
# Same, but through vsimem all in memory.

def ingr_16():

    tst = gdaltest.GDALTest( 'INGR', 'frmt24.cit', 1, 23035,
                             options = ['FORMAT=CCITT Group 4'] )
    return tst.testCreateCopy( vsimem = 1 )

###############################################################################
# Cleanup.

def ingr_cleanup():
    gdaltest.clean_tmp()
    return 'success'

gdaltest_list = [
    ingr_1,
    ingr_2,
    ingr_3,
    ingr_4,
    ingr_5,
    ingr_6,
    ingr_7,
    ingr_8,
    ingr_9,
    ingr_10,
    ingr_11,
    ingr_12,
    ingr_13,
    ingr_14,
    ingr_15,
    ingr_16,
    ingr_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'ingr' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

