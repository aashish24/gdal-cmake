
GDAL_ROOT	=	.

include GDALmake.opt

default:	GDALmake.opt lib py-target apps-target

lib:	port-target core-target frmts-target ogr-target check-lib

force-lib:	
	$(AR) r $(GDAL_LIB) $(GDAL_OBJ)
	$(RANLIB) $(GDAL_LIB)
	$(LD_SHARED) $(GDAL_SLIB_SONAME) $(GDAL_OBJ) $(GDAL_LIBS) $(LIBS) \
		-o $(GDAL_SLIB)

$(GDAL_LIB):	$(GDAL_OBJ)
	$(AR) r $(GDAL_LIB) $(GDAL_OBJ)
	$(RANLIB) $(GDAL_LIB)

$(GDAL_SLIB):	$(GDAL_LIB) $(GDALA_DEP)
	$(LD_SHARED) $(GDAL_SLIB_SONAME) $(GDAL_OBJ) $(GDAL_LIBS) $(LIBS) \
		-o $(GDAL_SLIB)

check-lib:
	$(MAKE) $(GDAL_LIB)
ifeq ($(HAVE_LD_SHARED),yes)
	$(MAKE) $(GDAL_SLIB)
endif

port-target:
	(cd port; $(MAKE))

ifeq ($(OGR_ENABLED),yes)

ogr-target:
	(cd ogr; $(MAKE) sublibs lib )

else

ogr-target:
	(cd ogr; $(MAKE) lib )

endif

core-target:
	(cd core; $(MAKE))
	(cd alg; $(MAKE))

frmts-target:
	(cd frmts; $(MAKE))

ogr-all:
	(cd ogr; $(MAKE) all)

apps-target:	ogr-apps
	(cd apps; $(MAKE))


ifeq ($(OGR_ENABLED),yes)

ogr-apps:
	(cd ogr; $(MAKE) apps)

else

ogr-apps:

endif

#
#	We only make python a default target if we think python is installed.
#
ifeq ($(PYTHON),no)
py-target:
else
py-target:	py-module
endif

clean:	lclean
	(cd port; $(MAKE) clean)
	(cd ogr; $(MAKE) clean)
	(cd core; $(MAKE) clean)
	(cd frmts; $(MAKE) clean)
	(cd alg; $(MAKE) clean)
	(cd apps; $(MAKE) clean)
	(cd pymod; $(MAKE) clean)

py-module:
	(cd pymod; $(MAKE))

lclean:
	rm -f *.a *.so config.log config.cache html/*.*

distclean:	dist-clean

dist-clean:	clean
	rm -f GDALmake.opt port/cpl_config.h config.cache config.status

config:	configure
	./configure

configure:	configure.in aclocal.m4
	autoconf

GDALmake.opt:	GDALmake.opt.in config.status
	./config.status

docs:
	(cd html; rm -f *.*)
	doxygen
	cp data/gdalicon.png html
	cp frmts/*.html frmts/*/frmt_*.html html

all:	default ogr-all

install-docs:	
	$(INSTALL_DIR) $(INST_DOCS)
	cp html/*.* $(INST_DOCS)

web-update:	docs
	rm -rf /u/www/gdal/html
	mkdir /u/www/gdal/html
	cp html/*.* /u/www/gdal/html

install:	lib install-actions

install-actions:
	$(INSTALL_DIR) $(INST_BIN)
	$(INSTALL_DIR) $(INST_DATA)
	$(INSTALL_DIR) $(INST_LIB)
	$(INSTALL_DIR) $(INST_INCLUDE)
	(cd port; $(MAKE) install)
	(cd core; $(MAKE) install)
	(cd frmts; $(MAKE) install)
	(cd alg; $(MAKE) install)
	(cd ogr; $(MAKE) install)
	(cd apps; $(MAKE) install)
ifneq ($(PYTHON),no)
	(cd pymod; $(MAKE) install)
endif
	$(INSTALL) $(GDAL_LIB) $(INST_LIB)
ifeq ($(HAVE_LD_SHARED),yes)
	$(INSTALL) $(GDAL_SLIB) $(INST_LIB)
endif
	for f in data/*.* ; do $(INSTALL) -m 0644 $$f $(INST_DATA) ; done

