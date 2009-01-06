#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read/write functionality for JP2KAK JPEG2000 driver.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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

def jp2kak_1():

    gdaltest.jp2kak_drv = gdal.GetDriverByName( 'JP2KAK' )
    if gdaltest.jp2kak_drv is None:
        return 'skip'

    # Clear out potentially conflicting drivers.  Re-register them after us.
    try:
	drv = gdal.GetDriverByName( 'JP2ECW' )
	drv.Deregister();
	drv.Register()
	print 'Moved JP2ECW'
    except:
        pass

    try:
	drv = gdal.GetDriverByName( 'JP2MRSID' )
	drv.Deregister();
	drv.Register()
	print 'Moved JP2MRSID'
    except:
        pass

    try:
	drv = gdal.GetDriverByName( 'JPEG2000' )
	drv.Deregister();
	drv.Register()
	print 'Moved JPEG2000'
    except:
        pass

    tst = gdaltest.GDALTest( 'JP2KAK', 'byte.jp2', 1, 50054 )
    return tst.testOpen()

###############################################################################
# Read test of simple 16bit reference data. 

def jp2kak_2():

    if gdaltest.jp2kak_drv is None:
        return 'skip'
    
    tst = gdaltest.GDALTest( 'JP2KAK', 'int16.jp2', 1, 4598 )
    return tst.testOpen()

###############################################################################
# Test lossless copying.

def jp2kak_3():

    if gdaltest.jp2kak_drv is None:
        return 'skip'
    
    tst = gdaltest.GDALTest( 'JP2KAK', 'byte.jp2', 1, 50054,
                             options = [ 'QUALITY=100' ] )

    return tst.testCreateCopy()
    
###############################################################################
# Test GeoJP2 production with geotransform.

def jp2kak_4():

    if gdaltest.jp2kak_drv is None:
        return 'skip'
    
    tst = gdaltest.GDALTest( 'JP2KAK', 'rgbsmall.tif', 0, 0,
                             options = [ 'GMLJP2=OFF' ] )

    return tst.testCreateCopy( check_srs = 1, check_gt = 1)
    
###############################################################################
# Test GeoJP2 production with gcps.

def jp2kak_5():

    if gdaltest.jp2kak_drv is None:
        return 'skip'
    
    tst = gdaltest.GDALTest( 'JP2KAK', 'rgbsmall.tif', 0, 0,
                             options = [ 'GEOJP2=OFF' ] )

    return tst.testCreateCopy( check_srs = 1, check_gt = 1)
    
###############################################################################
# Test reading GMLJP2 file with srsName only on the Envelope, and lots of other
# metadata junk.  This file is also handled currently with axis reordering
# disabled. 

def jp2kak_6():

    if gdaltest.jp2kak_drv is None:
        return 'skip'

    gdal.SetConfigOption( 'GDAL_IGNORE_AXIS_ORIENTATION', 'YES' )
    
    exp_wkt = 'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"],AXIS["Latitude",NORTH],AXIS["Longitude",EAST]]'

    ds = gdal.Open( 'data/ll.jp2' )
    wkt = ds.GetProjection()

    if wkt != exp_wkt:
        gdaltest.post_reason( 'did not get expected WKT, should be WGS84' )
        print 'got: ', wkt
        print 'exp: ', exp_wkt
        return 'fail'

    gt = ds.GetGeoTransform()
    if abs(gt[0] - 8) > 0.0000001 or abs(gt[3] - 50) > 0.000001 \
       or abs(gt[1] - 0.000761397164) > 0.000000000005 \
       or abs(gt[2] - 0.0) > 0.000000000005 \
       or abs(gt[4] - 0.0) > 0.000000000005 \
       or abs(gt[5] - -0.000761397164) > 0.000000000005:
        gdaltest.post_reason( 'did not get expected geotransform' )
        print 'got: ', gt
        return 'fail'
       
    ds = None

    gdal.SetConfigOption( 'GDAL_IGNORE_AXIS_ORIENTATION', 'NO' )
    
    return 'success'
    
###############################################################################
# Test reading a file with axis orientation set properly for an alternate
# axis order coordinate system (urn:...:EPSG::4326). 

def jp2kak_7():

    if gdaltest.jp2kak_drv is None:
        return 'skip'

    exp_wkt = 'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"],AXIS["Latitude",NORTH],AXIS["Longitude",EAST]]'

    ds = gdal.Open( 'data/gmljp2_dtedsm_epsg_4326_axes.jp2' )
    wkt = ds.GetProjection()

    if wkt != exp_wkt:
        gdaltest.post_reason( 'did not get expected WKT, should be WGS84' )
        print 'got: ', wkt
        print 'exp: ', exp_wkt
        return 'fail'

    gt = ds.GetGeoTransform()
    gte = (42.999583333333369,0.008271349862259,0,
           34.000416666666631,0,-0.008271349862259)
    
    if abs(gt[0] - gte[0]) > 0.0000001 or abs(gt[3] - gte[3]) > 0.000001 \
       or abs(gt[1] - gte[1]) > 0.000000000005 \
       or abs(gt[2] - gte[2]) > 0.000000000005 \
       or abs(gt[4] - gte[4]) > 0.000000000005 \
       or abs(gt[5] - gte[5]) > 0.000000000005:
        gdaltest.post_reason( 'did not get expected geotransform' )
        print 'got: ', gt
        return 'fail'
       
    ds = None

    return 'success'
    
###############################################################################
# Test VSI*L support with a JPC rather than jp2 datastream.
#

def jp2kak_8():

    if gdaltest.jp2kak_drv is None:
        return 'skip'
    
    tst = gdaltest.GDALTest( 'JP2KAK', 'byte.jp2', 1, 50054,
                             options = [ 'QUALITY=100' ] )

    return tst.testCreateCopy( vsimem = 1,
                               new_filename = '/vsimem/jp2kak_8.jpc' )
    
###############################################################################
# Test checksum values for a YCbCr color model file.
#

def jp2kak_9():

    if gdaltest.jp2kak_drv is None:
        return 'skip'
    
    tst = gdaltest.GDALTest( 'JP2KAK', 'rgbwcmyk01_YeGeo_kakadu.jp2', 2, 32141 )
    return tst.testOpen()
    
###############################################################################
# Confirm that we can also read this file using the DirectRasterIO()
# function and get appropriate values.
#

def jp2kak_10():

    if gdaltest.jp2kak_drv is None:
        return 'skip'

    ds = gdal.Open('data/rgbwcmyk01_YeGeo_kakadu.jp2')
    data = ds.ReadRaster( 0, 0, 800, 100, band_list = [2,3] )
    ds = None

    expected = [ (0,0), (255,0), (0, 255), (255,255),
                 (255,255), (0,255), (255,0), (0,0)]
    got = []
    
    for x in range(8):
        got.append( (ord(data[x*100]), ord(data[80000 + x*100])) )

    if got != expected:
        print got
        gdaltest.post_reason( 'did not get expected values.' )
        return 'fail'

    return 'success'
    
###############################################################################
# Cleanup.

def jp2kak_cleanup():
    gdaltest.jp2kak_drv = None

    return 'success'

gdaltest_list = [
    jp2kak_1,
    jp2kak_2,
    jp2kak_3,
    jp2kak_4,
    jp2kak_5,
    jp2kak_6,
    jp2kak_7,
    jp2kak_8,
    jp2kak_9,
    jp2kak_10,
    jp2kak_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'jp2kak' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

