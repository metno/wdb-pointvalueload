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
#include "FeltLoader.hpp"
#include "WciTransactors.hpp"

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

// std
#include <algorithm>
#include <functional>
#include <cmath>
#include <sstream>

using namespace std;
using namespace wdb;
using namespace wdb::load;
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

    struct EntryToLoad {
        std::string name_;
        std::string unit_;
        std::string provider_;
        std::string levelname_;
        std::set<double> levels_;
    };

    FeltLoader::FeltLoader(WdbConnection& connection, const CmdLine& cmdLine)
        : connection_(connection), options_(cmdLine),
          northBound_(90.0), southBound_(-90.0), westBound_(-180.0), eastBound_(180.0)
    {

        if(cmdLine.loading().validtimeConfig.empty())
            throw std::runtime_error("Can't open validtime.config file [empty string?]");
        if(cmdLine.loading().dataproviderConfig.empty())
            throw std::runtime_error("Can't open dataprovider.config file [empty string?]");
        if(cmdLine.loading().valueparameterConfig.empty())
            throw std::runtime_error("Can't open valueparameter.config file [empty string?]");
        if(cmdLine.loading().levelparameterConfig.empty())
            throw std::runtime_error("Can't open levelparameter.config file [empty string?]");
        if(cmdLine.loading().leveladditionsConfig.empty())
            throw std::runtime_error("Can't open leveladditions.config file [empty string?]");

        point2ValidTime_.open(getConfigFile(cmdLine.loading().validtimeConfig).file_string());
        point2DataProviderName_.open(getConfigFile(cmdLine.loading().dataproviderConfig).file_string());
        point2ValueParameter_.open(getConfigFile(cmdLine.loading().valueparameterConfig).file_string());
        point2LevelParameter_.open(getConfigFile(cmdLine.loading().levelparameterConfig).file_string());
        point2LevelAdditions_.open(getConfigFile(cmdLine.loading().leveladditionsConfig).file_string());
    }

    FeltLoader::~FeltLoader()
    {
        // NOOP
    }

    bool FeltLoader::openTemplateCDM(const std::string& fileName)
    {
        if(fileName.empty())
            throw std::runtime_error(" Can't open template interpolation file! ");

        cdmTemplate_ =
                MetNoFimex::CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, fileName);

        assert(extractPointIds());

//        cdmTemplate_->getCDM().toXMLStream(std::cerr);
        return true;
    }

    bool FeltLoader::openDataCDM(const std::string& fileName, const std::string& fimexCfgFileName)
    {
        if(fimexCfgFileName.empty())
            throw std::runtime_error(" Can't open fimex reader configuration file!");

        cdmData_ =
                MetNoFimex::CDMFileReaderFactory::create(MIFI_FILETYPE_FELT, fileName, fimexCfgFileName);

//        cdmReader_->getCDM().toXMLStream(std::cerr);
        return true;
    }

    bool FeltLoader::extractPointIds()
    {
        if(not cdmTemplate_.get())
            return false;

        const MetNoFimex::CDM& cdmRef = cdmTemplate_->getCDM();
        const std::string stationIdVarName("stationid");

        assert(cdmRef.hasVariable(stationIdVarName));

        pointids_ = cdmTemplate_->getData(stationIdVarName)->asUInt();
        unsigned int* sIt = &pointids_[0];
        unsigned int* eIt = &pointids_[cdmTemplate_->getData(stationIdVarName)->size()];
        for(; sIt!=eIt; ++sIt)
            placenames_.push_back(boost::lexical_cast<std::string>(*sIt));

        return true;
    }

    bool FeltLoader::extractBounds()
    {
        if(not cdmTemplate_.get())
            return false;

        const MetNoFimex::CDM& cdmRef = cdmTemplate_->getCDM();

        assert(cdmRef.hasVariable("latitude"));
        assert(cdmRef.hasVariable("longitude"));


        boost::shared_array<double> lats = cdmTemplate_->getData("latitude")->asDouble();
        northBound_ = *(std::max_element(&lats[0], &lats[cdmTemplate_->getData("latitude")->size()]));
        southBound_ = *(std::min_element(&lats[0], &lats[cdmTemplate_->getData("latitude")->size()]));

        boost::shared_array<double> lons = cdmTemplate_->getData("longitude")->asDouble();
        eastBound_ = *(std::max_element(&lons[0], &lons[cdmTemplate_->getData("longitude")->size()]));
        westBound_ = *(std::min_element(&lons[0], &lons[cdmTemplate_->getData("longitude")->size()]));

        return true;
    }

    bool FeltLoader::extractData()
    {
        extractBounds();

        boost::shared_ptr<MetNoFimex::CDMExtractor>
                extractor(new MetNoFimex::CDMExtractor(cdmData_));
        extractor->reduceLatLonBoundingBox(southBound_, northBound_, westBound_, eastBound_);

        cdmData_ = extractor;

//        cdmReader_->getCDM().toXMLStream(std::cerr);
        return true;
    }

    bool FeltLoader::interpolate(const std::string& templateFileName)
    {
        if(templateFileName.empty())
            return false;
        if(not cdmTemplate_.get())
            return false;
        if(not cdmData_.get())
            return false;

        boost::shared_ptr<MetNoFimex::CDMInterpolator> interpolator =
                boost::shared_ptr<MetNoFimex::CDMInterpolator>(new MetNoFimex::CDMInterpolator(cdmData_));

        interpolator->changeProjection(MIFI_INTERPOL_BICUBIC, templateFileName);

        cdmData_ = interpolator;

        return true;
    }

    void FeltLoader::load(const felt::FeltFile& file)
    {
        std::string feltFileName = file.fileName().native_file_string();
        std::string tmplFileName = options_.loading().fimexTemplate;
        std::string fimexCfgFileName = options_.loading().fimexConfig;

        openTemplateCDM(tmplFileName);

        openDataCDM(feltFileName, fimexCfgFileName);

//        extractData();

        assert(interpolate(tmplFileName));

        loadInterpolated(file);
    }

    void FeltLoader::loadInterpolated(const felt::FeltFile& file)
    {
        const MetNoFimex::CDM& cdmRef = cdmData_->getCDM();

//        cdmRef.toXMLStream(std::cerr);

        // eps - realization variable
        size_t epsLength = 1;
        std::string epsVariableName;
        std::string epsCFName = "realization";

        boost::shared_array<int> realizations;
        const MetNoFimex::CDMDimension* epsDim = 0;
        if(!cdmRef.findVariables("standard_name", epsCFName).empty()) {
            epsVariableName = cdmRef.findVariables("standard_name", epsCFName)[0];
            epsDim = &cdmRef.getDimension(epsVariableName);
            epsLength = epsDim->getLength();
            realizations = cdmData_->getData(epsVariableName)->asInt();
        }

        const MetNoFimex::CDMDimension* unlimited = cdmRef.getUnlimitedDim();
        if(unlimited == 0)
            return;

        size_t uDim = unlimited->getLength();

        boost::shared_array<unsigned long long> uValues =
                cdmData_->getScaledDataInUnit(unlimited->getName(), "seconds since 1970-01-01 00:00:00 +00:00")->asUInt64();

        for(size_t u = 0; u < uDim; ++u) {
            times_.push_back(boost::posix_time::to_iso_extended_string(boost::posix_time::from_time_t(uValues[u])) + "+00");
        }

        long long inserts = 0;
        std::map<std::string, EntryToLoad> entries;
        for(felt::FeltFile::const_iterator it = file.begin(); it != file.end(); ++it)
        {
            try{

                std::map<std::string, EntryToLoad>::iterator eIt;

                const felt::FeltField& flt(**it);
                std::string name = valueParameterName(flt);
                std::string unit = valueParameterUnit(flt);
                std::string provider = dataProviderName(flt);
                std::vector<Level> levels;
                levelValues(levels, **it);

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
                std::cerr << e.what() << " Data field not loaded.";
            } catch ( std::out_of_range &e ) {
                std::cerr << "Metadata missing for data value. " << e.what() << " Data field not loaded.";
            } catch ( std::exception & e ) {
                std::cerr << e.what() << " Data field not loaded.";
            }
        }

        for(std::map<std::string, EntryToLoad>::const_iterator it = entries.begin(); it != entries.end(); ++it)
        {
//            std::cerr<<flt.information()<<std::endl;

                const EntryToLoad& entry(it->second);
                std::string wdbUnit = entry.unit_;
                std::string dataProvider = entry.provider_;
                std::string standardName = entry.name_;
                std::string strReferenceTime = toString(MetNoFimex::getUniqueForecastReferenceTime(cdmData_));

//                std::string strReferenceTime = toString(referenceTime(**it));
//                std::string strValidTimeFrom = toString(validTimeFrom(**it));
//                std::string strValidTimeTo = toString(validTimeTo(**it));

                std::string cfname(standardName);
                boost::algorithm::replace_all(cfname, " ", "_");
                std::vector<std::string> variables = cdmRef.findVariables("standard_name", cfname);
                if(variables.empty()) {
//                    std::cerr << "cant find vars for cfname: " << cfname << std::endl;
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

                // we deal only with variable that are time dependant
                std::list<std::string> dims(fimexVar.getShape().begin(), fimexVar.getShape().end());
                if(std::find(dims.begin(), dims.end(), unlimited->getName()) == dims.end()) {
                    continue;
                }


                for(size_t i = 0; i < yDim.getLength(); ++i) {
                    for(size_t j = 0; j < xDim.getLength(); ++j){
                        std::string buffer;
                        buffer.reserve(100 * yDim.getLength() * xDim.getLength() * entry.levels_.size() * 70 * 50);
                        for(std::set<double>::const_iterator lIt = entry.levels_.begin(); lIt != entry.levels_.end(); ++lIt) {
//                            std::cerr<<"x= "<<j<<"   "<<"y= "<<i<<"   "<<"z= "<<l<<std::endl;
//                           std::cerr << " LEVEL NAME: " << levels[l].levelParameter_ << " LEVEL FROM:" << levels[l].levelFrom_ << " LEVEL TO:" << levels[l].levelTo_ << std::endl;
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
                                for(size_t u = 0; u < uDim; ++u)
                                {
                                    double value;

                                    if(hasEpsAsDim) {
                                        version = realizations[e];
                                        value = *
                                                (data4places.get() // jump to data start
                                                + u*(epsLength * fimexLevelLength * xDim.getLength() * yDim.getLength() ) // jump to u-th time slice
                                                + e * fimexLevelLength * xDim.getLength() * yDim.getLength() // jump to e-th realization-slice
                                                + fimexLevelIndex * xDim.getLength() * yDim.getLength() // jump to right level
                                                + i * xDim.getLength() + j); // jump to right x,y coordinate
                                    } else {
                                        version = 0;
                                        value = *
                                                (data4places.get() // jump to data start
                                                + u*(fimexLevelLength * xDim.getLength() * yDim.getLength() ) // jump to u-th time slice
                                                + fimexLevelIndex * xDim.getLength() * yDim.getLength() // jump to right level
                                                + i * xDim.getLength() + j); // jump to right x,y coordinate
                                    }

                                    std::string varname = fimexVar.getName();
                                    std::string placename = placenames_[i * xDim.getLength() + j];
                                    std::string validtime = times_[u];

                                    try {

                                        if(options_.output().dry_run) {
                                            std::stringstream sstream;

//                                            sstream << " VAR NAME: "<< varname
//                                                      << " CF NAME: " << standardName
//                                                      << " DATA PROVIDER: " << dataProvider
//                                                      << " PLACENAME: " << placename
//                                                      << " REF TIME:" << strReferenceTime
//                                                      << " VALID FROM:" << validtime
//                                                      << " VALID TO:" << validtime
//                                                      << " LEVEL NAME: " << entry.levelname_
//                                                      << " LEVEL FROM:" << wdbLevel
//                                                      << " LEVEL TO:" << wdbLevel
//                                                      << " VERSION: " << version
////                                              << " DATA VERSION:" << dataVersion(**it)
////                                              << " CONFIDENCE CODE: " << confidenceCode(**it)
//                                                      << " VALUE: " << value
//                                                      << std::endl;

                                            sstream << " <"<< varname
                                                      << "|" << standardName
                                                      << "|" << dataProvider
                                                      << "|" << placename
                                                      << "|" << strReferenceTime
                                                      << "|" << validtime
                                                      << "|" << validtime
                                                      << "|" << entry.levelname_
                                                      << "|" << wdbLevel
                                                      << "|" << wdbLevel
                                                      << "|" << version
                                                      << "|" << value << "> ";

                                            buffer.append(sstream.str());
                                        } else {

                                            std::cerr<<"*";
                                            connection_.write(
                                                              value,
                                                              dataProvider,
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


                                    } catch ( wdb::ignore_value &e ) {
                                        std::cerr << e.what() << " Data field not loaded.";
                                    } catch ( std::out_of_range &e ) {
                                        std::cerr << "Metadata missing for data value. " << e.what() << " Data field not loaded.";
                                    } catch ( std::exception & e ) {
                                        std::cerr << e.what() << " Data field not loaded.";
                                    }

                            } // time slices end
                        } // eps slices
                    } // z slices
                    std::clog << buffer;
                    buffer.clear();
                } // x slices
            } // y slices

        }

//         std::cerr<<"Num of INSERTS: "<<inserts<<std::endl;
    }

    std::string FeltLoader::dataProviderName(const felt::FeltField & field)
    {
        stringstream keyStr;
        keyStr << field.producer() << ", " << field.gridArea();
        std::string ret = point2DataProviderName_[keyStr.str()];
        return ret;
    }

    boost::posix_time::ptime FeltLoader::referenceTime(const felt::FeltField & field)
    {
        return field.referenceTime();
    }

    boost::posix_time::ptime FeltLoader::validTimeFrom(const felt::FeltField & field)
    {
        stringstream keyStr;
        keyStr << field.parameter();
        std::string modifier;
        try {
            modifier = point2ValidTime_[keyStr.str()];
        } catch ( std::out_of_range & e ) {
            return field.validTime();
        }
        // Infinite Duration
        if( modifier == "infinite" ) {
            return boost::posix_time::neg_infin;
        } else {
            if( modifier == "referencetime" ) {
                return field.referenceTime();
            } else {
                std::istringstream duration(modifier);
                int hour, minute, second;
                char dummy;
                duration >> hour >> dummy >> minute >> dummy >> second;
                boost::posix_time::time_duration period(hour, minute, second);
                boost::posix_time::ptime ret = field.validTime() + period;
                return ret;
            }
        }
    }

    boost::posix_time::ptime FeltLoader::validTimeTo(const felt::FeltField & field)
    {
        stringstream keyStr;
        keyStr << field.parameter();
        std::string modifier;
        try {
            modifier = point2ValidTime_[ keyStr.str() ];
        } catch ( std::out_of_range & e ) {
            return field.validTime();
        }
        // Infinite Duration
        if ( modifier == "infinite" ) {
            return boost::posix_time::pos_infin;
        } else {
            // For everything else...
            return field.validTime();
        }
    }

    std::string FeltLoader::valueParameterName(const felt::FeltField & field)
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointFeltLoader.valueparametername" );
        stringstream keyStr;
        keyStr << field.parameter() << ", " << field.verticalCoordinate() << ", " << field.level1();
        std::string ret;
        try {
            ret = point2ValueParameter_[keyStr.str()];
        } catch ( std::out_of_range & e ) {
            // Check if we match on any (level1)
            stringstream akeyStr;
            akeyStr << field.parameter() << ", " << field.verticalCoordinate() << ", " << "any";
            log.debugStream() << "Did not find " << keyStr.str() << ". Trying to find " << akeyStr.str();
            ret = point2ValueParameter_[akeyStr.str()];
        }
        ret = ret.substr( 0, ret.find(',') );
        boost::trim( ret );
        log.debugStream() << "Value parameter " << ret << " found.";
        return ret;
    }

    std::string FeltLoader::valueParameterUnit(const felt::FeltField & field)
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
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointFeltLoader.levelValues" );
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
            connection_.readUnit( levelUnit, &coeff, &term );
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
            log.infoStream() << e.what();
        }
        // Find additional level
        try {
            stringstream keyStr;
            keyStr << field.parameter() << ", "
                   << field.verticalCoordinate() << ", "
                   << field.level1() << ", "
                   << field.level2();
            log.debugStream() << "Looking for levels matching " << keyStr.str();
            std::string ret = point2LevelAdditions_[ keyStr.str() ];
            std::string levelParameter = ret.substr( 0, ret.find(',') );
            boost::trim( levelParameter );
            string levFrom = ret.substr( ret.find_first_of(',') + 1, ret.find_last_of(',') - (ret.find_first_of(',') + 1) );
            boost::trim( levFrom );
            string levTo = ret.substr( ret.find_last_of(',') + 1 );
            boost::trim( levTo );
            log.debugStream() << "Found levels from " << levFrom << " to " << levTo;
            float levelFrom = boost::lexical_cast<float>( levFrom );
            float levelTo = boost::lexical_cast<float>( levTo );
            wdb::load::Level level( levelParameter, levelFrom, levelTo );
            levels.push_back( level );
        } catch ( wdb::ignore_value &e ) {
            log.infoStream() << e.what();
        } catch ( std::out_of_range &e ) {
            log.debugStream() << "No additional levels found.";
        }
        if(levels.size() == 0) {
            throw wdb::ignore_value( "No valid level key values found." );
        }
    }

    int FeltLoader::dataVersion(const felt::FeltField & field)
    {
        return field.dataVersion();
    }


//    void FeltLoader::load(const felt::FeltField & field, const std::string& placename, boost::shared_ptr<MetNoFimex::CDMInterpolator>& interpolator)
//    {
//        try {
//            const MetNoFimex::CDM& cdmRef = interpolator->getCDM();

//            std::string strReferenceTime = toString(referenceTime(field));
//            std::string strValidTimeFrom = toString(validTimeFrom(field));
//            std::string strValidTimeTo = toString(validTimeTo(field));
//            std::string dataProvider = dataProviderName(field);

//            std::string standardName = valueParameterName(field);
//            std::string unitInWdb = valueParameterUnit(field);
//            std::string cfname(standardName);
//            boost::algorithm::replace_all(cfname, " ", "_");
//            std::vector<std::string> variables = cdmRef.findVariables("standard_name", cfname);
//            if(variables.empty()) {
//                std::cerr << "cant find vars for cfname: " << cfname << std::endl;
//                return;
//            } else if(variables.size() > 1) {
//                std::cerr << "several vars for cfname: " << cfname << std::endl;
//                return;
//            }

//            const MetNoFimex::CDMVariable fimexVar = cdmRef.getVariable(variables[0]);

//            std::vector<wdb::load::Level> levels;
//            levelValues(levels, field);

////            size_t lDim = 1;
////            std::string fimexLevelName = cdmRef.getVerticalAxis(var.getName());
////            std::string standardlevelname = "NULL";
////            boost::shared_array<double> fimexLevels;
////            if(!fimexLevelName.empty()) {
////                MetNoFimex::CDMDimension levelDim = cdmRef.getDimension(fimexLevelName);
////                MetNoFimex::CDMAttribute levelAttr;
////                if(cdmRef.getAttribute(levelDim.getName(), "standard_name", levelAttr)) {
////                    standardlevelname = zAttr.getStringValue();
////                    boost::algorithm::replace_all(levelname, "_", " ");
////                }
////                lDim = zDim.getLength();
////                MetNoFimex::CDMVariable levelVar = cdmRef.getVariable(levelDim.getName());
////                fimexLevels = levelVar.getData()->asDouble();
////            }

//            for(size_t i = 0; i < levels.size(); i++)
//            {
////                double lvlFrom;
////                double lvlTo;

////                if(zName.empty()) {
////                    std::vector<wdb::load::Level> levels;
////                    levelValues(levels, field);
////                    levelname = levels[i].levelParameter_;
////                    lvlFrom = levels[i].levelFrom_;
////                    lvlTo = levels[i].levelTo_;
////                } else {
////                    lvlFrom = lvls[i];
////                    lvlTo =lvlFrom;
////                }
////                size_t levelIndex = i;

//                std::vector<float> data;
//                size_t wdbLevel = levels[i].levelFrom_;

//                bool found = getValuesForLevel(interpolator, fimexVar, unitInWdb, wdbLevel, data);

//                if(!found) {
//                    continue;
//                } else {
//                    std::cerr << " VAR NAME: "<< fimexVar.getName()
//                              << " CF NAME: " << standardName
//                              << " DATA PROVIDER: " << dataProvider
//                              << " PLACENAME: " << placename
//                              << " REF TIME:" << strReferenceTime
//                              << " VALID FROM:" << strValidTimeFrom
//                              << " VALID TO:" << strValidTimeTo
//                              << " LEVEL NAME: " << levels[i].levelParameter_
//                              << " LEVEL FROM:" << levels[i].levelFrom_
//                              << " LEVEL TO:" << levels[i].levelTo_
//                              << " DATA VERSION:" << dataVersion(field)
//                              << " CONFIDENCE CODE: " << confidenceCode(field)
//                              << " DATA SIZE: " << data.size() << "" << std::endl;

//                    connection_.write(
//                                      data[0],
//                                      placename,
//                                      strReferenceTime,
//                                      strValidTimeFrom,
//                                      strValidTimeTo,
//                                      standardName,
//                                      levels[i].levelParameter_,
//                                      levels[i].levelFrom_,
//                                      levels[i].levelTo_
//                                     );
//                }
//            }
//        } catch ( wdb::ignore_value &e ) {
//            std::cerr << e.what() << " Data field not loaded.";
//        } catch ( std::out_of_range &e ) {
//            std::cerr << "Metadata missing for data value. " << e.what() << " Data field not loaded.";
//        } catch ( std::exception & e ) {
//            std::cerr << e.what() << " Data field not loaded.";
//        }
//    }
} } } // end namespaces
