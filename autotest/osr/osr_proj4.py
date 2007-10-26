#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test some PROJ.4 specific translation issues.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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
import gdal

sys.path.append( '../pymod' )

import gdaltest
import osr
import ogr

###############################################################################
# Test the the +k_0 flag works as well as +k when consuming PROJ.4 format.
# This is from Bugzilla bug 355.
#

def osr_proj4_1():
    
    srs = osr.SpatialReference()
    srs.ImportFromProj4( '+proj=tmerc +lat_0=53.5000000000 +lon_0=-8.0000000000 +k_0=1.0000350000 +x_0=200000.0000000000 +y_0=250000.0000000000 +a=6377340.189000 +rf=299.324965 +towgs84=482.530,-130.596,564.557,-1.042,-0.214,-0.631,8.15' )

    if abs(srs.GetProjParm( osr.SRS_PP_SCALE_FACTOR )-1.000035) > 0.0000005:
        gdaltest.post_reason( '+k_0 not supported on import from PROJ.4?' )
        return 'fail'

    return 'success'

###############################################################################
# Verify that we can import strings with parameter values that are exponents
# and contain a plus sign.  As per bug 355 in GDAL/OGR's bugzilla. 
#

def osr_proj4_2():
    
    srs = osr.SpatialReference()
    srs.ImportFromProj4( "+proj=lcc +x_0=0.6096012192024384e+06 +y_0=0 +lon_0=90dw +lat_0=42dn +lat_1=44d4'n +lat_2=42d44'n +a=6378206.400000 +rf=294.978698 +nadgrids=conus,ntv1_can.dat" )

    if abs(srs.GetProjParm( osr.SRS_PP_FALSE_EASTING )-609601.219) > 0.0005:
        gdaltest.post_reason( 'Parsing exponents not supported?' )
        return 'fail'

    return 'success'

###############################################################################
# Verify that empty srs'es don't cause a crash (#1718).
#

def osr_proj4_3():
    
    srs = osr.SpatialReference()

    try:
        proj4 = srs.ExportToProj4()
        
    except RuntimeError:
        pass

    if string.find(gdal.GetLastErrorMsg(),'No translation') != -1:
        return 'success'

    gdaltest.post_reason( 'empty srs not handled properly' )
    return 'fail'

###############################################################################
# Verify that unrecognised projections return an error, not those
# annoying ellipsoid-only results.
#

def osr_proj4_4():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( '+proj=utm +zone=11 +datum=WGS84' )
    srs.SetAttrValue( 'PROJCS|PROJECTION', 'FakeTransverseMercator' )
    
    try:
        proj4 = srs.ExportToProj4()
        
    except RuntimeError:
        pass

    if string.find(gdal.GetLastErrorMsg(),'No translation') != -1:
        return 'success'

    gdaltest.post_reason( 'unknown srs not handled properly' )
    return 'fail'

###############################################################################
# Verify that prime meridians are preserved when round tripping. (#1940)
#

def osr_proj4_5():
    
    srs = osr.SpatialReference()

    srs.ImportFromProj4( '+proj=lcc +lat_1=46.8 +lat_0=46.8 +lon_0=0 +k_0=0.99987742 +x_0=600000 +y_0=2200000 +a=6378249.2 +b=6356515 +towgs84=-168,-60,320,0,0,0,0 +pm=paris +units=m +no_defs' )

    if abs(float(srs.GetAttrValue('PRIMEM',1)) - 2.3372291667) > 0.00000001:
        gdaltest.post_reason('prime meridian lost?')
        return 'fail'

    if abs(srs.GetProjParm('central_meridian')) != 0.0:
        gdaltest.post_reason( 'central meridian altered?' )
        return 'fail'

    p4 = srs.ExportToProj4()
    srs2 = osr.SpatialReference()
    srs2.ImportFromProj4( p4 )

    if not srs.IsSame(srs2):
        gdaltest.post_reason( 'round trip via PROJ.4 damaged srs?' )
        print srs.ExportToPrettyWkt()
        print srs2.ExportToPrettyWkt()
    
    return 'success'

gdaltest_list = [ 
    osr_proj4_1,
    osr_proj4_2,
    osr_proj4_3,
    osr_proj4_4,
    osr_proj4_5,
    None ]

if __name__ == '__main__':

    gdaltest.setup_run( 'osr_proj4' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

