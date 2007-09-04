#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test JPEG format driver.
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
import gdalconst

###############################################################################
# Perform simple read test.

def jpeg_1():

    tst = gdaltest.GDALTest( 'JPEG', 'albania.jpg', 2, 17016 )
    return tst.testOpen()

###############################################################################
# Verify EXIF metadata, and color interpretation

def jpeg_2():

    ds = gdal.Open( 'data/albania.jpg' )

    md = ds.GetMetadata()

    try:
        if md['EXIF_GPSLatitudeRef'] != 'N' \
           or md['EXIF_GPSLatitude'] != '(41) (1) (22.91)' \
           or md['EXIF_PixelXDimension'] != '361' \
           or md['EXIF_GPSVersionID'] != '0x2 00 00 00' \
           or md['EXIF_ExifVersion'] != '021' \
           or md['EXIF_XResolution'] != '(96)' \
           or md['EXIF_GPSLatitudeRef'] != 'N':
            print md
            gdaltest.post_reason( 'Exif metadata wrong.' )
            return 'fail'
    except:
        print md
        gdaltest.post_reason( 'Exit metadata apparently missing.' )
        return 'fail'

    if ds.GetRasterBand(3).GetRasterColorInterpretation() != gdal.GCI_BlueBand:
        gdaltest.post_reason( 'Did not get expected color interpretation.' )
        return 'fail'

    return 'success'

###############################################################################
# Create simple copy and check (greyscale) using progressive option.

def jpeg_3():

    ds = gdal.Open('data/byte.tif')

    options = ['PROGRESSIVE=YES',
               'QUALITY=50']
    ds = gdal.GetDriverByName('JPEG').CreateCopy( 'tmp/byte.jpg', ds,
                                                  options = options )
                                                  
    if ds.GetRasterBand(1).Checksum() != 4794:
        gdaltest.post_reason( 'Wrong checksum on copied image.')
        print ds.GetRasterBand(1).Checksum()
        return 'fail'

    return 'success'
    
###############################################################################
# Verify masked jpeg. 

def jpeg_4():

    try:
        gdalconst.GMF_ALL_VALID
    except:
        return 'skip'
    
    ds = gdal.Open('data/masked.jpg')

    refband = ds.GetRasterBand(1)

    if refband.GetMaskFlags() != gdalconst.GMF_PER_DATASET:
        gdaltest.post_reason( 'wrong mask flags' )
        return 'fail'

    cs = refband.GetMaskBand().Checksum()
    if cs != 770:
        gdaltest.post_reason( 'Wrong mask checksum' )
        print cs
        return 'fail'

    return 'success'
    
###############################################################################
# Verify CreateCopy() of masked jpeg.

def jpeg_5():

    try:
        gdalconst.GMF_ALL_VALID
    except:
        return 'skip'
    
    ds = gdal.Open('data/masked.jpg')

    ds2 = gdal.GetDriverByName('JPEG').CreateCopy( 'tmp/masked.jpg', ds )

    refband = ds2.GetRasterBand(1)

    if refband.GetMaskFlags() != gdalconst.GMF_PER_DATASET:
        gdaltest.post_reason( 'wrong mask flags' )
        return 'fail'

    cs = refband.GetMaskBand().Checksum()
    if cs != 770:
        gdaltest.post_reason( 'Wrong checksum on copied images mask.')
        print cs
        return 'fail'

    return 'success'
    
###############################################################################
# Create simple copy and check (greyscale) using progressive option.

def jpeg_cleanup():
    gdaltest.clean_tmp()
    return 'success'

gdaltest_list = [
    jpeg_1,
    jpeg_2,
    jpeg_3,
    jpeg_4,
    jpeg_5,
    jpeg_cleanup
    ]
  


if __name__ == '__main__':

    gdaltest.setup_run( 'jpeg' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

