/******************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * gdalallregister.cpp
 *
 * Main format registration function.
 * 
 * $Log$
 * Revision 1.39  2001/12/08 04:43:48  warmerda
 * added BSB registration
 *
 * Revision 1.38  2001/11/27 14:39:41  warmerda
 * added usgsdem
 *
 * Revision 1.37  2001/11/16 21:13:47  warmerda
 * added VRT dataset
 *
 * Revision 1.36  2001/09/17 18:05:20  warmerda
 * Register DOQ2 format.
 *
 * Revision 1.35  2001/07/18 04:51:56  warmerda
 * added CPL_CVSID
 *
 * Revision 1.34  2001/07/05 23:53:53  nemec
 * Add FIT file format
 *
 * Revision 1.33  2001/05/15 13:24:42  warmerda
 * added fujibas
 *
 * Revision 1.32  2001/04/02 17:11:45  warmerda
 * added ecw support
 *
 * Revision 1.31  2001/03/12 15:17:32  warmerda
 * added aaigrid
 *
 * Revision 1.30  2001/03/06 03:53:44  sperkins
 * Added FITS format support.
 *
 * Revision 1.29  2001/02/06 16:34:30  warmerda
 * moved format registration entry points to gdal_frmts.h
 *
 * Revision 1.28  2001/01/15 14:32:30  warmerda
 * added envisat
 *
 * Revision 1.27  2001/01/10 04:41:09  warmerda
 * added gif support
 *
 * Revision 1.26  2001/01/03 18:53:23  warmerda
 * Added PNM
 *
 * Revision 1.25  2000/11/27 19:03:44  warmerda
 * added JDEM format
 *
 * Revision 1.24  2000/11/16 14:48:53  warmerda
 * moved GXF down in driver ordering
 *
 * Revision 1.23  2000/09/11 13:32:26  warmerda
 * added grass
 *
 * Revision 1.22  2000/07/19 19:06:39  warmerda
 * added mem
 *
 * Revision 1.21  2000/06/20 17:36:38  warmerda
 * added eosat fast format support
 *
 * Revision 1.20  2000/04/28 20:59:03  warmerda
 * added jpeg
 *
 * Revision 1.19  2000/04/27 20:02:17  warmerda
 * added png
 *
 * Revision 1.18  2000/04/04 23:44:45  warmerda
 * also call auto register function
 *
 * Revision 1.17  2000/03/31 13:35:32  warmerda
 * added SAR_CEOS
 *
 * Revision 1.16  2000/03/07 21:34:50  warmerda
 * added HKV
 *
 * Revision 1.15  2000/03/06 21:51:09  warmerda
 * Added MFF
 *
 * Revision 1.14  2000/01/31 16:24:37  warmerda
 * added aigrid2
 *
 * Revision 1.13  1999/12/29 20:42:45  warmerda
 * Added DOQ1
 *
 * Revision 1.12  1999/10/21 13:24:52  warmerda
 * Added documentation.
 *
 * Revision 1.11  1999/08/13 03:25:58  warmerda
 * add paux
 *
 * Revision 1.10  1999/07/23 19:36:41  warmerda
 * added raw/ehdr support
 *
 * Revision 1.9  1999/06/03 14:05:33  warmerda
 * added SDTS support
 *
 * Revision 1.8  1999/05/17 01:51:43  warmerda
 * Removed unused variable.
 *
 * Revision 1.7  1999/05/13 15:28:19  warmerda
 * Added elas format.
 *
 * Revision 1.6  1999/05/05 17:32:53  warmerda
 * added ceos
 *
 * Revision 1.5  1999/02/04 22:14:46  warmerda
 * added aigrid format
 *
 * Revision 1.4  1999/01/27 18:33:45  warmerda
 * Use FMRT_ macros to test if format avail
 *
 * Revision 1.3  1999/01/11 15:30:16  warmerda
 * added OGDI
 *
 * Revision 1.2  1998/12/03 18:37:26  warmerda
 * Drop GDB, add geotiff.
 *
 * Revision 1.1  1998/11/29 22:22:14  warmerda
 * New
 *
 */

#include "gdal_priv.h"
#include "gdal_frmts.h"

CPL_CVSID("$Id$");

#ifdef notdef
// we may have a use for this some day
static char *szConfiguredFormats = "GDAL_FORMATS";
#endif

/************************************************************************/
/*                          GDALAllRegister()                           */
/*                                                                      */
/*      Register all identifiably supported formats.                    */
/************************************************************************/

/**
 * Register all known configured GDAL drivers.
 *
 * This function will drive any of the following that are configured into
 * GDAL.  Possible others as well that haven't been updated in this
 * documentation:
 *
 * <ul>
 * <li> GeoTIFF (GTiff)
 * <li> Geosoft GXF (GXF)
 * <li> Erdas Imagine (HFA)
 * <li> CEOS (CEOS)
 * <li> ELAS (ELAS)
 * <li> Arc/Info Binary Grid (AIGrid)
 * <li> SDTS Raster DEM (SDTS)
 * <li> OGDI (OGDI)
 * <li> ESRI Labelled BIL (EHdr)
 * <li> PCI .aux Labelled Raw Raster (PAux)
 * </ul>
 *
 */

void GDALAllRegister()

{
    GetGDALDriverManager()->AutoLoadDrivers();

#ifdef FRMT_vrt
    GDALRegister_VRT();
#endif    

#ifdef FRMT_gdb    
    GDALRegister_GDB();
#endif    

#ifdef FRMT_gtiff    
    GDALRegister_GTiff();
#endif    

#ifdef FRMT_hfa
    GDALRegister_HFA();
#endif
    
#ifdef FRMT_ceos2
    GDALRegister_SAR_CEOS();
#endif
    
#ifdef FRMT_ceos
    GDALRegister_CEOS();
#endif
    
#ifdef FRMT_elas
    GDALRegister_ELAS();
#endif
    
#ifdef FRMT_aigrid
    GDALRegister_AIGrid2();
    GDALRegister_AIGrid();
#endif

#ifdef FRMT_aaigrid
    GDALRegister_AAIGrid();
#endif

#ifdef FRMT_sdts
    GDALRegister_SDTS();
#endif

#ifdef FRMT_ogdi
    GDALRegister_OGDI();
#endif

#ifdef FRMT_dted
    GDALRegister_DTED();
#endif

#ifdef FRMT_png
    GDALRegister_PNG();
#endif

#ifdef FRMT_jpeg
    GDALRegister_JPEG();
#endif

#ifdef FRMT_mem
    GDALRegister_MEM();
#endif

#ifdef FRMT_jdem
    GDALRegister_JDEM();
#endif

#ifdef FRMT_gif
    GDALRegister_GIF();
#endif

#ifdef FRMT_envisat
    GDALRegister_Envisat();
#endif

#ifdef FRMT_fits
    GDALRegister_FITS();
#endif

#ifdef FRMT_bsb
    GDALRegister_BSB();
#endif

#ifdef FRMT_ecw
    GDALRegister_ECW();
#endif

#ifdef FRMT_raw
    GDALRegister_PNM();
    GDALRegister_DOQ1();
    GDALRegister_DOQ2();
    GDALRegister_EHdr();
    GDALRegister_PAux();
    GDALRegister_MFF();
    GDALRegister_HKV();
    GDALRegister_EFF();
    GDALRegister_FujiBAS();
#endif

#ifdef FRMT_fit
    GDALRegister_FIT();
#endif

/* -------------------------------------------------------------------- */
/*      Our test for the following is weak or expensive so we try       */
/*      them last.                                                      */
/* -------------------------------------------------------------------- */
#ifdef FRMT_usgsdem
    GDALRegister_USGSDEM();
#endif

#ifdef FRMT_gxf
    GDALRegister_GXF();
#endif    

#ifdef FRMT_grass
    GDALRegister_GRASS();
#endif
}
