#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test basic GDAL open
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

sys.path.append( '../pymod' )

import gdaltest
import gdal

# Nothing exciting here. Just trying to open non existing files,
# or empty names, or files that are not valid datasets...

def basic_test_1():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('non_existing_ds', gdal.GA_ReadOnly)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`non_existing_ds\' does not exist in the file system,\nand is not recognised as a supported dataset name.\n':
        return 'success'
    else:
        return 'fail'

def basic_test_2():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('non_existing_ds', gdal.GA_Update)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`non_existing_ds\' does not exist in the file system,\nand is not recognised as a supported dataset name.\n':
        return 'success'
    else:
        return 'fail'

def basic_test_3():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('', gdal.GA_ReadOnly)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`\' does not exist in the file system,\nand is not recognised as a supported dataset name.\n':
        return 'success'
    else:
        return 'fail'

def basic_test_4():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('', gdal.GA_Update)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`\' does not exist in the file system,\nand is not recognised as a supported dataset name.\n':
        return 'success'
    else:
        return 'fail'

def basic_test_5():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('data/doctype.xml', gdal.GA_ReadOnly)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`data/doctype.xml\' not recognised as a supported file format.\n':
        return 'success'
    else:
        return 'fail'

###############################################################################
# Issue several AllRegister() to check that GDAL drivers are good citizens

def basic_test_6():
    gdal.AllRegister()
    gdal.AllRegister()
    gdal.AllRegister()

    return 'success'

###############################################################################
# Test fix for #3077 (check that errors are cleared when using UseExceptions())

def basic_test_7_internal():
    try:
        ds = gdal.Open('non_existing_ds', gdal.GA_ReadOnly)
        gdaltest.post_reason('opening should have thrown an exception')
        return 'fail'
    except:
        # Special case: we should still be able to get the error message
        # until we call a new GDAL function
        if gdal.GetLastErrorMsg() != '`non_existing_ds\' does not exist in the file system,\nand is not recognised as a supported dataset name.\n':
            gdaltest.post_reason('did not get expected error message')
            return 'fail'

        if gdal.GetLastErrorType() == 0:
            gdaltest.post_reason('did not get expected error type')
            return 'fail'

        # Should issue an implicit CPLErrorReset()
        gdal.GetCacheMax()

        if gdal.GetLastErrorType() != 0:
            gdaltest.post_reason('got unexpected error type')
            return 'fail'

        return 'success'

def basic_test_7():
    old_use_exceptions_status = gdal.GetUseExceptions()
    gdal.UseExceptions()
    ret = basic_test_7_internal()
    if old_use_exceptions_status == 0:
        gdal.DontUseExceptions()
    return ret


gdaltest_list = [ basic_test_1,
                  basic_test_2,
                  basic_test_3,
                  basic_test_4,
                  basic_test_5,
                  basic_test_6,
                  basic_test_7 ]


if __name__ == '__main__':

    gdaltest.setup_run( 'basic_test' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

