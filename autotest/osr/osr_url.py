#!/usr/bin/env python
###############################################################################
# $Id: osr_proj4.py 11065 2007-03-24 09:35:32Z mloskot $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test some URL specific translation issues.
# Author:   Howard Butler <hobu.inc@gmail.com>
# 
###############################################################################
# Copyright (c) 2007, Howard Butler <hobu.inc@gmail.com>
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
import osr

expected_wkt = 'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]'


def osr_url_1():
    """Depend on the Accepts headers that ImportFromUrl sets to request SRS from sr.org"""
    srs = osr.SpatialReference()
    import gdal
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' ) 
    try:
        srs.ImportFromUrl( 'http://spatialreference.org/ref/epsg/4326/' )
    except AttributeError: # old-gen bindings don't have this method yet
        return 'skip'
    except Exception, msg:
        gdal.PopErrorHandler()
        if gdal.GetLastErrorMsg() == "GDAL/OGR not compiled with libcurl support, remote requests not supported.":
            return 'skip'
        else:
            gdaltest.post_reason( 'exception: ' + str(msg) )
            return 'fail'


    if not gdaltest.equal_srs_from_wkt( expected_wkt,
                                        srs.ExportToWkt() ):
        return 'fail'

    return 'success'

def osr_url_2():
    """Blissfully set request a URL that has OGC WKT"""
    srs = osr.SpatialReference()
    import gdal
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' ) 
    try:
        srs.ImportFromUrl( 'http://spatialreference.org/ref/epsg/4326/ogcwkt/' )
    except AttributeError: # old-gen bindings don't have this method yet
        return 'skip'
    except Exception, msg:
        gdal.PopErrorHandler()
        if gdal.GetLastErrorMsg() == "GDAL/OGR not compiled with libcurl support, remote requests not supported.":
            return 'skip'
        else:
            gdaltest.post_reason( 'exception: ' + str(msg) )
            return 'fail'


    if not gdaltest.equal_srs_from_wkt( expected_wkt,
                                        srs.ExportToWkt() ):
        return 'fail'

    return 'success'
    
gdaltest_list = [ 
    osr_url_1,
    osr_url_2,
    None ]

if __name__ == '__main__':

    gdaltest.setup_run( 'osr_url' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

