#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test PostGIS driver functionality.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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
import string

sys.path.append( '../pymod' )

import gdaltest
import ogrtest
import ogr
import osr
import gdal

###############################################################################
# Return true if 'layer_name' is one of the reported layers of pg_ds

def ogr_pg_check_layer_in_list(ds, layer_name):

    for i in range(0, ds.GetLayerCount()):
        name = ds.GetLayer(i).GetName()
        if name == layer_name:
            return True
    return False

#
# To create the required PostGIS instance do something like:
#
#  $ createdb autotest
#  $ createlang plpgsql autotest
#  $ psql autotest < ~/postgis.sql
#

###############################################################################
# Open Database.

def ogr_pg_1():

    gdaltest.pg_ds = None
    gdaltest.pg_connection_string='dbname=autotest'

    try:
        gdaltest.pg_dr = ogr.GetDriverByName( 'PostgreSQL' )
    except:
        return 'skip'

    if gdaltest.pg_dr is None:
        return 'skip'

    try:
        gdaltest.pg_ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )
    except:
        gdaltest.pg_ds = None

    if gdaltest.pg_ds is None:
        return 'skip'

    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    sql_lyr = gdaltest.pg_ds.ExecuteSQL('SELECT postgis_version()')
    gdaltest.pg_has_postgis = sql_lyr is not None
    gdaltest.pg_ds.ReleaseResultSet(sql_lyr)
    gdal.PopErrorHandler()

    if gdaltest.pg_has_postgis:
        if gdal.GetConfigOption('PG_USE_POSTGIS', 'YES') == 'YES':
            print 'PostGIS available !'
        else:
            gdaltest.pg_has_postgis = False
            print 'PostGIS available but will NOT be used because of PG_USE_POSTGIS=NO !'
    else:
        gdaltest.pg_has_postgis = False
        print 'PostGIS NOT available !'

    return 'success'

###############################################################################
# Create table from data/poly.shp

def ogr_pg_2():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:tpoly' )
    gdal.PopErrorHandler()

    ######################################################
    # Create Layer
    gdaltest.pg_lyr = gdaltest.pg_ds.CreateLayer( 'tpoly',
                                                  options = [ 'DIM=3' ] )

    ######################################################
    # Setup Schema
    ogrtest.quick_create_layer_def( gdaltest.pg_lyr,
                                    [ ('AREA', ogr.OFTReal),
                                      ('EAS_ID', ogr.OFTInteger),
                                      ('PRFEDEA', ogr.OFTString),
                                      ('SHORTNAME', ogr.OFTString, 8) ] )

    ######################################################
    # Copy in poly.shp

    dst_feat = ogr.Feature( feature_def = gdaltest.pg_lyr.GetLayerDefn() )

    shp_ds = ogr.Open( 'data/poly.shp' )
    gdaltest.shp_ds = shp_ds
    shp_lyr = shp_ds.GetLayer(0)
    
    feat = shp_lyr.GetNextFeature()
    gdaltest.poly_feat = []
    
    while feat is not None:

        gdaltest.poly_feat.append( feat )

        dst_feat.SetFrom( feat )
        gdaltest.pg_lyr.CreateFeature( dst_feat )

        feat = shp_lyr.GetNextFeature()

    dst_feat.Destroy()
        
    return 'success'

###############################################################################
# Verify that stuff we just wrote is still OK.

def ogr_pg_3():
    if gdaltest.pg_ds is None:
        return 'skip'

    if gdaltest.pg_lyr.GetFeatureCount() != 10:
        gdaltest.post_reason( 'GetFeatureCount() returned %d instead of 10' % gdaltest.pg_lyr.GetFeatureCount() )
        return 'fail'

    expect = [168, 169, 166, 158, 165]

    gdaltest.pg_lyr.SetAttributeFilter( 'eas_id < 170' )
    tr = ogrtest.check_features_against_list( gdaltest.pg_lyr,
                                              'eas_id', expect )

    if gdaltest.pg_lyr.GetFeatureCount() != 5:
        gdaltest.post_reason( 'GetFeatureCount() returned %d instead of 5' % gdaltest.pg_lyr.GetFeatureCount() )
        return 'fail'

    gdaltest.pg_lyr.SetAttributeFilter( None )

    for i in range(len(gdaltest.poly_feat)):
        orig_feat = gdaltest.poly_feat[i]
        read_feat = gdaltest.pg_lyr.GetNextFeature()
        
        if ogrtest.check_feature_geometry(read_feat,orig_feat.GetGeometryRef(),
                                          max_error = 0.001 ) != 0:
            return 'fail'

        for fld in range(3):
            if orig_feat.GetField(fld) != read_feat.GetField(fld):
                gdaltest.post_reason( 'Attribute %d does not match' % fld )
                return 'fail'

        read_feat.Destroy()
        orig_feat.Destroy()

    gdaltest.poly_feat = None
    gdaltest.shp_ds.Destroy()

    if tr:
        return 'success'
    else:
        return 'fail'

###############################################################################
# Write more features with a bunch of different geometries, and verify the
# geometries are still OK.

def ogr_pg_4():

    if gdaltest.pg_ds is None:
        return 'skip'

    dst_feat = ogr.Feature( feature_def = gdaltest.pg_lyr.GetLayerDefn() )
    wkt_list = [ '10', '2', '1', '3d_1', '4', '5', '6' ]

    for item in wkt_list:

        wkt = open( 'data/wkb_wkt/'+item+'.wkt' ).read()
        geom = ogr.CreateGeometryFromWkt( wkt )
        
        ######################################################################
        # Write geometry as a new Oracle feature.
    
        dst_feat.SetGeometryDirectly( geom )
        dst_feat.SetField( 'PRFEDEA', item )
        gdaltest.pg_lyr.CreateFeature( dst_feat )
        
        ######################################################################
        # Read back the feature and get the geometry.

        gdaltest.pg_lyr.SetAttributeFilter( "PRFEDEA = '%s'" % item )
        feat_read = gdaltest.pg_lyr.GetNextFeature()
        geom_read = feat_read.GetGeometryRef()

        if ogrtest.check_feature_geometry( feat_read, geom ) != 0:
            return 'fail'

        feat_read.Destroy()

    dst_feat.Destroy()
    
    return 'success'
    
###############################################################################
# Test ExecuteSQL() results layers without geometry.

def ogr_pg_5():

    if gdaltest.pg_ds is None:
        return 'skip'

    expect = [ None, 179, 173, 172, 171, 170, 169, 168, 166, 165, 158 ]

    sql_lyr = gdaltest.pg_ds.ExecuteSQL( 'select distinct eas_id from tpoly order by eas_id desc' )

    if sql_lyr.GetFeatureCount() != 11:
        gdaltest.post_reason( 'GetFeatureCount() returned %d instead of 11' % sql_lyr.GetFeatureCount() )
        return 'fail'

    tr = ogrtest.check_features_against_list( sql_lyr, 'eas_id', expect )

    gdaltest.pg_ds.ReleaseResultSet( sql_lyr )

    if tr:
        return 'success'
    else:
        return 'fail'
    
###############################################################################
# Test ExecuteSQL() results layers with geometry.

def ogr_pg_6():

    if gdaltest.pg_ds is None:
        return 'skip'

    sql_lyr = gdaltest.pg_ds.ExecuteSQL( "select * from tpoly where prfedea = '2'" )

    tr = ogrtest.check_features_against_list( sql_lyr, 'prfedea', [ '2' ] )
    if tr:
        sql_lyr.ResetReading()
        feat_read = sql_lyr.GetNextFeature()
        if ogrtest.check_feature_geometry( feat_read, 'MULTILINESTRING ((5.00121349 2.99853132,5.00121349 1.99853133),(5.00121349 1.99853133,5.00121349 0.99853133),(3.00121351 1.99853127,5.00121349 1.99853133),(5.00121349 1.99853133,6.00121348 1.99853135))' ) != 0:
            tr = 0
        feat_read.Destroy()


    sql_lyr.ResetReading()

    geom = ogr.CreateGeometryFromWkt( \
        'LINESTRING(-10 -10,0 0)' )
    sql_lyr.SetSpatialFilter( geom )
    geom.Destroy()

    if sql_lyr.GetFeatureCount() != 0:
        gdaltest.post_reason( 'GetFeatureCount() returned %d instead of 0' % sql_lyr.GetFeatureCount() )
        return 'fail'

    if sql_lyr.GetNextFeature() != None:
        gdaltest.post_reason( 'GetNextFeature() didn not return None' )
        return 'fail'

    gdaltest.pg_ds.ReleaseResultSet( sql_lyr )

    if tr:
        return 'success'
    else:
        return 'fail'
    
###############################################################################
# Test spatial filtering. 

def ogr_pg_7():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdaltest.pg_lyr.SetAttributeFilter( None )
    
    geom = ogr.CreateGeometryFromWkt( \
        'LINESTRING(479505 4763195,480526 4762819)' )
    gdaltest.pg_lyr.SetSpatialFilter( geom )
    geom.Destroy()

    if gdaltest.pg_lyr.GetFeatureCount() != 1:
        gdaltest.post_reason( 'GetFeatureCount() returned %d instead of 1' % gdaltest.pg_lyr.GetFeatureCount() )
        return 'fail'

    tr = ogrtest.check_features_against_list( gdaltest.pg_lyr, 'eas_id',
                                              [ 158 ] )

    gdaltest.pg_lyr.SetAttributeFilter( 'eas_id = 158' )

    if gdaltest.pg_lyr.GetFeatureCount() != 1:
        gdaltest.post_reason( 'GetFeatureCount() returned %d instead of 1' % gdaltest.pg_lyr.GetFeatureCount() )
        return 'fail'

    gdaltest.pg_lyr.SetAttributeFilter( None )

    gdaltest.pg_lyr.SetSpatialFilter( None )
    
    if tr:
        return 'success'
    else:
        return 'fail'
    
###############################################################################
# Write a feature with too long a text value for a fixed length text field.
# The driver should now truncate this (but with a debug message).  Also,
# put some crazy stuff in the value to verify that quoting and escaping
# is working smoothly.
#
# No geometry in this test.

def ogr_pg_8():

    if gdaltest.pg_ds is None:
        return 'skip'

    dst_feat = ogr.Feature( feature_def = gdaltest.pg_lyr.GetLayerDefn() )

    dst_feat.SetField( 'PRFEDEA', 'CrazyKey' )
    dst_feat.SetField( 'SHORTNAME', 'Crazy"\'Long' )
    gdaltest.pg_lyr.CreateFeature( dst_feat )
    dst_feat.Destroy()
    
    gdaltest.pg_lyr.SetAttributeFilter( "PRFEDEA = 'CrazyKey'" )
    feat_read = gdaltest.pg_lyr.GetNextFeature()

    if feat_read is None:
        gdaltest.post_reason( 'creating crazy feature failed!' )
        return 'fail'

    if feat_read.GetField( 'shortname' ) != 'Crazy"\'L':
        gdaltest.post_reason( 'Vvalue not properly escaped or truncated:' \
                              + feat_read.GetField( 'shortname' ) )
        return 'fail'
                              
    feat_read.Destroy()
    
    return 'success'
    
###############################################################################
# Verify inplace update of a feature with SetFeature().

def ogr_pg_9():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdaltest.pg_lyr.SetAttributeFilter( "PRFEDEA = 'CrazyKey'" )
    feat = gdaltest.pg_lyr.GetNextFeature()
    gdaltest.pg_lyr.SetAttributeFilter( None )

    feat.SetField( 'SHORTNAME', 'Reset' )

    point = ogr.Geometry( ogr.wkbPoint25D )
    point.SetPoint( 0, 5, 6, 7 )
    feat.SetGeometryDirectly( point )

    if gdaltest.pg_lyr.SetFeature( feat ) != 0:
        feat.Destroy()
        gdaltest.post_reason( 'SetFeature() method failed.' )
        return 'fail'

    fid = feat.GetFID()
    feat.Destroy()

    feat = gdaltest.pg_lyr.GetFeature( fid )
    if feat is None:
        gdaltest.post_reason( 'GetFeature(%d) failed.' % fid )
        return 'fail'
        
    shortname = feat.GetField( 'SHORTNAME' )
    if shortname[:5] != 'Reset':
        gdaltest.post_reason( 'SetFeature() did not update SHORTNAME, got %s.'\
                              % shortname )
        return 'fail'

    if ogrtest.check_feature_geometry( feat, 'POINT(5 6 7)' ) != 0:
        print feat.GetGeometryRef()
        gdaltest.post_reason( 'Geometry update failed' )
        return 'fail'

    feat.Destroy()

    return 'success'

###############################################################################
# Verify that DeleteFeature() works properly.

def ogr_pg_10():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdaltest.pg_lyr.SetAttributeFilter( "PRFEDEA = 'CrazyKey'" )
    feat = gdaltest.pg_lyr.GetNextFeature()
    gdaltest.pg_lyr.SetAttributeFilter( None )

    fid = feat.GetFID()
    feat.Destroy()

    if gdaltest.pg_lyr.DeleteFeature( fid ) != 0:
        gdaltest.post_reason( 'DeleteFeature() method failed.' )
        return 'fail'

    gdaltest.pg_lyr.SetAttributeFilter( "PRFEDEA = 'CrazyKey'" )
    feat = gdaltest.pg_lyr.GetNextFeature()
    gdaltest.pg_lyr.SetAttributeFilter( None )

    if feat is None:
        return 'success'

    feat.Destroy()
    gdaltest.post_reason( 'DeleteFeature() seems to have had no effect.' )
    return 'fail'

###############################################################################
# Create table from data/poly.shp in COPY mode.

def ogr_pg_11():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:tpolycopy' )
    gdal.PopErrorHandler()

    gdal.SetConfigOption( 'PG_USE_COPY', 'YES' )

    ######################################################
    # Create Layer
    gdaltest.pgc_lyr = gdaltest.pg_ds.CreateLayer( 'tpolycopy',
                                                  options = [ 'DIM=3' ] )

    ######################################################
    # Setup Schema
    ogrtest.quick_create_layer_def( gdaltest.pgc_lyr,
                                    [ ('AREA', ogr.OFTReal),
                                      ('EAS_ID', ogr.OFTInteger),
                                      ('PRFEDEA', ogr.OFTString),
                                      ('SHORTNAME', ogr.OFTString, 8) ] )

    ######################################################
    # Copy in poly.shp

    dst_feat = ogr.Feature( feature_def = gdaltest.pgc_lyr.GetLayerDefn() )

    shp_ds = ogr.Open( 'data/poly.shp' )
    gdaltest.shp_ds = shp_ds
    shp_lyr = shp_ds.GetLayer(0)
    
    feat = shp_lyr.GetNextFeature()
    gdaltest.poly_feat = []
    
    while feat is not None:

        gdaltest.poly_feat.append( feat )

        dst_feat.SetFrom( feat )
        gdaltest.pgc_lyr.CreateFeature( dst_feat )

        feat = shp_lyr.GetNextFeature()

    dst_feat.Destroy()
        
    gdal.SetConfigOption( 'PG_USE_COPY', 'NO' )
    
    return 'success'

###############################################################################
# Verify that stuff we just wrote is still OK.

def ogr_pg_12():
    if gdaltest.pg_ds is None:
        return 'skip'

    gdaltest.pgc_lyr.ResetReading()
    gdaltest.pgc_lyr.SetAttributeFilter( None )

    for i in range(len(gdaltest.poly_feat)):
        orig_feat = gdaltest.poly_feat[i]
        read_feat = gdaltest.pgc_lyr.GetNextFeature()
        
        if ogrtest.check_feature_geometry(read_feat,orig_feat.GetGeometryRef(),
                                          max_error = 0.001 ) != 0:
            return 'fail'

        for fld in range(3):
            if orig_feat.GetField(fld) != read_feat.GetField(fld):
                gdaltest.post_reason( 'Attribute %d does not match' % fld )
                return 'fail'

        read_feat.Destroy()
        orig_feat.Destroy()

    gdaltest.poly_feat = None
    gdaltest.shp_ds.Destroy()
    gdaltest.shp_ds = None

    return 'success'

###############################################################################
# Create a table with some date fields.

def ogr_pg_13():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:datetest' )
    gdal.PopErrorHandler()

    ######################################################
    # Create Table
    lyr = gdaltest.pg_ds.CreateLayer( 'datetest' )

    ######################################################
    # Setup Schema
    ogrtest.quick_create_layer_def( lyr, [ ('ogrdate', ogr.OFTDate),
                                           ('ogrtime', ogr.OFTTime),
                                           ('ogrdatetime', ogr.OFTDateTime)] )

    ######################################################
    # add some custom date fields.
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datetest ADD COLUMN tsz timestamp with time zone' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datetest ADD COLUMN ts timestamp without time zone' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datetest ADD COLUMN dt date' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datetest ADD COLUMN tm time' )

    ######################################################
    # Create a populated records.
    gdaltest.pg_ds.ExecuteSQL( "INSERT INTO datetest ( ogrdate, ogrtime, ogrdatetime, tsz, ts, dt, tm) VALUES ( '2005-10-12 10:41:33-05', '2005-10-12 10:41:33-05', '2005-10-12 10:41:33-05', '2005-10-12 10:41:33-05','2005-10-12 10:41:33-05','2005-10-12 10:41:33-05','2005-10-12 10:41:33-05' )" )

    return 'success'

###############################################################################
# Verify that stuff we just wrote is still OK.
# Fetch in several timezones to test our timezone processing.

def ogr_pg_14():
    if gdaltest.pg_ds is None:
        return 'skip'

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )

    ds.ExecuteSQL( 'set timezone to "UTC"' )

    lyr = ds.GetLayerByName( 'datetest' )

    feat = lyr.GetNextFeature()
    if feat.GetFieldAsString('ogrdatetime') != '2005/10/12 15:41:33+00' \
       or feat.GetFieldAsString('ogrdate') != '2005/10/12' \
       or feat.GetFieldAsString('ogrtime') != '10:41:33' \
       or feat.GetFieldAsString('tsz') != '2005/10/12 15:41:33+00' \
       or feat.GetFieldAsString('ts') != '2005/10/12 10:41:33' \
       or feat.GetFieldAsString('dt') != '2005/10/12' \
       or feat.GetFieldAsString('tm') != '10:41:33':
        gdaltest.post_reason( 'UTC value wrong' )
        feat.DumpReadable()
        return 'fail'
    
    ds.ExecuteSQL( 'set timezone to "Canada/Newfoundland"' )

    lyr.ResetReading()
    
    feat = lyr.GetNextFeature()

    if feat.GetFieldAsString('ogrdatetime') != '2005/10/12 13:11:33-0230' \
       or feat.GetFieldAsString('tsz') != '2005/10/12 13:11:33-0230' \
       or feat.GetFieldAsString('ts') != '2005/10/12 10:41:33' \
       or feat.GetFieldAsString('dt') != '2005/10/12' \
       or feat.GetFieldAsString('tm') != '10:41:33':
        gdaltest.post_reason( 'Newfoundland value wrong' )
        feat.DumpReadable()
        return 'fail'
    
    ds.ExecuteSQL( 'set timezone to "+5"' )

    lyr.ResetReading()
    
    feat = lyr.GetNextFeature()
    if feat.GetFieldAsString('ogrdatetime') != '2005/10/12 20:41:33+05' \
       or feat.GetFieldAsString('tsz') != '2005/10/12 20:41:33+05':
        gdaltest.post_reason( '+5 value wrong' )
        feat.DumpReadable()
        return 'fail'

    feat = None
    ds.Destroy()

    return 'success'

###############################################################################
# Test very large query.

def ogr_pg_15():

    if gdaltest.pg_ds is None:
        return 'skip'

    expect = [169]

    query = 'eas_id = 169'
    
    for id in range(1000):
        query = query + (' or eas_id = %d' % (id+1000)) 

    gdaltest.pg_lyr.SetAttributeFilter( query )
    tr = ogrtest.check_features_against_list( gdaltest.pg_lyr,
                                              'eas_id', expect )
    gdaltest.pg_lyr.SetAttributeFilter( None )

    if tr:
        return 'success'
    else:
        return 'fail'


###############################################################################
# Test very large statement.

def ogr_pg_16():

    if gdaltest.pg_ds is None:
        return 'skip'

    expect = [169]

    query = 'eas_id = 169'
    
    for id in range(1000):
        query = query + (' or eas_id = %d' % (id+1000))

    statement = 'select eas_id from tpoly where ' + query

    lyr = gdaltest.pg_ds.ExecuteSQL( statement )

    tr = ogrtest.check_features_against_list( lyr, 'eas_id', expect )

    gdaltest.pg_ds.ReleaseResultSet( lyr )

    if tr:
        return 'success'
    else:
        return 'fail'

###############################################################################
# Test requesting a non-existent table by name (bug 1480).

def ogr_pg_17():

    if gdaltest.pg_ds is None:
        return 'skip'

    count = gdaltest.pg_ds.GetLayerCount()
    try:
        layer = gdaltest.pg_ds.GetLayerByName( 'JunkTableName' )
    except:
        layer = None
        
    if layer is not None:
        gdaltest.post_reason( 'got layer for non-existant table!' )
        return 'fail'

    if count != gdaltest.pg_ds.GetLayerCount():
        gdaltest.post_reason( 'layer count changed unexpectedly.' )
        return 'fail'

    return 'success'

###############################################################################
# Test getting a layer by name that wasn't previously a layer.

def ogr_pg_18():

    if gdaltest.pg_ds is None or not gdaltest.pg_has_postgis:
        return 'skip'

    count = gdaltest.pg_ds.GetLayerCount()
    layer = gdaltest.pg_ds.GetLayerByName( 'geometry_columns' )
    if layer is None:
        gdaltest.post_reason( 'did not get geometry_columns layer' )
        return 'fail'

    if count+1 != gdaltest.pg_ds.GetLayerCount():
        gdaltest.post_reason( 'layer count unexpectedly unchanged.' )
        return 'fail'

    return 'success'

###############################################################################
# Test reading a layer extent

def ogr_pg_19():

    if gdaltest.pg_ds is None:
        return 'skip'

    layer = gdaltest.pg_ds.GetLayerByName( 'tpoly' )
    if layer is None:
        gdaltest.post_reason( 'did not get tpoly layer' )
        return 'fail'
    
    extent = layer.GetExtent()
    expect = ( 478315.53125, 481645.3125, 4762880.5, 4765610.5 )

    minx = abs(extent[0] - expect[0])
    maxx = abs(extent[1] - expect[1])
    miny = abs(extent[2] - expect[2])
    maxy = abs(extent[3] - expect[3])

    if max(minx, maxx, miny, maxy) > 0.0001:
        gdaltest.post_reason( 'Extents do not match' )
        return 'fail'

    return 'success'

###############################################################################
# Test reading a SQL result layer extent

def ogr_pg_19_2():

    if gdaltest.pg_ds is None:
        return 'skip'

    sql_lyr = gdaltest.pg_ds.ExecuteSQL( 'select * from tpoly' )

    extent = sql_lyr.GetExtent()
    expect = ( 478315.53125, 481645.3125, 4762880.5, 4765610.5 )

    minx = abs(extent[0] - expect[0])
    maxx = abs(extent[1] - expect[1])
    miny = abs(extent[2] - expect[2])
    maxy = abs(extent[3] - expect[3])

    if max(minx, maxx, miny, maxy) > 0.0001:
        gdaltest.post_reason( 'Extents do not match' )
        return 'fail'

    gdaltest.pg_ds.ReleaseResultSet(sql_lyr)

    return 'success'

###############################################################################
# Test reading 4-dim geometry in EWKT format

def ogr_pg_20():

    if gdaltest.pg_ds is None or not gdaltest.pg_has_postgis:
        return 'skip'

    #
    # Prepare test layer with 4-dim geometries.
    #


    # Collection of test geometry pairs:
    # ( <EWKT>, <WKT> ) <=> ( <tested>, <expected> )
    geometries = (
        ( 'POINT (10 20 5 5)',
          'POINT (10 20 5)' ),
        ( 'LINESTRING (10 10 1 2,20 20 3 4,30 30 5 6,40 40 7 8)',
          'LINESTRING (10 10 1,20 20 3,30 30 5,40 40 7)' ),
        ( 'POLYGON ((0 0 1 2,4 0 3 4,4 4 5 6,0 4 7 8,0 0 1 2))',
          'POLYGON ((0 0 1,4 0 3,4 4 5,0 4 7,0 0 1))' ),
        ( 'MULTIPOINT (10 20 5 5,30 30 7 7)',
          'MULTIPOINT (10 20 5,30 30 7)' ),
        ( 'MULTILINESTRING ((10 10 1 2,20 20 3 4),(30 30 5 6,40 40 7 8))',
          'MULTILINESTRING ((10 10 1,20 20 3),(30 30 5,40 40 7))' ),
        ( 'MULTIPOLYGON(((0 0 0 1,4 0 0 1,4 4 0 1,0 4 0 1,0 0 0 1),(1 1 0 5,2 1 0 5,2 2 0 5,1 2 0 5,1 1 0 5)),((-1 -1 0 10,-1 -2 0 10,-2 -2 0 10,-2 -1 0 10,-1 -1 0 10)))',
          'MULTIPOLYGON (((0 0 0,4 0 0,4 4 0,0 4 0,0 0 0),(1 1 0,2 1 0,2 2 0,1 2 0,1 1 0)),((-1 -1 0,-1 -2 0,-2 -2 0,-2 -1 0,-1 -1 0)))' ),
        ( 'GEOMETRYCOLLECTION(POINT(2 3 11 101),LINESTRING(2 3 12 102,3 4 13 103))',
          'GEOMETRYCOLLECTION (POINT (2 3 11),LINESTRING (2 3 12,3 4 13))' )
    )

    # This layer is also used in ogr_pg_21() test.
    gdaltest.pg_ds.ExecuteSQL( "CREATE TABLE testgeom (ogc_fid integer)" )

    # XXX - mloskot - if 'public' is omitted, then OGRPGDataSource::DeleteLayer fails, line 438
    sql_lyr = gdaltest.pg_ds.ExecuteSQL( "SELECT AddGeometryColumn('public','testgeom','wkb_geometry',-1,'GEOMETRY',4)" )
    gdaltest.pg_ds.ReleaseResultSet(sql_lyr)
    for i in range(len(geometries)):
        gdaltest.pg_ds.ExecuteSQL( "INSERT INTO testgeom (ogc_fid,wkb_geometry) \
                                    VALUES (%d,GeomFromEWKT('%s'))" % ( i, geometries[i][0])  )

    # We need to re-read layers
    gdaltest.pg_ds.Destroy()
    gdaltest.pg_ds = None
    try:
        gdaltest.pg_ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )
    except:
        gdaltest.pg_ds = None
        gdaltest.post_reason( 'can not re-open datasource' )
        return 'fail'

    #
    # Test reading 4-dim geometries normalized to OGC WKT form.
    #

    layer = gdaltest.pg_ds.GetLayerByName( 'testgeom' )
    if layer is None:
        gdaltest.post_reason( 'did not get testgeom layer' )
        return 'fail'
 
    for i in range(len(geometries)):
        feat = layer.GetFeature(i)
        geom = feat.GetGeometryRef()
        wkt = geom.ExportToWkt()
        feat.Destroy()
        feat = None

        if wkt != geometries[i][1]:
            gdaltest.post_reason( 'WKT do not match' )
            return 'fail'

    layer = None

    return 'success'

###############################################################################
# Test reading 4-dimension geometries in EWKB format

def ogr_pg_21():

    if gdaltest.pg_ds is None or not gdaltest.pg_has_postgis:
        return 'skip'

    layer = gdaltest.pg_ds.ExecuteSQL( "SELECT wkb_geometry FROM testgeom" )
    if layer is None:
        gdaltest.post_reason( 'did not get testgeom layer' )
        return 'fail'

    fail = False

    feat = layer.GetNextFeature()
    while feat is not None:
        geom = feat.GetGeometryRef()
        if geom is not None:
            gdaltest.post_reason( 'expected feature with None geometry' )
            feat.Destroy()
            feat = None
            return 'fail'

        feat.Destroy()
        feat = layer.GetNextFeature()

    feat = None
    gdaltest.pg_ds.ReleaseResultSet(layer)
    layer = None

    return 'success'

###############################################################################
# Create table from data/poly.shp under specified SCHEMA
# This test checks if schema support and schema name quoting works well.

def ogr_pg_22():

    if gdaltest.pg_ds is None:
        return 'skip'

    ######################################################
    # Create Schema 

    schema_name = 'AutoTest-schema'
    layer_name = schema_name + '.tpoly2'

    gdaltest.pg_ds.ExecuteSQL( 'CREATE SCHEMA \"' + schema_name + '\"')

    ######################################################
    # Create Layer
    gdaltest.pg_lyr = gdaltest.pg_ds.CreateLayer( layer_name,
                                                  options = [
                                                      'DIM=3',
                                                      'SCHEMA=' + schema_name ]
                                                )

    ######################################################
    # Setup Schema
    ogrtest.quick_create_layer_def( gdaltest.pg_lyr,
                                    [ ('AREA', ogr.OFTReal),
                                      ('EAS_ID', ogr.OFTInteger),
                                      ('PRFEDEA', ogr.OFTString),
                                      ('SHORTNAME', ogr.OFTString, 8) ] )

    ######################################################
    # Copy 3 features from the poly.shp

    dst_feat = ogr.Feature( feature_def = gdaltest.pg_lyr.GetLayerDefn() )

    shp_ds = ogr.Open( 'data/poly.shp' )
    gdaltest.shp_ds = shp_ds
    shp_lyr = shp_ds.GetLayer(0)
    
    # Insert 3 features only
    for id in range(0, 3):
        feat = shp_lyr.GetFeature(id)
        dst_feat.SetFrom( feat )
        gdaltest.pg_lyr.CreateFeature( dst_feat )

    dst_feat.Destroy()

    # Test if test layer under custom schema is listed

    found = ogr_pg_check_layer_in_list(gdaltest.pg_ds, layer_name)

    if found is False:
        gdaltest.post_reason( 'layer from schema \''+schema_name+'\' not listed' )
        return 'fail'

    return 'success'

###############################################################################
# Create table with all data types

def ogr_pg_23():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:datatypetest' )
    gdal.PopErrorHandler()

    ######################################################
    # Create Table
    lyr = gdaltest.pg_ds.CreateLayer( 'datatypetest' )

    ######################################################
    # Setup Schema
    # ogrtest.quick_create_layer_def( lyr, None )

    ######################################################
    # add some custom date fields.
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_numeric5 numeric(5)' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_numeric5_3 numeric(5,3)' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_bool bool' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_int2 int2' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_int4 int4' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_int8 int8' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_float4 float4' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_float8 float8' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_char char' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_varchar character varying' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_text text' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_bytea bytea' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_time time' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_date date' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_timestamp timestamp without time zone' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_timestamptz timestamp with time zone' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_chararray char(1)[]' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_textarray text[]' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_varchararray character varying[]' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_int4array int4[]' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_float4array float4[]' )
    gdaltest.pg_ds.ExecuteSQL( 'ALTER TABLE datatypetest ADD COLUMN my_float8array float8[]' )

    ######################################################
    # Create a populated records.

    if gdaltest.pg_has_postgis:
        geom_str = "GeomFromEWKT('POINT(10 20)')"
    else:
        geom_str = "'\\\\001\\\\001\\\\000\\\\000\\\\000\\\\000\\\\000\\\\000\\\\000\\\\000\\\\000$@\\\\000\\\\000\\\\000\\\\000\\\\000\\\\0004@'"
    gdaltest.pg_ds.ExecuteSQL( "INSERT INTO datatypetest ( my_numeric5, my_numeric5_3, my_bool, my_int2, my_int4, my_int8, my_float4, my_float8, my_char, my_varchar, my_text, my_bytea, my_time, my_date, my_timestamp, my_timestamptz, my_chararray, my_textarray, my_varchararray, my_int4array, my_float4array, my_float8array, wkb_geometry) VALUES ( 12345, 0.123, 'T', 12345, 12345678, 1234567901234, 0.123, 0.12345678, 'a', 'ab', 'abc', 'xyz', '12:34:56', '2000-01-01', '2000-01-01 00:00:00', '2000-01-01 00:00:00+00', '{a,b}', '{aa,bb}', '{cc,dd}', '{100,200}', '{100.1,200.1}', '{100.12,200.12}', " + geom_str + " )" )


    return 'success'

###############################################################################

def test_val_test_23(feat):
    if feat.my_numeric5 != 12345 or \
    feat.my_numeric5_3 != 0.123 or \
    feat.my_bool != 1 or \
    feat.my_int2 != 12345 or \
    feat.my_int4 != 12345678 or \
    abs(feat.my_float4 - 0.123) > 1e-8 or \
    feat.my_float8 != 0.12345678 or \
    feat.my_char != 'a' or \
    feat.my_varchar != 'ab' or \
    feat.my_text != 'abc' or \
    feat.GetFieldAsString('my_bytea') != '78797A' or \
    feat.GetFieldAsString('my_time') != '12:34:56' or \
    feat.GetFieldAsString('my_date') != '2000/01/01' or \
    (feat.GetFieldAsString('my_timestamp') != '2000/01/01  0:00:00' and feat.GetFieldAsString('my_timestamp') != '2000/01/01  0:00:00+00') or \
    feat.GetFieldAsString('my_timestamptz') != '2000/01/01  0:00:00+00' or \
    feat.GetFieldAsString('my_chararray') != '(2:a,b)' or \
    feat.GetFieldAsString('my_textarray') != '(2:aa,bb)' or \
    feat.GetFieldAsString('my_varchararray') != '(2:cc,dd)' or \
    feat.GetFieldAsString('my_int4array') != '(2:100,200)' :
#    feat.my_float4array != '(2:100.1,200.1)'
#    feat.my_float4array != '(2:100.12,200.12)'
#    feat.my_int8 != 1234567901234
        gdaltest.post_reason( 'Wrong values' )
        feat.DumpReadable()
        return 'fail'

    geom = feat.GetGeometryRef()
    if geom is None:
        gdaltest.post_reason( 'geom is none' )
        return 'fail'

    wkt = geom.ExportToWkt()
    if wkt != 'POINT (10 20)':
        gdaltest.post_reason( 'Wrong WKT :' + wkt )
        return 'fail'

    return 'success'

###############################################################################
# Test with PG: connection

def ogr_pg_24():

    if gdaltest.pg_ds is None:
        return 'skip'

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )

    ds.ExecuteSQL( 'set timezone to "UTC"' )

    lyr = ds.GetLayerByName( 'datatypetest' )

    feat = lyr.GetNextFeature()
    if test_val_test_23(feat) != 'success':
        return 'fail'

    feat = None

    ds.Destroy()

    return 'success'

###############################################################################
# Test with PG: connection and SELECT query

def ogr_pg_25():

    if gdaltest.pg_ds is None:
        return 'skip'

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )

    ds.ExecuteSQL( 'set timezone to "UTC"' )

    sql_lyr = ds.ExecuteSQL( 'select * from datatypetest' )

    feat = sql_lyr.GetNextFeature()
    if test_val_test_23(feat) != 'success':
        return 'fail'

    ds.ReleaseResultSet( sql_lyr )

    feat = None

    ds.Destroy()

    return 'success'

###############################################################################
# Test with PGB: connection

def ogr_pg_26():

    if gdaltest.pg_ds is None:
        return 'skip'

    ds = ogr.Open( 'PGB:' + gdaltest.pg_connection_string, update = 1 )

    ds.ExecuteSQL( 'set timezone to "UTC"' )

    lyr = ds.GetLayerByName( 'datatypetest' )

    feat = lyr.GetNextFeature()
    if test_val_test_23(feat) != 'success':
        return 'fail'

    feat = None

    ds.Destroy()

    return 'success'

###############################################################################
# Test with PGB: connection and SELECT query

def ogr_pg_27():

    if gdaltest.pg_ds is None:
        return 'skip'

    ds = ogr.Open( 'PGB:' + gdaltest.pg_connection_string, update = 1 )

    ds.ExecuteSQL( 'set timezone to "UTC"')

    sql_lyr = ds.ExecuteSQL( 'select * from datatypetest' )

    feat = sql_lyr.GetNextFeature()
    if test_val_test_23(feat) != 'success':
        return 'fail'

    ds.ReleaseResultSet( sql_lyr )

    feat = None

    ds.Destroy()

    return 'success'

###############################################################################
# Duplicate all data types

def ogr_pg_28():

    if gdaltest.pg_ds is None:
        return 'skip'

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )

    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds.ExecuteSQL( 'DELLAYER:datatypetest2' )
    gdal.PopErrorHandler()

    ds.ExecuteSQL( 'set timezone to "UTC"' )

    src_lyr = ds.GetLayerByName( 'datatypetest' )

    dst_lyr = ds.CreateLayer( 'datatypetest2' )

    src_lyr.ResetReading()

    for i in range(src_lyr.GetLayerDefn().GetFieldCount()):
        field_defn = src_lyr.GetLayerDefn().GetFieldDefn(i)
        dst_lyr.CreateField( field_defn )

    dst_feat = ogr.Feature( feature_def = dst_lyr.GetLayerDefn() )

    feat = src_lyr.GetNextFeature()
    if feat is None:
        return 'fail'

    dst_feat.SetFrom( feat )
    if dst_lyr.CreateFeature( dst_feat ) != 0:
        gdaltest.post_reason('CreateFeature failed.')
        return 'fail'

    dst_feat.Destroy()

    src_lyr = None
    dst_lyr = None

    ds.Destroy()

    return 'success'

###############################################################################
# Test with PG: connection

def ogr_pg_29():

    if gdaltest.pg_ds is None:
        return 'skip'

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )

    ds.ExecuteSQL( 'set timezone to "UTC"' )

    lyr = ds.GetLayerByName( 'datatypetest2' )

    # my_timestamp has now a time zone...
    feat = lyr.GetNextFeature()
    if test_val_test_23(feat) != 'success':
        return 'fail'

    geom = feat.GetGeometryRef()
    wkt = geom.ExportToWkt()
    if wkt != 'POINT (10 20)':
        gdaltest.post_reason( 'Wrong WKT :' + wkt )
        return 'fail'

    feat = None

    ds.Destroy()

    return 'success'

###############################################################################
# Duplicate all data types in PG_USE_COPY mode

def ogr_pg_30():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdal.SetConfigOption( 'PG_USE_COPY', 'YES' )

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )

    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds.ExecuteSQL( 'DELLAYER:datatypetest2' )
    gdal.PopErrorHandler()

    ds.ExecuteSQL( 'set timezone to "UTC"' )

    src_lyr = ds.GetLayerByName( 'datatypetest' )

    dst_lyr = ds.CreateLayer( 'datatypetest2' )

    src_lyr.ResetReading()

    for i in range(src_lyr.GetLayerDefn().GetFieldCount()):
        field_defn = src_lyr.GetLayerDefn().GetFieldDefn(i)
        dst_lyr.CreateField( field_defn )

    dst_feat = ogr.Feature( feature_def = dst_lyr.GetLayerDefn() )

    feat = src_lyr.GetNextFeature()
    if feat is None:
        return 'fail'

    dst_feat.SetFrom( feat )
    if dst_lyr.CreateFeature( dst_feat ) != 0:
        gdaltest.post_reason('CreateFeature failed.')
        return 'fail'

    dst_feat.Destroy()

    ds.Destroy()

    gdal.SetConfigOption( 'PG_USE_COPY', 'NO' )

    return 'success'


###############################################################################
# Test the tables= connection string option

def ogr_pg_31():

    if gdaltest.pg_ds is None:
        return 'skip'

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string + ' tables=tpoly,tpolycopy', update = 1 )

    if ds is None or ds.GetLayerCount() != 2:
        return 'fail'

    ds.Destroy()

    return 'success'

###############################################################################
# Test approximate srtext (#2123)

def ogr_pg_32():

    if gdaltest.pg_ds is None or not gdaltest.pg_has_postgis:
        return 'skip'

    gdaltest.pg_ds.ExecuteSQL("DELETE FROM spatial_ref_sys");

    ######################################################
    # Create Layer with EPSG:4326
    srs = osr.SpatialReference()
    srs.ImportFromEPSG( 4326 )

    gdaltest.pg_lyr = gdaltest.pg_ds.CreateLayer( 'testsrtext', srs = srs);

    sql_lyr = gdaltest.pg_ds.ExecuteSQL("SELECT COUNT(*) FROM spatial_ref_sys");
    feat = sql_lyr.GetNextFeature()
    if  feat.count != 1:
        return 'fail'
    gdaltest.pg_ds.ReleaseResultSet(sql_lyr)

    ######################################################
    # Create second layer with very approximative EPSG:4326

    srs = osr.SpatialReference()
    srs.SetFromUserInput('GEOGCS["WGS 84",AUTHORITY["EPSG","4326"]]')
    gdaltest.pg_lyr = gdaltest.pg_ds.CreateLayer( 'testsrtext2', srs = srs);

    # Must still be 1
    sql_lyr = gdaltest.pg_ds.ExecuteSQL("SELECT COUNT(*) FROM spatial_ref_sys");
    feat = sql_lyr.GetNextFeature()
    if  feat.count != 1:
        return 'fail'
    gdaltest.pg_ds.ReleaseResultSet(sql_lyr)

    return 'success'

###############################################################################
# Test encoding as UTF8

def ogr_pg_33():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdaltest.pg_lyr = gdaltest.pg_ds.GetLayerByName( 'tpoly' )
    if gdaltest.pg_lyr is None:
        gdaltest.post_reason( 'did not get tpoly layer' )
        return 'fail'

    dst_feat = ogr.Feature( feature_def = gdaltest.pg_lyr.GetLayerDefn() )
    # eacute in UTF8 : 0xc3 0xa9
    dst_feat.SetField( 'SHORTNAME', '\xc3\xa9' )
    gdaltest.pg_lyr.CreateFeature( dst_feat )
    dst_feat.Destroy()

    return 'success'

###############################################################################
# Test encoding as Latin1

def ogr_pg_34():

    if gdaltest.pg_ds is None:
        return 'skip'

    # We only test that on Linux since setting os.environ['XXX']
    # is not guaranteed to have effects on system not supporting putenv
    if sys.platform != 'linux2':
        return 'skip'

    os.environ['PGCLIENTENCODING'] = 'LATIN1' 
    ogr_pg_1()
    del os.environ['PGCLIENTENCODING']

    gdaltest.pg_lyr = gdaltest.pg_ds.GetLayerByName( 'tpoly' )
    if gdaltest.pg_lyr is None:
        gdaltest.post_reason( 'did not get tpoly layer' )
        return 'fail'

    dst_feat = ogr.Feature( feature_def = gdaltest.pg_lyr.GetLayerDefn() )
    # eacute in Latin1 : 0xe9
    dst_feat.SetField( 'SHORTNAME', '\xe9' )
    gdaltest.pg_lyr.CreateFeature( dst_feat )
    dst_feat.Destroy()

    return 'success'


###############################################################################
# Test for buffer overflows

def ogr_pg_35():

    if gdaltest.pg_ds is None:
        return 'skip'

    try:
        gdaltest.pg_lyr = gdaltest.pg_ds.CreateLayer( 'testoverflows' )
        ogrtest.quick_create_layer_def( gdaltest.pg_lyr, [ ('0123456789' * 1000, ogr.OFTReal)] )
    except:
        pass

    try:
        gdaltest.pg_lyr = gdaltest.pg_ds.CreateLayer( 'testoverflows', options = [ 'OVERWRITE=YES', 'GEOMETRY_NAME=' + ('0123456789' * 1000) ] )
    except:
        pass

    return 'success'

###############################################################################
# Test support for inherited tables : tables inherited from a Postgis Table

def ogr_pg_36():

    if gdaltest.pg_ds is None:
        return 'skip'

    if gdaltest.pg_has_postgis:
        gdaltest.pg_lyr = gdaltest.pg_ds.CreateLayer( 'table36_base', geom_type = ogr.wkbPoint )
    else:
        gdaltest.pg_lyr = gdaltest.pg_ds.CreateLayer( 'table36_base' )

    gdaltest.pg_ds.ExecuteSQL('CREATE TABLE table36_inherited ( col1 CHAR(1) ) INHERITS ( table36_base )')
    gdaltest.pg_ds.ExecuteSQL('CREATE TABLE table36_inherited2 ( col2 CHAR(1) ) INHERITS ( table36_inherited )')

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )

    found = ogr_pg_check_layer_in_list(ds, 'table36_inherited')
    if found is False:
        gdaltest.post_reason( 'layer table36_inherited not listed' )
        return 'fail'

    found = ogr_pg_check_layer_in_list(ds, 'table36_inherited2')
    if found is False:
        gdaltest.post_reason( 'layer table36_inherited2 not listed' )
        return 'fail'

    lyr = ds.GetLayerByName( 'table36_inherited2' )
    if lyr is None:
        return 'fail'
    if gdaltest.pg_has_postgis and lyr.GetLayerDefn().GetGeomType() != ogr.wkbPoint:
        return 'fail'
    ds.Destroy()

    return 'success'

def ogr_pg_36_bis():

    if gdaltest.pg_ds is None:
        return 'skip'

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string + ' TABLES=table36_base', update = 1 )

    found = ogr_pg_check_layer_in_list(ds, 'table36_inherited')
    if found is True:
        gdaltest.post_reason( 'layer table36_inherited is listed but it should not' )
        return 'fail'

    lyr = ds.GetLayerByName( 'table36_inherited' )
    if lyr is None:
        return 'fail'
    if gdaltest.pg_has_postgis and lyr.GetLayerDefn().GetGeomType() != ogr.wkbPoint:
        return 'fail'

    ds.Destroy()

    return 'success'

###############################################################################
# Test support for inherited tables : Postgis table inherited from a non-Postgis table

def ogr_pg_37():

    if gdaltest.pg_ds is None or not gdaltest.pg_has_postgis:
        return 'skip'

    gdaltest.pg_ds.ExecuteSQL('CREATE TABLE table37_base ( col1 CHAR(1) )')
    gdaltest.pg_ds.ExecuteSQL('CREATE TABLE table37_inherited ( col2 CHAR(1) ) INHERITS ( table37_base )')
    sql_lyr = gdaltest.pg_ds.ExecuteSQL( "SELECT AddGeometryColumn('public','table37_inherited','wkb_geometry',-1,'POINT',2)" )
    gdaltest.pg_ds.ReleaseResultSet(sql_lyr)

    ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )

    found = ogr_pg_check_layer_in_list(ds, 'table37_inherited')
    if found is False:
        gdaltest.post_reason( 'layer table37_inherited not listed' )
        return 'fail'

    lyr = ds.GetLayerByName( 'table37_inherited' )
    if lyr is None:
        return 'fail'
    if gdaltest.pg_has_postgis and lyr.GetLayerDefn().GetGeomType() != ogr.wkbPoint:
        return 'fail'

    ds.Destroy()

    return 'success'

###############################################################################
# 

def ogr_pg_table_cleanup():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:tpoly' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:tpolycopy' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:datetest' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:testgeom' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:datatypetest' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:datatypetest2' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:testsrtext' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:testsrtext2' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:testoverflows' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:table36_inherited2' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:table36_inherited' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:table36_base' )
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:table37_inherited' )
    gdaltest.pg_ds.ExecuteSQL( 'DROP TABLE table37_base')
    
    # Drop second 'tpoly' from schema 'AutoTest-schema' (do NOT quote names here)
    gdaltest.pg_ds.ExecuteSQL( 'DELLAYER:AutoTest-schema.tpoly2' )
    # Drop 'AutoTest-schema' (here, double qoutes are required)
    gdaltest.pg_ds.ExecuteSQL( 'DROP SCHEMA \"AutoTest-schema\" CASCADE')
    gdal.PopErrorHandler()

    return 'success'

def ogr_pg_cleanup():

    if gdaltest.pg_ds is None:
        return 'skip'

    gdaltest.pg_ds = ogr.Open( 'PG:' + gdaltest.pg_connection_string, update = 1 )
    ogr_pg_table_cleanup();

    gdaltest.pg_ds.Destroy()
    gdaltest.pg_ds = None

    return 'success'

# NOTE: The ogr_pg_19 intentionally executed after ogr_pg_2

gdaltest_list_internal = [ 
    ogr_pg_table_cleanup,
    ogr_pg_2,
    ogr_pg_19,
    ogr_pg_19_2,
    ogr_pg_3,
    ogr_pg_4,
    ogr_pg_5,
    ogr_pg_6,
    ogr_pg_7,
    ogr_pg_8,
    ogr_pg_9,
    ogr_pg_10,
    ogr_pg_11,
    ogr_pg_12,
    ogr_pg_13,
    ogr_pg_14,
    ogr_pg_15,
    ogr_pg_16,
    ogr_pg_17,
    ogr_pg_18,
    ogr_pg_20,
    ogr_pg_21,
    ogr_pg_22,
    ogr_pg_23,
    ogr_pg_24,
    ogr_pg_25,
    ogr_pg_26,
    ogr_pg_27,
    ogr_pg_28,
    ogr_pg_29,
    ogr_pg_30,
    ogr_pg_29,
    ogr_pg_31,
    ogr_pg_32,
    ogr_pg_33,
    ogr_pg_34,
    ogr_pg_35,
    ogr_pg_36,
    ogr_pg_36_bis,
    ogr_pg_37,
    ogr_pg_cleanup ]


###############################################################################
# Run gdaltest_list_internal with PostGIS enabled and then with PostGIS disabled

def ogr_pg_with_and_without_postgis():

    gdaltest.run_tests( [ ogr_pg_1 ] )
    if gdaltest.pg_ds is None:
        return 'skip'

    gdaltest.run_tests( gdaltest_list_internal )

    if gdaltest.pg_has_postgis:
        gdal.SetConfigOption("PG_USE_POSTGIS", "NO")
        gdaltest.run_tests( [ ogr_pg_1 ] )
        gdaltest.run_tests( gdaltest_list_internal )
        gdal.SetConfigOption("PG_USE_POSTGIS", "YES")

    return 'success'

gdaltest_list = [
    ogr_pg_with_and_without_postgis
]

if __name__ == '__main__':

    gdaltest.setup_run( 'ogr_pg' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

