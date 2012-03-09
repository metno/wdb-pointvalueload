/*
    wdb - weather and water data storage

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

#include "GribHandleReader.hpp"
#include <grib_api.h>
#include <string>
#include <sstream>
#include <stdexcept>

namespace wdb { namespace load { namespace point {


GribHandleReader::GribHandleReader(grib_handle * gribHandle) :
        gribHandle_(gribHandle)
{
        // NOOP
}

GribHandleReader::~GribHandleReader()
{
        // NOOP
}

long GribHandleReader::getLong( const char * name )
{
        long ret;
        errorCheck( grib_get_long(gribHandle_, name, &ret), name );
        return ret;
}

double GribHandleReader::getDouble( const char * name )
{
        double ret;
        errorCheck( grib_get_double(gribHandle_, name, &ret), name );
        return ret;
}

std::string GribHandleReader::getString( const char * name )
{
        size_t retLn = 20;
        char ret[retLn];
        errorCheck( grib_get_string(gribHandle_, name, &ret[0], &retLn), name );
        std::string retS = &ret[0];
        return retS;
}

double * GribHandleReader::getValues( )
{
        size_t size = getValuesSize( );
        double * ret = new double[ size ];
        errorCheck( grib_get_double_array( gribHandle_, "values", ret, &size ), "values" );
        return ret;
}


size_t GribHandleReader::getValuesSize( )
{
        size_t ret;
        errorCheck( grib_get_size( gribHandle_, "values",  &ret), "size_of_values" );
        return ret;
}

void GribHandleReader::errorCheck( int returnCode, const char * variable )
{
        if (returnCode == 0)
                return;
        std::stringstream errorMessage;
        errorMessage << "Error while decoding the variable "
                                 << variable
                                 << ". GRIB API: "
                                 << grib_get_error_message( -returnCode ); // GRIB api uses the negative value of return code
        throw std::runtime_error( errorMessage.str() );
}

} } } // end namespace
