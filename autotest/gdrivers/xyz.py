#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read/write functionality for XYZ driver.
# Author:   Even Rouault <even dot rouault at mines dash paris dot org>
# 
###############################################################################
# Copyright (c) 2010, Even Rouault <even dot rouault at mines dash paris dot org>
# 
# Permission is hereby granted, free of charge, to any person oxyzaining a
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
import string
import struct
import gdal
import osr

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Test CreateCopy() of byte.tif

def xyz_1():

    tst = gdaltest.GDALTest( 'XYZ', 'byte.tif', 1, 4672 )
    return tst.testCreateCopy( vsimem = 1, check_gt = ( -67.00041667, 0.00083333, 0.0, 50.000416667, 0.0, -0.00083333 ) )

###############################################################################
# Test CreateCopy() of float.img

def xyz_2():

    tst = gdaltest.GDALTest( 'XYZ', 'float.img', 1, 23529, options = ['COLUMN_SEPARATOR=,', 'ADD_HEADER_LINE=YES'] )
    return tst.testCreateCopy()

###############################################################################
# Test random access to lines of imagery

def xyz_3():

    content = """Y X Z
0 0 65


0 1 66

1 0 67

1 1 68
2 0 69
2 1 70


"""
    gdal.FileFromMemBuffer('/vsimem/grid.xyz', content)
    ds = gdal.Open('/vsimem/grid.xyz')
    buf = ds.ReadRaster(0,2,2,1)
    bytes = struct.unpack('B' * 2, buf)
    if bytes != (69, 70):
        print(buf)
        return 'fail'
    buf = ds.ReadRaster(0,1,2,1)
    bytes = struct.unpack('B' * 2, buf)
    if bytes != (67, 68):
        print(buf)
        return 'fail'
    buf = ds.ReadRaster(0,0,2,1)
    bytes = struct.unpack('B' * 2, buf)
    if bytes != (65, 66):
        print(buf)
        return 'fail'
    buf = ds.ReadRaster(0,2,2,1)
    bytes = struct.unpack('B' * 2, buf)
    if bytes != (69, 70):
        print(buf)
        return 'fail'
    ds = None
    gdal.Unlink('/vsimem/grid.xyz')
    return 'success'

###############################################################################
# Cleanup

def xyz_cleanup():

    return 'success'


gdaltest_list = [
    xyz_1,
    xyz_2,
    xyz_3,
    xyz_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'xyz' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

