#!/usr/bin/env python
###############################################################################
# $Id: test_ogr2ogr.py $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  ogr2ogr testing
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
sys.path.append( '../ogr' )

import gdal
import ogr
import gdaltest
import ogrtest
import test_cli_utilities

###############################################################################
# Simple test

def test_ogr2ogr_1():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' tmp/poly.shp ../ogr/data/poly.shp').read()

    ds = ogr.Open('tmp/poly.shp')
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -sql

def test_ogr2ogr_2():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' tmp/poly.shp ../ogr/data/poly.shp -sql "select * from poly"').read()

    ds = ogr.Open('tmp/poly.shp')
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -spat

def test_ogr2ogr_3():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' tmp/poly.shp ../ogr/data/poly.shp -spat 479609 4764629 479764 4764817').read()

    ds = ogr.Open('tmp/poly.shp')
    if ogrtest.have_geos():
        if ds is None or ds.GetLayer(0).GetFeatureCount() != 4:
            return 'fail'
    else:
        if ds is None or ds.GetLayer(0).GetFeatureCount() != 5:
            return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -where

def test_ogr2ogr_4():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' tmp/poly.shp ../ogr/data/poly.shp -where "EAS_ID=171"').read()

    ds = ogr.Open('tmp/poly.shp')
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 1:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -append

def test_ogr2ogr_5():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' tmp/poly.shp ../ogr/data/poly.shp').read()
    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -update -append tmp/poly.shp ../ogr/data/poly.shp').read()

    ds = ogr.Open('tmp/poly.shp')
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 20:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -overwrite

def test_ogr2ogr_6():

    import ogr_pg

    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'
    if test_cli_utilities.get_ogrinfo_path() is None:
        return 'skip'

    ogr_pg.ogr_pg_1()
    if gdaltest.pg_ds is None:
        return 'skip'
    gdaltest.pg_ds.Destroy()

    os.popen(test_cli_utilities.get_ogrinfo_path() + ' PG:' + gdaltest.pg_connection_string + ' -sql "DELLAYER:tpoly"')

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -f PostgreSQL PG:' + gdaltest.pg_connection_string + ' ../ogr/data/poly.shp -nln tpoly').read()
    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -update -overwrite -f PostgreSQL PG:' + gdaltest.pg_connection_string + ' ../ogr/data/poly.shp -nln tpoly').read()

    ds = ogr.Open('PG:' + gdaltest.pg_connection_string)
    if ds is None or ds.GetLayerByName('tpoly').GetFeatureCount() != 10:
        return 'fail'
    ds.Destroy()

    os.popen(test_cli_utilities.get_ogrinfo_path() + ' PG:' + gdaltest.pg_connection_string + ' -sql "DELLAYER:tpoly"')

    return 'success'

###############################################################################
# Test -gt

def test_ogr2ogr_7():

    import ogr_pg

    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'
    if test_cli_utilities.get_ogrinfo_path() is None:
        return 'skip'

    ogr_pg.ogr_pg_1()
    if gdaltest.pg_ds is None:
        return 'skip'
    gdaltest.pg_ds.Destroy()

    os.popen(test_cli_utilities.get_ogrinfo_path() + ' PG:' + gdaltest.pg_connection_string + ' -sql "DELLAYER:tpoly"')

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -f PostgreSQL PG:' + gdaltest.pg_connection_string + ' ../ogr/data/poly.shp -nln tpoly -gt 1').read()

    ds = ogr.Open('PG:' + gdaltest.pg_connection_string)
    if ds is None or ds.GetLayerByName('tpoly').GetFeatureCount() != 10:
        return 'fail'
    ds.Destroy()

    os.popen(test_cli_utilities.get_ogrinfo_path() + ' PG:' + gdaltest.pg_connection_string + ' -sql "DELLAYER:tpoly"')

    return 'success'

###############################################################################
# Test -t_srs

def test_ogr2ogr_8():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -t_srs EPSG:4326 tmp/poly.shp ../ogr/data/poly.shp').read()

    ds = ogr.Open('tmp/poly.shp')
    if str(ds.GetLayer(0).GetSpatialRef()).find('1984') == -1:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -a_srs

def test_ogr2ogr_9():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -a_srs EPSG:4326 tmp/poly.shp ../ogr/data/poly.shp').read()

    ds = ogr.Open('tmp/poly.shp')
    if str(ds.GetLayer(0).GetSpatialRef()).find('1984') == -1:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -select

def test_ogr2ogr_10():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -select AREA tmp/poly.shp ../ogr/data/poly.shp').read()

    ds = ogr.Open('tmp/poly.shp')
    if ds.GetLayer(0).GetLayerDefn().GetFieldCount() != 1:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -lco

def test_ogr2ogr_11():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -lco SHPT=POLYGONZ tmp/poly.shp ../ogr/data/poly.shp').read()

    ds = ogr.Open('tmp/poly.shp')
    if ds.GetLayer(0).GetLayerDefn().GetGeomType() != ogr.wkbPolygon25D:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -nlt

def test_ogr2ogr_12():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -nlt POLYGON25D tmp/poly.shp ../ogr/data/poly.shp').read()

    ds = ogr.Open('tmp/poly.shp')
    if ds.GetLayer(0).GetLayerDefn().GetGeomType() != ogr.wkbPolygon25D:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Add explicit source layer name

def test_ogr2ogr_13():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' tmp/poly.shp ../ogr/data/poly.shp poly').read()

    ds = ogr.Open('tmp/poly.shp')
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -segmentize

def test_ogr2ogr_14():
    if test_cli_utilities.get_ogr2ogr_path() is None:
        return 'skip'

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    os.popen(test_cli_utilities.get_ogr2ogr_path() + ' -segmentize 100 tmp/poly.shp ../ogr/data/poly.shp poly').read()

    ds = ogr.Open('tmp/poly.shp')
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'
    feat = ds.GetLayer(0).GetNextFeature()
    if feat.GetGeometryRef().GetGeometryRef(0).GetPointCount() != 70:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

gdaltest_list = [
    test_ogr2ogr_1,
    test_ogr2ogr_2,
    test_ogr2ogr_3,
    test_ogr2ogr_4,
    test_ogr2ogr_5,
    test_ogr2ogr_6,
    test_ogr2ogr_7,
    test_ogr2ogr_8,
    test_ogr2ogr_9,
    test_ogr2ogr_10,
    test_ogr2ogr_11,
    test_ogr2ogr_12,
    test_ogr2ogr_13,
    test_ogr2ogr_14
    ]


if __name__ == '__main__':

    gdaltest.setup_run( 'test_ogr2ogr' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

