#!/usr/bin/env python
###############################################################################
# $Id: leveller.py 11674 2007-06-19 19:10:19Z mloskot $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test PDS/ISIS formats.
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
# Perform simple read test on isis3 dataset.

def isis_1():
    srs = """PROJCS["Equirectangular Mars",
    GEOGCS["GCS_Mars",
        DATUM["D_Mars",
            SPHEROID["Mars_localRadius",3394813857.975946,0]],
        PRIMEM["Reference_Meridian",0],
        UNIT["degree",0.0174532925199433]],
    PROJECTION["Equirectangular"],
    PARAMETER["latitude_of_origin",-15.14700031280518],
    PARAMETER["central_meridian",184.4129943847656],
    PARAMETER["false_easting",0],
    PARAMETER["false_northing",0]]
"""  
    gt = (-4766.96484375, 10.102499961853027, 0.0,
          -872623.625, 0.0, 10.102499961853027)
    
    tst = gdaltest.GDALTest( 'ISIS3', 'isis3_detached.lbl', 1, 9978 )
    return tst.testOpen( check_prj = srs, check_gt = gt )
    
gdaltest_list = [
    isis_1  ]


if __name__ == '__main__':

    gdaltest.setup_run( 'isis' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

