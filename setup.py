#!/usr/bin/env python
#******************************************************************************
#  $Id$
# 
#  Name:     setup.py
#  Project:  GDAL
#  Purpose:  Installation / Install script.
#  Author:   Evgeniy Cherkashin <eugeneai@icc.ru>
# 
#******************************************************************************
#  Copyright (c) 2003, Evgeniy Cherkashin <eugeneai@icc.ru>
# 
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#******************************************************************************
# 
# $Log$
# Revision 1.2  2003/02/07 14:11:03  warmerda
# corrected a few info items, added header
#

import string
from distutils.core import setup, Extension
import os, os.path, glob

#
# debug
#
#import sys
#sys.argv.append("build")

f=open(os.path.join("VERSION"))
v=f.read()
f.close()
del f

version=v.strip()
dllversion=version.replace(".","")
soversion=v[:3]

SOURCES=glob.glob(os.path.join("pymod","*.c*"))
print SOURCES
INCLUDE_DIRS=[os.path.join("core"), os.path.join("port"), os.path.join("ogr"), os.path.join("pymod"), ] # only necessary
LIBRARY_DIRS = ["."]

INCLUDE_FILES = [
	glob.glob(os.path.join("core", "*.h")),
	glob.glob(os.path.join("port", "*.h")),
	glob.glob(os.path.join("alg", "*.h")),
	glob.glob(os.path.join("ogr", "*.h")), 
	glob.glob(os.path.join("ogr", "ogrsf_frmts", "*.h")),
	glob.glob(os.path.join("pymod", "*.h"))
]

IF=[]
for i in INCLUDE_FILES:
	IF.extend(i)
INCLUDE_FILES=IF
del IF

HTML_FILES=glob.glob(os.path.join("html", "*"))

#HTML_FILES.remove(os.path.join("html", ".cvsignore"))

#print INCLUDE_FILES

if os.name=="nt":
	DLL="gdal%s.dll" % dllversion
	DATA_FILES=[
		("", [DLL]),
#		("libs", [os.path.join("gdal_i.lib"), os.path.join("pymod","_gdal.lib")]),
		("libs", [os.path.join("gdal_i.lib")]),
	]
	LIBRARIES = ["gdal_i"]
	EXTRA_LINK_ARGS=[]
else:
	DATA_FILES=[
		("lib", ['libgdal.%s.so' % soversion]),
	]
	LIBRARIES = []
	#EXTRA_LINK_ARGS=[os.path.join("gdal.a")]
	LIBRARIES = ["gdal.%s" % soversion]
	EXTRA_LINK_ARGS=[]
	
DATA_FILES.append(("include", INCLUDE_FILES))
DATA_FILES.append((os.path.join("doc","gdal"), HTML_FILES))

setup (name = "Pyhton_GDAL",
       version = version,
       description = "Geospatial Data Abstraction Library: Python Bindings",
       author = "Frank Warmerdam",
       #packager = "Evgeniy Cherkashin",
       author_email = "warmerdam@pobox.com",
       #packager_email = "eugeneai@icc.ru",
       url = "http://www.remotesensing.org/gdal/",
       packages = [''],
       package_dir = {'': 'pymod'},
       extra_path = "gdal",
       ext_modules = [Extension('_gdal',
			sources = SOURCES,
			include_dirs = INCLUDE_DIRS,
			libraries = LIBRARIES,
			library_dirs = LIBRARY_DIRS,
			extra_link_args=EXTRA_LINK_ARGS,
			),
		],
		data_files = DATA_FILES
	)

