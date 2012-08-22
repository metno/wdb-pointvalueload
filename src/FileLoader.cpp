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

    FileLoader::FileLoader(Loader& controller)
        : controller_(controller)
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
    }

    FileLoader::~FileLoader()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
    }

    void FileLoader::setup()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
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

        point2DataProviderName_.open(getConfigFile(options().loading().dataproviderConfig).file_string());
        point2ValueParameter_.open(getConfigFile(options().loading().valueparameterConfig).file_string());
        point2LevelParameter_.open(getConfigFile(options().loading().levelparameterConfig).file_string());
        point2LevelAdditions_.open(getConfigFile(options().loading().leveladditionsConfig).file_string());
        point2Units_.open(getConfigFile(options().loading().unitsConfig).file_string());

        //log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
    }

    void FileLoader::readUnit(const string& unitname, float& coeff, float& term)
    {
//        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
//        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        string ret = point2Units_[unitname];
        vector<string> strs;
        boost::split(strs, ret, boost::is_any_of("|"));
        string c = strs.at(0); boost::algorithm::trim(c);
        string t = strs.at(1); boost::algorithm::trim(t);
        coeff = boost::lexical_cast<float>(c);
        term = boost::lexical_cast<float>(t);
//        cout << __FILE__ << " | " << __FUNCTION__ << " @ " << __LINE__ << " : " << " CHECK " << endl;
     }

    bool FileLoader::openCDM(const string& fileName)
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        string fType = options().input().type;
        string fimexConfig = options().loading().fimexConfig;

        if(fType == "felt" or fType == "grib1" or fType == "grib2") {
            if(fimexConfig.empty())
                throw runtime_error(" Can't open fimex reader configuration file (must have for FELT/GRIB1/GRIB2 formats)!");
            if(!boost::filesystem::exists(fimexConfig))
                throw runtime_error(" Fimex configuration file: " + fimexConfig + " doesn't exist!");
        }

        if(fType == "felt")
            cdmData_ = CDMFileReaderFactory::create(MIFI_FILETYPE_FELT, fileName, fimexConfig);
        else if(fType == "grib1" or fType == "grib2")
            cdmData_ = CDMFileReaderFactory::create(MIFI_FILETYPE_GRIB, fileName, fimexConfig);
        else if(fType == "netcdf")
            cdmData_ = CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, fileName);
        else
            throw runtime_error("Unknow file type!");

        return true;
    }

    bool FileLoader::interpolateCDM()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        string templateFile = options().loading().fimexTemplate;

        if(templateFile.empty())
            return false;
        if(not cdmTemplate().get())
            return false;
        if(not cdmData_.get())
            return false;

        boost::shared_ptr<CDMInterpolator> interpolator = boost::shared_ptr<CDMInterpolator>(new CDMInterpolator(cdmData_));

        interpolator->changeProjection(controller_.interpolatemethod(), templateFile);

        cdmData_ = interpolator;

//        cout << __FILE__ << " | " << __FUNCTION__ << " @ " << __LINE__ << " : " << " CHECK " << endl;
        return true;
    }

    bool FileLoader::timeFromCDM()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
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

//        cout << __FILE__ << " | " << __FUNCTION__ << " @ " << __LINE__ << " : " << " CHECK " << endl;

        return true;
    }

    bool FileLoader::processCDM()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        uwinds().clear();
        vwinds().clear();

        if(options().loading().fimexProcessRotateVectorToLatLonX.empty()
                && options().loading().fimexProcessRotateVectorToLatLonY.empty())
            return true;

        boost::shared_ptr<CDMProcessor> processor(new CDMProcessor(cdmData_));

        boost::split(uwinds(), options().loading().fimexProcessRotateVectorToLatLonX, boost::is_any_of(" ,"));
        boost::split(vwinds(), options().loading().fimexProcessRotateVectorToLatLonY, boost::is_any_of(" ,"));

        if(uwinds().size() != vwinds().size())
            throw runtime_error("not the same number of x and y wind components\n");

        processor->rotateVectorToLatLon(true, uwinds(), vwinds());

        cdmData_ = processor;
//         log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        return true;
    }

    void FileLoader::load(const string& fileName)
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        openCDM(fileName);

        processCDM();

        interpolateCDM();

        timeFromCDM();

        loadInterpolated(fileName);
    }

    void FileLoader::loadWindEntries()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        if(uwinds().size() != vwinds().size())
            throw runtime_error("uwinds and veinds sizes don't match");

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

            if(uEntry.unit_ != vEntry.unit_)
                throw runtime_error("units for wind componenets don't match");
            string wdbUnit = uEntry.unit_;

            if(uEntry.provider_ != vEntry.provider_)
                throw runtime_error("providers for wind componenets don't match");
            string provider = uEntry.provider_;

            if(uEntry.levelname_ != vEntry.levelname_)
                throw runtime_error("levelnames for wind componenets don't match");
            string levelname = uEntry.levelname_;

            if(uEntry.levels_ != vEntry.levels_)
                throw runtime_error("levels for wind componenets don't match");
            set<double> levels(uEntry.levels_);

            string xName = cdmData_->getCDM().getHorizontalXAxis(uwinds()[i]);
            string yName = cdmData_->getCDM().getHorizontalYAxis(uwinds()[i]);
            size_t xDimLength = cdmData_->getCDM().getDimension(xName).getLength();
            size_t yDimLength = cdmData_->getCDM().getDimension(yName).getLength();

            const CDMVariable& uVariabe = cdmData_->getCDM().getVariable(uwinds()[i]);
            vector<string> shape(uVariabe.getShape().begin(), uVariabe.getShape().end());
            string lDimName = cdmData_->getCDM().getVerticalAxis(uwinds()[i]);

            boost::shared_ptr<Data> udata = cdmData_->getScaledDataInUnit(uwinds()[i], wdbUnit);
            boost::shared_ptr<Data> vdata = cdmData_->getScaledDataInUnit(vwinds()[i], wdbUnit);
            boost::shared_array<double> uwinddata = udata->asDouble();
            boost::shared_array<double> vwinddata = vdata->asDouble();

            size_t utds = udata->size();
            size_t vtds = vdata->size();
            if(utds != vtds)
                throw runtime_error("datasizes for wind componenets don't match");
            size_t tds = utds;

            EntryToLoad speed;
            speed.name_ = "wind speed";
            speed.unit_ = wdbUnit;
            speed.provider_ = provider;
            speed.levelname_ = levelname;
            speed.levels_ = levels;
            speed.data_ = boost::shared_array<double>(new double[tds]);
            speed.fimexName = "wind_speed";
            speed.fimexShape = shape;
            speed.fimexLevelName = lDimName;
            speed.fimexXDimLength = xDimLength;
            speed.fimexYDimLength = yDimLength;

            EntryToLoad direction;
            direction.name_ = "wind from direction";
            direction.unit_ = "degrees";
            direction.provider_ = provider;
            direction.levelname_ = levelname;
            direction.levels_ = levels;
            direction.data_ = boost::shared_array<double>(new double[tds]);
            direction.fimexName = "wind_from_direction";
            direction.fimexShape = shape;
            direction.fimexLevelName = lDimName;
            direction.fimexXDimLength = xDimLength;
            direction.fimexYDimLength = yDimLength;

            for(size_t t = 0; t < tds; ++t) {
                speed.data_[t] = sqrt(uwinddata[t]*uwinddata[t] + vwinddata[t]*vwinddata[t]);
                double dir = (3/2)*PI - atan2(vwinddata[t], uwinddata[t]);
                while(dir > 2*PI) {
                    dir = dir - 2*PI;
                }
                direction.data_[t] = dir;
            }

            winds.insert(make_pair<string, EntryToLoad>(speed.name_+boost::lexical_cast<string>(i), speed));
            winds.insert(make_pair<string, EntryToLoad>(direction.name_+boost::lexical_cast<string>(i), direction));
        }

        entries2load().clear();

        entries2load() = winds;

        loadEntries();
    }

    void FileLoader::loadEntries()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.pointLoad.FileLoader" );
        log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
        string dataprovider;
        const CDM& cdmRef = cdmData_->getCDM();
//        cdmRef.toXMLStream(cout);

        // eps - realization variable
        size_t epsLength = 1;
        size_t epsMaxVersion = 0;
        string epsVariableName;
        string epsCFName = "realization";

        boost::shared_array<int> realizations;
        const CDMDimension* epsDim = 0;
        if(!cdmRef.findVariables("standard_name", epsCFName).empty()) {
            epsVariableName = cdmRef.findVariables("standard_name", epsCFName)[0];
            epsDim = &cdmRef.getDimension(epsVariableName);
            epsLength = epsDim->getLength();
            realizations = cdmData_->getData(epsVariableName)->asInt();
            epsMaxVersion = realizations[epsLength - 1];
        }

        string strReferenceTime;
	try {
	    strReferenceTime = toString(MetNoFimex::getUniqueForecastReferenceTime(cdmData_));
	} catch (CDMException& cdmex) {
            strReferenceTime = times_[0];
	}

        for(map<string, EntryToLoad>::const_iterator it = entries2load().begin(); it != entries2load().end(); ++it)
        {
//            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";
            const EntryToLoad& entry(it->second);
            string wdbUnit = entry.unit_;
            if(dataprovider != entry.provider_) {
                dataprovider = entry.provider_;
                cout << endl << dataprovider<< '\t' << "88,0,88" <<endl;
            }

//            log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "] CHECK POINT ";

                string fimexname;
                string fimexlevelname;
                size_t fimexXDimLength;
                size_t fimexYDimLength;
                vector<string> fimexshape;
                boost::shared_array<double> values;

                log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]" << " entry.name_: "<< entry.name_ ;
                log.debugStream() <<__FUNCTION__<< " @ line["<< __LINE__ << "]"  << " entry.fimexName: "<< entry.fimexName;

                string fimexstandardname(entry.fimexName.empty() ? entry.name_ : entry.fimexName);
                string wdbstandardname(entry.name_);

                if(entry.data_.get() == 0) {
                    boost::algorithm::replace_all(fimexstandardname, " ", "_");
                    vector<string> variables = cdmRef.findVariables("standard_name", fimexstandardname);
                    if(variables.empty()) {
                        cerr << "cant find vars for fimexstandardname: " << fimexstandardname << endl;
                        continue;
                    } else if(variables.size() > 1) {
                        cerr << "several vars for fimexstandardname: " << fimexstandardname << endl;
                        continue;
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


                    if(!cdmRef.getLatitudeLongitude(fimexVar.getName(), latName, lonName))
                        throw runtime_error("lat and lon not defined for fimexvarname");

                    boost::shared_ptr<MetNoFimex::Data> raw = cdmData_->getScaledDataInUnit(fimexVar.getName(), wdbUnit);
                    if(raw->size() == 0)
                        continue;

                    values = raw->asDouble();

                    // we deal only with variable that are time dependant
                    list<string> dims(fimexVar.getShape().begin(), fimexVar.getShape().end());
                    if(find(dims.begin(), dims.end(), "time") == dims.end()) {
                        cerr << "not time dependent: " << fimexstandardname << endl;
                        continue;
                    }
                    fimexname = fimexVar.getName();
                    fimexshape = fimexVar.getShape();
                    fimexlevelname = cdmRef.getVerticalAxis(fimexname);
                } else {
                    values = entry.data_;
                    fimexname = entry.fimexName;
                    fimexshape = entry.fimexShape;
                    fimexlevelname = entry.fimexLevelName;
                    fimexXDimLength = entry.fimexXDimLength;
                    fimexYDimLength = entry.fimexYDimLength;
                }

                for(size_t i = 0; i < fimexYDimLength; ++i) {
                    for(size_t j = 0; j < fimexXDimLength; ++j){

                        stringstream wkt;
                        wkt <<  "point" << "(" << controller_.longitudes()[i * fimexXDimLength + j] << " " << controller_.latitudes()[i * fimexXDimLength + j] << ")";

                        for(set<double>::const_iterator lIt = entry.levels_.begin(); lIt != entry.levels_.end(); ++lIt) {
                            size_t wdbLevel = *lIt;
                            size_t fimexLevelIndex = 0;
                            size_t fimexLevelLength = 1;
                            if(!fimexlevelname.empty()) {
                                // match wdbIndex to index in fimex data
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

                            if(epsDim != 0) {
                                list<string> dims(fimexshape.begin(), fimexshape.end());
                                if(find(dims.begin(), dims.end(), epsVariableName) != dims.end()) {
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
                                                          << entry.levelname_ << "\t"
                                                          << wdbLevel         << "\t"
                                                          << wdbLevel         << "\t"
                                                          << version          << "\t"
                                                          << epsMaxVersion
                                                          << endl;

                                               if(value != value) {
                                                   // IEEE way tom test for NaN
                                                   cerr << cmd.str();
                                                   continue;
                                               } else {
                                                   controller_.write(cmd.str());
                                               }

                                    } catch ( wdb::ignore_value &e ) {
                                        cerr << e.what() << " Data field not loaded."<< endl;
                                    } catch ( out_of_range &e ) {
                                        cerr << "Metadata missing for data value. " << e.what() << " Data field not loaded." << endl;
                                    } catch ( exception & e ) {
                                        cerr << e.what() << " Data field not loaded." << endl;
                                    }
                            } // time slices end
                        } // eps slices
                    } // z slices
                } // x slices
            } // y slices
        } // entries2load
    }

} } } // end namespaces
