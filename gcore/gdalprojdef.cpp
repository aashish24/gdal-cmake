/******************************************************************************
 * $Id$
 *
 * Name:     gdalprojdef.cpp
 * Project:  GDAL Core Projections
 * Purpose:  Implementation of the GDALProjDef class, a class abstracting
 *           the interface to PROJ.4 projection services.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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
 * $Log$
 * Revision 1.1  1999/01/11 15:36:26  warmerda
 * New
 *
 */

#include "gdal_priv.h"
#include "cpl_conv.h"
#include "cpl_string.h"

typedef struct { double u, v; }	UV;

#define PJ void

static PJ	*(*pfn_pj_init)(int, char**) = NULL;
static UV	(*pfn_pj_fwd)(UV, PJ *) = NULL;
static UV	(*pfn_pj_inv)(UV, PJ *) = NULL;
static void	(*pfn_pj_free)(PJ *) = NULL;

#define RAD_TO_DEG	57.29577951308232
#define DEG_TO_RAD	.0174532925199432958

/************************************************************************/
/*                          LoadProjLibrary()                           */
/************************************************************************/

static int LoadProjLibrary()

{
    static int	bTriedToLoad = FALSE;
    
    if( bTriedToLoad )
        return( pfn_pj_init != NULL );

    bTriedToLoad = TRUE;
    
    pfn_pj_init = (PJ *(*)(int, char**)) CPLGetSymbol( "libproj.so",
                                                       "pj_init" );
    pfn_pj_fwd = (UV (*)(UV,PJ*)) CPLGetSymbol( "libproj.so", "pj_fwd" );
    pfn_pj_inv = (UV (*)(UV,PJ*)) CPLGetSymbol( "libproj.so", "pj_inv" );
    pfn_pj_free = (void (*)(PJ*)) CPLGetSymbol( "libproj.so", "pj_free" );

    return( pfn_pj_init != NULL );
}

/************************************************************************/
/*                            GDALProjDef()                             */
/************************************************************************/

GDALProjDef::GDALProjDef( const char * pszProjectionIn )

{
    if( pszProjectionIn == NULL )
        pszProjection = CPLStrdup( "" );
    else
        pszProjection = CPLStrdup( pszProjectionIn );

    psPJ = NULL;

    SetProjectionString( pszProjectionIn );
}

/************************************************************************/
/*                         GDALCreateProjDef()                          */
/************************************************************************/

GDALProjDefH GDALCreateProjDef( const char * pszProjection )

{
    return( (GDALProjDefH) (new GDALProjDef( pszProjection )) );
}

/************************************************************************/
/*                            ~GDALProjDef()                            */
/************************************************************************/

GDALProjDef::~GDALProjDef()

{
    CPLFree( pszProjection );
    if( psPJ != NULL )
        pfn_pj_free( psPJ );
}

/************************************************************************/
/*                         GDALDestroyProjDef()                         */
/************************************************************************/

void GDALDestroyProjDef( GDALProjDefH hProjDef )

{
    delete (GDALProjDef *) hProjDef;
}

/************************************************************************/
/*                        SetProjectionString()                         */
/************************************************************************/

CPLErr GDALProjDef::SetProjectionString( const char * pszProjectionIn )

{
    char	**args;

    pszProjection = CPLStrdup( pszProjectionIn );
    
    if( !LoadProjLibrary() )
    {
        return CE_Failure;
    }

    args = CSLTokenizeStringComplex( pszProjection, " +", TRUE, FALSE );

    psPJ = pfn_pj_init( CSLCount(args), args );

    CSLDestroy( args );

    if( psPJ == NULL )
        return CE_Failure;
    else
        return CE_None;
}

/************************************************************************/
/*                             ToLongLat()                              */
/************************************************************************/

CPLErr GDALProjDef::ToLongLat( double * padfX, double * padfY )

{
    UV		uv;
    
    CPLAssert( padfX != NULL && padfY != NULL );
    
    if( psPJ == NULL )
        return CE_Failure;

    if( strstr(pszProjection,"+proj=longlat") != NULL
        || strstr(pszProjection,"+proj=latlong") != NULL )
        return CE_None;

    uv.u = *padfX;
    uv.v = *padfY;

    uv = pfn_pj_inv( uv, psPJ );

    *padfX = uv.u * RAD_TO_DEG;
    *padfY = uv.v * RAD_TO_DEG;
    
    return( CE_None );
}

/************************************************************************/
/*                       GDALReprojectToLongLat()                       */
/************************************************************************/

CPLErr GDALReprojectToLongLat( GDALProjDefH hProjDef,
                               double * padfX, double * padfY )

{
    return( ((GDALProjDef *) hProjDef)->ToLongLat(padfX, padfY) );
}

/************************************************************************/
/*                            FromLongLat()                             */
/************************************************************************/

CPLErr GDALProjDef::FromLongLat( double * padfX, double * padfY )

{
    UV		uv;
    
    CPLAssert( padfX != NULL && padfY != NULL );
    
    if( psPJ == NULL )
        return CE_Failure;

    if( strstr(pszProjection,"+proj=longlat") != NULL
        || strstr(pszProjection,"+proj=latlong") != NULL )
        return CE_None;

    uv.u = *padfX * DEG_TO_RAD;
    uv.v = *padfY * DEG_TO_RAD;

    uv = pfn_pj_fwd( uv, psPJ );

    *padfX = uv.u;
    *padfY = uv.v;
    
    return( CE_None );
}

/************************************************************************/
/*                      GDALReprojectFromLongLat()                      */
/************************************************************************/

CPLErr GDALReprojectFromLongLat( GDALProjDefH hProjDef,
                                 double * padfX, double * padfY )

{
    return( ((GDALProjDef *) hProjDef)->FromLongLat(padfX, padfY) );
}

