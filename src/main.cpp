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
#include "CmdLine.hpp"

// libfelt
#include <felt/FeltFile.h>

// libfimex
#include <fimex/CDM.h>
#include <fimex/CDMReader.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/NetCDF_CDMWriter.h>
#include <fimex/CDMFileReaderFactory.h>

// wdb
#include <wdbLogHandler.h>
#include <wdb/LoaderConfiguration.h>

// boost
#include <boost/filesystem.hpp>

// std
#include <iostream>
#include <map>
#include <vector>

using namespace std;
using namespace wdb;
using namespace wdb::load;

//// Support Functions
namespace
{

    /**
      * Write the program version to stream
      * @param	out		Stream to write to
      */
    void version( ostream & out )
    {
        out << PACKAGE_STRING << endl;
    }

    /**
      * Write help information to stram
      * @param	options		Description of the program options
      * @param	out			Stream to write to
      */
    void help( const boost::program_options::options_description & options, ostream & out )
    {
        version( out );
        out << '\n';
        out << "Usage: PACKAGE_NAME [OPTIONS] FILE...\n\n";
        out << "Options:\n";
        out << options << endl;
    }
} // namespace

int main(int argc, char ** argv)
{
    wdb::load::point::CmdLine cmdLine;
    try {
        cmdLine.parse( argc, argv );

        if(cmdLine.general().help) {
            help(cmdLine.shownOptions(), cout);
            return 0;
        }

        if(cmdLine.general().version) {
            version( cout );
            return 0;
        }
    } catch(exception& e) {
        cerr << e.what() << endl;
        help(cmdLine.shownOptions(), clog);
        return 1;
    }

    WdbLogHandler logHandler( cmdLine.logging().loglevel, cmdLine.logging().logfile );
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointload.main" );
    log.infoStream() << "Starting pointLoad";

    try {
        wdb::load::point::Loader loader(cmdLine);
        loader.load();
    } catch(std::exception& e) {
        log.fatalStream() << "Reason: " << e.what();
        return -1;
    }

    log.infoStream() << "Exiting pointLoad";

    return 0;
}
