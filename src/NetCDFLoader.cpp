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
#include <wdbLogHandler.h>

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

    NetCDFLoader::~NetCDFLoader() { }

    void NetCDFLoader::setup()
    {
        // check for excess parameters
        if(!options().loading().dataproviderConfig.empty())
            throw runtime_error("dataprovider.config file not required");
        if(!options().loading().leveladditionsConfig.empty())
            throw runtime_error("leveladditions.config file [empty string?]");
        if(!options().loading().valueparameter2Config.empty())
            throw std::runtime_error("valueparameter2.config file not required");
        if(!options().loading().levelparameter2Config.empty())
            throw std::runtime_error("levelparameter2.config file not required");
        if(!options().loading().leveladditions2Config.empty())
            throw std::runtime_error("Can't open leveladditions2.config file [empty string?]");

        if(options().loading().valueparameterConfig.empty())
            throw runtime_error("Can't open valueparameter.config file [empty string?]");
        if(options().loading().levelparameterConfig.empty())
            throw runtime_error("Can't open levelparameter.config file [empty string?]");
        if(options().loading().unitsConfig.empty())
            throw runtime_error("Can't open units.config file [empty string?]");

        point2ValueParameter_.open(getConfigFile(options().loading().valueparameterConfig).string());
        point2LevelParameter_.open(getConfigFile(options().loading().levelparameterConfig).string());
        point2Units_.open(getConfigFile(options().loading().unitsConfig).string());
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
        return ret;
    }

    void NetCDFLoader::levelValues(vector<Level> & levels, const string& varname)
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointload.NetCDFLoader" );
        const CDM& cdmRef = cdmData_->getCDM();
        string verticalCoordinate = cdmRef.getVerticalAxis(varname);
	if(verticalCoordinate.empty())
	  throw wdb::ignore_value( "No vertical coordinate found for " + varname);
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
            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << " verticalCoordinate : " << verticalCoordinate;
            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << " levelParameter : " << levelParameter;
            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << " levelUnit : " << levelUnit;
            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] "<< " lvls : " << lvls;
            readUnit( levelUnit, coeff, term );
        } catch ( wdb::ignore_value &e ) {
            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << e.what();
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
                log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << levelFrom << " to " << levelTo;
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
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointload.NetCDFLoader" );
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
                CDMAttribute uAtt;
                cdmRef.getAttribute(varname, "standard_name", uAtt);
                string standardname = uAtt.getStringValue();
                string wdbname = valueParameterName(varname);
                string wdbunit = valueParameterUnit(varname);
                string wdbdataprovider = dataProviderName(varname);
                vector<Level> levels;
                levelValues(levels, varname);

                if(entries2load().find(wdbname) == entries2load().end()) {
                    EntryToLoad entry;
                    entry.cdmName_ = varname;
                    entry.wdbName_ = wdbname;
                    entry.standardName_ = standardname;
                    entry.wdbUnit_ = wdbunit;
                    entry.wdbDataProvider_ = wdbdataprovider;
                    entries2load().insert(std::make_pair<std::string, EntryToLoad>(entry.wdbName_, entry));
                }

                eIt = entries2load().find(wdbname);
                for(size_t i = 0; i < levels.size(); ++i) {
                    eIt->second.wdbLevels_.insert(levels[i].levelFrom_);
                    eIt->second.wdbLevelName_ = levels[i].levelParameter_;
                }

            } catch ( wdb::ignore_value &e ) {
                log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << e.what() << " Data field not loaded.";
            } catch ( std::out_of_range &e ) {
                log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << "Metadata missing for data value. " << e.what() << " Data field not loaded.";
            } catch ( std::exception & e ) {
                log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << e.what() << " Data field not loaded.";
            }
        }

        loadEntries();

        loadWindEntries();
    }
} } } // namespaces
