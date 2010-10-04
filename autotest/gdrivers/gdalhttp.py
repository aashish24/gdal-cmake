#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test HTTP Driver.
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
import string
import array
import gdal

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Verify we have the driver.

def http_1():

    try:
        drv = gdal.GetDriverByName( 'HTTP' )
    except:
        drv = None

    if drv is None:
        return 'skip'

    conn = gdaltest.gdalurlopen('http://home.gdal.org/~warmerda/frank.gif')
    if conn is None:
        print('cannot open URL')
        return 'skip'
    conn.close()

    tst = gdaltest.GDALTest( 'GIF','http://home.gdal.org/~warmerda/frank.gif',
                             1, 35415, filename_absolute = 1 )
    return tst.testOpen()

###############################################################################
# Verify /vsicurl (subversion file listing)

def http_2():

    try:
        drv = gdal.GetDriverByName( 'HTTP' )
    except:
        drv = None

    if drv is None:
        return 'skip'

    conn = gdaltest.gdalurlopen('http://svn.osgeo.org/gdal/trunk/autotest/gcore/data/byte.tif')
    if conn is None:
        print('cannot open URL')
        return 'skip'
    conn.close()
    
    tst = gdaltest.GDALTest( 'GTiff','/vsicurl/http://svn.osgeo.org/gdal/trunk/autotest/gcore/data/byte.tif',
                             1, 4672, filename_absolute = 1 )
    return tst.testOpen()

###############################################################################
# Verify /vsicurl (apache file listing)

def http_3():

    try:
        drv = gdal.GetDriverByName( 'HTTP' )
    except:
        drv = None

    if drv is None:
        return 'skip'

    conn = gdaltest.gdalurlopen('http://download.osgeo.org/gdal/data/ehdr/elggll.bil')
    if conn is None:
        print('cannot open URL')
        return 'skip'
    conn.close()

    ds = gdal.Open('/vsicurl/http://download.osgeo.org/gdal/data/ehdr/elggll.bil')
    if ds is None:
        return 'fail'

    return 'success'

###############################################################################
# Verify /vsicurl (ftp)

def http_4():

    try:
        drv = gdal.GetDriverByName( 'HTTP' )
    except:
        drv = None

    if drv is None:
        return 'skip'

    conn = gdaltest.gdalurlopen('http://download.osgeo.org/gdal/data/ehdr/elggll.bil')
    if conn is None:
        print('cannot open URL')
        return 'skip'
    conn.close()

    ds = gdal.Open('/vsicurl/ftp://ftp.remotesensing.org/gdal/data/ehdr/elggll.bil')
    if ds is None:
        return 'fail'
        
    filelist = ds.GetFileList()
    if filelist[1] != '/vsicurl/ftp://ftp.remotesensing.org/gdal/data/ehdr/elggll.hdr':
        print(filelist)
        return 'fail'

    return 'success'

gdaltest_list = [ http_1,
                  http_2,
                  http_3,
                  http_4 ]

if __name__ == '__main__':

    gdaltest.setup_run( 'http' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

