dist_noinst_DATA = \
	etc/felt/data.dat \
	etc/felt/stations.nc \
	etc/felt/expected.txt \
	etc/grib1/data.grib \
	etc/grib1/stations.nc \
	etc/grib1/expected.txt \
	etc/grib2/data.grib \
	etc/grib2/stations.nc \
	etc/netcdf/data.nc \
	etc/netcdf/stations.nc \
	etc/netcdf/expected.txt \
	etc/common/empty.file

confdir        = $(datadir)/wdb-pointLoad
commondir = $(confdir)/common
dist_common_DATA = etc/common/units.conf
	
feltdir = $(confdir)/felt
dist_felt_DATA = \
                                           etc/felt/dataprovider.conf \
                                           etc/felt/fimexreader.xml \
                                           etc/felt/leveladditions.conf \
                                           etc/felt/levelparameter.conf \
                                           etc/felt/load.conf \
                                           etc/felt/validtime.conf \
                                           etc/felt/valueparameter.conf
                                           

grib1dir = $(confdir)/grib1
dist_grib1_DATA = \
                                           etc/grib1/dataprovider.conf \
                                           etc/grib1/gribreader.xml \
                                           etc/grib1/leveladditions1.conf \
                                           etc/grib1/levelparameter1.conf \
                                           etc/grib1/load.conf \
                                           etc/grib1/valueparameter1.conf
grib2dir = $(confdir)/grib2
dist_grib2_DATA = \
                                           etc/grib2/dataprovider.conf \
                                           etc/grib2/gribreader.xml \
                                           etc/grib2/leveladditions2.conf \
                                           etc/grib2/levelparameter2.conf \
                                           etc/grib2/load.conf \
                                           etc/grib2/valueparameter2.conf

netcdfdir = $(confdir)/netcdf
dist_netcdf_DATA = \
                                           etc/netcdf/data.nc \
                                           etc/netcdf/leveladditions.conf \
                                           etc/netcdf/levelparameter.conf \
                                           etc/netcdf/load.conf \
                                           etc/netcdf/valueparameter.conf

CLEANFILES += \
           result.txt
