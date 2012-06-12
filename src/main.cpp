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
        out << "Usage: PACKAGE_NAME [OPTIONS] FILES...\n\n";
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

    wdb::WdbLogHandler logHandler(cmdLine.logging().loglevel, cmdLine.logging().logfile);
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.feltload.main" );
    log.debug( "Starting feltLoad" );

    // Get list of files
//    const vector<string> & file = cmdLine.input().file;
//    vector<boost::filesystem::path> files;
//    copy(file.begin(), file.end(), back_inserter(files));

    try {
//        wdb::load::point::DBConnection dbConnection(cmdLine);
//        wdb::load::point::FeltLoader loader(dbConnection, cmdLine);
        wdb::load::point::Loader loader(cmdLine);
        loader.load();
//        for(vector<boost::filesystem::path>::const_iterator it = files.begin(); it != files.end(); ++ it)
//        {
//            try {
//                felt::FeltFile feltFile(* it);
//                loader.load(feltFile);
//                std::cerr<<__FILE__<<"|"<<__FUNCTION__<<"|"<<__LINE__<<": CHECK"<<std::endl;
//            } catch (exception& e) {
//                std::cerr << "Unable to load file " << it->native_file_string();
//                std::cerr << "Reason: " << e.what();
//            }
//        }
    } catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
