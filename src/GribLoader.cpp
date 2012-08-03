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

//project
#include "GribFile.hpp"
#include "GribField.hpp"
#include "GribLoader.hpp"
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

    GribLoader::GribLoader(Loader& controller)
        : FileLoader(controller)
    {
        setup();
    }

    GribLoader::~GribLoader() { }

    void GribLoader::loadInterpolated(const string& fileName)
    {
        GribFile file(fileName);

        if(times_.size() == 0)
            return;

        // Get first field, and check if it exists
        GribFile::Field gribField = file.next();
        if(!gribField) {
            // If the file is empty, we need to throw an error
            std::string errorMessage = "End of file was hit before a product was read into file ";
            errorMessage += file.fileName();
            throw std::runtime_error( errorMessage );
        }

        for( ; gribField; gribField = file.next())
        {
            try{
                std::map<std::string, EntryToLoad>::iterator eIt;

                const GribField& field = *gribField;
                editionNumber_ = editionNumber(field);
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

    string GribLoader::dataProviderName(const GribField & field) const
    {
        string ret = mappingConfig_->dataProviderName4Grib(boost::lexical_cast<string>(field.getGeneratingCenter()), boost::lexical_cast<string>(field.getGeneratingProcess()));
        if(ret.empty()) {
            stringstream keyStr;
            keyStr << field.getGeneratingCenter() << ", " << field.getGeneratingProcess();
            throw wdb::ignore_value( "No dataprovider found for " + keyStr.str());
        }
        return ret;
    }

    string GribLoader::valueParameterName(const GribField & field) const
    {
        string ret;
        if(editionNumber_ == 1) {
            ret = mappingConfig_->valueParameterName4Grib1(boost::lexical_cast<string>(field.getGeneratingCenter()),
                                                           boost::lexical_cast<string>(field.getCodeTableVersionNumber()),
                                                           boost::lexical_cast<string>(field.getParameter1()),
                                                           boost::lexical_cast<string>(field.getTimeRange()),
                                                            "0", "0", "0", "0");
            if(ret.empty()) {
                throw wdb::ignore_value("value parameter name for GRIB1 not found");
            }
        } else if(editionNumber_ == 2) {
            ret = mappingConfig_->valueParameterName4Grib2(boost::lexical_cast<string>(field.getParameter2()));
            if(ret.empty()) {
                throw wdb::ignore_value("value parameter unit for GRIB12 not found");
            }
        }
        return ret;
    }

    string GribLoader::valueParameterUnit(const GribField & field) const
    {
        string ret;
        if(editionNumber_ == 1) {
            ret = mappingConfig_->valueParameterUnits4Grib1(boost::lexical_cast<string>(field.getGeneratingCenter()),
                                                            boost::lexical_cast<string>(field.getCodeTableVersionNumber()),
                                                            boost::lexical_cast<string>(field.getParameter1()),
                                                            boost::lexical_cast<string>(field.getTimeRange()),
                                                            "0", "0", "0", "0");
            if(ret.empty()) {
                throw wdb::ignore_value("value parameter unit for GRIB1 not found");
            }
        } else if(editionNumber_ == 2) {
            ret = mappingConfig_->valueParameterUnits4Grib2(boost::lexical_cast<string>(field.getParameter2()));
            if(ret.empty()) {
                throw wdb::ignore_value("value parameter unit for GRIB12 not found");
            }
        }
        return ret;
    }

    void GribLoader::levelValues( std::vector<wdb::load::Level> & levels, const GribField & field )
    {
        if(editionNumber_ == 1) {
            string scenter = boost::lexical_cast<string>(field.getGeneratingCenter());
            string scodetable2version = boost::lexical_cast<string>(field.getCodeTableVersionNumber());
            string sparameterid = boost::lexical_cast<string>(field.getParameter1());
            string stimerange = boost::lexical_cast<string>(field.getTimeRange());
            string stypeoflevel = boost::lexical_cast<string>(field.getLevelFrom());
            string slevel = boost::lexical_cast<string>(field.getLevelFrom());
            mappingConfig_->levelValues4Grib1(levels, scenter, scodetable2version, sparameterid, stimerange, "0", "0", "0", "0", stypeoflevel, slevel);
        } else if(editionNumber_ == 2) {
            string sparameterid = boost::lexical_cast<string>(field.getParameter2());
            string stypeoflevel = field.getLevelParameter2();
            string slevel = boost::lexical_cast<string>(field.getLevelFrom());
            mappingConfig_->levelValues4Grib2(levels, sparameterid, stypeoflevel, slevel);
        }

        if(levels.size() == 0) throw std::out_of_range( "No valid level key values found." );
    }

    int GribLoader::editionNumber(const GribField & field) const
    {
        return field.getEditionNumber();
    }

} } } // end namespaces
