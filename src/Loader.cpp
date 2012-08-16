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

// boost
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
    : options_(cmdLine),
      northBound_(90.0), southBound_(-90.0), westBound_(-180.0), eastBound_(180.0)
{
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
        std::cerr << "WARNING: unknown interpolate.method: " << options().loading().fimexInterpolateMethod << " using bilinear" << std::endl;
    }

    if(!options().output().outFileName.empty()) {
        output_.open(options().output().outFileName);
    }
}

Loader::~Loader()
{
    if(output_.is_open())
        output_.close();
}

void Loader::load()
{
    if(options_.input().type.empty()) {
        std::cerr << "Missing input file type"<<std::endl;
        return;
    }

    if(options_.input().type != "felt" and options_.input().type !="grib" and options_.input().type !="netcdf") {
        std::cerr << "Unrecognized input file type"<<std::endl;
        return;
    }

    if(options_.input().type == "felt") {
        felt_ = boost::shared_ptr<FeltLoader>(new FeltLoader(*this));
    } else if(options_.input().type == "grib") {
        grib_ = boost::shared_ptr<GribLoader>(new GribLoader(*this));
    } else if(options_.input().type == "netcdf") {
        netcdf_ = boost::shared_ptr<NetCDFLoader>(new NetCDFLoader(*this));
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
                felt_->load(it->native_file_string());
            } else if(options_.input().type == "grib") {
                grib_->load(it->native_file_string());
            } else if(options_.input().type == "netcdf") {
                netcdf_->load(it->native_file_string());
            }
        } catch (std::exception& e) {
            std::cerr << "Unable to load file " << it->native_file_string();
            std::cerr << "Reason: " << e.what();
        }
    }
}

    bool Loader::openTemplateCDM(const std::string& fileName)
    {
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

    bool Loader::extractBounds()
    {
        if(not cdmTemplate_.get())
            return false;

        const MetNoFimex::CDM& cdmRef = cdmTemplate_->getCDM();

        assert(cdmRef.hasVariable("latitude"));
        assert(cdmRef.hasVariable("longitude"));

        boost::shared_array<double> lats = cdmTemplate_->getData("latitude")->asDouble();
        northBound_ = *(std::max_element(&lats[0], &lats[cdmTemplate_->getData("latitude")->size()]));
        southBound_ = *(std::min_element(&lats[0], &lats[cdmTemplate_->getData("latitude")->size()]));

        boost::shared_array<double> lons = cdmTemplate_->getData("longitude")->asDouble();
        eastBound_ = *(std::max_element(&lons[0], &lons[cdmTemplate_->getData("longitude")->size()]));
        westBound_ = *(std::min_element(&lons[0], &lons[cdmTemplate_->getData("longitude")->size()]));

        return true;
    }

    void Loader::write(const string &str)
    {
        if(output_.is_open())
            output_ << str;
        else
            cout << str;
    }

} } } // end namespaces
