#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read functionality for HDF5 driver.
# Author:   Even Rouault <even dot rouault at mines dash paris dot org>
# 
###############################################################################
# Copyright (c) 2008, Even Rouault <even dot rouault at mines dash paris dot org>
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
# Test if HDF5 driver is present

def hdf5_1():

    try:
        gdaltest.hdf5_drv = gdal.GetDriverByName( 'HDF5' )
    except:
        gdaltest.hdf5_drv = None
        return 'skip'

    return 'success'

###############################################################################
# Confirm expected subdataset information.

def hdf5_2():

    if gdaltest.hdf5_drv is None:
        return 'skip'

    ds = gdal.Open( 'data/u8be.h5' )

    sds_list = ds.GetMetadata('SUBDATASETS')

    if len(sds_list) != 2:
        print sds_list
        gdaltest.post_reason( 'Did not get expected subdataset count.' )
        return 'fail'

    if sds_list['SUBDATASET_1_NAME'] != 'HDF5:"data/u8be.h5"://TestArray':
        print sds_list
        gdaltest.post_reason( 'did not get expected subdatasets.' )
        return 'fail'
    
    return 'success'

###############################################################################
# Confirm subdataset access, and checksum.

def hdf5_3():

    if gdaltest.hdf5_drv is None:
        return 'skip'

    ds = gdal.Open( 'HDF5:"data/u8be.h5"://TestArray' )

    cs = ds.GetRasterBand(1).Checksum()
    if cs != 135:
        gdaltest.post_reason( 'did not get expected checksum' )
        return 'fail'
    
    return 'success'

###############################################################################
# Similar check on a 16bit dataset.

def hdf5_4():

    if gdaltest.hdf5_drv is None:
        return 'skip'

    ds = gdal.Open( 'HDF5:"data/groups.h5"://MyGroup/dset1' )

    cs = ds.GetRasterBand(1).Checksum()
    if cs != 9:
        gdaltest.post_reason( 'did not get expected checksum' )
        return 'fail'
    
    return 'success'

###############################################################################
# 
class TestHDF5:
    def __init__( self, downloadURL, fileName, subdatasetname, checksum, download_size ):
        self.downloadURL = downloadURL
        self.fileName = fileName
        self.subdatasetname = subdatasetname
        self.checksum = checksum
        self.download_size = download_size

    def test( self ):
        if gdaltest.hdf5_drv is None:
            return 'skip'

        if not gdaltest.download_file(self.downloadURL + '/' + self.fileName, self.fileName, self.download_size):
            return 'skip'

        ds = gdal.Open('HDF5:"tmp/cache/' + self.fileName + '"://' +  self.subdatasetname)

        if ds.GetRasterBand(1).Checksum() != self.checksum:
            gdaltest.post_reason('Bad checksum. Expected %d, got %d' % (self.checksum, ds.GetRasterBand(1).Checksum()))
            return 'failure'

        return 'success'



gdaltest_list = [ hdf5_1,
                  hdf5_2,
                  hdf5_3,
                  hdf5_4 ]

hdf5_list = [ ('ftp://ftp.hdfgroup.uiuc.edu/hdf_files/hdf5/samples/convert', 'C1979091.h5',
                                     'HDF4_PALGROUP/HDF4_PALETTE_2', 7488, -1),
              ('ftp://ftp.hdfgroup.uiuc.edu/hdf_files/hdf5/samples/convert', 'C1979091.h5',
                                     'Raster_Image_#0', 3661, -1),
              ('ftp://ftp.hdfgroup.uiuc.edu/hdf_files/hdf5/geospatial/DEM', 'half_moon_bay.grid',
                                     'HDFEOS/GRIDS/DEMGRID/Data_Fields/Elevation', 30863, -1),
            ]

for item in hdf5_list:
    ut = TestHDF5( item[0], item[1], item[2], item[3], item[4] )
    gdaltest_list.append( (ut.test, 'HDF5:"' + item[1]+ '"://' +   item[2]) )


if __name__ == '__main__':

    gdaltest.setup_run( 'hdf5' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

