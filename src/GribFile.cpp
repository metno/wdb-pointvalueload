/*
 gribPointLoad

 Copyright (C) 2012 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 E-mail: post@met.no

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
#include "GribFile.hpp"
#include "GribField.hpp"

// grib
#include <grib_api.h>

// std
#include <sstream>

namespace wdb { namespace load { namespace point {

    GribFile::GribFile(const std::string & fileName)
        : fileName_(fileName), gribFile_(0)
    {
        std::clog<< "Attempting to open GRIB file " << fileName_ << std::endl;

        grib_multi_support_on(0);

        gribFile_ = fopen( fileName_.c_str(), "r" );

        if ( ! gribFile_ ) {
            std::ostringstream errorMessage;
            errorMessage << "Could not open the GRIB file " << fileName_;
            std::cerr << errorMessage.str() << std::endl;
            throw std::runtime_error( errorMessage.str() );
        }

        std::clog << "Opened GRIB file successfully" << std::endl;
    }

    GribFile::~GribFile()
    {
        if ( gribFile_ )
            fclose(gribFile_);
    }

    GribFile::Field GribFile::next()
    {
        grib_handle *  gribHandle = getNextHandle_();

        if (gribHandle == 0)
            return Field();

        Field ret( new GribField( gribHandle ));

        return ret;
    }

    grib_handle * GribFile::getNextHandle_()
    {
        int errorCode = 0;
        grib_handle * gribHandle = grib_handle_new_from_file( 0, gribFile_, & errorCode );

        GRIB_CHECK( errorCode, 0 );

        return gribHandle;
    }

} } } // end namespace
