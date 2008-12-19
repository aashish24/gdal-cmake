#!/usr/bin/env python
###############################################################################
# $Id: test_cli_utilities.py $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Helper functions for testing CLI utilities
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
import gdal

cli_exe_path = { }

###############################################################################
# 
def get_cli_utility_path_internal(cli_utility_name):
    # First try : in the apps directory of the GDAL source tree
    # This is the case for the buildbot directory tree
    try:
        cli_utility_path = os.path.join(os.getcwd(), '..', '..', 'gdal', 'apps', cli_utility_name)
        (child_stdin, child_stdout, child_stderr) = os.popen3(cli_utility_path + ' --utility_version')
        ret = child_stdout.read()
        child_stdin.close()
        child_stdout.close()
        child_stderr.close()

        if ret.find('GDAL') != -1:
            print ret
            return cli_utility_path
    except:
        pass

    # Otherwise look up in the system path
    try:
        cli_utility_path = cli_utility_name
        (child_stdin, child_stdout, child_stderr) = os.popen3(cli_utility_path + ' --utility_version')
        ret = child_stdout.read()
        child_stdin.close()
        child_stdout.close()
        child_stderr.close()

        if ret.find('GDAL') != -1:
            print ret
            return cli_utility_path
    except:
        pass

    return None

###############################################################################
# 
def get_cli_utility_path(cli_utility_name):
    global cli_exe_path
    if cli_exe_path.has_key(cli_utility_name):
        return cli_exe_path[cli_utility_name]
    else:
        cli_exe_path[cli_utility_name] = get_cli_utility_path_internal(cli_utility_name)
        return cli_exe_path[cli_utility_name]

###############################################################################
# 
def get_gdalinfo_path():
    return get_cli_utility_path('gdalinfo')

###############################################################################
# 
def get_gdal_translate_path():
    return get_cli_utility_path('gdal_translate')

###############################################################################
# 
def get_gdalwarp_path():
    return get_cli_utility_path('gdalwarp')

###############################################################################
# 
def get_gdaladdo_path():
    return get_cli_utility_path('gdaladdo')

###############################################################################
# 
def get_gdaltransform_path():
    return get_cli_utility_path('gdaltransform')

###############################################################################
# 
def get_gdaltindex_path():
    return get_cli_utility_path('gdaltindex')

###############################################################################
# 
def get_gdal_grid_path():
    return get_cli_utility_path('gdal_grid')

###############################################################################
# 
def get_ogrinfo_path():
    return get_cli_utility_path('ogrinfo')

###############################################################################
# 
def get_ogr2ogr_path():
    return get_cli_utility_path('ogr2ogr')

###############################################################################
# 
def get_ogrtindex_path():
    return get_cli_utility_path('ogrtindex')

###############################################################################
# 
def get_gdalbuildvrt_path():
    return get_cli_utility_path('gdalbuildvrt')
