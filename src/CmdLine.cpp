/*
 feltLoad

 Copyright (C) 2009 met.no

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

#include "CmdLine.h"

namespace
{
using namespace boost::program_options;

options_description getLoading(wdb::load::point::CmdLine::LoadingOptions & out )
{
    options_description input("Point Loading");
    input.add_options()
    ( "dry-run", bool_switch( & out.dryRun ), "no database insertions done)" )
    ( "loaderconfig", value<std::string>( & out.mainCfgFileName ), "main config file (points to other config files needed by loader)" )
    ( "input.type", value<std::string>( & out.inputFileType ), "file type to be loaded -- felt,grib)" )
    ( "referenceTime,t", value<std::string>( & out.referenceTime ), "Store data into database using the given reference time, instead of whatever the given document(s) say" )
    ( "fimex.config", value<std::string>( & out.fimexReaderConfig), "Path to fimex reader configuration file" )
    ( "fimex.interpolate.template", value<std::string>( & out.fimexReaderTemplate), "Path to template file tha fimex reader will use for point interpolation" )
//    ( "fimex.reduceToBoundingBox.south", value<std::string>( & out.fimexReduceSouth), "geographical bounding-box in degree" )
//    ( "fimex.reduceToBoundingBox.north", value<std::string>( & out.fimexReduceNorth), "geographical bounding-box in degree" )
//    ( "fimex.reduceToBoundingBox.east", value<std::string>( & out.fimexReduceEast), "geographical bounding-box in degree" )
//    ( "fimex.reduceToBoundingBox.west", value<std::string>( & out.fimexReduceWest), "geographical bounding-box in degree" )
    ;


    // norway
    // --fimex.reduceToBoundingBox.south 55 --fimex.reduceToBoundingBox.north 80 --fimex.reduceToBoundingBox.west 5 --fimex.reduceToBoundingBox.east 30

	return input;
}
}

namespace wdb { namespace load { namespace point {

    CmdLine::CmdLine() : wdb::load::LoaderConfiguration("")
    {
        cmdOptions().add(getLoading(loading_));
        configOptions().add(getLoading(loading_));
        shownOptions().add(getLoading(loading_));
    }

    CmdLine::~CmdLine()
    {
        // NOOP
    }

} } }
