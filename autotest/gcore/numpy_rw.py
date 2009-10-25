#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test basic integration with Numeric Python.
# Author:   Frank Warmerdam, warmerdam@pobox.com
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

sys.path.append( '../pymod' )

import gdaltest
import gdal

###############################################################################
# verify that we can load Numeric python, and find the Numpy driver.

def numpy_rw_1():
	
    gdaltest.numpy_drv = None
    try:
        import gdalnumeric
        gdalnumeric.zeros
    except:
        try:
            import osgeo.gdal_array as gdalnumeric
        except ImportError:
            return 'skip'

    try:
        import _gdal
        _gdal.GDALRegister_NUMPY()  # only needed for old style bindings.
        gdal.AllRegister()
    except:
        pass

    gdaltest.numpy_drv = gdal.GetDriverByName( 'NUMPY' )
    if gdaltest.numpy_drv is None:
        gdaltest.post_reason( 'NUMPY driver not found!' )
        return 'fail'
    
    return 'success'

###############################################################################
# Load a test file into a memory Numpy array, and verify the checksum.

def numpy_rw_2():

    if gdaltest.numpy_drv is None:
	return 'skip'

    import gdalnumeric

    array = gdalnumeric.LoadFile( 'data/utmsmall.tif' )
    if array is None:
	gdaltest.post_reason( 'Failed to load utmsmall.tif into array')
	return 'fail'

    ds = gdalnumeric.OpenArray( array )
    if ds is None:
 	gdaltest.post_reason( 'Failed to open memory array as dataset.' )
	return 'fail'

    bnd = ds.GetRasterBand(1)
    if bnd.Checksum() != 50054:
        gdaltest.post_reason( 'Didnt get expected checksum on reopened file')
        return 'fail'
    ds = None

    return 'success'

###############################################################################
# Test loading complex data.

def numpy_rw_3():

    if gdaltest.numpy_drv is None:
	return 'skip'

    import gdalnumeric

    ds = gdal.Open( 'data/cint_sar.tif' )
    array = ds.ReadAsArray()

    if array[2][3] != 116-16j:
        print array[0][2][3]
        gdaltest.post_reason( 'complex value read improperly.' )
        return 'fail'

    return 'success'

###############################################################################
# Test a band read with downsampling.

def numpy_rw_4():

    if gdaltest.numpy_drv is None:
	return 'skip'

    ds = gdal.Open( 'data/byte.tif' )
    array = ds.GetRasterBand(1).ReadAsArray(0,0,20,20,5,5)

    if array[2][3] != 123:
        print array[2][3]
        gdaltest.post_reason( 'Read wrong value - perhaps downsampling algorithm has changed subtly?' )
        return 'fail'

    return 'success'

###############################################################################
# Test reading a multi-band file.

def numpy_rw_5():

    if gdaltest.numpy_drv is None:
	return 'skip'

    import gdalnumeric

    array = gdalnumeric.LoadFile('data/rgbsmall.tif',35,21,1,1)

    if array[0][0][0] != 78:
        print array
        gdaltest.post_reason( 'value read improperly.' )
        return 'fail'

    if array[1][0][0] != 117:
        print array
        gdaltest.post_reason( 'value read improperly.' )
        return 'fail'

    if array[2][0][0] != 24:
        print array
        gdaltest.post_reason( 'value read improperly.' )
        return 'fail'

    return 'success'

###############################################################################
# Check that Band.ReadAsArray() can accept an already allocated array (#2658, #3028)

def numpy_rw_6():

    if gdaltest.numpy_drv is None:
        return 'skip'

    import numpy
    import gdalnumeric
        
    ds = gdal.Open( 'data/byte.tif' )
    array = numpy.zeros( [ds.RasterYSize, ds.RasterXSize], numpy.uint8 )
    array_res = ds.GetRasterBand(1).ReadAsArray(buf_obj = array)
    
    if array is not array_res:
        return 'fail'

    ds2 = gdalnumeric.OpenArray( array )
    if ds2.GetRasterBand(1).Checksum() != ds.GetRasterBand(1).Checksum():
        return 'fail'

    return 'success'

###############################################################################
# Check that Dataset.ReadAsArray() can accept an already allocated array (#2658, #3028)

def numpy_rw_7():

    if gdaltest.numpy_drv is None:
        return 'skip'

    import numpy
    import gdalnumeric
        
    ds = gdal.Open( 'data/byte.tif' )
    array = numpy.zeros( [1, ds.RasterYSize, ds.RasterXSize], numpy.uint8 )
    array_res = ds.ReadAsArray(buf_obj = array)
    
    if array is not array_res:
        return 'fail'

    ds2 = gdalnumeric.OpenArray( array )
    if ds2.GetRasterBand(1).Checksum() != ds.GetRasterBand(1).Checksum():
        return 'fail'
        
    # Try again with a 2D array
    array = numpy.zeros( [ds.RasterYSize, ds.RasterXSize], numpy.uint8 )
    array_res = ds.ReadAsArray(buf_obj = array)
    
    if array is not array_res:
        return 'fail'

    ds2 = gdalnumeric.OpenArray( array )
    if ds2.GetRasterBand(1).Checksum() != ds.GetRasterBand(1).Checksum():
        return 'fail'
        
    return 'success'
    
###############################################################################
# Check that Dataset.ReadAsArray() with multi-band data

def numpy_rw_8():

    if gdaltest.numpy_drv is None:
        return 'skip'

    import numpy
    import gdalnumeric
        
    ds = gdal.Open( 'data/rgbsmall.tif' )
    array = numpy.zeros( [ds.RasterCount,ds.RasterYSize, ds.RasterXSize], numpy.uint8 )
    array_res = ds.ReadAsArray(buf_obj = array)

    ds2 = gdalnumeric.OpenArray( array )
    for i in range(1, ds.RasterCount):
        if ds2.GetRasterBand(i).Checksum() != ds.GetRasterBand(i).Checksum():
            return 'fail'
        
    return 'success'
    
def numpy_rw_cleanup():
    gdaltest.numpy_drv = None

    return 'success'

gdaltest_list = [
    numpy_rw_1,
    numpy_rw_2,
    numpy_rw_3,
    numpy_rw_4,
    numpy_rw_5,
    numpy_rw_6,
    numpy_rw_7,
    numpy_rw_8,
    numpy_rw_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'numpy_rw' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

