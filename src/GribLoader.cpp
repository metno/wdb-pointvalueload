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
        if(options().loading().valueparameter2Config.empty())
            throw std::runtime_error("Can't open valueparameter2.config file [empty string?]");
        if(options().loading().levelparameter2Config.empty())
            throw std::runtime_error("Can't open levelparameter2.config file [empty string?]");
        if(options().loading().leveladditions2Config.empty())
            throw std::runtime_error("Can't open leveladditions2.config file [empty string?]");

        // load GRIB2 metadata
        point2ValueParameter2_.open(getConfigFile(options().loading().valueparameter2Config).file_string());
        point2LevelParameter2_.open(getConfigFile(options().loading().levelparameter2Config).file_string());
        point2LevelAdditions2_.open(getConfigFile(options().loading().leveladditions2Config).file_string());
    }

    GribLoader::~GribLoader()
    {
        // NOOP
    }

    void GribLoader::loadInterpolated(const string& fileName)
    {
        GribFile file(fileName);

        const MetNoFimex::CDM& cdmRef = cdmData_->getCDM();

        // eps - realization variable
        size_t epsLength = 1;
        size_t epsMaxVersion = 0;
        std::string epsVariableName;
        std::string epsCFName = "realization";

        boost::shared_array<int> realizations;
        const MetNoFimex::CDMDimension* epsDim = 0;
        if(!cdmRef.findVariables("standard_name", epsCFName).empty()) {
            epsVariableName = cdmRef.findVariables("standard_name", epsCFName)[0];
            epsDim = &cdmRef.getDimension(epsVariableName);
            epsLength = epsDim->getLength();
            realizations = cdmData_->getData(epsVariableName)->asInt();
            epsMaxVersion = realizations[epsLength - 1];
        }

        if(times_.size() == 0)
            return;

        std::string strReferenceTime = toString(MetNoFimex::getUniqueForecastReferenceTime(cdmData_));

        // Get first field, and check if it exists
        GribFile::Field gribField = file.next();
        if(!gribField) {
            // If the file is empty, we need to throw an error
            std::string errorMessage = "End of file was hit before a product was read into file ";
            errorMessage += file.fileName();
            throw std::runtime_error( errorMessage );
        }

        std::map<std::string, EntryToLoad> entries;

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

                if(entries.find(name) == entries.end()) {
                    EntryToLoad entry;
                    entry.name_ = name;
                    entry.unit_ = unit;
                    entry.provider_ = provider;
                    entries.insert(std::make_pair<std::string, EntryToLoad>(entry.name_, entry));
                }

                eIt = entries.find(name);
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

        std::string dataprovider;

        for(std::map<std::string, EntryToLoad>::const_iterator it = entries.begin(); it != entries.end(); ++it)
        {
                const EntryToLoad& entry(it->second);
                std::string wdbUnit = entry.unit_;
                if(dataprovider != entry.provider_) {
                    dataprovider = entry.provider_;
                    std::cout << dataprovider<<std::endl;
                }

                std::cerr << " LOADING param: " << entry.name_ << " in units: "<<entry.unit_<<std::endl;

                std::string standardName = entry.name_;

                std::string cfname(standardName);
                boost::algorithm::replace_all(cfname, " ", "_");
                std::vector<std::string> variables = cdmRef.findVariables("standard_name", cfname);
                if(variables.empty()) {
                    std::cerr << "cant find vars for cfname: " << cfname << std::endl;
                    continue;
                } else if(variables.size() > 1) {
                    std::cerr << "several vars for cfname: " << cfname << std::endl;
                    continue;
                }

                const MetNoFimex::CDMVariable fimexVar = cdmRef.getVariable(variables[0]);

                std::string xName = cdmRef.getHorizontalXAxis(fimexVar.getName());
                std::string yName = cdmRef.getHorizontalYAxis(fimexVar.getName());
                MetNoFimex::CDMDimension xDim = cdmRef.getDimension(xName);
                MetNoFimex::CDMDimension yDim = cdmRef.getDimension(yName);

                std::string lonName;
                std::string latName;
                assert(cdmRef.getLatitudeLongitude(fimexVar.getName(), latName, lonName));

                boost::shared_ptr<MetNoFimex::Data> raw = cdmData_->getScaledDataInUnit(fimexVar.getName(), wdbUnit);
                if(raw->size() == 0)
                    continue;

                boost::shared_array<double> data4places = raw->asDouble();
                boost::shared_array<double> uwinddata;
                boost::shared_array<double> vwinddata;
                // we deal only with variable that are time dependant
                std::list<std::string> dims(fimexVar.getShape().begin(), fimexVar.getShape().end());
                if(std::find(dims.begin(), dims.end(), "time") == dims.end()) {
                    std::cerr << "not time dependent: " << cfname << std::endl;
                    continue;
                }

                // winds
                string uWind;
                string vWind;
                int position = -1;
                if(find(uwinds().begin(), uwinds().end(), fimexVar.getName()) != uwinds().end()) {
                    uWind = fimexVar.getName();
                    position = find(uwinds().begin(), uwinds().end(), uWind) - uwinds().begin();
                    if(position >= uwinds().size())
                        throw runtime_error("can't find U wind component");

                    vWind = vwinds()[position];
                    uwinddata = data4places;
                    vwinddata = cdmData_->getScaledDataInUnit(vWind, wdbUnit)->asDouble();
                } else if(find(vwinds().begin(), vwinds().end(), fimexVar.getName()) != vwinds().end()) {
                    vWind = fimexVar.getName();
                    position = find(vwinds().begin(), vwinds().end(), vWind) - vwinds().begin();
                    if(position >= vwinds().size())
                        throw runtime_error("can't find V wind component");

                    uWind = uwinds()[position];
                    uwinddata = cdmData_->getScaledDataInUnit(uWind, wdbUnit)->asDouble();
                    vwinddata = data4places;
                }

                if(!uWind.empty() || !vWind.empty()) {
                    vector<string>::iterator it;
                    it = find(uwinds().begin(), uwinds().end(), uWind);
                    uwinds().erase(it);
                    it = find(vwinds().begin(), vwinds().end(), vWind);
                    vwinds().erase(it);
                }

                for(size_t i = 0; i < yDim.getLength(); ++i) {
                    for(size_t j = 0; j < xDim.getLength(); ++j){

                        std::string placename = placenames()[i * xDim.getLength() + j];
                        if(!stations2load().empty() and stations2load().find(placename) == stations2load().end() ) {
                            continue;
                        }

                        for(std::set<double>::const_iterator lIt = entry.levels_.begin(); lIt != entry.levels_.end(); ++lIt) {
                            size_t wdbLevel = *lIt;
                            size_t fimexLevelIndex = 0;
                            size_t fimexLevelLength = 1;
                            std::string fimexLevelName = cdmRef.getVerticalAxis(fimexVar.getName());
                            if(!fimexLevelName.empty()) {
                                // match wdbIndex to index in fimex data
                                MetNoFimex::CDMVariable fimexLevelVar = cdmRef.getVariable(fimexLevelName);
                                fimexLevelLength = cdmRef.getDimension(fimexLevelName).getLength();
                                boost::shared_array<double> fimexLevels = fimexLevelVar.getData()->asDouble();
                                for(size_t index = 0; index < fimexLevelLength; ++ index) {
                                    if(wdbLevel == fimexLevels[index]) {
                                        fimexLevelIndex = index;
                                        break;
                                    }
                                }
                            }

                            int version = 0;
                            bool hasEpsAsDim = false;

                            if(epsDim != 0) {
                                std::list<std::string> dims(fimexVar.getShape().begin(), fimexVar.getShape().end());
                                if(std::find(dims.begin(), dims.end(), epsVariableName) != dims.end()) {
                                    hasEpsAsDim = true;
                                }
                            } else {
                                version = 0;
                                epsLength = 1;
                            }

                            // eps members
                            for(size_t e = 0; e < epsLength; ++e)
                            {
                                // time by time slice
                                for(size_t u = 0; u < times().size(); ++u)
                                {
                                    double value;
                                    double uvalue = 0;
                                    double vvalue = 0;
                                    double windspeed = 0;
                                    double windfromdirection = 0;

                                    if(hasEpsAsDim) {
                                        version = realizations[e];

                                        value = *
                                                (data4places.get() // jump to data start
                                                + u*(epsLength * fimexLevelLength * xDim.getLength() * yDim.getLength() ) // jump to u-th time slice
                                                + e * fimexLevelLength * xDim.getLength() * yDim.getLength() // jump to e-th realization-slice
                                                + fimexLevelIndex * xDim.getLength() * yDim.getLength() // jump to right level
                                                + i * xDim.getLength() + j); // jump to right x,y coordinate

                                        if(uwinddata.get() != 0 && vwinddata.get() != 0) {
                                            uvalue = *
                                                    (uwinddata.get() // jump to data start
                                                    + u*(epsLength * fimexLevelLength * xDim.getLength() * yDim.getLength() ) // jump to u-th time slice
                                                    + e * fimexLevelLength * xDim.getLength() * yDim.getLength() // jump to e-th realization-slice
                                                    + fimexLevelIndex * xDim.getLength() * yDim.getLength() // jump to right level
                                                    + i * xDim.getLength() + j); // jump to right x,y coordinate
                                            vvalue = *
                                                    (vwinddata.get() // jump to data start
                                                    + u*(epsLength * fimexLevelLength * xDim.getLength() * yDim.getLength() ) // jump to u-th time slice
                                                    + e * fimexLevelLength * xDim.getLength() * yDim.getLength() // jump to e-th realization-slice
                                                    + fimexLevelIndex * xDim.getLength() * yDim.getLength() // jump to right level
                                                    + i * xDim.getLength() + j); // jump to right x,y coordinate
                                        }
                                    } else {
                                        version = 0;

                                        value = *
                                                (data4places.get() // jump to data start
                                                + u*(fimexLevelLength * xDim.getLength() * yDim.getLength() ) // jump to u-th time slice
                                                + fimexLevelIndex * xDim.getLength() * yDim.getLength() // jump to right level
                                                + i * xDim.getLength() + j); // jump to right x,y coordinate

                                        if(uwinddata.get() != 0 && vwinddata.get() != 0) {
                                            uvalue = *
                                                    (uwinddata.get() // jump to data start
                                                    + u*(fimexLevelLength * xDim.getLength() * yDim.getLength() ) // jump to u-th time slice
                                                    + fimexLevelIndex * xDim.getLength() * yDim.getLength() // jump to right level
                                                    + i * xDim.getLength() + j); // jump to right x,y coordinate
                                            vvalue = *
                                                    (vwinddata.get() // jump to data start
                                                    + u*(fimexLevelLength * xDim.getLength() * yDim.getLength() ) // jump to u-th time slice
                                                    + fimexLevelIndex * xDim.getLength() * yDim.getLength() // jump to right level
                                                    + i * xDim.getLength() + j); // jump to right x,y coordinate

                                        }
                                    }

                                    std::string validtime = times()[u];
                                    if(uwinddata.get() != 0 && vwinddata.get() != 0) {
                                        windspeed = std::sqrt(uvalue*uvalue + vvalue*vvalue);
                                        windfromdirection = 270.0 - std::atan2(vvalue, uvalue) * 180 / 3.14159265;
                                        windfromdirection = windfromdirection - (static_cast<int>(windfromdirection) / static_cast<int>(360)) * 360;
                                    }

                                    try {

                                        if(options().output().dry_run) {
                                            if(uwinddata.get() != 0 && vwinddata.get() != 0) {
//                                                std::cout << uvalue        << "\t"
//                                                          << placename        << "\t"
//                                                          << strReferenceTime << "\t"
//                                                          << validtime        << "\t"
//                                                          << validtime        << "\t"
//                                                          << uWind     << "\t"
//                                                          << entry.levelname_ << "\t"
//                                                          << wdbLevel         << "\t"
//                                                          << wdbLevel         << "\t"
//                                                          << version          << "\t"
//                                                          << epsMaxVersion
//                                                          << std::endl;

//                                                std::cout << vvalue        << "\t"
//                                                          << placename        << "\t"
//                                                          << strReferenceTime << "\t"
//                                                          << validtime        << "\t"
//                                                          << validtime        << "\t"
//                                                          << vWind     << "\t"
//                                                          << entry.levelname_ << "\t"
//                                                          << wdbLevel         << "\t"
//                                                          << wdbLevel         << "\t"
//                                                          << version          << "\t"
//                                                          << epsMaxVersion
//                                                          << std::endl;

                                                std::cout << windspeed        << "\t"
                                                          << placename        << "\t"
                                                          << strReferenceTime << "\t"
                                                          << validtime        << "\t"
                                                          << validtime        << "\t"
                                                          << "wind speed"     << "\t"
                                                          << entry.levelname_ << "\t"
                                                          << wdbLevel         << "\t"
                                                          << wdbLevel         << "\t"
                                                          << version          << "\t"
                                                          << epsMaxVersion
                                                          << std::endl;
                                                std::cout << windfromdirection       << "\t"
                                                          << placename        << "\t"
                                                          << strReferenceTime << "\t"
                                                          << validtime        << "\t"
                                                          << validtime        << "\t"
                                                          << "wind from direction"     << "\t"
                                                          << entry.levelname_ << "\t"
                                                          << wdbLevel         << "\t"
                                                          << wdbLevel         << "\t"
                                                          << version          << "\t"
                                                          << epsMaxVersion
                                                          << std::endl;
                                            } else {
                                                std::cout << value            << "\t"
                                                          << placename        << "\t"
                                                          << strReferenceTime << "\t"
                                                          << validtime        << "\t"
                                                          << validtime        << "\t"
                                                          << standardName     << "\t"
                                                          << entry.levelname_ << "\t"
                                                          << wdbLevel         << "\t"
                                                          << wdbLevel         << "\t"
                                                          << version          << "\t"
                                                          << epsMaxVersion
                                                          << std::endl;
                                            }

                                        } else {
                                            if(uwinddata.get() != 0 && vwinddata.get() != 0) {
                                                wdbConnection().write(
                                                                      windspeed,
                                                                      dataprovider,
                                                                      placename,
                                                                      strReferenceTime,
                                                                      validtime,
                                                                      validtime,
                                                                      "wind speed",
                                                                      entry.levelname_,
                                                                      wdbLevel,
                                                                      wdbLevel,
                                                                      version
                                                                     );
                                                wdbConnection().write(
                                                                      windfromdirection,
                                                                      dataprovider,
                                                                      placename,
                                                                      strReferenceTime,
                                                                      validtime,
                                                                      validtime,
                                                                      "wind from direction",
                                                                      entry.levelname_,
                                                                      wdbLevel,
                                                                      wdbLevel,
                                                                      version
                                                                     );
                                            } else {
                                                wdbConnection().write(
                                                                      value,
                                                                      dataprovider,
                                                                      placename,
                                                                      strReferenceTime,
                                                                      validtime,
                                                                      validtime,
                                                                      standardName,
                                                                      entry.levelname_,
                                                                      wdbLevel,
                                                                      wdbLevel,
                                                                      version
                                                                     );
                                            }
                                        }

                                    } catch ( wdb::ignore_value &e ) {
                                        std::cerr << e.what() << " Data field not loaded."<< std::endl;
                                    } catch ( std::out_of_range &e ) {
                                        std::cerr << "Metadata missing for data value. " << e.what() << " Data field not loaded." << std::endl;
                                    } catch ( std::exception & e ) {
                                        std::cerr << e.what() << " Data field not loaded." << std::endl;
                                    }
                            } // time slices end
                        } // eps slices
                    } // z slices
                } // x slices
            } // y slices

        }
    }

    std::string GribLoader::dataProviderName(const GribField & field) const
    {
        stringstream keyStr;
        keyStr << field.getGeneratingCenter() << ", "
               << field.getGeneratingProcess();
        std::cerr << __FUNCTION__ << " keyStr " << keyStr.str() << std::endl;
        try {
            std::string ret = point2DataProviderName_[ keyStr.str() ];
            return ret;
        }
        catch ( std::out_of_range &e ) {
            WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
            log.errorStream() << "Could not identify the data provider.";
            throw;
        }
    }

    std::string GribLoader::valueParameterName(const GribField & field) const
    {
        stringstream keyStr;
        std::string ret;
        if (editionNumber_ == 1) {
            keyStr << field.getGeneratingCenter() << ", "
                   << field.getCodeTableVersionNumber() << ", "
                   << field.getParameter1() << ", "
                   << field.getTimeRange() << ", "
                   << "0, 0, 0, 0"; // Default values for thresholds
            try {
                ret = point2ValueParameter_[keyStr.str()];
            }
            catch ( std::out_of_range &e ) {
                WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
                log.errorStream() << "Could not identify the value parameter.";
                throw;
            }
        }
        else {
            keyStr << field.getParameter2();
            std::cerr << __FUNCTION__ << " keyStr " << keyStr.str() << std::endl;
            try {
                ret = point2ValueParameter2_[keyStr.str()];
            }
            catch ( std::out_of_range &e ) {
                WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
                log.errorStream() << "Could not identify the value parameter.";
                throw;
            }
        }
        ret = ret.substr( 0, ret.find(',') );
        boost::trim( ret );
        return ret;
    }

    std::string GribLoader::valueParameterUnit(const GribField & field) const
    {
        stringstream keyStr;
        std::string ret;
        if (editionNumber_ == 1) {
            keyStr << field.getGeneratingCenter() << ", "
                   << field.getCodeTableVersionNumber() << ", "
                   << field.getParameter1() << ", "
                   << field.getTimeRange() << ", "
                   << "0, 0, 0, 0"; // Default values for thresholds
            try {
                ret = point2ValueParameter_[keyStr.str()];
            }
            catch ( std::out_of_range &e ) {
                WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
                log.errorStream() << "Could not identify the value parameter identified by " << keyStr.str();
                throw;
            }
        }
        else {
            keyStr << field.getParameter2();
            std::cerr << __FUNCTION__ << " keyStr " << keyStr.str() << std::endl;
            try {
                ret = point2ValueParameter2_[keyStr.str()];
            }
            catch ( std::out_of_range &e ) {
                WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
                log.errorStream() << "Could not identify the value parameter identified by " << keyStr.str();
                throw;
            }
        }
        ret = ret.substr( ret.find(',') + 1 );
        boost::trim( ret );
        return ret;
    }

    void GribLoader::levelValues( std::vector<wdb::load::Level> & levels, const GribField & field )
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
        bool ignored = false;
        stringstream keyStr;
        std::string ret;
        try {
            if (editionNumber_ == 1) {
                keyStr << field.getLevelParameter1();
                std::cerr << __FUNCTION__ << " field.getLevelParameter1() "<<keyStr.str() << std::endl;
                ret = point2LevelParameter_[keyStr.str()];
            }
            else {
                keyStr << field.getLevelParameter2();
                std::cerr << __FUNCTION__ << " keyStr "<<keyStr.str() << std::endl;
                ret = point2LevelParameter2_[keyStr.str()];
            }
            std::string levelParameter = ret.substr( 0, ret.find(',') );
            boost::trim( levelParameter );
            std::string levelUnit = ret.substr( ret.find(',') + 1 );
            boost::trim( levelUnit );
            float coeff = 1.0;
            float term = 0.0;
            wdbConnection().readUnit( levelUnit, &coeff, &term );
            float lev1 = field.getLevelFrom();
            float lev2 = field.getLevelTo();
            if ( ( coeff != 1.0 )&&( term != 0.0) ) {
                lev1 =   ( ( lev1 * coeff ) + term );
                lev2 =   ( ( lev2 * coeff ) + term );
            }
            wdb::load::Level baseLevel( levelParameter, lev1, lev2 );
            levels.push_back( baseLevel );
        }
        catch ( wdb::ignore_value &e )
        {
            log.infoStream() << e.what();
            ignored = true;
        }
        catch ( std::out_of_range &e ) {
            log.errorStream() << "Could not identify the level parameter identified by " << keyStr.str();
        }
        // Find additional level
        try {
            stringstream keyStr;
            std::string ret;
            if (editionNumber_ == 1) {
                keyStr << field.getGeneratingCenter() << ", "
                       << field.getCodeTableVersionNumber() << ", "
                       << field.getParameter1() << ", "
                       << field.getTimeRange() << ", "
                       << "0, 0, 0, 0, "
                       << field.getLevelParameter1(); // Default values for thresholds
                ret = point2LevelAdditions_[keyStr.str()];
            }
            else {
                keyStr << field.getParameter2();
                std::cerr <<__FUNCTION__<<" keyStr "<< keyStr.str() << std::endl;
                ret = point2LevelAdditions2_[keyStr.str()];
            }
            if ( ret.length() != 0 ) {
                std::string levelParameter = ret.substr( 0, ret.find(',') );
                boost::trim( levelParameter );
                string levFrom = ret.substr( ret.find_first_of(',') + 1, ret.find_last_of(',') - (ret.find_first_of(',') + 1) );
                boost::trim( levFrom );
                string levTo = ret.substr( ret.find_last_of(',') + 1 );
                boost::trim( levTo );
                float levelFrom = boost::lexical_cast<float>( levFrom );
                float levelTo = boost::lexical_cast<float>( levTo );
                wdb::load::Level level( levelParameter, levelFrom, levelTo );
                levels.push_back( level );
            }
        }
        catch ( wdb::ignore_value &e )
        {
            log.infoStream() << e.what();
        }
        catch ( std::out_of_range &e )
        {
            // NOOP
        }
        if ( levels.size() == 0 ) {
            if ( ignored )
                throw wdb::ignore_value( "Level key is ignored" );
            else
                throw std::out_of_range( "No valid level key values found." );
        }
    }

    int GribLoader::editionNumber(const GribField & field) const
    {
        return field.getEditionNumber();
    }

} } } // end namespaces
