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
//
#include <GridGeometry.h>

// libfelt
//
#include <felt/FeltFile.h>

// libfimex
//
#include <fimex/CDM.h>
#include <fimex/CDMReader.h>
#include <fimex/CDMExtractor.h>
#include <fimex/CDMException.h>
#include <fimex/CDMconstants.h>
#include <fimex/CDMDimension.h>
#include <fimex/CDMReaderUtils.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/CDMFileReaderFactory.h>

//#include <fimex/NetCDF_CDMReader.h>

// libpqxx
//
#include <pqxx/util>

// boost
//
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>

// std
//
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

    GribLoader::GribLoader(WdbConnection& wdbConnection, const CmdLine& cmdLine) //, WdbLogHandler& logHandler)
        : connection_(wdbConnection), options_(cmdLine) //, logHandler_(logHandler)
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
        if(cmdLine.loading().valueparameter2Config.empty())
            throw std::runtime_error("Can't open valueparameter2.config file [empty string?]");
        if(cmdLine.loading().levelparameter2Config.empty())
            throw std::runtime_error("Can't open levelparameter2.config file [empty string?]");
        if(cmdLine.loading().leveladditions2Config.empty())
            throw std::runtime_error("Can't open leveladditions2.config file [empty string?]");

        point2DataProviderName_.open(getConfigFile(cmdLine.loading().dataproviderConfig).file_string());
        point2ValueParameter1_.open(getConfigFile(cmdLine.loading().valueparameterConfig).file_string());
        point2LevelParameter1_.open(getConfigFile(cmdLine.loading().levelparameterConfig).file_string());
        point2LevelAdditions1_.open(getConfigFile(cmdLine.loading().leveladditionsConfig).file_string());
        point2ValueParameter2_.open(getConfigFile(cmdLine.loading().valueparameter2Config).file_string());
        point2LevelParameter2_.open(getConfigFile(cmdLine.loading().levelparameter2Config).file_string());
        point2LevelAdditions2_.open(getConfigFile(cmdLine.loading().leveladditions2Config).file_string());

    }

    GribLoader::~GribLoader()
    {
        // NOOP
    }

    bool GribLoader::openTemplateCDM(const std::string& fileName)
    {
        if(fileName.empty())
            throw std::runtime_error(" Can't open template interpolation file! ");

        cdmTemplate_ =
                MetNoFimex::CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, fileName);

        assert(extractPointIds());

//        cdmTemplate_->getCDM().toXMLStream(std::cerr);
        return true;
    }

    bool GribLoader::openDataCDM(const std::string& fileName, const std::string& fimexCfgFileName)
    {
        if(fimexCfgFileName.empty())
            throw std::runtime_error(" Can't open fimex reader configuration file!");

        cdmData_ =
                MetNoFimex::CDMFileReaderFactory::create(MIFI_FILETYPE_GRIB, fileName, fimexCfgFileName);

//        cdmReader_->getCDM().toXMLStream(std::cerr);
        return true;
    }

    bool GribLoader::extractPointIds()
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

    bool GribLoader::extractBounds()
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

    bool GribLoader::extractData()
    {
        extractBounds();

        boost::shared_ptr<MetNoFimex::CDMExtractor>
                extractor(new MetNoFimex::CDMExtractor(cdmData_));
        extractor->reduceLatLonBoundingBox(southBound_, northBound_, westBound_, eastBound_);

        cdmData_ = extractor;

//        cdmReader_->getCDM().toXMLStream(std::cerr);
        return true;
    }

    bool GribLoader::interpolate(const std::string& templateFileName)
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

    void GribLoader::load(GribFile& file)
    {
        std::string gribFileName = file.fileName();
        std::string tmplFileName = options_.loading().fimexTemplate;
        std::string fimexCfgFileName = options_.loading().fimexConfig;

        openTemplateCDM(tmplFileName);

        openDataCDM(gribFileName, fimexCfgFileName);

//        extractData();

        assert(interpolate(tmplFileName));

        loadInterpolated(file);
    }

    void GribLoader::loadInterpolated(GribFile& file)
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

        // Get first field, and check if it exists
        GribFile::Field gribField = file.next();
        if(!gribField) {
            // If the file is empty, we need to throw an error
            std::string errorMessage = "End of file was hit before a product was read into file ";
            errorMessage += file.fileName();
            throw std::runtime_error( errorMessage );
        }

//        int failCount = 0;
//        int fieldNumber = -1;
        long long inserts = 0;
        std::map<std::string, EntryToLoad> entries;

        for( ; gribField; gribField = file.next())
        {
//            logHandler_.setObjectNumber(fieldNumber);
            try{
                std::map<std::string, EntryToLoad>::iterator eIt;

                const GribField& field = *gribField;
                std::cerr << field.toString() << std::endl;
                editionNumber_ = editionNumber(field);
                std::string name = valueParameterName(field);
                if(name == "snow density") {
                    std::string unit = valueParameterUnit(field);
                    std::string provider = dataProviderName(field);
                    std::vector<Level> levels;
                    levelValues(levels, field);
                }
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
                WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
                log.infoStream() << e.what() << " Data field not loaded.";
            } catch ( wdb::missing_metadata &e ) {
                WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
                log.warnStream() << e.what() << " Data field not loaded.";
            } catch ( std::exception & e ) {
                WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
                log.errorStream() << e.what() << " Data field not loaded.";
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
                        // do point by point for same value parameter
//                        std::cerr << "LEVELS SIZE .... "<< levels.size() << std::endl;
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

                            // time by time slice
                            for(size_t e = 0; e < epsLength; ++e)
                            {
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
                                            std::cerr << ++inserts << std::endl;
                                            std::cerr<<"*";
                                            std::cerr << " VAR NAME: "<< varname
                                                      << " CF NAME: " << standardName
                                                      << " DATA PROVIDER: " << dataProvider
                                                      << " PLACENAME: " << placename
                                                      << " REF TIME:" << strReferenceTime
                                                      << " VALID FROM:" << validtime
                                                      << " VALID TO:" << validtime
                                                      << " LEVEL NAME: " << entry.levelname_
                                                      << " LEVEL FROM:" << wdbLevel
                                                      << " LEVEL TO:" << wdbLevel
                                                      << " VERSION: " << version
                                                         //                                              << " DATA VERSION:" << dataVersion(**it)
                                                         //                                              << " CONFIDENCE CODE: " << confidenceCode(**it)
                                                      << " VALUE: " << value << ""
                                                      << std::endl;
                                        } else {

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
//                                        std::cerr << e.what() << " Data field not loaded.";
                                    } catch ( std::out_of_range &e ) {
//                                        std::cerr << "Metadata missing for data value. " << e.what() << " Data field not loaded.";
                                    } catch ( std::exception & e ) {
//                                        std::cerr << e.what() << " Data field not loaded.";
                                    }

                                }
                            }
                        }
                    }
                }


        }

//         std::cerr<<"Num of INSERTS: "<<inserts<<std::endl;
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
                ret = point2ValueParameter1_[keyStr.str()];
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
                ret = point2ValueParameter1_[keyStr.str()];
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

    void GribLoader::levelValues( std::vector<wdb::load::Level> & levels, const GribField & field ) const
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.grib.gribloader" );
        bool ignored = false;
        stringstream keyStr;
        std::string ret;
        try {
            if (editionNumber_ == 1) {
                keyStr << field.getLevelParameter1();
                std::cerr << __FUNCTION__ << " field.getLevelParameter1() "<<keyStr.str() << std::endl;
                ret = point2LevelParameter1_[keyStr.str()];
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
            connection_.readUnit( levelUnit, &coeff, &term );
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
                ret = point2LevelAdditions1_[keyStr.str()];
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
