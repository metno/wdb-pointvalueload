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
#include "FeltLoader.hpp"

// wdb
#include <GridGeometry.h>

// libfelt
#include <felt/FeltFile.h>

// libfimex
#include <fimex/CDM.h>
#include <fimex/CDMReader.h>
#include <fimex/CDMExtractor.h>
#include <fimex/CDMException.h>
#include <fimex/CDMconstants.h>
#include <fimex/CDMDimension.h>
#include <fimex/CDMReaderUtils.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/CDMFileReaderFactory.h>

// libpqxx
#include <pqxx/util>

// boost
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

// std
#include <algorithm>
#include <functional>
#include <cmath>
#include <sstream>

using namespace std;
using namespace boost::posix_time;
using namespace boost::filesystem;

namespace {

    path getConfigFile(const path& fileName)
    {
        static const path sysConfDir = "";//./etc/";//SYSCONFDIR;
        path confPath = sysConfDir/fileName;
        return confPath;
    }

    std::string toString(const boost::posix_time::ptime & time )
    {
        if ( time == boost::posix_time::ptime(neg_infin) )
            return "-infinity"; //"1900-01-01 00:00:00+00";
        else if ( time == boost::posix_time::ptime(pos_infin) )
            return "infinity";//"2100-01-01 00:00:00+00";
        // ...always convert to zulu time
        std::string ret = to_iso_extended_string(time) + "+00";
        return ret;
    }
}

namespace wdb { namespace load { namespace point {

    FeltLoader::FeltLoader(Loader& controller)
        : FileLoader(controller)
    {
        setup();
    }

    FeltLoader::~FeltLoader()
    {
        // NOOP
    }

    void FeltLoader::loadInterpolated(const string& fileName)
    {
        felt::FeltFile file(fileName);

        if(times_.size() == 0)
            return;

        for(felt::FeltFile::const_iterator it = file.begin(); it != file.end(); ++it)
        {
            try{
                std::map<std::string, EntryToLoad>::iterator eIt;

                const felt::FeltField& field(**it);
                std::string name = valueParameterName(field);
                std::string unit = valueParameterUnit(field);
                std::string provider = dataProviderName(field);
                std::vector<Level> levels;
                levelValues(levels, field);

                if(entries2load().find(name) == entries2load().end()) {
                    EntryToLoad entry;
                    entry.name_ = name;
                    entry.unit_ = unit;
                    entry.provider_ = provider;
                    entries2load().insert(std::make_pair<std::string, EntryToLoad>(entry.name_, entry));
                }

                eIt = entries2load().find(name);
                for(size_t i = 0; i < levels.size(); ++i) {
                    eIt->second.levels_.insert(levels[i].levelFrom_);
                    eIt->second.levelname_ = levels[i].levelParameter_;
                }

            } catch ( wdb::ignore_value &e ) {
                std::cerr << e.what() << " Data field not loaded." << std::endl;
            } catch ( std::out_of_range &e ) {
                std::cerr << "Metadata missing for data value. " << e.what() << " Data field not loaded." << std::endl;
            } catch ( std::exception & e ) {
                std::cerr << e.what() << " Data field not loaded." << std::endl;
            }
        }

        loadEntries();

        loadWindEntries();
    }

    std::string FeltLoader::dataProviderName(const felt::FeltField & field)
    {
        stringstream keyStr;
        keyStr << field.producer() << ", " << field.gridArea();
        std::string ret = point2DataProviderName_[keyStr.str()];
        return ret;
    }

    string FeltLoader::valueParameterName(const felt::FeltField & field)
    {
        stringstream keyStr;
        keyStr << field.parameter() << ", " << field.verticalCoordinate() << ", " << field.level1();
        std::string ret;
        try {
            ret = point2ValueParameter_[keyStr.str()];
        } catch ( std::out_of_range & e ) {
            // Check if we match on any (level1)
            stringstream akeyStr;
            akeyStr << field.parameter() << ", " << field.verticalCoordinate() << ", " << "any";
            std::clog << "Did not find " << keyStr.str() << ". Trying to find " << akeyStr.str() << std::endl;
            ret = point2ValueParameter_[akeyStr.str()];
        }
        ret = ret.substr( 0, ret.find(',') );
        boost::trim( ret );
        std::clog << "Value parameter " << ret << " found." << std::endl;
        return ret;
    }

    string FeltLoader::valueParameterUnit(const felt::FeltField & field)
    {
        stringstream keyStr;
        keyStr << field.parameter() << ", " << field.verticalCoordinate() << ", " << field.level1();
        std::string ret;
        try {
            ret = point2ValueParameter_[keyStr.str()];
        }
        catch ( std::out_of_range & e ) {
            // Check if we match on any (level1)
            stringstream akeyStr;
            akeyStr << field.parameter() << ", " << field.verticalCoordinate() << ", " << "any";
            ret = point2ValueParameter_[akeyStr.str()];
        }
        ret = ret.substr( ret.find(',') + 1 );
        boost::trim( ret );
        return ret;
    }

    void FeltLoader::levelValues( std::vector<wdb::load::Level> & levels, const felt::FeltField & field )
    {
        try {
            stringstream keyStr;
            keyStr << field.verticalCoordinate() << ", " << field.level1();
            std::string ret;
            try {
                ret = point2LevelParameter_[keyStr.str()];
            } catch ( std::out_of_range & e ) {
                // Check if we match on any (level1)
                stringstream akeyStr;
                akeyStr << field.verticalCoordinate() << ", any";
                ret = point2LevelParameter_[akeyStr.str()];
            }
            std::string levelParameter = ret.substr( 0, ret.find(',') );
            boost::trim( levelParameter );
            std::string levelUnit = ret.substr( ret.find(',') + 1 );
            boost::trim( levelUnit );
            float coeff = 1.0;
            float term = 0.0;
            readUnit(levelUnit, coeff, term);
            float lev1 = field.level1();
            if ( ( coeff != 1.0 )&&( term != 0.0) ) {
                        lev1 =   ( ( lev1 * coeff ) + term );
            }
            float lev2;
            if ( field.level2() == 0 ) {
                lev2 = lev1;
            } else {
                lev2 = field.level2();
                if ( ( coeff != 1.0 )&&( term != 0.0) ) {
                    lev2 =   ( ( lev2 * coeff ) + term );
                }
            }
            wdb::load::Level baseLevel( levelParameter, lev1, lev2 );
            levels.push_back( baseLevel );
        } catch ( wdb::ignore_value &e ) {
            std::clog << e.what() << std::endl;
        }
        // Find additional level
        try {
            stringstream keyStr;
            keyStr << field.parameter() << ", "
                   << field.verticalCoordinate() << ", "
                   << field.level1() << ", "
                   << field.level2();
            std::clog << "Looking for levels matching " << keyStr.str() << std::endl;
            std::string ret = point2LevelAdditions_[ keyStr.str() ];
            std::string levelParameter = ret.substr( 0, ret.find(',') );
            boost::trim( levelParameter );
            string levFrom = ret.substr( ret.find_first_of(',') + 1, ret.find_last_of(',') - (ret.find_first_of(',') + 1) );
            boost::trim( levFrom );
            string levTo = ret.substr( ret.find_last_of(',') + 1 );
            boost::trim( levTo );
            std::clog << "Found levels from " << levFrom << " to " << levTo << std::endl;
            float levelFrom = boost::lexical_cast<float>( levFrom );
            float levelTo = boost::lexical_cast<float>( levTo );
            wdb::load::Level level( levelParameter, levelFrom, levelTo );
            levels.push_back( level );
        } catch ( wdb::ignore_value &e ) {
            std::clog << e.what() << std::endl;
        } catch ( std::out_of_range &e ) {
            std::clog << "No additional levels found." << std::endl;
        }
        if(levels.size() == 0) {
            stringstream key;
            key << field.parameter() << ", "
                << field.verticalCoordinate() << ", "
                << field.level1() << ", "
                << field.level2();
            throw wdb::ignore_value( "No valid level key values found for " + key.str());
        }
    }

} } } // end namespaces
