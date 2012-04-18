/*
 feltLoad

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
#include "CmdLine.hpp"

// std
#include <fstream>

using namespace std;
using namespace boost::program_options;

namespace
{

    options_description getInput(wdb::load::point::CmdLine::InputOptions & out)
    {
        options_description input("Input");
        input.add_options()
        ( "type", value( & out.type ), "File type to be loaded [felt/grib]" )
        ( "name", value<vector<string> >(&out.file)->multitoken(), "Name of file to process" )
        ;

        return input;
    }

    options_description getOutput(wdb::load::point::CmdLine::OutputOptions & out)
    {
        options_description output( "Output" );
        output.add_options()
        ( "dry-run", bool_switch(& out.dry_run), "List SQL commands instead of inserting into database" )
        ;

        return output;
    }

    options_description getLoading(wdb::load::point::CmdLine::LoadingOptions & out )
    {
        options_description input("Point Loading");
        input.add_options()
        ( "placenamespaceid", value(& out.nameSpace), "Specify a non-default namespace. ")
        ( "dataprovidername", value(& out.dataProviderName), "Specify a non-default namespace. ")
        ( "validtime.config", value(& out.validtimeConfig), "Specify path to validtime configuration file")
        ( "dataprovider.config", value(& out.dataproviderConfig), "Specify path to dataprovider configuration file")
        ( "valueparameter.config", value(& out.valueparameterConfig), "Specify path to valueparameter [FELT/GRIB1] configuration file")
        ( "levelparameter.config", value(& out.levelparameterConfig), "Specify path to levelparameter [FELT/GRIB1] configuration file")
        ( "leveladditions.config", value(& out.leveladditionsConfig), "Specify path to leveladditiond [FELT/GRIB1] configuration file")
        ( "valueparameter2.config", value(& out.valueparameter2Config), "Specify path to valueparameter [GRIB2] configuration file")
        ( "levelparameter2.config", value(& out.levelparameter2Config), "Specify path to levelparameter [GRIB2] configuration file")
        ( "leveladditions2.config", value(& out.leveladditions2Config), "Specify path to leveladditiond [GRIB2] configuration file")
        ( "fimex.config", value(& out.fimexConfig), "Path to fimex reader configuration file" )
        ( "fimex.process.rotateVectorToLatLonX", value(&out.fimexProcessRotateVectorToLatLonX), "Rotate X wind component to lat/lon" )
        ( "fimex.process.rotateVectorToLatLonY", value(&out.fimexProcessRotateVectorToLatLonY), "Rotate Y wind component to lat/lon" )
        ( "fimex.interpolate.template", value(& out.fimexTemplate), "Path to template file tha fimex reader will use for point interpolation" )
        ( "fimex.interpolate.method", value(& out.fimexInterpolateMethod), "Interpolation method [nearestneighbor, bilinear, bicubic, coord_nearestneighbor, coord_kdtree, forward_max, forward_mean, forward_median or forward_sum]" )
        ( "stations", value(& out.stations), "station ids to be selected from template (between qoutes '18700 1456') " )
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

    CmdLine::CmdLine() : WdbConfiguration("")
    {

        cmdOptions().add(getInput(input_));
        cmdOptions().add(getOutput(output_));
        cmdOptions().add(getLoading(loading_));

        configOptions().add(getInput(input_));
        configOptions().add(getOutput(output_));
        configOptions().add(getLoading(loading_));

        shownOptions().add(getInput(input_));
        shownOptions().add(getOutput(output_));
        shownOptions().add(getLoading(loading_));
    }

    CmdLine::~CmdLine()
    {
        // NOOP
    }

} } }




