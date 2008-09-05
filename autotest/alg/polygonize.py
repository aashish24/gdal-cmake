#!/usr/bin/env python
###############################################################################
# $Id: proximity.py 14976 2008-07-19 13:10:51Z rouault $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test Polygonize() algorithm.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2008, Frank Warmerdam <warmerdam@pobox.com>
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
import ogrtest

try:
    from osgeo import gdal, gdalconst, ogr
except:
    import gdal
    import gdalconst
    import ogr

###############################################################################
# Test a fairly simple case, with nodata masking.

def polygonize_1():

    try:
        x = gdal.Polygonize
        gdaltest.have_ng = 1
    except:
        gdaltest.have_ng = 0
        return 'skip'

    src_ds = gdal.Open('data/polygonize_in.grd')
    src_band = src_ds.GetRasterBand(1)

    # Create a memory OGR datasource to put results in. 
    mem_drv = ogr.GetDriverByName( 'Memory' )
    mem_ds = mem_drv.CreateDataSource( 'out' )

    mem_layer = mem_ds.CreateLayer( 'poly', None, ogr.wkbPolygon )

    fd = ogr.FieldDefn( 'DN', ogr.OFTInteger )
    mem_layer.CreateField( fd )

    # run the algorithm.
    result = gdal.Polygonize( src_band, src_band.GetMaskBand(), mem_layer, 0 )

    # Confirm we get the set of expected features in the output layer.

    if mem_layer.GetFeatureCount() != 13:
        gdaltest.post_reason( 'GetFeatureCount() returned %d instead of 11' % mem_layer.GetFeatureCount() )
        return 'fail'

    expect = [ 107, 123, 115, 115, 140, 148, 123, 140, 100, 101,
               102, 156, 103 ]
    
    tr = ogrtest.check_features_against_list( mem_layer, 'DN', expect )

    # check at least one geometry.
    if tr:
        mem_layer.SetAttributeFilter( 'dn = 156' )
        feat_read = mem_layer.GetNextFeature()
        if ogrtest.check_feature_geometry( feat_read, 'POLYGON ((440720 3751200,440720 3751080,440720 3751020,440840 3751020,440900 3751020,440900 3751080,440900 3751200,440840 3751200,440720 3751200),(440780 3751140,440840 3751140,440840 3751080,440780 3751080,440780 3751140))' ) != 0:
            tr = 0
        feat_read.Destroy()
        
    if tr:
        return 'success'
    else:
        return 'fail'

###############################################################################
# Test a simple case without masking.

def polygonize_2():

    if not gdaltest.have_ng:
        return 'skip'
    
    src_ds = gdal.Open('data/polygonize_in.grd')
    src_band = src_ds.GetRasterBand(1)

    # Create a memory OGR datasource to put results in. 
    mem_drv = ogr.GetDriverByName( 'Memory' )
    mem_ds = mem_drv.CreateDataSource( 'out' )

    mem_layer = mem_ds.CreateLayer( 'poly', None, ogr.wkbPolygon )

    fd = ogr.FieldDefn( 'DN', ogr.OFTInteger )
    mem_layer.CreateField( fd )

    # run the algorithm.
    result = gdal.Polygonize( src_band, None, mem_layer, 0 )

    # Confirm we get the set of expected features in the output layer.

    if mem_layer.GetFeatureCount() != 17:
        gdaltest.post_reason( 'GetFeatureCount() returned %d instead of 11' % mem_layer.GetFeatureCount() )
        return 'fail'

    expect = [ 107, 123, 115, 132, 115, 132, 140, 132, 148, 123, 140, 132,
               100, 101, 102, 156, 103 ]
    
    tr = ogrtest.check_features_against_list( mem_layer, 'DN', expect )

    if tr:
        return 'success'
    else:
        return 'fail'

gdaltest_list = [
    polygonize_1,
    polygonize_2
    ]

if __name__ == '__main__':

    gdaltest.setup_run( 'polygonize' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

