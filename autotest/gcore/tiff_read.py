#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test basic read support for a all datatypes from a TIFF file.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
# 
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
###############################################################################

import os
import sys

sys.path.append( '../pymod' )

import gdaltest
import gdal

###############################################################################
# When imported build a list of units based on the files available.

gdaltest_list = []

init_list = [ \
    ('byte.tif', 1, 4672, None),
    ('int10.tif', 1, 4672, None),
    ('int12.tif', 1, 4672, None),
    ('int16.tif', 1, 4672, None),
    ('uint16.tif', 1, 4672, None),
    ('int24.tif', 1, 4672, None),
    ('int32.tif', 1, 4672, None),
    ('uint32.tif', 1, 4672, None),
    ('float16.tif', 1, 4672, None),
    ('float24.tif', 1, 4672, None),
    ('float32.tif', 1, 4672, None),
    ('float32_minwhite.tif', 1, 1, None),
    ('float64.tif', 1, 4672, None),
    ('cint16.tif', 1, 5028, None),
    ('cint32.tif', 1, 5028, None),
    ('cfloat32.tif', 1, 5028, None),
    ('cfloat64.tif', 1, 5028, None),
# The following four related partial final strip/tiles (#1179)
    ('separate_tiled.tif', 2, 15234, None), 
    ('seperate_strip.tif', 2, 15234, None),
    ('contig_tiled.tif', 2, 15234, None),
    ('contig_strip.tif', 2, 15234, None),
    ('empty1bit.tif', 1, 0, None)]

###############################################################################
# Test absolute/offset && index directory access

def tiff_read_off():

    # Test absolute/offset directory access 
    ds = gdal.Open('GTIFF_DIR:off:408:data/byte.tif')
    if ds.GetRasterBand(1).Checksum() != 4672:
        return 'fail'

    # Test index directory access
    ds = gdal.Open('GTIFF_DIR:1:data/byte.tif')
    if ds.GetRasterBand(1).Checksum() != 4672:
        return 'fail'

    return 'success'


for item in init_list:
    ut = gdaltest.GDALTest( 'GTiff', item[0], item[1], item[2] )
    if ut is None:
	print( 'GTiff tests skipped' )
	sys.exit()
    gdaltest_list.append( (ut.testOpen, item[0]) )
gdaltest_list.append( (tiff_read_off) )

if __name__ == '__main__':

    gdaltest.setup_run( 'tiff_read' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

