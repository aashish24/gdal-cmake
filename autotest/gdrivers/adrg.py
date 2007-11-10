#!/usr/bin/env python
###############################################################################
# $Id: png.py 12180 2007-09-17 20:40:27Z warmerdam $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read/write functionality for ADRG driver.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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
# Read test of simple byte reference data.

def adrg_1():

    tst = gdaltest.GDALTest( 'ADRG', 'SMALL_ADRG/ABCDEF01.GEN', 1, 62833 )
    return tst.testOpen()

###############################################################################
# Test copying.

def adrg_2():

#    tst = gdaltest.GDALTest( 'ADRG', 'SMALL_ADRG/ABCDEF01.GEN', 1, 62833 )

#    return tst.testCreateCopy()

    drv = gdal.GetDriverByName( 'ADRG' )
    srcds = gdal.Open( 'data/SMALL_ADRG/ABCDEF01.GEN' )
    
    dstds = drv.CreateCopy( 'tmp/ABCDEF01.GEN', srcds )
    
    dstds = None
    
    drv.Delete( 'tmp/ABCDEF01.GEN' )

    return 'success'
    
###############################################################################
# Verify the geotransform, colormap, and nodata setting for test file. 

def png_3():

    ds = gdal.Open( 'data/test.png' )
    cm = ds.GetRasterBand(1).GetRasterColorTable()
    if cm.GetCount() != 16 \
       or cm.GetColorEntry(0) != (255,255,255,0) \
       or cm.GetColorEntry(1) != (255,255,208,255):
        gdaltest.post_reason( 'Wrong colormap entries' )
        return 'fail'

    cm = None

    if int(ds.GetRasterBand(1).GetNoDataValue()) != 0:
        gdaltest.post_reason( 'Wrong nodata value.' )
        return 'fail'

    # This geotransform test is also verifying the fix for bug 1414, as
    # the world file is in a mixture of numeric representations for the
    # numbers.  (mixture of "," and "." in file)

    gt_expected = (700000.305, 0.38, 0.01, 4287500.695, -0.01, -0.38)

    gt = ds.GetGeoTransform()
    for i in range(6):
        if abs(gt[i] - gt_expected[i]) > 0.0001:
            print 'expected:', gt_expected
            print 'got:', gt
            
            gdaltest.post_reason( 'Mixed locale world file read improperly.' )
            return 'fail'

    return 'success'
    
###############################################################################
# Test RGB mode creation and reading.

def png_4():

    tst = gdaltest.GDALTest( 'PNG', 'rgb.ntf', 3, 21349 )

    return tst.testCreateCopy()

###############################################################################
# Test RGBA 16bit read support.

def png_5():

    tst = gdaltest.GDALTest( 'PNG', 'rgba16.png', 3, 1815 )
    return tst.testOpen()

###############################################################################
# Test RGBA 16bit mode creation and reading.

def png_6():

    tst = gdaltest.GDALTest( 'PNG', 'rgba16.png', 4, 4873 )

    return tst.testCreateCopy()

###############################################################################
# Test RGB NODATA_VALUES metadata write (and read) support.
# This is handled via the tRNS block in PNG.

def png_7():

    drv = gdal.GetDriverByName( 'PNG' )
    srcds = gdal.Open( 'data/tbbn2c16.png' )
    
    dstds = drv.CreateCopy( 'tmp/png7.png', srcds )
    srcds = None

    dstds = gdal.Open( 'tmp/png7.png' )
    md = dstds.GetMetadata()
    dstds = None

    if md['NODATA_VALUES'] != '32639 32639 32639':
        gdaltest.post_reason( 'NODATA_VALUES wrong' )
        return 'fail'

    dstds = None

    drv.Delete( 'tmp/png7.png' )

    return 'success'


gdaltest_list = [
    adrg_1,
    adrg_2 ]

if __name__ == '__main__':

    gdaltest.setup_run( 'adrg' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

