#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  gdaldem testing
# Author:   Even Rouault <even dot rouault @ mines-paris dot org>
# 
###############################################################################
# Copyright (c) 2009, Even Rouault <even dot rouault @ mines-paris dot org>
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

import gdal
import gdaltest
import test_cli_utilities

###############################################################################
# Test gdaldem shade

def test_gdaldem_shade():
    if test_cli_utilities.get_gdaldem_path() is None:
        return 'skip'

    os.popen(test_cli_utilities.get_gdaldem_path() + ' shade -s 111120 -z 30 ../gdrivers/data/n43.dt0 tmp/n43_shade.tif').read()

    src_ds = gdal.Open('../gdrivers/data/n43.dt0')
    ds = gdal.Open('tmp/n43_shade.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 45587:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    src_gt = src_ds.GetGeoTransform()
    dst_gt = ds.GetGeoTransform()
    for i in range(6):
        if abs(src_gt[i] - dst_gt[i]) > 1e-10:
            gdaltest.post_reason('Bad geotransform')
            return 'fail'

    dst_wkt = ds.GetProjectionRef()
    if dst_wkt.find('AUTHORITY["EPSG","4326"]') == -1:
        gdaltest.post_reason('Bad projection')
        return 'fail'

    if ds.GetRasterBand(1).GetNoDataValue() != 0:
        gdaltest.post_reason('Bad nodata value')
        return 'fail'

    src_ds = None
    ds = None

    return 'success'

###############################################################################
# Test gdaldem slope

def test_gdaldem_slope():
    if test_cli_utilities.get_gdaldem_path() is None:
        return 'skip'

    os.popen(test_cli_utilities.get_gdaldem_path() + ' slope -s 111120 ../gdrivers/data/n43.dt0 tmp/n43_slope.tif').read()

    src_ds = gdal.Open('../gdrivers/data/n43.dt0')
    ds = gdal.Open('tmp/n43_slope.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 63748:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    src_gt = src_ds.GetGeoTransform()
    dst_gt = ds.GetGeoTransform()
    for i in range(6):
        if abs(src_gt[i] - dst_gt[i]) > 1e-10:
            gdaltest.post_reason('Bad geotransform')
            return 'fail'

    dst_wkt = ds.GetProjectionRef()
    if dst_wkt.find('AUTHORITY["EPSG","4326"]') == -1:
        gdaltest.post_reason('Bad projection')
        return 'fail'

    if ds.GetRasterBand(1).GetNoDataValue() != -9999.0:
        gdaltest.post_reason('Bad nodata value')
        return 'fail'

    src_ds = None
    ds = None

    return 'success'

###############################################################################
# Test gdaldem aspect

def test_gdaldem_aspect():
    if test_cli_utilities.get_gdaldem_path() is None:
        return 'skip'

    os.popen(test_cli_utilities.get_gdaldem_path() + ' aspect ../gdrivers/data/n43.dt0 tmp/n43_aspect.tif').read()

    src_ds = gdal.Open('../gdrivers/data/n43.dt0')
    ds = gdal.Open('tmp/n43_aspect.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 54885:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    src_gt = src_ds.GetGeoTransform()
    dst_gt = ds.GetGeoTransform()
    for i in range(6):
        if abs(src_gt[i] - dst_gt[i]) > 1e-10:
            gdaltest.post_reason('Bad geotransform')
            return 'fail'

    dst_wkt = ds.GetProjectionRef()
    if dst_wkt.find('AUTHORITY["EPSG","4326"]') == -1:
        gdaltest.post_reason('Bad projection')
        return 'fail'

    if ds.GetRasterBand(1).GetNoDataValue() != -9999.0:
        gdaltest.post_reason('Bad nodata value')
        return 'fail'

    src_ds = None
    ds = None

    return 'success'
###############################################################################
# Cleanup

def test_gdaldem_cleanup():
    try:
        os.remove('tmp/n43_shade.tif')
    except:
        pass
    try:
        os.remove('tmp/n43_slope.tif')
    except:
        pass
    try:
        os.remove('tmp/n43_aspect.tif')
    except:
        pass

    return 'success'

gdaltest_list = [
    test_gdaldem_shade,
    test_gdaldem_slope,
    test_gdaldem_aspect,
    test_gdaldem_cleanup
    ]


if __name__ == '__main__':

    gdaltest.setup_run( 'test_gdaldem' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()
