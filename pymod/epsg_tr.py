#!/usr/bin/env python
#******************************************************************************
#  $Id$
# 
#  Project:  CFS OGC MapServer
#  Purpose:  Script to create WKT and PROJ.4 dictionaries for EPSG GCS/PCS
#            codes.
#  Author:   Frank Warmerdam, warmerdam@pobox.com
# 
#******************************************************************************
#  Copyright (c) 2001, Frank Warmerdam
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
# Revision 1.4  2001/03/23 03:41:16  warmerda
# fixed bug in print statement
#
# Revision 1.3  2001/03/21 02:28:21  warmerda
# fixed proj.4 catalog output
#
# Revision 1.2  2001/03/15 03:20:12  warmerda
# various improvements
#
# Revision 1.1  2001/03/14 20:38:51  warmerda
# New
#
#

import osr
import sys
import string

# =============================================================================
def Usage():

    print 'Usage: epsg_tr.py [-wkt] [-pretty_wkt] [-proj4] start_code [end_code]'
    sys.exit(1)

# =============================================================================
def trHandleCode(code, gen_dict_line, report_error, output_format):

    import time
    old_time = time.clock()
    while time.clock() < old_time + 2.0:
        pass
    
    if prj_srs.ImportFromEPSG( code ) != 0:
        if report_error:
            print 'Unable to lookup ',code,', either not a valid EPSG'
            print 'code, or it the EPSG csv files are not accessable.'
            sys.exit(2)
    else:
        if output_format == '-pretty_wkt':
            if gen_dict_line:
                print 'EPSG:',code

            print prj_srs.ExportToPrettyWkt()

        if output_format == '-wkt':
            if gen_dict_line:
                print 'EPSG:',code
                    
            print prj_srs.ExportToWkt()
                
        if output_format == '-proj4':
            out_string = prj_srs.ExportToProj4()

            name = prj_srs.GetAttrValue('PROJCS')
            if name is None:
                name = prj_srs.GetAttrValue('GEOGCS')

            print '# '+name
            
            if string.find(out_string,'+proj=') > -1:
                print '<%s> %s no_defs <>' % (str(code), out_string)
            elif report_error:
                print '# Unable to translate coordinate system into PROJ.4 format.'


# =============================================================================

if __name__ == '__main__':

    start_code = -1
    end_code = -1
    list_file = None
    output_format = '-pretty_wkt'
    
    # Parse command line arguments.
    
    i = 1
    while i < len(sys.argv):
        arg = sys.argv[i]

        if arg == '-wkt' or arg == '-pretty_wkt' or arg == '-proj4':
            output_format = arg

        elif arg == '-list' and i < len(sys.argv)-1:
            i = i + 1
            list_file = sys.argv[i]
            
        elif arg[0] == '-':
            Usage()

        elif int(arg) > 0:
            
            if start_code == -1:
                start_code = int(arg)
                end_code = int(arg)
            elif end_code == start_code:
                end_code = int(arg)
            else:
                Usage()
        else:
            Usage()

        i = i + 1

    # Do we need to produce a single output definition, or include a
    # dictionary line for each entry?

    gen_dict_line = start_code != end_code

    # loop over all codes to generate output
    
    prj_srs = osr.SpatialReference()

    if start_code != -1:
        report_error = start_code == end_code
        for code in range(start_code,end_code+1):
            trHandleCode(code, gen_dict_line, report_error, output_format)

    # loop over codes read from list file.

    elif list_file is not None:

        list_fd = open( list_file )
        line = list_fd.readline()
        while len(line) > 0:
            try:
                c_offset = string.find(line,',')
                if c_offset > 0:
                    line = line[:c_offset]
                    
                code = string.atoi(line)
            except:
                code = -1

            if code <> -1:
                trHandleCode(code, gen_dict_line, 1, output_format)
                
            line = list_fd.readline()

    else:
        Usage()
        

