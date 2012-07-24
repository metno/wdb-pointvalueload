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
#include "NetCDFLoader.hpp"

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
using namespace MetNoFimex;

namespace {

    path getConfigFile(const path& fileName)
    {
        static const path sysConfDir = "";//./etc/";//SYSCONFDIR;
        path confPath = sysConfDir/fileName;
        return confPath;
    }
}

namespace wdb { namespace load { namespace point {

    NetCDFLoader::NetCDFLoader(Loader& controller)
        : FileLoader(controller)
    {
        setup();
    }

    NetCDFLoader::~NetCDFLoader()
    {
        // NOOP
    }

    void NetCDFLoader::setup()
    {
        if(options().loading().valueparameterConfig.empty())
            throw runtime_error("Can't open valueparameter.config file [empty string?]");
        if(options().loading().levelparameterConfig.empty())
            throw runtime_error("Can't open levelparameter.config file [empty string?]");
        if(options().loading().unitsConfig.empty())
            throw runtime_error("Can't open units.config file [empty string?]");

        point2ValueParameter_.open(getConfigFile(options().loading().valueparameterConfig).file_string());
        point2LevelParameter_.open(getConfigFile(options().loading().levelparameterConfig).file_string());
        point2Units_.open(getConfigFile(options().loading().unitsConfig).file_string());
    }

    string NetCDFLoader::dataProviderName(const string& varname)
    {
        string ret;
        if(options().loading().dataProviderName.empty()) {
            throw runtime_error("data provider name not defined");
        } else {
            ret = options().loading().dataProviderName;
        }
        return ret;
    }

    string NetCDFLoader::valueParameterUnit(const string& varname)
    {
        return cdmData_->getCDM().getUnits(varname);
    }

    string NetCDFLoader::valueParameterName(const string& varname)
    {
        stringstream keyStr;
        keyStr << varname;
        string ret;
        ret = point2ValueParameter_[keyStr.str()];
        ret = ret.substr(0, ret.find(','));
        boost::trim(ret);
        cerr << "Value parameter " << ret << " found." << endl;
        return ret;
    }

    void NetCDFLoader::levelValues(vector<Level> & levels, const string& varname)
    {
        const CDM& cdmRef = cdmData_->getCDM();
        string verticalCoordinate = cdmRef.getVerticalAxis(varname);
        string levelParameter;
        string levelUnit;
        string lvls;
        try {
            stringstream keyStr;
            keyStr << verticalCoordinate;
            string ret;
            try {
                ret = point2LevelParameter_[keyStr.str()];
            } catch ( std::out_of_range & e ) {
                ret = verticalCoordinate;
            }

            levelParameter = ret.substr( 0, ret.find(',') );
            boost::trim(levelParameter);

            levelUnit = ret.substr( ret.find(',') + 1 );
            boost::trim(levelUnit);

            lvls = levelUnit.substr( levelUnit.find(',') + 1 );
            boost::trim(lvls);
            if(lvls == levelUnit) lvls.clear();

            levelUnit = levelUnit.substr(0, levelUnit.find_first_of(", "));
            boost::trim(levelUnit);

            float coeff = 1.0;
            float term = 0.0;
//            cerr << __FUNCTION__ << " @ " << __LINE__ << " verticalCoordinate : " << verticalCoordinate << endl;
//            cerr << __FUNCTION__ << " @ " << __LINE__ << " levelParameter : " << levelParameter << endl;
//            cerr << __FUNCTION__ << " @ " << __LINE__ << " levelUnit : " << levelUnit << endl;
//            cerr << __FUNCTION__ << " @ " << __LINE__ << " lvls : " << lvls << endl;
            readUnit( levelUnit, coeff, term );
//            cerr << __FUNCTION__ << " @ " << __LINE__ << endl;
        } catch ( wdb::ignore_value &e ) {
            cerr<< e.what()<<endl;
        }

        if(!lvls.empty())
        {
            string ret = lvls;
            vector<string> levels2load;
            boost::split(levels2load, ret, boost::is_any_of(", "));
            for(size_t i = 0; i < levels2load.size(); ++i)
            {
                if(levels2load[i].empty())
                    continue;
                float levelTo = boost::lexical_cast<float>(levels2load[i]);
                float levelFrom = levelTo;
                cerr << "Found levels from " << levelFrom << " to " << levelTo<<endl;
                wdb::load::Level level(levelParameter, levelFrom, levelTo);
                levels.push_back(level);
            }
        }

        if(levels.size() == 0) {
            stringstream key;
            key << varname << ", " << verticalCoordinate;
            throw wdb::ignore_value( "No valid level key values found for " + key.str());
        }
    }

    void NetCDFLoader::loadInterpolated(const string& fileName)
    {
        if(times_.size() == 0)
            return;

        const CDM& cdmRef = cdmData_->getCDM();
        vector<CDMVariable> variables = cdmRef.getVariables();
        for(size_t i = 0; i < variables.size(); ++i)
        {
            try{
                std::map<std::string, EntryToLoad>::iterator eIt;

                CDMVariable variable = variables[i];
                string varname = variable.getName();
                string name = valueParameterName(varname);
                string unit = valueParameterUnit(varname);
                string provider = dataProviderName(varname);
                vector<Level> levels;
                levelValues(levels, varname);

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
} } } // namespaces
