#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  BSB Testing.
# Author:   Even Rouault <even dot rouault at mines dash paris dot org>
#
###############################################################################
# Copyright (c) 2008, Even Rouault <even dot rouault at mines dash paris dot org>
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
# Test driver availability

def bsb_0():
    try:
        gdaltest.bsb_dr = gdal.GetDriverByName( 'BSB' )
    except:
        gdaltest.bsb_dr = None

    if gdaltest.bsb_dr is None:
        return 'skip'

    return 'success'

###############################################################################
# Test Read

def bsb_1():
    if gdaltest.bsb_dr is None:
        return 'skip'

    tst = gdaltest.GDALTest( 'BSB', 'rgbsmall.kap', 1, 30321 )

    return tst.testOpen()

###############################################################################
# Test CreateCopy

def bsb_2():
    if gdaltest.bsb_dr is None:
        return 'skip'

    md = gdaltest.bsb_dr.GetMetadata()
    if not md.has_key('DMD_CREATIONDATATYPES'):
        return 'skip'

    tst = gdaltest.GDALTest( 'BSB', 'rgbsmall.kap', 1, 30321 )

    return tst.testCreateCopy()

###############################################################################
# Read a BSB with an index table at the end (#2782)
# The rgbsmall_index.kap has been generated from rgbsmall.kap by moving the
# data of first line from offset 2382 to offset 2384, and generating the index table
# --> This is probably not a valid BSB file, but it proves that we can read the index table

def bsb_3():
    if gdaltest.bsb_dr is None:
        return 'skip'

    tst = gdaltest.GDALTest( 'BSB', 'rgbsmall_index.kap', 1, 30321 )

    return tst.testOpen()

###############################################################################
# Read a BSB without an index table but with 0 in the middle of line data
# The rgbsmall_with_line_break.kap has been generated from rgbsmall.kap by
# adding a 0 character in the middle of line data

def bsb_4():
    if gdaltest.bsb_dr is None:
        return 'skip'

    tst = gdaltest.GDALTest( 'BSB', 'rgbsmall_with_line_break.kap', 1, 30321 )

    return tst.testOpen()

###############################################################################
# Read a truncated BSB (at the level of the written scanline number starting a new row)

def bsb_5():
    if gdaltest.bsb_dr is None:
        return 'skip'

    tst = gdaltest.GDALTest( 'BSB', 'rgbsmall_truncated.kap', 1, 29696 )

    gdal.PushErrorHandler('CPLQuietErrorHandler')
    ret = tst.testOpen()
    gdal.PopErrorHandler()

    return ret

###############################################################################
# Read another truncated BSB (in the middle of row data)

def bsb_6():
    if gdaltest.bsb_dr is None:
        return 'skip'

    tst = gdaltest.GDALTest( 'BSB', 'rgbsmall_truncated2.kap', 1, 29696 )

    gdal.PushErrorHandler('CPLQuietErrorHandler')
    ret = tst.testOpen()
    gdal.PopErrorHandler()

    return ret


###############################################################################
# Test a NOAA chart
# Under following user agreement : http://www.charts.noaa.gov/RNCs/Agreement.shtml?14850

def bsb_online_1():

    if not gdaltest.download_file('http://www.charts.noaa.gov/RNCs/14850.zip', '14850.zip'):
        return 'skip'

    try:
        os.stat('tmp/cache/14850_1.KAP')
        file_to_test = 'tmp/cache/14850_1.KAP'
    except:
        try:
            print 'Uncompressing ZIP file...'
            import zipfile
            zfobj = zipfile.ZipFile('tmp/cache/14850.zip')
            outfile = open('tmp/cache/14850_1.KAP', 'wb')
            outfile.write(zfobj.read('BSB_ROOT/14850/14850_1.KAP'))
            outfile.close()
            file_to_test = 'tmp/cache/14850_1.KAP'
        except:
            print 'Reading BSB from ZIP file (slow)...'
            file_to_test = '/vsizip/tmp/cache/14850.zip/BSB_ROOT/14850/14850_1.KAP'

    tst = gdaltest.GDALTest('BSB', file_to_test, 1, 54674, filename_absolute = 1 )
    return tst.testOpen()


gdaltest_list = [
    bsb_0,
    bsb_1,
    bsb_2,
    bsb_3,
    bsb_4,
    bsb_5,
    bsb_6,
    bsb_online_1
    ]


if __name__ == '__main__':

    gdaltest.setup_run( 'BSB' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

