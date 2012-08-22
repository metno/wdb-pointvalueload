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

// std
#include <stdexcept>

using namespace std;

const string etcDir = "./etc/";
const string commonDir = etcDir + "/common/";
const string stationsConf = commonDir + "testStations.nc";
const string unitsConf = commonDir + "units.conf";
const string dataproviderConf = commonDir + "dataprovider.conf";
const string feltDir = etcDir + "/felt/";
const string grib1Dir = etcDir + "/grib1/";
const string grib2Dir = etcDir + "/grib2/";
const string netcdfDir = etcDir + "/netcdf/";
const string feltConf = feltDir + "/load.conf";
const string grib1Conf = grib1Dir + "/load.conf";
const string grib2Conf = grib2Dir + "/load.conf";
const string netcdfConf = netcdfDir + "/load.conf";

BOOST_AUTO_TEST_CASE( parseCmdLine1 )
{
  const int argc = 3;
  char *argv[argc];
  argv[0]= strdup("wdb-fastload");
  argv[1]= strdup("--units.config");
  argv[2]= strdup("etc/units.cfg");
  wdb::load::point::CmdLine cmdLine;
  cmdLine.parse( argc, argv );
  
  BOOST_CHECK_EQUAL("etc/units.cfg", cmdLine.loading().unitsConfig);

  free(argv[0]);
  free(argv[1]);
  free(argv[2]);
}

BOOST_AUTO_TEST_CASE( parseCmdLine2 )
{
  int argc = 3;
  char *argv[argc];
  argv[0] = strdup("wdb-fastload");
  argv[1] = strdup("--stations");
  argv[2] = strdup("1002") ;
  wdb::load::point::CmdLine cmdLine;

  BOOST_CHECK_THROW( cmdLine.parse( argc, argv ), std::logic_error );

  free(argv[0]);
  free(argv[1]);
  free(argv[2]);
}

BOOST_AUTO_TEST_CASE( configFilesExist )
{
    BOOST_REQUIRE(boost::filesystem::exists( etcDir ));
    BOOST_REQUIRE(boost::filesystem::exists( commonDir ));
    BOOST_REQUIRE(boost::filesystem::exists( stationsConf ));
    BOOST_REQUIRE(boost::filesystem::exists( unitsConf ));
    //BOOST_REQUIRE(boost::filesystem::exists( dataproviderConf ));
    BOOST_REQUIRE(boost::filesystem::exists( feltDir ));
    BOOST_REQUIRE(boost::filesystem::exists( netcdfDir ));

    BOOST_REQUIRE(boost::filesystem::exists( feltConf ));
}

BOOST_AUTO_TEST_CASE( loadfelt )
{
    int argc = 5;
    char *argv[argc];
    argv[0] = strdup("wdb-fastload");
    argv[1] = strdup("--config");
    argv[2] = strdup(feltConf.c_str());
    argv[3] = strdup("");//strdup("--output");
    argv[4] = strdup("");//("felt-result.txt");

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    loader.load();

    free(argv[0]);
    free(argv[1]);
    free(argv[2]);
    free(argv[3]);
    free(argv[4]);
}

BOOST_AUTO_TEST_CASE( loadnetcdf )
{
    int argc = 5;
    char *argv[argc];
    argv[0] = strdup("wdb-fastload");
    argv[1] = strdup("--config");
    argv[2] = strdup(netcdfConf.c_str());
    argv[3] = strdup("");//strdup("--output");
    argv[4] = strdup("");//("felt-result.txt");

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    loader.load();

    free(argv[0]);
    free(argv[1]);
    free(argv[2]);
    free(argv[3]);
    free(argv[4]);
}

BOOST_AUTO_TEST_CASE( loadgrib1 )
{
    int argc = 5;
    char *argv[argc];
    argv[0] = strdup("wdb-fastload");
    argv[1] = strdup("--config");
    argv[2] = strdup(grib1Conf.c_str());
    argv[3] = strdup("");//strdup("--output");
    argv[4] = strdup("");//("felt-result.txt");

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    loader.load();

    free(argv[0]);
    free(argv[1]);
    free(argv[2]);
    free(argv[3]);
    free(argv[4]);
}

BOOST_AUTO_TEST_CASE( loadgrib2 )
{
    int argc = 5;
    char *argv[argc];
    argv[0] = strdup("wdb-fastload");
    argv[1] = strdup("--config");
    argv[2] = strdup(grib2Conf.c_str());
    argv[3] = strdup("");//strdup("--output");
    argv[4] = strdup("");//("felt-result.txt");

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    loader.load();

    free(argv[0]);
    free(argv[1]);
    free(argv[2]);
    free(argv[3]);
    free(argv[4]);
}
