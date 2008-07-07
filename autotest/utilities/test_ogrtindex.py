#!/usr/bin/env python
###############################################################################
# $Id: test_ogrtindex.py $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  ogrtindex testing
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

import ogr
import gdaltest
import test_cli_utilities

###############################################################################
# Simple test

def test_ogrtindex_1():
    if test_cli_utilities.get_ogrtindex_path() is None:
        return 'skip'

    shape_drv = ogr.GetDriverByName('ESRI Shapefile')

    shape_drv.DeleteDataSource('tmp/tileindex.shp')

    shape_ds = shape_drv.CreateDataSource( 'tmp' )

    shape_lyr = shape_ds.CreateLayer( 'point1' )
    dst_feat = ogr.Feature( feature_def = shape_lyr.GetLayerDefn() )
    dst_feat.SetGeometry(ogr.CreateGeometryFromWkt('POINT(49 2)'))
    shape_lyr.CreateFeature( dst_feat )
    dst_feat.Destroy()

    shape_lyr = shape_ds.CreateLayer( 'point2' )
    dst_feat = ogr.Feature( feature_def = shape_lyr.GetLayerDefn() )
    dst_feat.SetGeometry(ogr.CreateGeometryFromWkt('POINT(49 3)'))
    shape_lyr.CreateFeature( dst_feat )
    dst_feat.Destroy()

    shape_lyr = shape_ds.CreateLayer( 'point3' )
    dst_feat = ogr.Feature( feature_def = shape_lyr.GetLayerDefn() )
    dst_feat.SetGeometry(ogr.CreateGeometryFromWkt('POINT(48 2)'))
    shape_lyr.CreateFeature( dst_feat )
    dst_feat.Destroy()

    shape_lyr = shape_ds.CreateLayer( 'point4' )
    dst_feat = ogr.Feature( feature_def = shape_lyr.GetLayerDefn() )
    dst_feat.SetGeometry(ogr.CreateGeometryFromWkt('POINT(48 3)'))
    shape_lyr.CreateFeature( dst_feat )
    dst_feat.Destroy()

    shape_ds.Destroy()

    os.popen(test_cli_utilities.get_ogrtindex_path() + ' -skip_different_projection tmp/tileindex.shp tmp/point1.shp tmp/point2.shp tmp/point3.shp tmp/point4.shp').read()

    ds = ogr.Open('tmp/tileindex.shp')
    if ds.GetLayer(0).GetFeatureCount() != 4:
        return 'fail'

    expected_wkts =['POLYGON ((49 2,49 2,49 2,49 2,49 2))',
                    'POLYGON ((49 3,49 3,49 3,49 3,49 3))',
                    'POLYGON ((48 2,48 2,48 2,48 2,48 2))',
                    'POLYGON ((48 3,48 3,48 3,48 3,48 3))' ]
    i = 0
    feat = ds.GetLayer(0).GetNextFeature()
    while feat is not None:
        if feat.GetGeometryRef().ExportToWkt() != expected_wkts[i]:
            print 'i=%d, wkt=%s' % (i, feat.GetGeometryRef().ExportToWkt())
            return 'fail'
        i = i + 1
        feat = ds.GetLayer(0).GetNextFeature()

    return 'success'

###############################################################################
# Cleanup

def test_ogrtindex_cleanup():

    shape_drv = ogr.GetDriverByName('ESRI Shapefile')
    shape_drv.DeleteDataSource('tmp/tileindex.shp')
    shape_drv.DeleteDataSource('tmp/point1.shp')
    shape_drv.DeleteDataSource('tmp/point2.shp')
    shape_drv.DeleteDataSource('tmp/point3.shp')
    shape_drv.DeleteDataSource('tmp/point4.shp')

    return 'success'


gdaltest_list = [
    test_ogrtindex_1,
    test_ogrtindex_cleanup
    ]


if __name__ == '__main__':

    gdaltest.setup_run( 'test_ogrtindex' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()





