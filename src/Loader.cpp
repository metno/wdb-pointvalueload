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
#include "FileLoader.hpp"

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
#include <boost/assign/list_of.hpp>

// std
#include <string>
#include <vector>
#include <iostream>

using namespace std;

namespace wdb { namespace load { namespace point {

namespace
{
const std::map<std::string, int> interpolationNames = boost::assign::map_list_of
		("bilinear", MIFI_INTERPOL_BILINEAR)
		("nearestneighbor", MIFI_INTERPOL_NEAREST_NEIGHBOR)
		("bicubic",MIFI_INTERPOL_BICUBIC)
		("coord_nearestneighbor", MIFI_INTERPOL_COORD_NN)
		("coord_kdtree", MIFI_INTERPOL_COORD_NN_KD)
		("forward_sum", MIFI_INTERPOL_FORWARD_SUM)
		("forward_mean", MIFI_INTERPOL_FORWARD_MEAN)
		("forward_median", MIFI_INTERPOL_FORWARD_MEDIAN)
		("forward_max", MIFI_INTERPOL_FORWARD_MAX)
		("forward_min", MIFI_INTERPOL_FORWARD_MIN);
}

    Loader::Loader(const CmdLine& cmdLine) : options_(cmdLine)
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.Loader" );

        // find wanted interpolation method
        std::map<std::string, int>::const_iterator find = interpolationNames.find(options().loading().fimexInterpolateMethod);
        if ( find == interpolationNames.end() )
        	throw std::runtime_error("Unknown interpolate.method: " + options().loading().fimexInterpolateMethod);
        interpolateMethod_ = find->second;

        if ( !options().output().outFileName.empty() ) {
            output_.open(options().output().outFileName);
        }
    }

    Loader::~Loader()
    {
        if(output_.is_open()) {
            output_.close();
        }
    }

//    The first method called by the main function.
//    Template file (holds stations information) is opened using CDMreader.
//    The actual loader object "floader_" (based on the input file type) is created.
//    For each file in the input list the floader_.load(...) is executed.
    void Loader::load()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.Loader" );

        if(options_.input().type.empty()) throw runtime_error("Missing input file type");

        floader_ = boost::shared_ptr<FileLoader>(FileLoaderFactory::createFileLoader(options_.input().type, *this));

        std::string tmplFileName = options().loading().fimexTemplate;
        openTemplateCDM(tmplFileName);

        vector<string> filenames;
        boost::split(filenames, options().input().file[0], boost::is_any_of(","));

        for(size_t i = 0; i < filenames.size(); ++i)
        {
            string gridded = filenames[i];
            boost::trim(gridded);
            if(gridded.empty()) {
                log.debugStream() << "Skipping to load file with the empty name";
                continue;
            }
            try {
                floader_->load(gridded);
            } catch (MetNoFimex::CDMException& e) {
                log.errorStream() << "Unable to load file [" << gridded << "]";
                throw e;
            } catch (std::exception& e) {
                log.errorStream() << " @ line["<< __LINE__ << "]" << "Unable to load file " << gridded;
                throw e;
            }
        }
    }

//    We are using fimex and the process of template interpolation to extract point related data.
//    One must supply the list of lat/lon values presenting the geographical positions of the points.
//    The file must be in the netcdf format.
//    Checks if such template file exists and create corresponding CDMReader object.
    bool Loader::openTemplateCDM(const std::string& fileName)
    {
        if(fileName.empty()) {
            stringstream ss;
            ss << " Can't open template interpolation file: " << fileName;
            throw std::runtime_error(ss.str());
        }

        if(!boost::filesystem::exists(fileName))  {
            stringstream ss;
            ss << " Template file: " << fileName << " doesn't exist!";
            throw std::runtime_error(ss.str());
        }

        cdmTemplate_ = MetNoFimex::CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, fileName);

        if(!extractPointIds()) {
            stringstream ss;
            ss << " Can't extract data from : " << fileName << " interpolation template!";
            throw std::runtime_error(ss.str());
        }

        return true;
    }

    // Extracting lat/long positions from the template file.
    // used when generating data lines for each point.
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

    // Writes either to standard output or to a file-
    void Loader::write(const string &str)
    {
        if(output_.is_open()) {
            output_ << str;
            flush(output_);
        } else {
            cout << str;
        }
    }

} } } // end namespaces
