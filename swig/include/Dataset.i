/******************************************************************************
 * $Id$
 *
 * Name:     Dataset.i
 * Project:  GDAL Python Interface
 * Purpose:  GDAL Core SWIG Interface declarations.
 * Author:   Kevin Ruland, kruland@ku.edu
 *

 *
 * $Log$
 * Revision 1.7  2005/02/21 14:51:32  kruland
 * Needed to rename GDALDriver to GDALDriverShadow in the last commit.
 *
 * Revision 1.6  2005/02/20 19:42:53  kruland
 * Rename the Swig shadow classes so the names do not give the impression that
 * they are any part of the GDAL/OSR apis.  There were no bugs with the old
 * names but they were confusing.
 *
 * Revision 1.5  2005/02/17 17:27:13  kruland
 * Changed the handling of fixed size double arrays to make it fit more
 * naturally with GDAL/OSR usage.  Declare as typedef double * double_17;
 * If used as return argument use:  function ( ... double_17 argout ... );
 * If used as value argument use: function (... double_17 argin ... );
 *
 * Revision 1.4  2005/02/16 17:41:19  kruland
 * Added a few more methods to Dataset and marked the ones still missing.
 *
 * Revision 1.3  2005/02/15 20:50:49  kruland
 * Added SetProjection.
 *
 * Revision 1.2  2005/02/15 16:53:36  kruland
 * Removed use of vector<double> in the ?etGeoTransform() methods.  Use fixed
 * length double array type instead.
 *
 * Revision 1.1  2005/02/15 05:56:49  kruland
 * Created the Dataset shadow class definition.  Does not rely on the C++ api
 * in gdal_priv.h.  Need to remove the vector<>s and replace with fixed
 * size arrays.
 *
 *
*/

//************************************************************************
//
// Define the extensions for Dataset (nee GDALDatasetShadow)
//
//************************************************************************

%rename (Dataset) GDALDatasetShadow;

class GDALDatasetShadow {
private:
  GDALDatasetShadow();
public:
%extend {

%immutable;
  int RasterXSize;
  int RasterYSize;
%mutable;

  ~GDALDatasetShadow() {
    if ( GDALDereferenceDataset( self ) <= 0 ) {
      GDALClose(self);
    }
  }

  GDALDriverShadow* GetDriver() {
    return (GDALDriverShadow*) GDALGetDatasetDriver( self );
  }

  GDALRasterBandShadow* GetRasterBand(int nBand ) {
    return (GDALRasterBandShadow*) GDALGetRasterBand( self, nBand );
  }

  char const *GetProjection() {
    return GDALGetProjectionRef( self );
  }

  CPLErr SetProjection( char const *prj ) {
    return GDALSetProjection( self, prj );
  }

  void GetGeoTransform( double_6 argout ) {
    if ( GDALGetGeoTransform( self, argout ) != 0 ) {
      argout[0] = 0.0;
      argout[1] = 1.0;
      argout[2] = 0.0;
      argout[3] = 0.0;
      argout[4] = 0.0;
      argout[5] = 1.0;
    }
  }

  CPLErr SetGeoTransform( double_6 argin ) {
    return GDALSetGeoTransform( self, argin );
  }

%apply (char **dict) { char ** };
  char ** GetMetadata( const char * pszDomain = "" ) {
    return GDALGetMetadata( self, pszDomain );
  }
%clear char **;

%apply (char **dict) { char ** papszMetadata };
  CPLErr SetMetadata( char ** papszMetadata, const char * pszDomain = "" ) {
    return GDALSetMetadata( self, papszMetadata, pszDomain );
  }
%clear char **papszMetadata;

  // The int,int* arguments are typemapped.  The name of the first argument
  // becomes the kwarg name for it.
%feature("kwargs") BuildOverviews;
%apply (int nList, int* pList) { (int overviewlist, int *pOverviews) };
  int BuildOverviews( const char *resampling = "NEAREST", int overviewlist = 0 , int *pOverviews = 0 ) {
    return GDALBuildOverviews( self, resampling, overviewlist, pOverviews, 0, 0, 0, 0);
  }
%clear (int overviewlist, int *pOverviews);

  int GetGCPCount() {
    return GDALGetGCPCount( self );
  }

  const char *GetGCPProjection() {
    return GDALGetGCPProjection( self );
  }

  void GetGCPs( int *nGCPs, GDAL_GCP const **pGCPs ) {
    *nGCPs = GDALGetGCPCount( self );
    *pGCPs = GDALGetGCPs( self );
  }

  CPLErr SetGCPs( int nGCPs, GDAL_GCP const *pGCPs, const char *pszGCPProjection ) {
    return GDALSetGCPs( self, nGCPs, pGCPs, pszGCPProjection );
  }

  void FlushCache() {
    GDALFlushCache( self );
  }

/* NEEDED */
/* GetSubDatasets */
/* ReadAsArray */
/* AddBand */
/* AdviseRead */
/* ReadRaster */
/* WriteRaster */
  
}
};

%{
int GDALDatasetShadow_RasterXSize_get( GDALDatasetShadow *h ) {
  return GDALGetRasterXSize( h );
}
int GDALDatasetShadow_RasterYSize_get( GDALDatasetShadow *h ) {
  return GDALGetRasterYSize( h );
}
%}
