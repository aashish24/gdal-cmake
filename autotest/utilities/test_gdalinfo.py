#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  gdalinfo testing
# Author:   Even Rouault <even dot rouault @ mines-paris dot org>
# 
###############################################################################
# Copyright (c) 2008, Even Rouault <even dot rouault @ mines-paris dot org>
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

import sys
import os

sys.path.append( '../pymod' )

import gdaltest
import test_cli_utilities

###############################################################################
# Simple test

def test_gdalinfo_1():
    if test_cli_utilities.get_gdalinfo_path() is None:
        return 'skip'

    ret = os.popen(test_cli_utilities.get_gdalinfo_path() + ' ../gcore/data/byte.tif').read()
    if ret.find('Driver: GTiff/GeoTIFF') == -1:
        return 'fail'

    return 'success'

###############################################################################
# Test -checksum option

def test_gdalinfo_2():
    if test_cli_utilities.get_gdalinfo_path() is None:
        return 'skip'

    ret = os.popen(test_cli_utilities.get_gdalinfo_path() + ' -checksum ../gcore/data/byte.tif').read()
    if ret.find('Checksum=4672') == -1:
        return 'fail'

    return 'success'

###############################################################################
# Test -nomd option

def test_gdalinfo_3():
    if test_cli_utilities.get_gdalinfo_path() is None:
        return 'skip'

    ret = os.popen(test_cli_utilities.get_gdalinfo_path() + ' ../gcore/data/byte.tif').read()
    if ret.find('Metadata') == -1:
        return 'fail'

    ret = os.popen(test_cli_utilities.get_gdalinfo_path() + ' -nomd ../gcore/data/byte.tif').read()
    if ret.find('Metadata') != -1:
        return 'fail'

    return 'success'

###############################################################################
# Test -noct option

def test_gdalinfo_4():
    if test_cli_utilities.get_gdalinfo_path() is None:
        return 'skip'

    ret = os.popen(test_cli_utilities.get_gdalinfo_path() + ' ../gdrivers/data/bug407.gif').read()
    if ret.find('0: 255,255,255,255') == -1:
        return 'fail'

    ret = os.popen(test_cli_utilities.get_gdalinfo_path() + ' -noct ../gdrivers/data/bug407.gif').read()
    if ret.find('0: 255,255,255,255') != -1:
        return 'fail'

    return 'success'

###############################################################################
# Test -stats option

def test_gdalinfo_5():
    if test_cli_utilities.get_gdalinfo_path() is None:
        return 'skip'

    ret = os.popen(test_cli_utilities.get_gdalinfo_path() + ' ../gcore/data/byte.tif').read()
    if ret.find('STATISTICS_MINIMUM=74') != -1:
        return 'fail'

    ret = os.popen(test_cli_utilities.get_gdalinfo_path() + ' -stats ../gcore/data/byte.tif').read()
    if ret.find('STATISTICS_MINIMUM=74') == -1:
        return 'fail'

    return 'success'

###############################################################################
# Test a dataset with overviews and RAT

def test_gdalinfo_6():
    if test_cli_utilities.get_gdalinfo_path() is None:
        return 'skip'

    ret = os.popen(test_cli_utilities.get_gdalinfo_path() + ' ../gdrivers/data/int.img').read()
    if ret.find('Overviews') == -1:
        return 'fail'
    if ret.find('GDALRasterAttributeTable') == -1:
        return 'fail'

    return 'success'

gdaltest_list = [
    test_gdalinfo_1,
    test_gdalinfo_2,
    test_gdalinfo_3,
    test_gdalinfo_4,
    test_gdalinfo_5,
    test_gdalinfo_6
    ]


if __name__ == '__main__':

    gdaltest.setup_run( 'test_gdalinfo' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()





