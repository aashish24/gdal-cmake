#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test some functions of HFA driver.  Most testing in ../gcore/hfa_*
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Verify we can read the special histogram metadata from a provided image.

def hfa_histread():

    ds = gdal.Open('../gcore/data/utmsmall.img')
    md = ds.GetRasterBand(1).GetMetadata()
    ds = None

    if md['STATISTICS_MINIMUM'] != '8':
        gdaltest.post_reason( 'STATISTICS_MINIMUM is wrong.' )
        return 'fail'
    
    if md['STATISTICS_MEDIAN'] != '148':
        gdaltest.post_reason( 'STATISTICS_MEDIAN is wrong.' )
        return 'fail'

    if md['STATISTICS_HISTOMAX'] != '255':
        gdaltest.post_reason( 'STATISTICS_HISTOMAX is wrong.' )
        return 'fail'


    if md['STATISTICS_HISTOBINVALUES'] != '0|0|0|0|0|0|0|0|8|0|0|0|0|0|0|0|23|0|0|0|0|0|0|0|0|29|0|0|0|0|0|0|0|46|0|0|0|0|0|0|0|69|0|0|0|0|0|0|0|99|0|0|0|0|0|0|0|0|120|0|0|0|0|0|0|0|178|0|0|0|0|0|0|0|193|0|0|0|0|0|0|0|212|0|0|0|0|0|0|0|281|0|0|0|0|0|0|0|0|365|0|0|0|0|0|0|0|460|0|0|0|0|0|0|0|533|0|0|0|0|0|0|0|544|0|0|0|0|0|0|0|0|626|0|0|0|0|0|0|0|653|0|0|0|0|0|0|0|673|0|0|0|0|0|0|0|629|0|0|0|0|0|0|0|0|586|0|0|0|0|0|0|0|541|0|0|0|0|0|0|0|435|0|0|0|0|0|0|0|348|0|0|0|0|0|0|0|341|0|0|0|0|0|0|0|0|284|0|0|0|0|0|0|0|225|0|0|0|0|0|0|0|237|0|0|0|0|0|0|0|172|0|0|0|0|0|0|0|0|159|0|0|0|0|0|0|0|105|0|0|0|0|0|0|0|824|':
        gdaltest.post_reason( 'STATISTICS_HISTOBINVALUES is wrong.' )
        return 'fail'

    return 'success'
    
###############################################################################
# Verify that if we copy this test image to a new Imagine file the histofram
# info is preserved.

def hfa_histwrite():

    drv = gdal.GetDriverByName('HFA')
    ds_src = gdal.Open('../gcore/data/utmsmall.img')
    drv.CreateCopy( 'tmp/work.img', ds_src )
    ds_src = None

    ds = gdal.Open('tmp/work.img')
    md = ds.GetRasterBand(1).GetMetadata()
    ds = None
    
    drv.Delete( 'tmp/work.img' )

    if md['STATISTICS_MINIMUM'] != '8':
        gdaltest.post_reason( 'STATISTICS_MINIMUM is wrong.' )
        return 'fail'
    
    if md['STATISTICS_MEDIAN'] != '148':
        gdaltest.post_reason( 'STATISTICS_MEDIAN is wrong.' )
        return 'fail'

    if md['STATISTICS_HISTOMAX'] != '255':
        gdaltest.post_reason( 'STATISTICS_HISTOMAX is wrong.' )
        return 'fail'


    if md['STATISTICS_HISTOBINVALUES'] != '0|0|0|0|0|0|0|0|8|0|0|0|0|0|0|0|23|0|0|0|0|0|0|0|0|29|0|0|0|0|0|0|0|46|0|0|0|0|0|0|0|69|0|0|0|0|0|0|0|99|0|0|0|0|0|0|0|0|120|0|0|0|0|0|0|0|178|0|0|0|0|0|0|0|193|0|0|0|0|0|0|0|212|0|0|0|0|0|0|0|281|0|0|0|0|0|0|0|0|365|0|0|0|0|0|0|0|460|0|0|0|0|0|0|0|533|0|0|0|0|0|0|0|544|0|0|0|0|0|0|0|0|626|0|0|0|0|0|0|0|653|0|0|0|0|0|0|0|673|0|0|0|0|0|0|0|629|0|0|0|0|0|0|0|0|586|0|0|0|0|0|0|0|541|0|0|0|0|0|0|0|435|0|0|0|0|0|0|0|348|0|0|0|0|0|0|0|341|0|0|0|0|0|0|0|0|284|0|0|0|0|0|0|0|225|0|0|0|0|0|0|0|237|0|0|0|0|0|0|0|172|0|0|0|0|0|0|0|0|159|0|0|0|0|0|0|0|105|0|0|0|0|0|0|0|824|':
        gdaltest.post_reason( 'STATISTICS_HISTOBINVALUES is wrong.' )
        return 'fail'

    return 'success'
    
###############################################################################
# Verify we can read metadata of int.img.

def hfa_int_stats_1():

    ds = gdal.Open('data/int.img')
    md = ds.GetRasterBand(1).GetMetadata()
    ds = None

    if md['STATISTICS_MINIMUM'] != '40918':
        gdaltest.post_reason( 'STATISTICS_MINIMUM is wrong.' )
        return 'fail'

    if md['STATISTICS_MAXIMUM'] != '41134':
        gdaltest.post_reason( 'STATISTICS_MAXIMUM is wrong.' )
        return 'fail'

    if md['STATISTICS_MEDIAN'] != '41017':
        gdaltest.post_reason( 'STATISTICS_MEDIAN is wrong.' )
        return 'fail'

    if md['STATISTICS_MODE'] != '41013':
        gdaltest.post_reason( 'STATISTICS_MODE is wrong.' )
        return 'fail'

    if md['STATISTICS_HISTOMIN'] != '40918':
        gdaltest.post_reason( 'STATISTICS_HISTOMIN is wrong.' )
        return 'fail'

    if md['STATISTICS_HISTOMAX'] != '41134':
        gdaltest.post_reason( 'STATISTICS_HISTOMAX is wrong.' )
        return 'fail'

    if md['LAYER_TYPE'] != 'athematic':
        gdaltest.post_reason( 'LAYER_TYPE is wrong.' )
        return 'fail'

    return 'success'

###############################################################################
# Verify we can read band statistics of int.img.

def hfa_int_stats_2():

    ds = gdal.Open('data/int.img')
    stats = ds.GetRasterBand(1).GetStatistics(False, True)
    ds = None

    tolerance = 0.0001

    if abs(stats[0] - 40918.0) > tolerance:
        gdaltest.post_reason( 'Minimum value is wrong.' )
        return 'fail'

    if abs(stats[1] - 41134.0) > tolerance:
        gdaltest.post_reason( 'Maximum value is wrong.' )
        return 'fail'

    if abs(stats[2] - 41019.784218148) > tolerance:
        gdaltest.post_reason( 'Mean value is wrong.' )
        return 'fail'

    if abs(stats[3] - 44.637237445468) > tolerance:
        gdaltest.post_reason( 'StdDev value is wrong.' )
        return 'fail'

    return 'success'

###############################################################################
# Verify we can read metadata of float.img.

def hfa_float_stats_1():

    ds = gdal.Open('data/float.img')
    md = ds.GetRasterBand(1).GetMetadata()
    ds = None

    tolerance = 0.0001

    min = float(md['STATISTICS_MINIMUM'])
    if abs(min - 40.91858291626) > tolerance:
        gdaltest.post_reason( 'STATISTICS_MINIMUM is wrong.' )
        return 'fail'

    max = float(md['STATISTICS_MAXIMUM'])
    if abs(max - 41.134323120117) > tolerance:
        gdaltest.post_reason( 'STATISTICS_MAXIMUM is wrong.' )
        return 'fail'

    median = float(md['STATISTICS_MEDIAN'])
    if abs(median - 41.017182931304) > tolerance:
        gdaltest.post_reason( 'STATISTICS_MEDIAN is wrong.' )
        return 'fail'

    mod = float(md['STATISTICS_MODE'])
    if abs(mod - 41.0104410499) > tolerance:
        gdaltest.post_reason( 'STATISTICS_MODE is wrong.' )
        return 'fail'

    histMin = float(md['STATISTICS_HISTOMIN'])
    if abs(histMin - 40.91858291626) > tolerance:
        gdaltest.post_reason( 'STATISTICS_HISTOMIN is wrong.' )
        return 'fail'

    histMax = float(md['STATISTICS_HISTOMAX'])
    if abs(histMax - 41.134323120117) > tolerance:
        gdaltest.post_reason( 'STATISTICS_HISTOMAX is wrong.' )
        return 'fail'

    if md['LAYER_TYPE'] != 'athematic':
        gdaltest.post_reason( 'LAYER_TYPE is wrong.' )
        return 'fail'

    return 'success'

###############################################################################
# Verify we can read band statistics of float.img.

def hfa_float_stats_2():

    ds = gdal.Open('data/float.img')
    stats = ds.GetRasterBand(1).GetStatistics(False, True)
    ds = None
    
    tolerance = 0.0001

    if abs(stats[0] - 40.91858291626) > tolerance:
        gdaltest.post_reason( 'Minimum value is wrong.' )
        return 'fail'

    if abs(stats[1] - 41.134323120117) > tolerance:
        gdaltest.post_reason( 'Maximum value is wrong.' )
        return 'fail'

    if abs(stats[2] - 41.020284249223) > tolerance:
        gdaltest.post_reason( 'Mean value is wrong.' )
        return 'fail'

    if abs(stats[3] - 0.044636441749041) > tolerance:
        gdaltest.post_reason( 'StdDev value is wrong.' )
        return 'fail'

    return 'success'

###############################################################################
# Verify we can read image data.

def hfa_int_read():

    ds = gdal.Open('data/int.img')
    band = ds.GetRasterBand(1)
    cs = band.Checksum()
    data = band.ReadRaster(100, 100, 1, 1)
    ds = None

    if cs != 6691:
        gdaltest.post_reason( 'Checksum value is wrong.' )
        return 'fail'

    return 'success'

###############################################################################
# Verify we can read image data.

def hfa_float_read():

    ds = gdal.Open('data/float.img')
    band = ds.GetRasterBand(1)
    cs = band.Checksum()
    data = band.ReadRaster(100, 100, 1, 1)
    ds = None

    if cs != 23529:
        gdaltest.post_reason( 'Checksum value is wrong.' )
        return 'fail'

    # Read raw data into tuple of float numbers
    import struct
    value = struct.unpack('f' * 1, data)[0]

    if abs(value - 41.021659851074219) > 0.0001:
        gdaltest.post_reason( 'Pixel value is wrong.' )
        return 'fail'

    return 'success'
 
###############################################################################
# verify we can read PE_STRING coordinate system.

def hfa_pe_read():

    ds = gdal.Open('data/87test.img')
    wkt = ds.GetProjectionRef()
    expected = 'PROJCS["World_Cube",GEOGCS["GCS_WGS_1984",DATUM["WGS_1984",SPHEROID["WGS_1984",6378137.0,298.257223563]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]],PROJECTION["Cube"],PARAMETER["False_Easting",0.0],PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",0.0],PARAMETER["Option",1.0],UNIT["Meter",1.0]]'

    if wkt != expected:
        print wkt
        gdaltest.post_reason( 'failed to read pe string as expected.' )
        return 'fail'

    return 'success'
 
###############################################################################
# Verify we can write PE_STRING nodes.

def hfa_pe_write():

    drv = gdal.GetDriverByName('HFA')
    ds_src = gdal.Open('data/87test.img')
    drv.CreateCopy( 'tmp/87test.img', ds_src )
    ds_src = None

    expected = 'PROJCS["World_Cube",GEOGCS["GCS_WGS_1984",DATUM["WGS_1984",SPHEROID["WGS_1984",6378137.0,298.257223563]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.017453292519943295]],PROJECTION["Cube"],PARAMETER["False_Easting",0.0],PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",0.0],PARAMETER["Option",1.0],UNIT["Meter",1.0]]'
    
    ds = gdal.Open('tmp/87test.img')
    wkt = ds.GetProjectionRef()

    if wkt != expected:
        print
        print expected
        print wkt
        gdaltest.post_reason( 'failed to write pe string as expected.' )
        return 'fail'

    ds = None
    drv.Delete( 'tmp/87test.img' )
    return 'success'
 
###############################################################################
# Verify we can write and read large metadata items.

def hfa_metadata_1():

    drv = gdal.GetDriverByName('HFA')
    ds = drv.Create( 'tmp/md_1.img', 100, 150, 1, gdal.GDT_Byte )
    
    md_val = '0123456789' * 60
    md = { 'test' : md_val }
    ds.GetRasterBand(1).SetMetadata( md )
    ds = None

    ds = gdal.Open( 'tmp/md_1.img' )
    md = ds.GetRasterBand(1).GetMetadata()
    if md['test'] != md_val:
        print md['test']
        print md_val
        gdaltest.post_reason( 'got wrong metadata back' )
        return 'fail'
    ds = None
    
    return 'success'
 
###############################################################################
# Verify that writing metadata multiple times does not result in duplicate
# nodes.

def hfa_metadata_2():

    ds = gdal.Open( 'tmp/md_1.img', gdal.GA_Update )
    md = ds.GetRasterBand(1).GetMetadata()
    md['test'] = '0123456789'
    md['xxx'] = '123'
    ds.GetRasterBand(1).SetMetadata( md )
    ds = None

    ds = gdal.Open( 'tmp/md_1.img' )
    md = ds.GetRasterBand(1).GetMetadata()
    if not md.has_key('xxx'):
        gdaltest.post_reason('metadata rewrite seems not to have worked')
        return 'fail'

    if md['xxx'] != '123' or md['test'] != '0123456789':
        print md
        gdaltest.post_reason('got wrong metadata back')
        return 'fail'

    ds = None
    gdal.GetDriverByName('HFA').Delete( 'tmp/md_1.img' )
    
    return 'success'
 
###############################################################################
# Verify we can grow the RRD list in cases where this requires
# moving the HFAEntry to the end of the file.  (bug #1109)

def hfa_grow_rrdlist():

    import shutil

    shutil.copyfile('data/bug_1109.img' , 'tmp/bug_1109.img')
    #os.system("copy data\\bug_1109.img tmp")

    # Add two overview levels.
    ds = gdal.Open('tmp/bug_1109.img',gdal.GA_Update)
    result = ds.BuildOverviews( overviewlist = [4,8] )
    ds = None

    if result != 0:
        gdaltest.post_reason( 'BuildOverviews failed.' )
        return 'fail'

    # Verify overviews are now findable.
    ds = gdal.Open( 'tmp/bug_1109.img' )
    if ds.GetRasterBand(1).GetOverviewCount() != 3:
        gdaltest.post_reason( 'Overview count wrong.' )
        print ds.GetRasterBand(1).GetOverviewCount()
        return 'fail'

    ds = None
    gdal.GetDriverByName('HFA').Delete( 'tmp/bug_1109.img' )
    
    return 'success'
 
###############################################################################
# Make sure an old .ige file is deleted when creating a new dataset. (#1784)

def hfa_clean_ige():

    # Create an imagine file, forcing creation of an .ige file.

    drv = gdal.GetDriverByName('HFA')
    src_ds = gdal.Open('data/byte.tif')

    drv.CreateCopy( 'tmp/igetest.img', src_ds,
                    options = [ 'USE_SPILL=YES' ] )

    try:
        open( 'tmp/igetest.ige' )
    except:
        gdaltest.post_reason( 'ige file not created with USE_SPILL=YES' )
        return 'fail'

    # Create a file without a spill file, and verify old ige cleaned up.
    
    drv.CreateCopy( 'tmp/igetest.img', src_ds )

    try:
        open( 'tmp/igetest.ige' )
        gdaltest.post_reason( 'ige file not cleaned up properly.' )
        return 'fail'
    except:
        pass

    drv.Delete( 'tmp/igetest.img' )

    return 'success'
 
###############################################################################
#

gdaltest_list = [
    hfa_histread,
    hfa_histwrite,
    hfa_int_stats_1,
    hfa_int_stats_2,
    hfa_float_stats_1,
    hfa_float_stats_2,
    hfa_int_read,
    hfa_float_read,
    hfa_pe_read,
    hfa_pe_write,
    hfa_metadata_1,
    hfa_metadata_2,
    hfa_grow_rrdlist,
    hfa_clean_ige ]

if __name__ == '__main__':

    gdaltest.setup_run( 'hfa' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

