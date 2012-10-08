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


// fimex
#include <fimex/CDMException.h>

// boost
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/iostreams/filtering_stream.hpp>

// std
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

using namespace std;

#define compareFiles(name1, name2) compareFiles_(name1, name2, __func__, __LINE__)

bool compareFiles_(const string & name1, const string & name2, const std::string & function, int callLine)
{
    BOOST_REQUIRE_MESSAGE(boost::filesystem::exists(name1), function + "(" + boost::lexical_cast<std::string>(callLine) + "): " + name1 + " does not exist");
    BOOST_REQUIRE_MESSAGE(boost::filesystem::exists(name2), function + "(" + boost::lexical_cast<std::string>(callLine) + "): " + name2 + " does not exist");

    ifstream file1,file2;
    file1.open(name1.c_str(), ios::in | ios::binary);
    file2.open(name2.c_str(), ios::in | ios::binary);
    //---------- compare number of lines in both files ------------------
    size_t s1 = 0;
    size_t s2 = 0;

    string line;
    while(!file1.eof())
    {
        getline(file1, line);
        s1++;
    }
    while(!file2.eof())
    {
        getline(file2, line);
        s2++;
    }
    if(s1 != s2)
    {
        cout << "Different number of lines in files!" << "\n";
        cout << "file1 has " << s1 << " lines and file2 has ";
        cout << s2 << " lines" << "\n";
        return false;
    }

    file1.open(name1.c_str(), ios::in | ios::binary);
    file2.open(name2.c_str(), ios::in | ios::binary);
    //---------- compare two files line by line ------------------
    char string1[256], string2[256];
    int j = 0;
    while(!file1.eof())
    {
        file1.getline(string1,256);
        file2.getline(string2,256);
        j++;
        if(strcmp(string1,string2) != 0)
        {
            cout << j << "-th strings are not equal" << "\n";
            cout << "   " << string1 << "\n";
            cout << "   " << string2 << "\n";
            return false;
        }
    }

    return true;
}

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
    int argc = makeArgv("pointload --units.config "SRCDIR"/etc/units.cfg", argv);
  
    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );
    BOOST_CHECK_EQUAL(SRCDIR"/etc/units.cfg", cmdLine.loading().unitsConfig);

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
    BOOST_REQUIRE(boost::filesystem::exists( SRCDIR"/etc/felt/load.conf" ));
    BOOST_REQUIRE(boost::filesystem::exists( SRCDIR"/etc/netcdf/load.conf" ));
    BOOST_REQUIRE(boost::filesystem::exists( SRCDIR"/etc/grib1/load.conf" ));
    BOOST_REQUIRE(boost::filesystem::exists( SRCDIR"/etc/grib2/load.conf" ));
}

BOOST_AUTO_TEST_CASE( loadfelt )
{
    char **argv = 0;
    int argc = makeArgv("pointload --config "SRCDIR"/etc/felt/load.conf --output result.txt --name="SRCDIR"/etc/felt/data.dat", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    {
        // destructor will call close on output file
        wdb::load::point::Loader loader(cmdLine);
        loader.load();
    }

    BOOST_REQUIRE(compareFiles(SRCDIR"/etc/felt/expected.txt", "result.txt"));

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadfelt_missing )
{
    char **argv = 0;
    int argc = makeArgv("pointload --config "SRCDIR"/etc/felt/load.conf --name missing.nc", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    BOOST_REQUIRE_THROW(loader.load(), std::exception);


    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadfelt_empty )
{
    char **argv = 0;
    int argc = makeArgv("wdb-pointload --config "SRCDIR"/etc/felt/load.conf --name etc/common/empty.file", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    BOOST_REQUIRE_THROW(loader.load(), std::exception);


    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadnetcdf )
{
    char **argv = 0;
    int argc = makeArgv("wdb-pointload --config "SRCDIR"/etc/netcdf/load.conf --output result.txt --name="SRCDIR"/etc/netcdf/data.nc", argv);

    wdb::load::point::CmdLine cmdLine;
        cmdLine.parse( argc, argv );

    {
        // destructor will call close on output file
        wdb::load::point::Loader loader(cmdLine);
        loader.load();
    }

    BOOST_REQUIRE(compareFiles(SRCDIR"/etc/netcdf/expected.txt", "result.txt"));

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadnetcdf_missing )
{
    char **argv = 0;
    int argc = makeArgv("pointload --config "SRCDIR"/etc/netcdf/load.conf --name missing.nc", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    BOOST_REQUIRE_THROW(loader.load(), std::exception);

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadnetcdf_empty )
{
    char **argv = 0;
    int argc = makeArgv("pointload --config "SRCDIR"/etc/netcdf/load.conf --name etc/common/empty.file", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    BOOST_REQUIRE_THROW(loader.load(), MetNoFimex::CDMException);

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadgrib1 )
{
    char **argv = 0;
    int argc = makeArgv("pointLoad --config "SRCDIR"/etc/grib1/load.conf --output result.txt --name="SRCDIR"/etc/grib1/data.grib", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    {
        wdb::load::point::Loader loader(cmdLine);
        loader.load();
    }

    BOOST_REQUIRE(compareFiles(SRCDIR"/etc/grib1/expected.txt", "result.txt"));

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadgrib1_missing )
{
    char **argv = 0;
    int argc = makeArgv("pointload --config "SRCDIR"/etc/grib1/load.conf --name missing.nc", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    BOOST_REQUIRE_THROW(loader.load(), std::exception);

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadgrib1_empty )
{
    char **argv = 0;
    int argc = makeArgv("pointload --config "SRCDIR"/etc/grib1/load.conf --name "SRCDIR"/etc/common/empty.file", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    BOOST_REQUIRE_THROW(loader.load(), MetNoFimex::CDMException);

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadgrib2 )
{
//    char **argv = 0;
//    int argc = makeArgv("pointLoad --config "SRCDIR"/etc/grib2/load.conf --output result.txt --name="SRCDIR"/etc/grib2/data.grib", argv);
//
//    wdb::load::point::CmdLine cmdLine;
//    cmdLine.parse(argc, argv);
//
//    {
//        wdb::load::point::Loader loader(cmdLine);
//        loader.load();
//    }
//
//    BOOST_REQUIRE(compareFiles(SRCDIR"/etc/grib2/expected.txt", "result.txt"));
//
//    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadgrib2_missing )
{
    char **argv = 0;
    int argc = makeArgv("pointload --config "SRCDIR"/etc/grib2/load.conf --name missing.nc", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    BOOST_REQUIRE_THROW(loader.load(), std::exception);

    delete [] argv;
}

BOOST_AUTO_TEST_CASE( loadgrib2_empty )
{
    char **argv = 0;
    int argc = makeArgv("pointload --config "SRCDIR"/etc/grib2/load.conf --name="SRCDIR"/etc/common/empty.file", argv);

    wdb::load::point::CmdLine cmdLine;
    cmdLine.parse( argc, argv );

    wdb::load::point::Loader loader(cmdLine);
    BOOST_REQUIRE_THROW(loader.load(), MetNoFimex::CDMException);

    delete [] argv;
}
