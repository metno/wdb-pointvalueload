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

// project
#include "Loader.hpp"
#include "FeltLoader.hpp"
#include "GribFile.hpp"
#include "GribLoader.hpp"

// boost
#include <boost/filesystem.hpp>

// std
#include <string>
#include <vector>
#include <iostream>

namespace wdb { namespace load { namespace point {

Loader::Loader(const CmdLine& cmdLine)
    : options_(cmdLine), wdbConnection_(cmdLine, cmdLine.loading().nameSpace)
{
    // NOOP
	std::cerr << __FUNCTION__ << std::endl;
}

Loader::~Loader()
{
    //NOOP
}

void Loader::load()
{
		std::cerr << __FUNCTION__ << std::endl;
    if(options_.input().type.empty()) {
        std::cerr << "Missing input file type"<<std::endl;
        return;
    }

    if(options_.input().type != "felt" and options_.input().type !="grib") {
        std::cerr << "Unrecognized input file type"<<std::endl;
        return;
    }

    if(options_.input().type == "felt") {
        felt_ = boost::shared_ptr<FeltLoader>(new FeltLoader(wdbConnection_, options_));
    } else if(options_.input().type == "grib") {
        grib_ = boost::shared_ptr<GribLoader>(new GribLoader(wdbConnection_, options_));
    }

    std::vector<boost::filesystem::path> files;
    const std::vector<std::string>& file = options_.input().file;
    std::copy(file.begin(), file.end(), back_inserter(files));

    std::cerr << __FUNCTION__ << std::endl;
    for(std::vector<boost::filesystem::path>::const_iterator it = files.begin(); it != files.end(); ++ it)
    {
			std::cerr << __FUNCTION__ <<" "<< __LINE__ << std::endl;
        try {
            if(options_.input().type == "felt") {
                felt::FeltFile feltFile(*it);
                felt_->load(feltFile);
				std::cerr << __FUNCTION__ <<" "<< __LINE__ << std::endl;
            } else if(options_.input().type == "grib") {
                GribFile gribFile(it->native_file_string());
                grib_->load(gribFile);
            }

        } catch (std::exception& e) {
            std::cerr << "Unable to load file " << it->native_file_string();
            std::cerr << "Reason: " << e.what();
        }
    }

//    for ( std::vector<std::string>::const_iterator file = filesToLoad.begin(); file != filesToLoad.end(); ++ file )
//    {
//        logHandler.setObjectName( * file );
//        try {
//            GribFile gribFile(* file);
//            loader.load(gribFile);
//        } catch ( std::exception & e) {
//            log.errorStream()
//                    << "Unrecoverable error when reading file " << * file << ". "<< e.what();
//        }
//    }
}

} } } // end namespaces
