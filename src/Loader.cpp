/*
 wdb

 Copyright (C) 2012 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 E-mail: wdb@met.no

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 MA  02110-1301, USA
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// project
#include "Loader.hpp"
#include "FeltLoader.hpp"
#include "GribFile.hpp"
#include "GribLoader.hpp"
#include "NetCDFLoader.hpp"

// libfimex
#include <fimex/CDM.h>
#include <fimex/CDMReader.h>
#include <fimex/CDMExtractor.h>
#include <fimex/CDMException.h>
#include <fimex/CDMconstants.h>
#include <fimex/CDMDimension.h>
#include <fimex/CDMReaderUtils.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/CDMFileReaderFactory.h>

// wdb
#include <wdbLogHandler.h>

// boost
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

// std
#include <string>
#include <vector>
#include <iostream>

using namespace std;

namespace wdb { namespace load { namespace point {

    Loader::Loader(const CmdLine& cmdLine)
        : options_(cmdLine)
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.Loader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        // check interpolation method
        interpolateMethod_ = MIFI_INTERPOL_BILINEAR;

        if (options().loading().fimexInterpolateMethod == "bilinear") {
            interpolateMethod_ = MIFI_INTERPOL_BILINEAR;
        } else if (options().loading().fimexInterpolateMethod == "nearestneighbor") {
            interpolateMethod_ = MIFI_INTERPOL_NEAREST_NEIGHBOR;
        } else if (options().loading().fimexInterpolateMethod == "bicubic") {
            interpolateMethod_ = MIFI_INTERPOL_BICUBIC;
        } else if (options().loading().fimexInterpolateMethod == "coord_nearestneighbor") {
            interpolateMethod_ = MIFI_INTERPOL_COORD_NN;
        } else if (options().loading().fimexInterpolateMethod == "coord_kdtree") {
            interpolateMethod_ = MIFI_INTERPOL_COORD_NN_KD;
        } else if (options().loading().fimexInterpolateMethod == "forward_sum") {
            interpolateMethod_ = MIFI_INTERPOL_FORWARD_SUM;
        } else if (options().loading().fimexInterpolateMethod == "forward_mean") {
            interpolateMethod_ = MIFI_INTERPOL_FORWARD_MEAN;
        } else if (options().loading().fimexInterpolateMethod == "forward_median") {
            interpolateMethod_ = MIFI_INTERPOL_FORWARD_MEDIAN;
        } else if (options().loading().fimexInterpolateMethod == "forward_max") {
            interpolateMethod_ = MIFI_INTERPOL_FORWARD_MAX;
        } else if (options().loading().fimexInterpolateMethod == "forward_min") {
            interpolateMethod_ = MIFI_INTERPOL_FORWARD_MIN;
        } else {
            log.errorStream() << __FUNCTION__<< " @ line["<< __LINE__ << "] " << "WARNING: unknown interpolate.method: " << options().loading().fimexInterpolateMethod << " using bilinear";
        }

        if(!options().output().outFileName.empty()) {
            output_.open(options().output().outFileName);
        }
    }

    Loader::~Loader()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.Loader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        if(output_.is_open()) {
            output_.close();
            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        }
    }

    void Loader::load()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.Loader" );
        log.debugStream() << __FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        if(options_.input().type.empty()) {
            log.errorStream() << " @ line["<< __LINE__ << "] " << "Missing input file type";
            return;
        }

        if(options_.input().type != "felt" and options_.input().type !="grib1" and options_.input().type !="grib2" and options_.input().type !="netcdf") {
            log.errorStream() << " @ line[" << __LINE__ << "] " << "Unrecognized input file type";
            return;
        }

        if(options_.input().type == "felt") {
            felt_ = boost::shared_ptr<FeltLoader>(new FeltLoader(*this));
        } else if(options_.input().type == "grib1" or options_.input().type == "grib2") {
            grib_ = boost::shared_ptr<GribLoader>(new GribLoader(*this));
        } else if(options_.input().type == "netcdf") {
            netcdf_ = boost::shared_ptr<NetCDFLoader>(new NetCDFLoader(*this));
        } else {
            log.errorStream() << " @ line["<< __LINE__ << "] " << "Unrecognized input file type";
            return;
        }

        vector<boost::filesystem::path> files;
        vector<string> names;

        boost::split(names, options().input().file[0], boost::is_any_of(" ,"));

        copy(names.begin(), names.end(), back_inserter(files));

        std::string tmplFileName = options().loading().fimexTemplate;
        openTemplateCDM(tmplFileName);

        for(std::vector<boost::filesystem::path>::const_iterator it = files.begin(); it != files.end(); ++ it)
        {
            try {
                if(options_.input().type == "felt") {
                    felt_->load(it->string());
                } else if(options_.input().type == "grib1" or options_.input().type == "grib2") {
                    grib_->load(it->string());
                } else if(options_.input().type == "netcdf") {
                    netcdf_->load(it->string());
                }
            } catch (MetNoFimex::CDMException& e) {
                log.errorStream() << " @ line["<< __LINE__ << "]" << "Unable to load file " << it->string();
                log.errorStream() << " @ line["<< __LINE__ << "]"  << "Reason: " << e.what();
                throw e;
            } catch (std::exception& e) {
                log.errorStream() << " @ line["<< __LINE__ << "]" << "Unable to load file " << it->string();
                log.errorStream() << " @ line["<< __LINE__ << "]"  << "Reason: " << e.what();
                throw e;
            }
        }
    }

    bool Loader::openTemplateCDM(const std::string& fileName)
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.Loader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        if(fileName.empty())
            throw std::runtime_error(" Can't open template interpolation file! ");

        if(!boost::filesystem::exists(fileName))
                    throw std::runtime_error(" Template file: " + fileName + " doesn't exist!");

            cdmTemplate_ = MetNoFimex::CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, fileName);

            assert(extractPointIds());

//        cdmTemplate_->getCDM().toXMLStream(std::cerr);
            return true;
    }

    bool Loader::extractPointIds()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.Loader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        if(not cdmTemplate_.get())
            return false;

        const MetNoFimex::CDM& cdmRef = cdmTemplate_->getCDM();
        const std::string latVarName("latitude");
        const std::string lonVarName("longitude");
        assert(cdmRef.hasVariable(latVarName));
        assert(cdmRef.hasVariable(lonVarName));

        boost::shared_array<float> lats_ = cdmTemplate_->getData(latVarName)->asFloat();
        float* fsIt = &lats_[0];
        float* feIt = &lats_[cdmTemplate_->getData(latVarName)->size()];
        for(; fsIt!=feIt; ++fsIt)
            latitudes_.push_back(*fsIt);

        boost::shared_array<float> lons_ = cdmTemplate_->getData(lonVarName)->asFloat();
        fsIt = &lons_[0];
        feIt = &lons_[cdmTemplate_->getData(lonVarName)->size()];
        for(; fsIt!=feIt; ++fsIt)
            longitudes_.push_back(*fsIt);

        return true;
    }

    void Loader::write(const string &str)
    {
//        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.Loader" );
//        log.debugStream() << __FUNCTION__ << " @ line["<< __LINE__ << "] CHECK POINT ";
        if(output_.is_open()) {
            output_ << str;
            flush(output_);
        } else {
            cout << str;
        }
    }

} } } // end namespaces
