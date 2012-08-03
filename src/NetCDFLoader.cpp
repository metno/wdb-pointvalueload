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
#include "CfgXmlFileReader.hpp"

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
#include <boost/make_shared.hpp>
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
        static const path sysConfDir = "";
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

    string NetCDFLoader::dataProviderName(const string& varname)
    {
        string ret = mappingConfig_->dataProviderName4Netcdf(varname);
        if(ret.empty()) {
            throw runtime_error("NETCDF data provider name not defined for " + varname);
        }
        return ret;
    }

    string NetCDFLoader::valueParameterUnit(const string& varname)
    {
        return cdmData_->getCDM().getUnits(varname);
    }

    string NetCDFLoader::valueParameterName(const string& varname)
    {
        string ret =  mappingConfig_->valueParameterName4Netcdf(varname);
        if(ret.empty()) {
            throw runtime_error("NETCDF wdb name for variable name not defined for " + varname);
        }
        return ret;
    }

    void NetCDFLoader::levelValues(vector<Level>& levels, const string& varname)
    {
        vector<Level> tmpLevels;
        mappingConfig_->levelValues4Netcdf(tmpLevels, varname);

        const CDM& cdmRef = cdmData_->getCDM();
        string verticalCoordinate = cdmRef.getVerticalAxis(varname);
        boost::shared_array<float> levelData = cdmData_->getData(verticalCoordinate)->asFloat();

        // extract only those who really exist in the data files
        for(vector<Level>::iterator it = tmpLevels.begin(); it != tmpLevels.end(); ++it)
        {
             Level level = *it;
             float level2check = level.levelFrom_;
             for(size_t i = 0; i < cdmData_->getData(verticalCoordinate)->size(); ++i)
             {
                 if(levelData[i] == level2check) {
                     levels.push_back(level);
                     break;
                 }
             }
        }

        if(levels.size() == 0) {
            throw wdb::ignore_value( "No valid level key values found for " + varname);
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
