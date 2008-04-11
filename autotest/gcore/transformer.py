#!/usr/bin/env python
###############################################################################
# $Id: pam_md.py 11065 2007-03-24 09:35:32Z mloskot $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test the GenImgProjTransformer capabilities.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2008, Frank Warmerdam <warmerdam@pobox.com>
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
import gdal
import gdalconst

###############################################################################
# Test simple Geotransform based transformer.

def transformer_1():

    try:
        x = gdal.Transformer
        gdaltest.have_ng = 1
    except:
        gdaltest.have_ng = 0
        return 'skip'

    ds = gdal.Open('data/byte.tif')
    tr = gdal.Transformer( ds, None, [] )

    (success,pnt) = tr.TransformPoint( 0, 20, 10 )

    if not success \
       or abs(pnt[0]- 441920) > 0.00000001 \
       or abs(pnt[1]-3750720) > 0.00000001 \
       or pnt[2] != 0.0:
        print success, pnt
        gdaltest.post_reason( 'got wrong forward transform result.' )
        return 'fail'

    (success,pnt) = tr.TransformPoint( 1, pnt[0], pnt[1], pnt[2] )

    if not success \
       or abs(pnt[0]-20) > 0.00000001 \
       or abs(pnt[1]-10) > 0.00000001 \
       or pnt[2] != 0.0:
        print success, pnt
        gdaltest.post_reason( 'got wrong reverse transform result.' )
        return 'fail'

    return 'success' 

###############################################################################
# Test GCP based transformer with polynomials.

def transformer_2():

    if not gdaltest.have_ng:
        return 'skip'

    ds = gdal.Open('data/gcps.vrt')
    tr = gdal.Transformer( ds, None, [ 'METHOD=GCP_POLYNOMIAL' ] )

    (success,pnt) = tr.TransformPoint( 0, 20, 10 )

    if not success \
       or abs(pnt[0]-441920) > 0.001 \
       or abs(pnt[1]-3750720) > 0.001 \
       or pnt[2] != 0:
        print success, pnt
        gdaltest.post_reason( 'got wrong forward transform result.' )
        return 'fail'

    (success,pnt) = tr.TransformPoint( 1, pnt[0], pnt[1], pnt[2] )

    if not success \
       or abs(pnt[0]-20) > 0.001 \
       or abs(pnt[1]-10) > 0.001 \
       or pnt[2] != 0:
        print success, pnt
        gdaltest.post_reason( 'got wrong reverse transform result.' )
        return 'fail'

    return 'success' 

###############################################################################
# Test GCP based transformer with thin plate splines.

def transformer_3():

    if not gdaltest.have_ng:
        return 'skip'

    ds = gdal.Open('data/gcps.vrt')
    tr = gdal.Transformer( ds, None, [ 'METHOD=GCP_TPS' ] )

    (success,pnt) = tr.TransformPoint( 0, 20, 10 )

    if not success \
       or abs(pnt[0]-441920) > 0.001 \
       or abs(pnt[1]-3750720) > 0.001 \
       or pnt[2] != 0:
        print success, pnt
        gdaltest.post_reason( 'got wrong forward transform result.' )
        return 'fail'

    (success,pnt) = tr.TransformPoint( 1, pnt[0], pnt[1], pnt[2] )

    if not success \
       or abs(pnt[0]-20) > 0.001 \
       or abs(pnt[1]-10) > 0.001 \
       or pnt[2] != 0:
        print success, pnt
        gdaltest.post_reason( 'got wrong reverse transform result.' )
        return 'fail'

    return 'success' 

###############################################################################
# Test geolocation based transformer.

def transformer_4():

    if not gdaltest.have_ng:
        return 'skip'

    ds = gdal.Open('data/sstgeo.vrt')
    tr = gdal.Transformer( ds, None, [ 'METHOD=GEOLOC_ARRAY' ] )

    (success,pnt) = tr.TransformPoint( 0, 20, 10 )

    if not success \
       or abs(pnt[0]+81.961341857910156) > 0.000001 \
       or abs(pnt[1]-29.612689971923828) > 0.000001 \
       or pnt[2] != 0:
        print success, pnt
        gdaltest.post_reason( 'got wrong forward transform result.' )
        return 'fail'

    (success,pnt) = tr.TransformPoint( 1, pnt[0], pnt[1], pnt[2] )

    if not success \
       or abs(pnt[0]-20) > 0.001 \
       or abs(pnt[1]-10) > 0.001 \
       or pnt[2] != 0:
        print success, pnt
        gdaltest.post_reason( 'got wrong reverse transform result.' )
        return 'fail'

    return 'success' 

###############################################################################
# Test RPC based transformer.

def transformer_5():

    if not gdaltest.have_ng:
        return 'skip'

    ds = gdal.Open('data/rpc.vrt')
    tr = gdal.Transformer( ds, None, [ 'METHOD=RPC' ] )

    (success,pnt) = tr.TransformPoint( 0, 20, 10 )

    if not success \
       or abs(pnt[0]-125.64830100509131) > 0.000001 \
       or abs(pnt[1]-39.869433991997553) > 0.000001 \
       or pnt[2] != 0:
        print success, pnt
        gdaltest.post_reason( 'got wrong forward transform result.' )
        return 'fail'

    (success,pnt) = tr.TransformPoint( 1, pnt[0], pnt[1], pnt[2] )

    if not success \
       or abs(pnt[0]-20) > 0.001 \
       or abs(pnt[1]-10) > 0.001 \
       or pnt[2] != 0:
        print success, pnt
        gdaltest.post_reason( 'got wrong reverse transform result.' )
        return 'fail'

    # Try with a different height.
    
    (success,pnt) = tr.TransformPoint( 0, 20, 10, 30 )

    if not success \
       or abs(pnt[0]-125.64828521533849) > 0.000001 \
       or abs(pnt[1]-39.869345204440144) > 0.000001 \
       or pnt[2] != 30:
        print success, pnt
        gdaltest.post_reason( 'got wrong forward transform result.(2)' )
        return 'fail'

    (success,pnt) = tr.TransformPoint( 1, pnt[0], pnt[1], pnt[2] )

    if not success \
       or abs(pnt[0]-20) > 0.001 \
       or abs(pnt[1]-10) > 0.001 \
       or pnt[2] != 30:
        print success, pnt
        gdaltest.post_reason( 'got wrong reverse transform result.(2)' )
        return 'fail'

    return 'success' 


gdaltest_list = [
    transformer_1,
    transformer_2,
    transformer_3,
    transformer_4,
    transformer_5 ]

if __name__ == '__main__':

    gdaltest.setup_run( 'transformer' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

