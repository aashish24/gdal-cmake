#!/usr/bin/env python
###############################################################################
# $Id: lcp.py  rouault $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read functionality for LCP driver.
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

import os
import sys
import gdal
import array
import string

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
#  Test test_FARSITE_UTM12.LCP

def lcp_1():

    ds = gdal.Open('data/test_FARSITE_UTM12.LCP')
    if ds.RasterCount != 8:
        gdaltest.post_reason('wrong number of bands')
        return 'fail'

    dataPerBand = [ ( 18645, [  ('ELEVATION_UNIT', '0' ),
                                ('ELEVATION_UNIT_NAME', 'Meters' ),
                                ('ELEVATION_MIN', '67109928' ),
                                ('ELEVATION_MAX', '83887572' ),
                                ('ELEVATION_NUM_CLASSES', '-1' ),
                                ('ELEVATION_FILE', '' ) ] ),
                    ( 16431, [  ('SLOPE_UNIT', '0' ),
                                ('SLOPE_UNIT_NAME', 'Degrees' ),
                                ('SLOPE_MIN', '0' ),
                                ('SLOPE_MAX', '34' ),
                                ('SLOPE_NUM_CLASSES', '36' ),
                                ('SLOPE_FILE', 'slope.asc' ) ] ),
                    ( 18851, [  ('ASPECT_UNIT', '2' ),
                                ('ASPECT_UNIT_NAME', 'Azimuth degrees' ),
                                ('ASPECT_MIN', '0' ),
                                ('ASPECT_MAX', '16777573' ),
                                ('ASPECT_NUM_CLASSES', '-1' ),
                                ('ASPECT_FILE', 'aspect.asc' ) ] ),
                    ( 26182, [  ('FUEL_MODEL_OPTION', '0' ),
                                ('FUEL_MODEL_OPTION_DESC', 'no custom models AND no conversion file needed' ),
                                ('FUEL_MODEL_MIN', '1' ),
                                ('FUEL_MODEL_MAX', '99' ),
                                ('FUEL_MODEL_NUM_CLASSES', '6' ),
                                ('FUEL_MODEL_VALUES', '1,2,5,8,10,99' ),
                                ('FUEL_MODEL_FILE', 'fbfm13.asc' ) ] ),
                    ( 30038, [  ('CANOPY_COV_UNIT', '0' ),
                                ('CANOPY_COV_UNIT_NAME', 'Categories (0-4)' ),
                                ('CANOPY_COV_MIN', '0' ),
                                ('CANOPY_COV_MAX', '95' ),
                                ('CANOPY_COV_NUM_CLASSES', '10' ),
                                ('CANOPY_COV_FILE', 'cancov.asc' ) ] ),
                    ( 22077, [  ('CANOPY_HT_UNIT', '3' ),
                                ('CANOPY_HT_UNIT_NAME', 'Meters x 10' ),
                                ('CANOPY_HT_MIN', '0' ),
                                ('CANOPY_HT_MAX', '16777591' ),
                                ('CANOPY_HT_NUM_CLASSES', '5' ),
                                ('CANOPY_HT_FILE', 'canht.asc' ) ] ),
                    ( 30388, [  ('CBH_UNIT', '3' ),
                                ('CBH_UNIT_NAME', 'Meters x 10' ),
                                ('CBH_MIN', '0' ),
                                ('CBH_MAX', '100' ),
                                ('CBH_NUM_CLASSES', '33' ),
                                ('CBH_FILE', 'cbh.asc' ) ] ),
                    ( 23249, [  ('CBD_UNIT', '3' ),
                                ('CBD_UNIT_NAME', 'kg/m^3 x 100' ),
                                ('CBD_MIN', '0' ),
                                ('CBD_MAX', '21' ),
                                ('CBD_NUM_CLASSES', '20' ),
                                ('CBD_FILE', 'cbd.asc' ) ] )
                  ]

    for i in range(8):
        band = ds.GetRasterBand(i+1)
        if band.Checksum() != dataPerBand[i][0]:
            gdaltest.post_reason('wrong checksum for band %d. Got %d, expected %d' % (i+1, band.Checksum(), dataPerBand[i][0]))
            return 'fail'
        md = band.GetMetadata()
        for item in dataPerBand[i][1]:
            if md[item[0]] != item[1]:
                gdaltest.post_reason('wrong metadataitem for band %d. md[\'%s\']=\'%s\', expected \'%s\'' % (i+1, item[0], md[item[0]], item[1]))
                return 'fail'

    ds = None

    return 'success'

###############################################################################
# test test_USGS_LFNM_Alb83.lcp

def lcp_2():

    ds = gdal.Open('data/test_USGS_LFNM_Alb83.lcp')
    if ds.RasterCount != 8:
        gdal.post_reason('wrong number of bands')
        return 'fail'

    dataPerBand = [ ( 28381, [  ('ELEVATION_UNIT', '0' ),
                                ('ELEVATION_UNIT_NAME', 'Meters' ),
                                ('ELEVATION_MIN', '67109928' ),
                                ('ELEVATION_MAX', '83887572' ),
                                ('ELEVATION_NUM_CLASSES', '-1' ),
                                ('ELEVATION_FILE', 'd:\\scratch\\dist\\79990093\\Output\\rastert_elevation_1.txt' ) ] ),
                    ( 25824, [  ('SLOPE_UNIT', '0' ),
                                ('SLOPE_UNIT_NAME', 'Degrees' ),
                                ('SLOPE_MIN', '0' ),
                                ('SLOPE_MAX', '34' ),
                                ('SLOPE_NUM_CLASSES', '35' ),
                                ('SLOPE_FILE', 'd:\\scratch\\dist\\79990093\\Output\\rastert_slope_1.txt' ) ] ),
                    ( 28413, [  ('ASPECT_UNIT', '2' ),
                                ('ASPECT_UNIT_NAME', 'Azimuth degrees' ),
                                ('ASPECT_MIN', '0' ),
                                ('ASPECT_MAX', '16777573' ),
                                ('ASPECT_NUM_CLASSES', '-1' ),
                                ('ASPECT_FILE', 'd:\\scratch\\dist\\79990093\\Output\\rastert_aspect_1.txt' ) ] ),
                    ( 19052, [  ('FUEL_MODEL_OPTION', '0' ),
                                ('FUEL_MODEL_OPTION_DESC', 'no custom models AND no conversion file needed' ),
                                ('FUEL_MODEL_MIN', '1' ),
                                ('FUEL_MODEL_MAX', '10' ),
                                ('FUEL_MODEL_NUM_CLASSES', '5' ),
                                ('FUEL_MODEL_VALUES', '1,2,5,8,10' ),
                                ('FUEL_MODEL_FILE', 'd:\\scratch\\dist\\79990093\\Output\\rastert_fuel1.txt' ) ] ),
                    ( 30164, [  ('CANOPY_COV_UNIT', '1' ),
                                ('CANOPY_COV_UNIT_NAME', 'Percent' ),
                                ('CANOPY_COV_MIN', '0' ),
                                ('CANOPY_COV_MAX', '95' ),
                                ('CANOPY_COV_NUM_CLASSES', '10' ),
                                ('CANOPY_COV_FILE', 'd:\\scratch\\dist\\79990093\\Output\\rastert_canopy1.txt' ) ] ),
                    ( 22316, [  ('CANOPY_HT_UNIT', '3' ),
                                ('CANOPY_HT_UNIT_NAME', 'Meters x 10' ),
                                ('CANOPY_HT_MIN', '0' ),
                                ('CANOPY_HT_MAX', '16777591' ),
                                ('CANOPY_HT_NUM_CLASSES', '5' ),
                                ('CANOPY_HT_FILE', 'd:\\scratch\\dist\\79990093\\Output\\rastert_height_1.txt' ) ] ),
                    ( 30575, [  ('CBH_UNIT', '3' ),
                                ('CBH_UNIT_NAME', 'Meters x 10' ),
                                ('CBH_MIN', '0' ),
                                ('CBH_MAX', '100' ),
                                ('CBH_NUM_CLASSES', '33' ),
                                ('CBH_FILE', 'd:\\scratch\\dist\\79990093\\Output\\rastert_base_1.txt' ) ] ),
                    ( 23304, [  ('CBD_UNIT', '3' ),
                                ('CBD_UNIT_NAME', 'kg/m^3 x 100' ),
                                ('CBD_MIN', '0' ),
                                ('CBD_MAX', '21' ),
                                ('CBD_NUM_CLASSES', '20' ),
                                ('CBD_FILE', 'd:\\scratch\\dist\\79990093\\Output\\rastert_density_1.txt' ) ] )
                  ]

    for i in range(8):
        band = ds.GetRasterBand(i+1)
        if band.Checksum() != dataPerBand[i][0]:
            gdaltest.post_reason('wrong checksum for band %d. Got %d, expected %d' % (i+1, band.Checksum(), dataPerBand[i][0]))
            return 'fail'
        md = band.GetMetadata()
        for item in dataPerBand[i][1]:
            if md[item[0]] != item[1]:
                gdaltest.post_reason('wrong metadataitem for band %d. md[\'%s\']=\'%s\', expected \'%s\'' % (i+1, item[0], md[item[0]], item[1]))
                return 'fail'

    ds = None

    return 'success'

gdaltest_list = [
    lcp_1,
    lcp_2 ]

if __name__ == '__main__':

    gdaltest.setup_run( 'lcp' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

