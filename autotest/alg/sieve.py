#!/usr/bin/env python
###############################################################################
# $Id: proximity.py 15577 2008-10-22 05:06:16Z warmerdam $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test SieveFilter() algorithm.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2008, Frank Warmerdam <warmerdam@pobox.com>
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

sys.path.append( '../pymod' )

import gdaltest

try:
    from osgeo import gdal, gdalconst
except:
    import gdal
    import gdalconst

###############################################################################
# Test a fairly default case.

def sieve_1():

    try:
        x = gdal.SieveFilter
    except:
        return 'skip'

    drv = gdal.GetDriverByName( 'GTiff' )
    src_ds = gdal.Open('data/sieve_src.grd')
    src_band = src_ds.GetRasterBand(1)
    
    dst_ds = drv.Create('tmp/sieve_1.tif', 5, 7, 1, gdal.GDT_Byte )
    dst_band = dst_ds.GetRasterBand(1)
    
    gdal.SieveFilter( src_band, None, dst_band, 2, 4 )

    cs_expected = 364
    cs = dst_band.Checksum()
    
    dst_band = None
    dst_ds = None

    if cs == cs_expected \
       or gdal.GetConfigOption( 'CPL_DEBUG', 'OFF' ) != 'ON':
        drv.Delete( 'tmp/sieve_1.tif' )
    
    if cs != cs_expected:
        print('Got: ', cs)
        gdaltest.post_reason( 'got wrong checksum' )
        return 'fail'
    else:
        return 'success' 

###############################################################################
# Try eight connected.

def sieve_2():


    try:
        x = gdal.SieveFilter
    except:
        return 'skip'

    drv = gdal.GetDriverByName( 'GTiff' )
    src_ds = gdal.Open('data/sieve_src.grd')
    src_band = src_ds.GetRasterBand(1)
    
    dst_ds = drv.Create('tmp/sieve_2.tif', 5, 7, 1, gdal.GDT_Byte )
    dst_band = dst_ds.GetRasterBand(1)
    
    gdal.SieveFilter( src_band, None, dst_band, 2, 8 )

    cs_expected = 370
    cs = dst_band.Checksum()
    
    dst_band = None
    dst_ds = None

    if cs == cs_expected \
       or gdal.GetConfigOption( 'CPL_DEBUG', 'OFF' ) != 'ON':
        drv.Delete( 'tmp/sieve_2.tif' )
    
    if cs != cs_expected:
        print('Got: ', cs)
        gdaltest.post_reason( 'got wrong checksum' )
        return 'fail'
    else:
        return 'success' 

###############################################################################
# Do a sieve resulting in unmergable polygons.

def sieve_3():

    try:
        x = gdal.SieveFilter
    except:
        return 'skip'

    drv = gdal.GetDriverByName( 'GTiff' )
    src_ds = gdal.Open('data/unmergable.grd')
    src_band = src_ds.GetRasterBand(1)
    
    dst_ds = drv.Create('tmp/sieve_3.tif', 5, 7, 1, gdal.GDT_Byte )
    dst_band = dst_ds.GetRasterBand(1)
    
    gdal.SieveFilter( src_band, None, dst_band, 2, 8 )

    cs_expected = 472
    cs = dst_band.Checksum()
    
    dst_band = None
    dst_ds = None

    if cs == cs_expected \
       or gdal.GetConfigOption( 'CPL_DEBUG', 'OFF' ) != 'ON':
        drv.Delete( 'tmp/sieve_3.tif' )
    
    if cs != cs_expected:
        print('Got: ', cs)
        gdaltest.post_reason( 'got wrong checksum' )
        return 'fail'
    else:
        return 'success' 

###############################################################################
# Try the bug 2634 simplified data.

def sieve_4():

    try:
        x = gdal.SieveFilter
    except:
        return 'skip'

    drv = gdal.GetDriverByName( 'GTiff' )
    src_ds = gdal.Open('data/sieve_2634.grd')
    src_band = src_ds.GetRasterBand(1)
    
    dst_ds = drv.Create('tmp/sieve_4.tif', 10, 8, 1, gdal.GDT_Byte )
    dst_band = dst_ds.GetRasterBand(1)
    
    gdal.SieveFilter( src_band, None, dst_band, 2, 4 )

    cs_expected = 98
    cs = dst_band.Checksum()
    
    dst_band = None
    dst_ds = None

    if cs == cs_expected \
       or gdal.GetConfigOption( 'CPL_DEBUG', 'OFF' ) != 'ON':
        drv.Delete( 'tmp/sieve_4.tif' )
    
    if cs != cs_expected:
        print('Got: ', cs)
        gdaltest.post_reason( 'got wrong checksum' )
        return 'fail'
    else:
        return 'success' 


###############################################################################
# Same as sieve_1, but we provide a mask band
# This should yield the same result as we use an opaque band

def sieve_5():

    try:
        x = gdal.SieveFilter
    except:
        return 'skip'

    drv = gdal.GetDriverByName( 'GTiff' )
    src_ds = gdal.Open('data/sieve_src.grd')
    src_band = src_ds.GetRasterBand(1)
    
    dst_ds = drv.Create('tmp/sieve_1.tif', 5, 7, 1, gdal.GDT_Byte )
    dst_band = dst_ds.GetRasterBand(1)
    
    gdal.SieveFilter( src_band, dst_band.GetMaskBand(), dst_band, 2, 4 )

    cs_expected = 364
    cs = dst_band.Checksum()
    
    dst_band = None
    dst_ds = None

    if cs == cs_expected \
       or gdal.GetConfigOption( 'CPL_DEBUG', 'OFF' ) != 'ON':
        drv.Delete( 'tmp/sieve_1.tif' )
    
    if cs != cs_expected:
        print('Got: ', cs)
        gdaltest.post_reason( 'got wrong checksum' )
        return 'fail'
    else:
        return 'success' 

gdaltest_list = [
    sieve_1,
    sieve_2,
    sieve_3,
    sieve_4,
    sieve_5
    ]

if __name__ == '__main__':

    gdaltest.setup_run( 'sieve' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

