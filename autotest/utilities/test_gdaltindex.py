#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  gdaltindex testing
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

import gdal
import ogr
import gdaltest
import test_cli_utilities

###############################################################################
# Simple test

def test_gdaltindex_1():
    if test_cli_utilities.get_gdaltindex_path() is None:
        return 'skip'

    try:
        os.remove('tmp/tileindex.shp')
    except:
        pass
    try:
        os.remove('tmp/tileindex.dbf')
    except:
        pass
    try:
        os.remove('tmp/tileindex.shx')
    except:
        pass

    drv = gdal.GetDriverByName('GTiff')
    wkt = 'GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9108\"]],AUTHORITY[\"EPSG\",\"4326\"]]'

    ds = drv.Create('tmp/gdaltindex1.tif', 10, 10, 1)
    ds.SetProjection( wkt )
    ds.SetGeoTransform( [ 49, 0.1, 0, 2, 0, -0.1 ] )
    ds = None

    ds = drv.Create('tmp/gdaltindex2.tif', 10, 10, 1)
    ds.SetProjection( wkt )
    ds.SetGeoTransform( [ 49, 0.1, 0, 3, 0, -0.1 ] )
    ds = None

    ds = drv.Create('tmp/gdaltindex3.tif', 10, 10, 1)
    ds.SetProjection( wkt )
    ds.SetGeoTransform( [ 48, 0.1, 0, 2, 0, -0.1 ] )
    ds = None

    ds = drv.Create('tmp/gdaltindex4.tif', 10, 10, 1)
    ds.SetProjection( wkt )
    ds.SetGeoTransform( [ 48, 0.1, 0, 3, 0, -0.1 ] )
    ds = None

    os.popen(test_cli_utilities.get_gdaltindex_path() + ' tmp/tileindex.shp tmp/gdaltindex1.tif tmp/gdaltindex2.tif tmp/gdaltindex3.tif tmp/gdaltindex4.tif').read()

    ds = ogr.Open('tmp/tileindex.shp')
    if ds.GetLayer(0).GetFeatureCount() != 4:
        return 'fail'

    expected_wkts =['POLYGON ((49 2,50 2,50 1,49 1,49 2))',
                    'POLYGON ((49 3,50 3,50 2,49 2,49 3))',
                    'POLYGON ((48 2,49 2,49 1,48 1,48 2))',
                    'POLYGON ((48 3,49 3,49 2,48 2,48 3))' ]
    i = 0
    feat = ds.GetLayer(0).GetNextFeature()
    while feat is not None:
        if feat.GetGeometryRef().ExportToWkt() != expected_wkts[i]:
            print 'i=%d, wkt=%s' % (i, feat.GetGeometryRef().ExportToWkt())
            return 'fail'
        i = i + 1
        feat = ds.GetLayer(0).GetNextFeature()
    ds.Destroy()

    return 'success'

###############################################################################
# Try adding the same rasters again

def test_gdaltindex_2():
    if test_cli_utilities.get_gdaltindex_path() is None:
        return 'skip'

    os.popen(test_cli_utilities.get_gdaltindex_path() + ' tmp/tileindex.shp tmp/gdaltindex1.tif tmp/gdaltindex2.tif tmp/gdaltindex3.tif tmp/gdaltindex4.tif').read()

    ds = ogr.Open('tmp/tileindex.shp')
    if ds.GetLayer(0).GetFeatureCount() != 4:
        return 'fail'
    ds.Destroy()

    return 'success'


###############################################################################
# Try adding a raster in another projection with -skip_different_projection

def test_gdaltindex_3():
    if test_cli_utilities.get_gdaltindex_path() is None:
        return 'skip'

    drv = gdal.GetDriverByName('GTiff')
    wkt = 'GEOGCS[\"WGS 72\",DATUM[\"WGS_1972\"]]'

    ds = drv.Create('tmp/gdaltindex5.tif', 10, 10, 1)
    ds.SetProjection( wkt )
    ds.SetGeoTransform( [ 47, 0.1, 0, 2, 0, -0.1 ] )
    ds = None

    os.popen(test_cli_utilities.get_gdaltindex_path() + ' -skip_different_projection tmp/tileindex.shp tmp/gdaltindex5.tif').read()

    ds = ogr.Open('tmp/tileindex.shp')
    if ds.GetLayer(0).GetFeatureCount() != 4:
        return 'fail'
    ds.Destroy()

    return 'success'

###############################################################################
# Cleanup

def test_gdaltindex_cleanup():

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/tileindex.shp')

    drv = gdal.GetDriverByName('GTiff')

    drv.Delete('tmp/gdaltindex1.tif')
    drv.Delete('tmp/gdaltindex2.tif')
    drv.Delete('tmp/gdaltindex3.tif')
    drv.Delete('tmp/gdaltindex4.tif')
    drv.Delete('tmp/gdaltindex5.tif')

    return 'success'


gdaltest_list = [
    test_gdaltindex_1,
    test_gdaltindex_2,
    test_gdaltindex_3,
    test_gdaltindex_cleanup
    ]


if __name__ == '__main__':

    gdaltest.setup_run( 'test_gdaltindex' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()





