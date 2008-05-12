#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read/write functionality for NITF driver.
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
import gdal
import array
import string

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Write/Read test of simple byte reference data.

def nitf_1():

    tst = gdaltest.GDALTest( 'NITF', 'byte.tif', 1, 4672 )
    return tst.testCreateCopy()

###############################################################################
# Write/Read test of simple 16bit reference data. 

def nitf_2():

    tst = gdaltest.GDALTest( 'NITF', 'int16.tif', 1, 4672 )
    return tst.testCreateCopy()

###############################################################################
# Write/Read RGB image with lat/long georeferencing, and verify.

def nitf_3():

    tst = gdaltest.GDALTest( 'NITF', 'rgbsmall.tif', 3, 21349 )
    return tst.testCreateCopy()


###############################################################################
# Test direction creation of an NITF file.

def nitf_create(creation_options):

    drv = gdal.GetDriverByName( 'NITF' )

    try:
        os.remove( 'tmp/test_create.ntf' )
    except:
        pass

    ds = drv.Create( 'tmp/test_create.ntf', 200, 100, 3, gdal.GDT_Byte,
                     creation_options )
    ds.SetGeoTransform( (100, 0.1, 0.0, 30.0, 0.0, -0.1 ) )

    list = range(200) + range(20,220) + range(30,230)
    raw_data = array.array('h',list).tostring()

    for line in range(100):
        ds.WriteRaster( 0, line, 200, 1, raw_data,
                        buf_type = gdal.GDT_Int16,
                        band_list = [1,2,3] )

    ds.GetRasterBand( 1 ).SetRasterColorInterpretation( gdal.GCI_BlueBand )
    ds.GetRasterBand( 2 ).SetRasterColorInterpretation( gdal.GCI_GreenBand )
    ds.GetRasterBand( 3 ).SetRasterColorInterpretation( gdal.GCI_RedBand )

    ds = None

    return 'success'

###############################################################################
# Test direction creation of an non-compressed NITF file.

def nitf_4():

    return nitf_create([ 'ICORDS=G' ])


###############################################################################
# Verify created file

def nitf_check_created_file(checksum1, checksum2, checksum3):
    ds = gdal.Open( 'tmp/test_create.ntf' )
    
    chksum = ds.GetRasterBand(1).Checksum()
    chksum_expect = checksum1
    if chksum != chksum_expect:
	gdaltest.post_reason( 'Did not get expected chksum for band 1' )
	print chksum, chksum_expect
	return 'fail'

    chksum = ds.GetRasterBand(2).Checksum()
    chksum_expect = checksum2
    if chksum != chksum_expect:
	gdaltest.post_reason( 'Did not get expected chksum for band 2' )
	print chksum, chksum_expect
	return 'fail'

    chksum = ds.GetRasterBand(3).Checksum()
    chksum_expect = checksum3
    if chksum != chksum_expect:
	gdaltest.post_reason( 'Did not get expected chksum for band 3' )
	print chksum, chksum_expect
	return 'fail'

    geotransform = ds.GetGeoTransform()
    if abs(geotransform[0]-100) > 0.1 \
	or abs(geotransform[1]-0.1) > 0.001 \
	or abs(geotransform[2]-0) > 0.001 \
	or abs(geotransform[3]-30.0) > 0.1 \
	or abs(geotransform[4]-0) > 0.001 \
	or abs(geotransform[5]- -0.1) > 0.001:
	print geotransform
	gdaltest.post_reason( 'geotransform differs from expected' )
	return 'fail'

    if ds.GetRasterBand(1).GetRasterColorInterpretation() != gdal.GCI_BlueBand:
        gdaltest.post_reason( 'Got wrong color interpretation.' )
        return 'fail'

    if ds.GetRasterBand(2).GetRasterColorInterpretation() !=gdal.GCI_GreenBand:
        gdaltest.post_reason( 'Got wrong color interpretation.' )
        return 'fail'

    if ds.GetRasterBand(3).GetRasterColorInterpretation() != gdal.GCI_RedBand:
        gdaltest.post_reason( 'Got wrong color interpretation.' )
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Verify file created by nitf_4()

def nitf_5():

    return nitf_check_created_file(32498, 42602, 38982)
	
###############################################################################
# Read existing NITF file.  Verifies the new adjusted IGEOLO interp.

def nitf_6():

    tst = gdaltest.GDALTest( 'NITF', 'rgb.ntf', 3, 21349 )
    return tst.testOpen( check_prj = 'WGS84',
                         check_gt = (-44.842029478458, 0.003503401360, 0,
                                     -22.930748299319, 0, -0.003503401360) )

###############################################################################
# NITF in-memory.

def nitf_7():

    tst = gdaltest.GDALTest( 'NITF', 'rgbsmall.tif', 3, 21349 )
    return tst.testCreateCopy( vsimem = 1 )

###############################################################################
# Verify we can open an NSIF file, and get metadata including BLOCKA.

def nitf_8():

    ds = gdal.Open( 'data/fake_nsif.ntf' )
    
    chksum = ds.GetRasterBand(1).Checksum()
    chksum_expect = 12033
    if chksum != chksum_expect:
	gdaltest.post_reason( 'Did not get expected chksum for band 1' )
	print chksum, chksum_expect
	return 'fail'

    md = ds.GetMetadata()
    if md['NITF_FHDR'] != 'NSIF01.00':
        gdaltest.post_reason( 'Got wrong FHDR value' )
        return 'fail'

    if md['NITF_BLOCKA_BLOCK_INSTANCE_01'] != '01' \
       or md['NITF_BLOCKA_BLOCK_COUNT'] != '01' \
       or md['NITF_BLOCKA_N_GRAY_01'] != '00000' \
       or md['NITF_BLOCKA_L_LINES_01'] != '01000' \
       or md['NITF_BLOCKA_LAYOVER_ANGLE_01'] != '000' \
       or md['NITF_BLOCKA_SHADOW_ANGLE_01'] != '000' \
       or md['NITF_BLOCKA_FRLC_LOC_01'] != '+41.319331+020.078400' \
       or md['NITF_BLOCKA_LRLC_LOC_01'] != '+41.317083+020.126072' \
       or md['NITF_BLOCKA_LRFC_LOC_01'] != '+41.281634+020.122570' \
       or md['NITF_BLOCKA_FRFC_LOC_01'] != '+41.283881+020.074924':
        gdaltest.post_reason( 'BLOCKA metadata has unexpected value.' )
        return 'fail'
    
    return 'success'

###############################################################################
# Create and read a JPEG encoded NITF file.

def nitf_9():

    src_ds = gdal.Open( 'data/rgbsmall.tif' )
    ds = gdal.GetDriverByName('NITF').CreateCopy( 'tmp/nitf9.ntf', src_ds,
                                                  options = ['IC=C3'] )
    src_ds = None
    ds = None

    ds = gdal.Open( 'tmp/nitf9.ntf' )
    
    (exp_mean, exp_stddev) = (65.9532, 46.9026375565)
    (mean, stddev) = ds.GetRasterBand(1).ComputeBandStats()
    
    if abs(exp_mean-mean) > 0.01 or abs(exp_stddev-stddev) > 0.01:
        print mean, stddev
        gdaltest.post_reason( 'did not get expected mean or standard dev.' )
        return 'fail'

    md = ds.GetMetadata('IMAGE_STRUCTURE')
    if md['COMPRESSION'] != 'JPEG':
        gdaltest.post_reason( 'Did not get expected compression value.' )
        return 'fail'
    
    return 'success'

###############################################################################
# For esoteric reasons, createcopy from jpeg compressed nitf files can be
# tricky.  Verify this is working. 

def nitf_10():

    tst = gdaltest.GDALTest( 'NITF', '../tmp/nitf9.ntf', 2, 22296 )
    return tst.testCreateCopy()

###############################################################################
# Test 1bit file ... conveniently very small and easy to include! (#1854)

def nitf_11():

    # From http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv2_1/i_3034c.ntf
    tst = gdaltest.GDALTest( 'NITF', 'i_3034c.ntf', 1, 170 )
    return tst.testOpen()

###############################################################################
# Verify that TRE and CGM access via the metadata domain works.

def nitf_12():

    ds = gdal.Open( 'data/fake_nsif.ntf' )

    mdTRE = ds.GetMetadata( 'TRE' )

    try: # NG bindings
        blockA = ds.GetMetadataItem( 'BLOCKA', 'TRE' )
    except:
        blockA = mdTRE['BLOCKA']

    mdCGM = ds.GetMetadata( 'CGM' )

    try: # NG bindings
        segmentCount = ds.GetMetadataItem( 'SEGMENT_COUNT', 'CGM' )
    except:
        segmentCount = mdCGM['SEGMENT_COUNT']

    ds = None

    expectedBlockA = '010000001000000000                +41.319331+020.078400+41.317083+020.126072+41.281634+020.122570+41.283881+020.074924     '

    if mdTRE['BLOCKA'] != expectedBlockA:
        gdaltest.post_reason( 'did not find expected BLOCKA from metadata.' )
        return 'fail'

    if blockA != expectedBlockA:
        gdaltest.post_reason( 'did not find expected BLOCKA from metadata item.' )
        return 'fail'

    if mdCGM['SEGMENT_COUNT'] != '0':
        gdaltest.post_reason( 'did not find expected SEGMENT_COUNT from metadata.' )
        return 'fail'

    if segmentCount != '0':
        gdaltest.post_reason( 'did not find expected SEGMENT_COUNT from metadata item.' )
        return 'fail'

    return 'success'


###############################################################################
# Test creation of an NITF file in UTM Zone 11, Southern Hemisphere.

def nitf_13():
    drv = gdal.GetDriverByName( 'NITF' )
    ds = drv.Create( 'tmp/test_13.ntf', 200, 100, 1, gdal.GDT_Byte,
                     [ 'ICORDS=S' ] )
    ds.SetGeoTransform( (400000, 10, 0.0, 6000000, 0.0, -10 ) )
    ds.SetProjection('PROJCS["UTM Zone 11, Southern Hemisphere",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9108"]],AUTHORITY["EPSG","4326"]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",-117],PARAMETER["scale_factor",0.9996],PARAMETER["false_easting",500000],PARAMETER["false_northing",10000000],UNIT["Meter",1]]')

    list = range(200)
    raw_data = array.array('f',list).tostring()

    for line in range(100):
        ds.WriteRaster( 0, line, 200, 1, raw_data,
                        buf_type = gdal.GDT_Int16,
                        band_list = [1] )

    ds = None

    return 'success'

###############################################################################
# Verify previous file

def nitf_14():
    ds = gdal.Open( 'tmp/test_13.ntf' )

    chksum = ds.GetRasterBand(1).Checksum()
    chksum_expect = 55964
    if chksum != chksum_expect:
        gdaltest.post_reason( 'Did not get expected chksum for band 1' )
        print chksum, chksum_expect
        return 'fail'

    geotransform = ds.GetGeoTransform()
    if abs(geotransform[0]-400000) > .1 \
    or abs(geotransform[1]-10) > 0.001 \
    or abs(geotransform[2]-0) > 0.001 \
    or abs(geotransform[3]-6000000) > .1 \
    or abs(geotransform[4]-0) > 0.001 \
    or abs(geotransform[5]- -10) > 0.001:
        print geotransform
        gdaltest.post_reason( 'geotransform differs from expected' )
        return 'fail'

    prj = ds.GetProjectionRef()
    if string.find(prj,'UTM Zone 11, Southern Hemisphere') == -1:
        print prj
        gdaltest.post_reason( 'Coordinate system not UTM Zone 11, Southern Hemisphere' )
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test creating an in memory copy.

def nitf_15():

    tst = gdaltest.GDALTest( 'NITF', 'byte.tif', 1, 4672 )

    return tst.testCreateCopy( vsimem = 1 )

###############################################################################
# Checks a 1-bit mono with mask table having (0x00) black as transparent with white arrow.

def nitf_16():

    # From http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv2_1/ns3034d.nsf
    tst = gdaltest.GDALTest( 'NITF', 'ns3034d.nsf', 1, 170 )
    return tst.testOpen()


###############################################################################
# Checks a 1-bit RGB/LUT (green arrow) with a mask table (pad pixels having value of 0x00)
# and a transparent pixel value of 1 being mapped to green by the LUT

def nitf_17():

    # From http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv2_1/i_3034f.ntf
    tst = gdaltest.GDALTest( 'NITF', 'i_3034f.ntf', 1, 170 )
    return tst.testOpen()

###############################################################################
# Test NITF file without image segment

def nitf_18():

    # Shut up the warning about missing image segment
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    # From http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv1_1/U_0006A.NTF
    ds = gdal.Open("data/U_0006A.NTF")
    gdal.PopErrorHandler()

    if ds.RasterCount != 0:
        return 'fail'

    return 'success'

###############################################################################
# Test BILEVEL (C1) decompression

def nitf_19():

    # From http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv2_0/U_1050A.NTF
    tst = gdaltest.GDALTest( 'NITF', 'U_1050A.NTF', 1, 65024 )

    return tst.testOpen()


###############################################################################
# Test NITF file consiting only of an header

def nitf_20():

    # Shut up the warning about file either corrupt or empty
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    # From http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv1_1/U_0002A.NTF
    ds = gdal.Open("data/U_0002A.NTF")
    gdal.PopErrorHandler()

    if ds is not None:
        return 'fail'

    return 'success'


###############################################################################
# Verify that TEXT access via the metadata domain works.

def nitf_21():

    # Shut up the warning about missing image segment
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open( 'data/ns3114a.nsf' )
    gdal.PopErrorHandler()

    mdTEXT = ds.GetMetadata( 'TEXT' )

    try: # NG bindings
        data0 = ds.GetMetadataItem( 'DATA_0', 'TEXT' )
    except:
        data0 = mdTEXT['DATA_0']

    ds = None

    if mdTEXT['DATA_0'] != 'A':
        gdaltest.post_reason( 'did not find expected DATA_0 from metadata.' )
        return 'fail'

    if data0 != 'A':
        gdaltest.post_reason( 'did not find expected DATA_0 from metadata item.' )
        return 'fail'

    return 'success'


###############################################################################
# Write/Read test of simple int32 reference data. 

def nitf_22():

    tst = gdaltest.GDALTest( 'NITF', '../../gcore/data/int32.tif', 1, 4672 )
    return tst.testCreateCopy()

###############################################################################
# Write/Read test of simple float32 reference data. 

def nitf_23():

    tst = gdaltest.GDALTest( 'NITF', '../../gcore/data/float32.tif', 1, 4672 )
    return tst.testCreateCopy()

###############################################################################
# Write/Read test of simple float64 reference data. 

def nitf_24():

    tst = gdaltest.GDALTest( 'NITF', '../../gcore/data/float64.tif', 1, 4672 )
    return tst.testCreateCopy()

###############################################################################
# Write/Read test of simple uint16 reference data. 

def nitf_25():

    tst = gdaltest.GDALTest( 'NITF', '../../gcore/data/uint16.tif', 1, 4672 )
    return tst.testCreateCopy()

###############################################################################
# Write/Read test of simple uint32 reference data. 

def nitf_26():

    tst = gdaltest.GDALTest( 'NITF', '../../gcore/data/uint32.tif', 1, 4672 )
    return tst.testCreateCopy()

###############################################################################
# Test Create() with IC=NC compression, and multi-blocks

def nitf_27():

    if nitf_create([ 'ICORDS=G', 'IC=NC', 'BLOCKXSIZE=10', 'BLOCKYSIZE=10' ]) != 'success':
        return 'fail'

    return nitf_check_created_file(32498, 42602, 38982)


###############################################################################
# Test Create() with IC=C8 compression

def nitf_28():
    try:
        jp2ecw_drv = gdal.GetDriverByName( 'JP2ECW' )
    except:
        jp2ecw_drv = None
        return 'skip'

    if jp2ecw_drv is None:
        return 'skip'

    if nitf_create([ 'ICORDS=G', 'IC=C8' ]) != 'success':
        return 'fail'

    return nitf_check_created_file(32398, 42502, 38882)

###############################################################################
# Test Create() with a LUT

def nitf_29():

    drv = gdal.GetDriverByName( 'NITF' )

    ds = drv.Create( 'tmp/test_29.ntf', 1, 1, 1, gdal.GDT_Byte,
                     [ 'IREP=RGB/LUT', 'LUT_SIZE=128' ] )

    ct = gdal.ColorTable()
    ct.SetColorEntry( 0, (255,255,255,255) )
    ct.SetColorEntry( 1, (255,255,0,255) )
    ct.SetColorEntry( 2, (255,0,255,255) )
    ct.SetColorEntry( 3, (0,255,255,255) )

    ds.GetRasterBand( 1 ).SetRasterColorTable( ct )

    ds = None

    ds = gdal.Open( 'tmp/test_29.ntf' )

    ct = ds.GetRasterBand( 1 ).GetRasterColorTable()
    if ct.GetCount() != 129 or \
       ct.GetColorEntry(0) != (255,255,255,255) or \
       ct.GetColorEntry(1) != (255,255,0,255) or \
       ct.GetColorEntry(2) != (255,0,255,255) or \
       ct.GetColorEntry(3) != (0,255,255,255):
        gdaltest.post_reason( 'Wrong color table entry.' )
        return 'fail'

    new_ds = drv.CreateCopy( 'tmp/test_29_copy.ntf', ds )
    new_ds = None
    ds = None

    ds = gdal.Open( 'tmp/test_29_copy.ntf' )

    ct = ds.GetRasterBand( 1 ).GetRasterColorTable()
    if ct.GetCount() != 130 or \
       ct.GetColorEntry(0) != (255,255,255,255) or \
       ct.GetColorEntry(1) != (255,255,0,255) or \
       ct.GetColorEntry(2) != (255,0,255,255) or \
       ct.GetColorEntry(3) != (0,255,255,255):
        gdaltest.post_reason( 'Wrong color table entry.' )
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test NITF21_CGM_ANNO_Uncompressed_unmasked.ntf for bug #1313 and #1714

def nitf_online_1():

    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/nitf/bugs/NITF21_CGM_ANNO_Uncompressed_unmasked.ntf', 'NITF21_CGM_ANNO_Uncompressed_unmasked.ntf'):
        return 'skip'

    tst = gdaltest.GDALTest( 'NITF', 'tmp/cache/NITF21_CGM_ANNO_Uncompressed_unmasked.ntf', 1, 13123, filename_absolute = 1 )

    # Shut up the warning about missing image segment
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ret = tst.testOpen()
    gdal.PopErrorHandler()

    return ret

###############################################################################
# Test NITF file with multiple images

def nitf_online_2():

    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/nitf/nitf1.1/U_0001a.ntf', 'U_0001a.ntf'):
        return 'skip'

    ds = gdal.Open( 'tmp/cache/U_0001a.ntf' )

    md = ds.GetMetadata('SUBDATASETS')
    if not md.has_key('SUBDATASET_1_NAME'):
        gdaltest.post_reason( 'missing SUBDATASET_1_NAME metadata' )
        return 'fail'
    ds = None

    return 'success'

###############################################################################
# Test ARIDPCM (C2) image

def nitf_online_3():

    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/nitf/nitf1.1/U_0001a.ntf', 'U_0001a.ntf'):
        return 'skip'

    tst = gdaltest.GDALTest( 'NITF', 'NITF_IM:3:tmp/cache/U_0001a.ntf', 1, 23463, filename_absolute = 1 )

    return tst.testOpen()

###############################################################################
# Test Vector Quantization (VQ) (C4) file

def nitf_online_4():

    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/nitf/cadrg/001zc013.on1', '001zc013.on1'):
        return 'skip'

    tst = gdaltest.GDALTest( 'NITF', 'tmp/cache/001zc013.on1', 1, 53960, filename_absolute = 1 )

    return tst.testOpen()

###############################################################################
# Test Vector Quantization (VQ) (M4) file

def nitf_online_5():

    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/nitf/cadrg/overview.ovr', 'overview.ovr'):
        return 'skip'

    tst = gdaltest.GDALTest( 'NITF', 'tmp/cache/overview.ovr', 1, 60699, filename_absolute = 1 )

    return tst.testOpen()

###############################################################################
# Test a JPEG compressed, single blocked 2048x2048 mono image

def nitf_online_6():

    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/nitf/nitf2.0/U_4001b.ntf', 'U_4001b.ntf'):
        return 'skip'

    tst = gdaltest.GDALTest( 'NITF', 'tmp/cache/U_4001b.ntf', 1, 60030, filename_absolute = 1 )

    return tst.testOpen()


###############################################################################
# Test all combinations of IMODE (S,P,B,R) for an image with 6 bands whose 3 are RGB

def nitf_online_7():

    files = [ 'ns3228b.nsf', 'i_3228c.ntf', 'ns3228d.nsf', 'i_3228e.ntf' ]
    for file in files:
        if not gdaltest.download_file('http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv2_1/' + file, file):
            return 'skip'

        ds = gdal.Open('tmp/cache/' + file)
        if ds.RasterCount != 6:
            return 'fail'

        checksums = [ 48385, 48385, 40551, 54223, 48385, 33094 ]
        colorInterpretations = [ gdal.GCI_Undefined, gdal.GCI_Undefined, gdal.GCI_RedBand, gdal.GCI_BlueBand, gdal.GCI_Undefined, gdal.GCI_GreenBand ]

        for i in range(6):
            if ds.GetRasterBand(i+1).Checksum() != checksums[i]:
                return 'fail'
            if ds.GetRasterBand(i+1).GetRasterColorInterpretation() != colorInterpretations[i]:
                return 'fail'
        ds = None

    return 'success'

###############################################################################
# Test JPEG-compressed multi-block mono-band image with a data mask subheader (IC=M3, IMODE=B)

def nitf_online_8():

    if not gdaltest.download_file('http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv2_1/ns3301j.nsf', 'ns3301j.nsf'):
        return 'skip'

    tst = gdaltest.GDALTest( 'NITF', 'tmp/cache/ns3301j.nsf', 1, 56861, filename_absolute = 1 )

    return tst.testOpen()


###############################################################################
# Test JPEG-compressed multi-block mono-band image without a data mask subheader (IC=C3, IMODE=B)

def nitf_online_9():

    if not gdaltest.download_file('http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv2_1/ns3304a.nsf', 'ns3304a.nsf'):
        return 'skip'

    tst = gdaltest.GDALTest( 'NITF', 'tmp/cache/ns3304a.nsf', 1, 32419, filename_absolute = 1 )

    return tst.testOpen()


###############################################################################
# Verify that CGM access on a file with 8 CGM segments

def nitf_online_10():


    if not gdaltest.download_file('http://www.gwg.nga.mil/ntb/baseline/software/testfile/Nitfv2_1/ns3119b.nsf', 'ns3119b.nsf'):
        return 'skip'

    # Shut up the warning about missing image segment
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open( 'tmp/cache/ns3119b.nsf' )
    gdal.PopErrorHandler()

    mdCGM = ds.GetMetadata( 'CGM' )

    ds = None

    if mdCGM['SEGMENT_COUNT'] != '8':
        gdaltest.post_reason( 'wrong SEGMENT_COUNT.' )
        return 'fail'

    tab = [
        ('SEGMENT_0_SLOC_ROW', '00000'),
        ('SEGMENT_0_SLOC_COL', '00000'),
        ('SEGMENT_1_SLOC_ROW', '00000'),
        ('SEGMENT_1_SLOC_COL', '00684'),
        ('SEGMENT_2_SLOC_ROW', '00000'),
        ('SEGMENT_2_SLOC_COL', '01364'),
        ('SEGMENT_3_SLOC_ROW', '00270'),
        ('SEGMENT_3_SLOC_COL', '00000'),
        ('SEGMENT_4_SLOC_ROW', '00270'),
        ('SEGMENT_4_SLOC_COL', '00684'),
        ('SEGMENT_5_SLOC_ROW', '00270'),
        ('SEGMENT_5_SLOC_COL', '01364'),
        ('SEGMENT_6_SLOC_ROW', '00540'),
        ('SEGMENT_6_SLOC_COL', '00000'),
        ('SEGMENT_7_SLOC_ROW', '00540'),
        ('SEGMENT_7_SLOC_COL', '01364')
        ]

    for item in tab:
        if mdCGM[item[0]] != item[1]:
            gdaltest.post_reason( 'wrong value for %s.' % item[0] )
            return 'fail'

    return 'success'

###############################################################################
# 5 text files

def nitf_online_11():

    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/nitf/nitf2.0/U_1122a.ntf', 'U_1122a.ntf'):
        return 'skip'

    ds = gdal.Open( 'tmp/cache/U_1122a.ntf' )

    mdTEXT = ds.GetMetadata( 'TEXT' )

    ds = None

    if mdTEXT['DATA_0'] != 'This is test text file 01.\r\n':
        gdaltest.post_reason( 'did not find expected DATA_0 from metadata.' )
        return 'fail'
    if mdTEXT['DATA_1'] != 'This is test text file 02.\r\n':
        gdaltest.post_reason( 'did not find expected DATA_1 from metadata.' )
        return 'fail'
    if mdTEXT['DATA_2'] != 'This is test text file 03.\r\n':
        gdaltest.post_reason( 'did not find expected DATA_2 from metadata.' )
        return 'fail'
    if mdTEXT['DATA_3'] != 'This is test text file 04.\r\n':
        gdaltest.post_reason( 'did not find expected DATA_3 from metadata.' )
        return 'fail'
    if mdTEXT['DATA_4'] != 'This is test text file 05.\r\n':
        gdaltest.post_reason( 'did not find expected DATA_4 from metadata.' )
        return 'fail'

    return 'success'


###############################################################################
# Cleanup.

def nitf_cleanup():
    try:
        gdal.GetDriverByName('NITF').Delete( 'tmp/test_create.ntf' )
    except:
        pass

    try:
        gdal.GetDriverByName('NITF').Delete( 'tmp/nitf9.ntf' )
    except:
        pass

    try:
        gdal.GetDriverByName('NITF').Delete( 'tmp/test_13.ntf' )
    except:
        pass

    try:
        gdal.GetDriverByName('NITF').Delete( 'tmp/test_29.ntf' )
    except:
        pass

    try:
        gdal.GetDriverByName('NITF').Delete( 'tmp/test_29_copy.ntf' )
    except:
        pass

    return 'success'

gdaltest_list = [
    nitf_1,
    nitf_2,
    nitf_3,
    nitf_4,
    nitf_5,
    nitf_6,
    nitf_7,
    nitf_8,
    nitf_9,
    nitf_10,
    nitf_11,
    nitf_12,
    nitf_13,
    nitf_14,
    nitf_15,
    nitf_16,
    nitf_17,
    nitf_18,
    nitf_19,
    nitf_20,
    nitf_21,
    nitf_22,
    nitf_23,
    nitf_24,
    nitf_25,
    nitf_26,
    nitf_27,
    nitf_28,
    nitf_29,
    nitf_online_1,
    nitf_online_2,
    nitf_online_3,
    nitf_online_4,
    nitf_online_5,
    nitf_online_6,
    nitf_online_7,
    nitf_online_8,
    nitf_online_9,
    nitf_online_10,
    nitf_online_11,
    nitf_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'nitf' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

