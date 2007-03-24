#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test ERS format driver.
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
import gdal

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Perform simple read test.

def ers_1():

    tst = gdaltest.GDALTest( 'ERS', 'srtm.ers', 1, 244 )
    return tst.testOpen()

###############################################################################
# Create simple copy and check.

def ers_2():

    tst = gdaltest.GDALTest( 'ERS', 'float32.bil', 1, 27 )
    return tst.testCreateCopy( new_filename = 'tmp/float32.ers',
                               check_gt = 1, check_srs = 1 )
    
###############################################################################
# Test multi-band file.

def ers_3():

    tst = gdaltest.GDALTest( 'ERS', 'rgbsmall.tif', 2, 21053 )
    return tst.testCreate( new_filename = 'tmp/rgbsmall.ers' )
    
###############################################################################
# Create simple copy and check (greyscale) using progressive option.

def ers_cleanup():
    gdaltest.clean_tmp()
    return 'success'

gdaltest_list = [
    ers_1,
    ers_2,
    ers_3,
    ers_cleanup
    ]
  


if __name__ == '__main__':

    gdaltest.setup_run( 'ers' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

