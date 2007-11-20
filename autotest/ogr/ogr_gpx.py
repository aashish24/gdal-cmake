#!/usr/bin/env python
###############################################################################
# $Id: ogr_gpx.py $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test GPX driver functionality.
# Author:   Even Rouault <even dot rouault at mines dash paris dot org>
# 
###############################################################################
# Copyright (c) 2007, Even Rouault <even dot rouault at mines dash paris dot org>
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
import string

sys.path.append( '../pymod' )

import gdaltest
import ogrtest
import ogr
import osr
import gdal

def ogr_gpx_init():
    gdaltest.gpx_ds = None

    try:
        gdaltest.gpx_ds = ogr.Open( 'data/test.gpx' )
    except:
        gdaltest.gpx_ds = None

    if gdaltest.gpx_ds is None:
        gdaltest.have_gpx = 0
    else:
        gdaltest.have_gpx = 1

    if not gdaltest.have_gpx:
        return 'skip'

    if gdaltest.gpx_ds.GetLayerCount() != 3:
        gdaltest.post_reason( 'wrong number of layers' )
        return 'fail'

    return 'success'

###############################################################################
# Test waypoints gpx layer.

def ogr_gpx_1():
    if not gdaltest.have_gpx:
        return 'skip'
    
    if gdaltest.gpx_ds is None:
        return 'fail'

    lyr = gdaltest.gpx_ds.GetLayerByName( 'waypoints' )
    
    expect = [2, None]

    tr = ogrtest.check_features_against_list( lyr, 'ele', expect )
    
    lyr.ResetReading()
    
    expect = ['waypoint name', None]

    tr = ogrtest.check_features_against_list( lyr, 'name', expect )

    lyr.ResetReading()
    feat = lyr.GetNextFeature()
    if ogrtest.check_feature_geometry( feat, 'POINT (1 0)',
                                       max_error = 0.0001 ) != 0:
        return 'fail'
    feat.Destroy()
    
    feat = lyr.GetNextFeature()
    if ogrtest.check_feature_geometry( feat, 'POINT (4 3)',
                                       max_error = 0.0001 ) != 0:
        return 'fail'
    feat.Destroy()
    
    return 'success'

###############################################################################
# Test routes gpx layer.

def ogr_gpx_2():
    if not gdaltest.have_gpx:
        return 'skip'
    
    if gdaltest.gpx_ds is None:
        return 'fail'

    lyr = gdaltest.gpx_ds.GetLayerByName( 'routes' )

    lyr.ResetReading()
    feat = lyr.GetNextFeature()
    if ogrtest.check_feature_geometry( feat, 'LINESTRING (6 5,9 8,12 11)', max_error = 0.0001 ) != 0:
        return 'fail'
    feat.Destroy()
    
    return 'success'

###############################################################################
# Test tracks gpx layer.

def ogr_gpx_3():
    if not gdaltest.have_gpx:
        return 'skip'

    if gdaltest.gpx_ds is None:
        return 'fail'

    lyr = gdaltest.gpx_ds.GetLayerByName( 'tracks' )

    lyr.ResetReading()
    feat = lyr.GetNextFeature()
    if ogrtest.check_feature_geometry( feat, 'MULTILINESTRING ((15 14,18 17),(21 20,24 23))', max_error = 0.0001 ) != 0:
        return 'fail'
    feat.Destroy()
    
    return 'success'

###############################################################################
# Copy our small gpx file to a new gpx file. 

def ogr_gpx_4():
    if not gdaltest.have_gpx:
        return 'skip'
    
    if gdaltest.gpx_ds is None:
        return 'skip'
    
    try:
        gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
        ogr.GetDriverByName('CSV').DeleteDataSource( 'tmp/gpx.gpx' )
        gdal.PopErrorHandler()
    except:
        pass

    co_opts = [ ]
    
    # Duplicate waypoints
    gpx_lyr = gdaltest.gpx_ds.GetLayerByName( 'waypoints' )

    gpx2_ds = ogr.GetDriverByName('GPX').CreateDataSource('tmp/gpx.gpx',
                                                          options = co_opts )

    gpx2_lyr = gpx2_ds.CreateLayer( 'waypoints', geom_type = ogr.wkbPoint )

    gpx_lyr.ResetReading()

    dst_feat = ogr.Feature( feature_def = gpx2_lyr.GetLayerDefn() )
    
    feat = gpx_lyr.GetNextFeature()
    while feat is not None:
        dst_feat.SetFrom( feat )
        if gpx2_lyr.CreateFeature( dst_feat ) != 0:
            gdaltest.post_reason('CreateFeature failed.')
            return 'fail'

        feat = gpx_lyr.GetNextFeature()

    dst_feat.Destroy()

    # Duplicate routes
    gpx_lyr = gdaltest.gpx_ds.GetLayerByName( 'routes' )

    gpx2_lyr = gpx2_ds.CreateLayer( 'routes', geom_type = ogr.wkbLineString )

    gpx_lyr.ResetReading()

    dst_feat = ogr.Feature( feature_def = gpx2_lyr.GetLayerDefn() )
    
    feat = gpx_lyr.GetNextFeature()
    while feat is not None:
        dst_feat.SetFrom( feat )
        if gpx2_lyr.CreateFeature( dst_feat ) != 0:
            gdaltest.post_reason('CreateFeature failed.')
            return 'fail'

        feat = gpx_lyr.GetNextFeature()

    dst_feat.Destroy()
    
    # Duplicate tracks
    gpx_lyr = gdaltest.gpx_ds.GetLayerByName( 'tracks' )

    gpx2_lyr = gpx2_ds.CreateLayer( 'tracks', geom_type = ogr.wkbMultiLineString )

    gpx_lyr.ResetReading()

    dst_feat = ogr.Feature( feature_def = gpx2_lyr.GetLayerDefn() )
    
    feat = gpx_lyr.GetNextFeature()
    while feat is not None:
        dst_feat.SetFrom( feat )
        if gpx2_lyr.CreateFeature( dst_feat ) != 0:
            gdaltest.post_reason('CreateFeature failed.')
            return 'fail'

        feat = gpx_lyr.GetNextFeature()

    dst_feat.Destroy()
    
    gpx_lyr = None
    gpx2_lyr = None

    # Explicit destroy is required for old-gen python bindings
    gpx2_ds.Destroy()
    gdaltest.gpx_ds.Destroy()

    gdaltest.gpx_ds = ogr.Open( 'tmp/gpx.gpx' )

    return 'success'
    
###############################################################################
# 

def ogr_gpx_cleanup():

    if gdaltest.gpx_ds is not None:
        gdaltest.gpx_ds.Destroy()
    gdaltest.gpx_ds = None
    try:
        os.remove ('tmp/gpx.gpx')
    except:
        pass
    return 'success'

gdaltest_list = [ 
    ogr_gpx_init,
    ogr_gpx_1,
    ogr_gpx_2,
    ogr_gpx_3,
    ogr_gpx_4,
# Rerun test 1, 2 and 3 with generated tmp/tmp.gpx
    ogr_gpx_1,
    ogr_gpx_2,
    ogr_gpx_3,
    ogr_gpx_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'ogr_gpx' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

