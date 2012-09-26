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
#include "FileLoader.hpp"
#include "FeltLoader.hpp"
#include "GribLoader.hpp"
#include "NetCDFLoader.hpp"

// libfimex
#include <fimex/CDM.h>
#include <fimex/CDMReader.h>
#include <fimex/CDMExtractor.h>
#include <fimex/CDMProcessor.h>
#include <fimex/CDMException.h>
#include <fimex/CDMconstants.h>
#include <fimex/CDMDimension.h>
#include <fimex/CDMReaderUtils.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/CDMFileReaderFactory.h>

// wdb
#include <wdbException.h>
#include <wdbLogHandler.h>

// boost
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

using namespace std;
using namespace boost::posix_time;
using namespace boost::filesystem;
using namespace MetNoFimex;

#define PI 3.14159265

namespace {

    path getConfigFile(const path& fileName)
    {
        static const path sysConfDir = "";//./etc/";//SYSCONFDIR;
        path confPath = sysConfDir/fileName;
        return confPath;
    }

    string toString(const boost::posix_time::ptime & time )
    {
        if ( time == boost::posix_time::ptime(neg_infin) )
            return "-infinity"; //"1900-01-01 00:00:00+00";
        else if ( time == boost::posix_time::ptime(pos_infin) )
            return "infinity";//"2100-01-01 00:00:00+00";
        // ...always convert to zulu time
        string ret = to_iso_extended_string(time) + "+00";
        return ret;
    }
}

namespace wdb { namespace load { namespace point {

    FileLoader::FileLoader(Loader& controller) : controller_(controller) { }

    FileLoader::~FileLoader() { }

    void FileLoader::setup()
    {
        // check for excess parameters
        if(!options().loading().valueparameter2Config.empty())
            throw std::runtime_error("valueparameter2.config file not required");
        if(!options().loading().levelparameter2Config.empty())
            throw std::runtime_error("levelparameter2.config file not required");
        if(!options().loading().leveladditions2Config.empty())
            throw std::runtime_error("Can't open leveladditions2.config file [empty string?]");

        if(options().loading().validtimeConfig.empty())
            throw runtime_error("Can't open validtime.config file [empty string?]");
        if(options().loading().dataproviderConfig.empty())
            throw runtime_error("Can't open dataprovider.config file [empty string?]");
        if(options().loading().valueparameterConfig.empty())
            throw runtime_error("Can't open valueparameter.config file [empty string?]");
        if(options().loading().levelparameterConfig.empty())
            throw runtime_error("Can't open levelparameter.config file [empty string?]");
        if(options().loading().leveladditionsConfig.empty())
            throw runtime_error("Can't open leveladditions.config file [empty string?]");
        if(options().loading().unitsConfig.empty())
            throw runtime_error("Can't open units.config file [empty string?]");

        point2DataProviderName_.open(getConfigFile(options().loading().dataproviderConfig).string());
        point2ValueParameter_.open(getConfigFile(options().loading().valueparameterConfig).string());
        point2LevelParameter_.open(getConfigFile(options().loading().levelparameterConfig).string());
        point2LevelAdditions_.open(getConfigFile(options().loading().leveladditionsConfig).string());
        point2Units_.open(getConfigFile(options().loading().unitsConfig).string());
    }

    // find the unit applicable for WDB - reads units.conf file
    void FileLoader::readUnit(const string& unitname, float& coeff, float& term)
    {
        string ret = point2Units_[unitname];
        ret = (ret == "none") ? "1" : ret;
        vector<string> strs;
        boost::split(strs, ret, boost::is_any_of("|"));
        string c = strs.at(0); boost::algorithm::trim(c);
        string t = strs.at(1); boost::algorithm::trim(t);
        coeff = boost::lexical_cast<float>(c);
        term = boost::lexical_cast<float>(t);
     }

    // using fimex and template interpolation
    // to get the parameter values in predefined
    // geographical points
    bool FileLoader::interpolateCDM()
    {
        string templateFile = options().loading().fimexTemplate;

        // check & open template file
        if(templateFile.empty())
            return false;
        if(not cdmTemplate().get())
            return false;
        if(not cdmData_.get())
            return false;

        boost::shared_ptr<CDMInterpolator> interpolator = boost::shared_ptr<CDMInterpolator>(new CDMInterpolator(cdmData_));

        // interpolate in specific geographical (lat/lon) points
        interpolator->changeProjection(controller_.interpolatemethod(), templateFile);

        // set the interpolated structure
        // as new CDMReader source
        cdmData_ = interpolator;

        return true;
    }

    // extract time axis - used to set valid from & valid to times
    bool FileLoader::timeFromCDM()
    {
        const CDM& cdmRef = cdmData_->getCDM();
        const CDMDimension* unlimited = cdmRef.getUnlimitedDim();
        if(unlimited == 0)
            return false;

        size_t uDim = unlimited->getLength();

        boost::shared_array<unsigned long long> uValues =
                cdmData_->getScaledDataInUnit(unlimited->getName(), "seconds since 1970-01-01 00:00:00 +00:00")->asUInt64();

        for(size_t u = 0; u < uDim; ++u) {
            times_.push_back(boost::posix_time::to_iso_extended_string(boost::posix_time::from_time_t(uValues[u])) + "+00");
        }

        return true;
    }

    // read the u and v wind component names (when rotated)
    // uses CDMProcessor::rotateVectorToLatLon
    bool FileLoader::processCDM()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );

        uwinds().clear();
        vwinds().clear();

        if(options().loading().fimexProcessRotateVectorToLatLonX.empty()
                && options().loading().fimexProcessRotateVectorToLatLonY.empty())
            return true;

        // create CDMProcessor
        boost::shared_ptr<CDMProcessor> processor(new CDMProcessor(cdmData_));

        // create CDMInterpolator as we need to interpolate processor as well
        boost::shared_ptr<CDMInterpolator> interpolator = boost::shared_ptr<CDMInterpolator>(new CDMInterpolator(processor));

        string templateFile = options().loading().fimexTemplate;

        if(templateFile.empty())
            return false;
        if(not cdmTemplate().get())
            return false;

        // apply template interpolation
        interpolator->changeProjection(controller_.interpolatemethod(), templateFile);

        boost::split(uwinds(), options().loading().fimexProcessRotateVectorToLatLonX, boost::is_any_of(" ,"));
        boost::split(vwinds(), options().loading().fimexProcessRotateVectorToLatLonY, boost::is_any_of(" ,"));

        processor->rotateVectorToLatLon(true, uwinds(), vwinds());

        if(uwinds().size() != vwinds().size())
            throw runtime_error("not the same number of x and y wind components\n");

        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << " u wind size: " << uwinds().size();
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] " << " v wind size: " << vwinds().size();

        cdmData_ = processor;

        return true;
    }

    /*
     * Describes steps used to extract point data
     **/
    void FileLoader::load(const string& fileName)
    {
        // create CDMReader for the input file
        // some fule types need fimex reader xml config file
        openCDM(fileName);

        // if requested rotate wind components
        // to recalculate wind_speed and wind_direction
        processCDM();

        // use fimex and template interpolation to
        // interpolase CDMReader in wanted points
        interpolateCDM();

        // extract time axis values
        timeFromCDM();

        // read input file and check config files
        // to determine what parameters are to
        // be loaded and how are the mapped in wdb
        loadInterpolated(fileName);
    }

    /*
     * If requested it will extract the u and v wind components
     * to calculate wind_speed and wind_direction as prescribed
     * by met.no scientis
     * To make this happen, data must have u and w components
     * and fimex.process.rotateVectorToLatLonX, fimex.process.rotateVectorToLatLonY
     * command options have to hold fimex points for the u and w components
     **/
    // it creates 2 virtual parameters - u and w components
    void FileLoader::loadWindEntries()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );

        if(uwinds().size() != vwinds().size())
            throw runtime_error("uwinds and vWinds sizes don't match");

        map<string, EntryToLoad> winds;

        for(size_t i = 0; i < uwinds().size(); ++i) {
            CDMAttribute uAtt;
            cdmData_->getCDM().getAttribute(uwinds()[i], "standard_name", uAtt);
            string ucfname = uAtt.getStringValue();
            boost::algorithm::replace_all(ucfname, "_", " ");

            CDMAttribute vAtt;
            cdmData_->getCDM().getAttribute(vwinds()[i], "standard_name", vAtt);
            string vcfname = vAtt.getStringValue();
            boost::algorithm::replace_all(vcfname, "_", " ");

            if(entries2load().find(ucfname) == entries2load().end())
                throw runtime_error("can't find u wind entry");
            if(entries2load().find(vcfname) == entries2load().end())
                throw runtime_error("can't find v wind entry");

            EntryToLoad uEntry = entries2load()[ucfname];
            EntryToLoad vEntry = entries2load()[vcfname];

            if(uEntry.wdbUnit_ != vEntry.wdbUnit_)
                throw runtime_error("units for wind componenets don't match");
            // some configuration files have "none" as units
            // for Fimex this should be "1"
            string wdbunit = (uEntry.wdbUnit_ == "none") ? "1" : uEntry.wdbUnit_;

            if(uEntry.wdbDataProvider_ != vEntry.wdbDataProvider_)
                throw runtime_error("providers for wind componenets don't match");
            string wdbdataprovider = uEntry.wdbDataProvider_;

            if(uEntry.wdbLevelName_ != vEntry.wdbLevelName_)
                throw runtime_error("levelnames for wind componenets don't match");
            string levelname = uEntry.wdbLevelName_;

            if(uEntry.wdbLevels_ != vEntry.wdbLevels_)
                throw runtime_error("levels for wind componenets don't match");
            set<double> levels(uEntry.wdbLevels_);

            string xName = cdmData_->getCDM().getHorizontalXAxis(uwinds()[i]);
            string yName = cdmData_->getCDM().getHorizontalYAxis(uwinds()[i]);
            size_t xDimLength = cdmData_->getCDM().getDimension(xName).getLength();
            size_t yDimLength = cdmData_->getCDM().getDimension(yName).getLength();

            const CDMVariable& uVariabe = cdmData_->getCDM().getVariable(uwinds()[i]);
            vector<string> shape(uVariabe.getShape().begin(), uVariabe.getShape().end());
            string lDimName = cdmData_->getCDM().getVerticalAxis(uwinds()[i]);

            boost::shared_ptr<Data> udata = cdmData_->getScaledDataInUnit(uwinds()[i], wdbunit);
            boost::shared_ptr<Data> vdata = cdmData_->getScaledDataInUnit(vwinds()[i], wdbunit);
            boost::shared_array<double> uwinddata = udata->asDouble();
            boost::shared_array<double> vwinddata = vdata->asDouble();

            size_t utds = udata->size();
            size_t vtds = vdata->size();
            if(utds != vtds)
                throw runtime_error("datasizes for wind componenets don't match");
            size_t tds = utds;

            EntryToLoad speed;
            speed.wdbName_ = "wind speed";
            speed.cdmName_ = "wind_speed";
            speed.standardName_ = "wind_speed";
            speed.wdbUnit_ = wdbunit;
            speed.wdbDataProvider_ = wdbdataprovider;
            speed.wdbLevelName_ = levelname;
            speed.wdbLevels_ = levels;
            speed.cdmData_ = boost::shared_array<double>(new double[tds]);
            speed.cdmShape_ = shape;
            speed.cdmLevelName_ = lDimName;
            speed.cdmXDimLength_ = xDimLength;
            speed.cdmYDimLength_ = yDimLength;

            EntryToLoad direction;
            direction.wdbName_ = "wind from direction";
            direction.cdmName_ = "wind_from_direction";
            direction.standardName_ = "wind_from_direction";
            direction.wdbUnit_ = "degrees";
            direction.wdbDataProvider_ = wdbdataprovider;
            direction.wdbLevelName_ = levelname;
            direction.wdbLevels_ = levels;
            direction.cdmData_ = boost::shared_array<double>(new double[tds]);
            direction.cdmShape_ = shape;
            direction.cdmLevelName_ = lDimName;
            direction.cdmXDimLength_ = xDimLength;
            direction.cdmYDimLength_ = yDimLength;

            for(size_t t = 0; t < tds; ++t) {
                speed.cdmData_[t] = sqrt(uwinddata[t]*uwinddata[t] + vwinddata[t]*vwinddata[t]);
                double dir = (3/2)*PI - atan2(vwinddata[t], uwinddata[t]);
                while(dir > 2*PI) {
                    dir = dir - 2*PI;
                }
                direction.cdmData_[t] = dir;
            }

            winds.insert(make_pair<string, EntryToLoad>(speed.wdbName_+boost::lexical_cast<string>(i), speed));
            winds.insert(make_pair<string, EntryToLoad>(direction.wdbName_+boost::lexical_cast<string>(i), direction));
        }

        // clear Entry2Load items
        entries2load().clear();

        // insert wind related param as
        // new Entry2Load items
        entries2load() = winds;

        loadEntries();
    }

    /* iterates througth Entry2Load items
     * reads data from (earlier interpolated) CDMReader
     * for selected item iterates:
     * each geo positions (x, y)
     * each time step (valid from - valid to)
     * selected levels
     * each ensemble member - where applicable
     **/
    void FileLoader::loadEntries()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );

        string dataprovider;
        const CDM& cdmRef = cdmData_->getCDM();

        // eps - realization variable
        size_t epsLength = 1;
        size_t epsMaxVersion = 0;
        string epsVariableName;
        string epsCFName = "realization";

        // 1. check if there are ensemble members
        boost::shared_array<int> realizations;
        const CDMDimension* epsDim = 0;
        if(!cdmRef.findVariables("standard_name", epsCFName).empty()) {
            epsVariableName = cdmRef.findVariables("standard_name", epsCFName)[0];
            epsDim = &cdmRef.getDimension(epsVariableName);
            epsLength = epsDim->getLength();
            realizations = cdmData_->getData(epsVariableName)->asInt();
            epsMaxVersion = realizations[epsLength - 1];
        }

        // 2. extract unique reference time
        string strReferenceTime;
	try {
	    strReferenceTime = toString(MetNoFimex::getUniqueForecastReferenceTime(cdmData_));
	} catch (CDMException& cdmex) {
            strReferenceTime = times_[0];
	}

        // 3. iterate Entry2Load map
        for(map<string, EntryToLoad>::const_iterator it = entries2load().begin(); it != entries2load().end(); ++it)
        {
            const EntryToLoad& entry(it->second);
            // some configuration files have "none" as units
            // for Fimex this should be "1"
            string wdbunit = (entry.wdbUnit_ == "none") ? "1" : entry.wdbUnit_;
            // 4. write data privder on a separate line (required bywdb-fastload format)
            if(dataprovider != entry.wdbDataProvider_) {
                dataprovider = entry.wdbDataProvider_;
                string dpLine = "\n" + dataprovider + "\t88,0,88\n";
                controller_.write(dpLine);
            }

            string fimexname;
            string fimexlevelname;
            size_t fimexXDimLength;
            size_t fimexYDimLength;
            vector<string> fimexshape;
            boost::shared_array<double> values;

            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]" << " entry.wdbName_: "<< entry.wdbName_ ;
            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]"  << " entry.cdmName_: "<< entry.cdmName_;

            string fimexstandardname(entry.standardName_);
            string wdbstandardname(entry.wdbName_);

            // 5. see if we have already fetched data for param from CDMReader
            if(entry.cdmData_.get() == 0) {
                boost::algorithm::replace_all(fimexstandardname, " ", "_");
                vector<string> variables = cdmRef.findVariables("standard_name", fimexstandardname);
                if(variables.empty()) {
                    log.infoStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]" << "cant find vars for fimexstandardname: " << fimexstandardname;
                    continue;
                } else if(variables.size() > 1) {
                    stringstream ss;
                    ss << "several vars for fimexstandardname: " << fimexstandardname;
                    log.errorStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]" << ss.str();
                    throw std::runtime_error(ss.str());
                }
                const MetNoFimex::CDMVariable fimexVar = cdmRef.getVariable(variables[0]);
                if(find(uwinds().begin(), uwinds().end(), fimexVar.getName()) != uwinds().end()
                        ||
                        find(vwinds().begin(), vwinds().end(), fimexVar.getName()) != vwinds().end())
                {
                    continue;
                }

                string lonName;
                string latName;
                string xName = cdmRef.getHorizontalXAxis(fimexVar.getName());
                string yName = cdmRef.getHorizontalYAxis(fimexVar.getName());
                fimexXDimLength = cdmRef.getDimension(xName).getLength();
                fimexYDimLength = cdmRef.getDimension(yName).getLength();


                if(!cdmRef.getLatitudeLongitude(fimexVar.getName(), latName, lonName)) {
                    stringstream ss;
                    ss << "lat and lon not defined for fimex varName: " << fimexVar.getName();
                    throw runtime_error(ss.str());
                }

                boost::shared_ptr<MetNoFimex::Data> raw = cdmData_->getScaledDataInUnit(fimexVar.getName(), wdbunit);
                if(raw->size() == 0)
                    continue;

                values = raw->asDouble();

                // we deal only with variable that are time dependant
                list<string> dims(fimexVar.getShape().begin(), fimexVar.getShape().end());
                if(find(dims.begin(), dims.end(), "time") == dims.end()) {
                    log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]" << "not time dependent: " << fimexstandardname;
                    continue;
                }
                fimexname = fimexVar.getName();
                fimexshape = fimexVar.getShape();
                fimexlevelname = cdmRef.getVerticalAxis(fimexname);
            } else {
                values = entry.cdmData_;
                fimexname = entry.cdmName_;
                fimexshape = entry.cdmShape_;
                fimexlevelname = entry.cdmLevelName_;
                fimexXDimLength = entry.cdmXDimLength_;
                fimexYDimLength = entry.cdmYDimLength_;
            }

            // 6. iterate by each position
            // X dim grows faster than Y dim
            for(size_t i = 0; i < fimexYDimLength; ++i) {
                for(size_t j = 0; j < fimexXDimLength; ++j){

                    stringstream wkt;
                    wkt <<  "point" << "(" << controller_.longitudes()[i * fimexXDimLength + j] << " " << controller_.latitudes()[i * fimexXDimLength + j] << ")";

                    // 7. iterate all requested levels (as confgured by levelparameter.conf and/or leveladditions.conf)
                    for(set<double>::const_iterator lIt = entry.wdbLevels_.begin(); lIt != entry.wdbLevels_.end(); ++lIt) {
                        size_t wdbLevel = *lIt;
                        size_t fimexLevelIndex = 0;
                        size_t fimexLevelLength = 1;
                        if(!fimexlevelname.empty()) {
                            // match wdbIndex to index in fimex CDMReader (that is CDM modell)
                            MetNoFimex::CDMVariable fimexLevelVar = cdmRef.getVariable(fimexlevelname);
                            fimexLevelLength = cdmRef.getDimension(fimexlevelname).getLength();
                            boost::shared_ptr<Data> levelData = cdmData_->getData(fimexLevelVar.getName());
                            boost::shared_array<double> fimexLevels = levelData->asDouble();
                            for(size_t index = 0; index < fimexLevelLength; ++ index) {
                                if(wdbLevel == fimexLevels[index]) {
                                    fimexLevelIndex = index;
                                    break;
                                }
                            }
                        }

                        int version = 0;
                        bool hasEpsAsDim = false;

                        // 8. log the number of ensemble members (default 1)
                        if(epsDim != 0) {
                            list<string> dims(fimexshape.begin(), fimexshape.end());
                            if(find(dims.begin(), dims.end(), epsVariableName) != dims.end()) {
                                hasEpsAsDim = true;
                            }
                        } else {
                            version = 0;
                            epsLength = 1;
                        }

                        // 9. iterate each eps member
                        for(size_t e = 0; e < epsLength; ++e)
                        {
                            // 10. time by time slice
                            for(size_t u = 0; u < times().size(); ++u)
                            {
                                double value;

                                if(hasEpsAsDim) {
                                    version = realizations[e];

                                    value = *
                                            (values.get() // jump to data start
                                             + u*(epsLength * fimexLevelLength * fimexXDimLength * fimexYDimLength ) // jump to u-th time slice
                                             + e * fimexLevelLength * fimexXDimLength * fimexYDimLength // jump to e-th realization-slice
                                             + fimexLevelIndex * fimexXDimLength * fimexYDimLength // jump to right level
                                             + i * fimexXDimLength + j); // jump to right x,y coordinate

                                } else {
                                    version = 0;

                                    value = *
                                            (values.get() // jump to data start
                                             + u*(fimexLevelLength * fimexXDimLength * fimexYDimLength ) // jump to u-th time slice
                                             + fimexLevelIndex * fimexXDimLength * fimexYDimLength // jump to right level
                                             + i * fimexXDimLength + j); // jump to right x,y coordinate

                                }

                                string validtime = times()[u];

                                try {
                                    stringstream cmd;

                                    cmd << value            << "\t"
                                        << wkt.str()        << "\t"
                                        << strReferenceTime << "\t"
                                        << validtime        << "\t"
                                        << validtime        << "\t"
                                        << wdbstandardname     << "\t"
                                        << entry.wdbLevelName_ << "\t"
                                        << wdbLevel         << "\t"
                                        << wdbLevel         << "\t"
                                        << version          << "\t"
                                        << epsMaxVersion
                                        << endl;

                                    if(value != value) {
                                        // IEEE way tom test for NaN
                                        log.debugStream() << cmd.str();
                                        continue;
                                    } else {
                                        controller_.write(cmd.str());
                                    }

                                } catch ( wdb::ignore_value &e ) {
                                    log.warnStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]" << e.what() << " Data field not loaded.";
                                } catch ( out_of_range &e ) {
                                    log.warnStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]" << "Metadata missing for data value. " << e.what() << " Data field not loaded.";
                                } catch ( exception & e ) {
                                    log.warnStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]" << e.what() << " Data field not loaded.";
                                }
                            } // time slices end
                        } // eps slices
                    } // z slices
                } // x slices
            } // y slices
        } // entries2load
    }

    // TODO: Remove FileLoaderFactory to it's own file
    //       This way we can cut on dependency with specialized classes
    FileLoader *FileLoaderFactory::createFileLoader(const std::string &type, class Loader& controller)
    {
        if(type == "felt") {
            return new FeltLoader(controller);
        } else if(type == "grib1" or type == "grib2") {
            return new GribLoader(controller);
        } else if(type == "netcdf") {
            return new NetCDFLoader(controller);
        } else {
            stringstream ss;
            ss << "Unrecognized input file type: " << type;
            throw runtime_error(ss.str());
        }
    }

} } } // end namespaces
