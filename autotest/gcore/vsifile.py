#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test VSI file primitives
# Author:   Even Rouault <even dot rouault at mines dash parid dot org>
#
###############################################################################
# Copyright (c) 2011 Even Rouault <even dot rouault at mines dash parid dot org>
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

import gdal
import sys
import os

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Generic test

def vsifile_generic(filename):

    fp = gdal.VSIFOpenL(filename, 'wb+')
    gdal.VSIFWriteL('0123456789', 1, 10, fp)
    gdal.VSIFTruncateL(fp, 5)

    if gdal.VSIFTellL(fp) != 10:
        gdaltest.post_reason('failure')
        return 'fail'

    gdal.VSIFSeekL(fp, 0, 2)

    if gdal.VSIFTellL(fp) != 5:
        gdaltest.post_reason('failure')
        return 'fail'

    gdal.VSIFWriteL('XX', 1, 2, fp)
    gdal.VSIFCloseL(fp)

    statBuf = gdal.VSIStatL(filename, gdal.VSI_STAT_EXISTS_FLAG | gdal.VSI_STAT_NATURE_FLAG | gdal.VSI_STAT_SIZE_FLAG)
    if statBuf.size != 7:
        gdaltest.post_reason('failure')
        print(statBuf.size)
        return 'fail'

    fp = gdal.VSIFOpenL(filename, 'rb')
    buf = gdal.VSIFReadL(1, 7, fp)
    gdal.VSIFCloseL(fp)

    if buf.decode('ascii') != '01234XX':
        gdaltest.post_reason('failure')
        return 'fail'

    gdal.Unlink(filename)

    statBuf = gdal.VSIStatL(filename, gdal.VSI_STAT_EXISTS_FLAG)
    if statBuf is not None:
        gdaltest.post_reason('failure')
        return 'fail'

    return 'success'

###############################################################################
# Test /vsimem

def vsifile_1():
    return vsifile_generic('/vsimem/vsifile_1.bin')

###############################################################################
# Test regular file system

def vsifile_2():
    return vsifile_generic('tmp/vsifile_2.bin')


###############################################################################
# Test ftruncate >= 32 bit

def vsifile_3():

    if (gdaltest.filesystem_supports_sparse_files('tmp') == False):
        return 'skip'

    filename = 'tmp/vsifile_3'

    fp = gdal.VSIFOpenL(filename, 'wb+')
    gdal.VSIFTruncateL(fp, 10 * 1024 * 1024 * 1024)
    gdal.VSIFSeekL(fp, 0, 2)
    pos = gdal.VSIFTellL(fp)
    if pos != 10 * 1024 * 1024 * 1024:
        gdaltest.post_reason('failure')
        gdal.VSIFCloseL(fp)
        gdal.Unlink(filename)
        print(pos)
        return 'fail'
    gdal.VSIFSeekL(fp, 0, 0)
    gdal.VSIFSeekL(fp, pos, 0)
    pos = gdal.VSIFTellL(fp)
    if pos != 10 * 1024 * 1024 * 1024:
        gdaltest.post_reason('failure')
        gdal.VSIFCloseL(fp)
        gdal.Unlink(filename)
        print(pos)
        return 'fail'

    gdal.VSIFCloseL(fp)

    statBuf = gdal.VSIStatL(filename, gdal.VSI_STAT_EXISTS_FLAG | gdal.VSI_STAT_NATURE_FLAG | gdal.VSI_STAT_SIZE_FLAG)
    gdal.Unlink(filename)

    if statBuf.size != 10 * 1024 * 1024 * 1024:
        gdaltest.post_reason('failure')
        print(statBuf.size)
        return 'fail'

    return 'success'

gdaltest_list = [ vsifile_1,
                  vsifile_2,
                  vsifile_3 ]

if __name__ == '__main__':

    gdaltest.setup_run( 'vsifile' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

