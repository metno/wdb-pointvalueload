/*
 pointload 

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

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FastloadTest
#include <boost/test/unit_test.hpp>

#include <CmdLine.hpp>
#include <Loader.hpp>

// boost
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
// std
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

int makeArgv(const string& line, char** &argv)
{
    vector<string> args;
    boost::split(args, line, boost::is_any_of(" "));
    argv = new char*[args.size() + 1];
    for(size_t i = 0; i < args.size(); ++i)
    {
        boost::trim(args[i]);
        argv[i] = new char[args[i].size() + 1];
        strncpy(argv[i], args[i].c_str(), args[i].size());
        argv[i][args[i].size()] = 0;
    }
    argv[args.size()] = 0;
    return args.size();
}

BOOST_AUTO_TEST_CASE( parseCmdLine1 )
{
    char **argv = 0;
    int argc = makeArgv("pointload --units.config etc/units.cfg", argv);
  
    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );
    BOOST_CHECK_EQUAL("etc/units.cfg", cmdLine.loading().unitsConfig);

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( parseCmdLine2 )
{
    char **argv = 0;
    int argc = makeArgv("pointload --stations 1002", argv);

    wdb::load::point::CmdLine cmdLine;
    BOOST_CHECK_THROW( cmdLine.parse( argc, argv ), std::logic_error );
}

BOOST_AUTO_TEST_CASE( configFilesExist )
{
    BOOST_REQUIRE(boost::filesystem::exists( "etc/felt/load.conf" ));
    BOOST_REQUIRE(boost::filesystem::exists( "etc/netcdf/load.conf" ));
    BOOST_REQUIRE(boost::filesystem::exists( "etc/grib1/load.conf" ));
    BOOST_REQUIRE(boost::filesystem::exists( "etc/grib2/load.conf" ));
}

BOOST_AUTO_TEST_CASE( loadfelt )
{
    char **argv = 0;
    int argc = makeArgv("pointload --config etc/felt/load.conf", argv);


    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    loader.load();

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadnetcdf )
{
    char **argv = 0;
    int argc = makeArgv("pointLoad --config etc/netcdf/load.conf", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    loader.load();

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadgrib1 )
{
    char **argv = 0;
    int argc = makeArgv("pointLoad --config etc/grib1/load.conf", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    loader.load();

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadgrib2 )
{
    char **argv = 0;
    int argc = makeArgv("pointLoad --config etc/grib2/load.conf", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse(argc, argv);

    wdb::load::point::Loader loader(cmdLine);
    loader.load();

    delete [] argv;

}
