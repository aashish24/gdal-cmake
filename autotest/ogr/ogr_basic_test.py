#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test basic OGR functionality against test shapefiles.
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
import ogr

###############################################################################

def ogr_basic_1():

    gdaltest.ds = ogr.Open( 'data/poly.shp' )

    if gdaltest.ds is not None:
        return 'success'
    else:
        return 'fail'

###############################################################################
# Test Feature counting.

def ogr_basic_2():

    gdaltest.lyr = gdaltest.ds.GetLayerByName( 'poly' )

    count = gdaltest.lyr.GetFeatureCount()
    if count != 10:
        gdaltest.post_reason( 'Got wrong count with GetFeatureCount() - %d, expecting 10' % count )
        return 'fail'

    # Now actually iterate through counting the features and ensure they agree.
    gdaltest.lyr.ResetReading()

    count2 = 0
    feat = gdaltest.lyr.GetNextFeature()
    while feat is not None:
        count2 = count2 + 1
        feat.Destroy()
        feat = gdaltest.lyr.GetNextFeature()
        
    if count2 != 10:
        gdaltest.post_reason( 'Got wrong count with GetNextFeature() - %d, expecting 10' % count2 )
        return 'fail'

    return 'success'

###############################################################################
# Test Spatial Query.

def ogr_basic_3():

    minx = 479405
    miny = 4762826
    maxx = 480732
    maxy = 4763590

    ###########################################################################
    # Create query geometry.

    ring = ogr.Geometry( type = ogr.wkbLinearRing )
    ring.AddPoint( minx, miny )
    ring.AddPoint( maxx, miny )
    ring.AddPoint( maxx, maxy )
    ring.AddPoint( minx, maxy )
    ring.AddPoint( minx, miny )

    poly = ogr.Geometry( type = ogr.wkbPolygon )
    poly.AddGeometryDirectly( ring )

    gdaltest.lyr.SetSpatialFilter( poly )
    gdaltest.lyr.ResetReading()

    poly.Destroy()

    count = gdaltest.lyr.GetFeatureCount()
    if count != 1:
        gdaltest.post_reason( 'Got wrong feature count with spatial filter, expected 1, got %d' % count )
        return 'fail'

    feat1 = gdaltest.lyr.GetNextFeature()
    feat2 = gdaltest.lyr.GetNextFeature()

    if feat1 is None or feat2 is not None:
        gdaltest.post_reason( 'Got too few or too many features with spatial filter.' )
        return 'fail'

    feat1.Destroy()

    gdaltest.lyr.SetSpatialFilter( None )
    count = gdaltest.lyr.GetFeatureCount()
    if count != 10:
        gdaltest.post_reason( 'Clearing spatial query may not have worked properly, getting\n%d features instead of expected 10 features.' % count )
        return 'fail'

    return 'success'

###############################################################################
# Test GetDriver().

def ogr_basic_4():
    driver = gdaltest.ds.GetDriver()
    if driver is None:
        gdaltest.post_reason( 'GetDriver() returns None' )
        return 'fail'

    if driver.GetName() != 'ESRI Shapefile':
        gdaltest.post_reason( 'Got wrong driver name: ' + driver.GetName() )
        return 'fail'

    return 'success'

###############################################################################
# Test attribute query on special field fid - per bug 1468.

def ogr_basic_5():

    gdaltest.lyr.SetAttributeFilter( 'FID = 3' )
    gdaltest.lyr.ResetReading()
    
    feat1 = gdaltest.lyr.GetNextFeature()
    feat2 = gdaltest.lyr.GetNextFeature()

    gdaltest.lyr.SetAttributeFilter( None )
    
    if feat1 is None or feat2 is not None:
        gdaltest.post_reason( 'unexpected result count.' )
        return 'fail'

    if feat1.GetFID() != 3:
        gdaltest.post_reason( 'got wrong feature.' )
        return 'fail'

    feat1.Destroy()

    return 'success'


###############################################################################
# Test opening a dataset with an empty string and a non existing dataset
def ogr_basic_6():

    # Put inside try/except for OG python bindings
    try:
        if ogr.Open( '' ) is not None:
            return 'fail'
    except:
        pass

    try:
        if ogr.Open( 'non_existing' ) is not None:
            return 'fail'
    except:
        pass

    return 'success'

###############################################################################
# cleanup

def ogr_basic_cleanup():
    gdaltest.lyr = None
    gdaltest.ds.Destroy()
    gdaltest.ds = None

    return 'success'

gdaltest_list = [ 
    ogr_basic_1,
    ogr_basic_2,
    ogr_basic_3,
    ogr_basic_4,
    ogr_basic_5,
    ogr_basic_6,
    ogr_basic_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'ogr_basic_test' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

