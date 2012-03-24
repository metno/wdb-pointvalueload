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
#include <fimex/CDMException.h>
#include <fimex/CDMconstants.h>
#include <fimex/CDMDimension.h>
#include <fimex/CDMReaderUtils.h>
#include <fimex/CDMInterpolator.h>
#include <fimex/CDMFileReaderFactory.h>

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

    FileLoader::FileLoader(Loader& controller)
        : controller_(controller)
    {
        if(options().loading().validtimeConfig.empty())
            throw std::runtime_error("Can't open validtime.config file [empty string?]");
        if(options().loading().dataproviderConfig.empty())
            throw std::runtime_error("Can't open dataprovider.config file [empty string?]");
        if(options().loading().valueparameterConfig.empty())
            throw std::runtime_error("Can't open valueparameter.config file [empty string?]");
        if(options().loading().levelparameterConfig.empty())
            throw std::runtime_error("Can't open levelparameter.config file [empty string?]");
        if(options().loading().leveladditionsConfig.empty())
            throw std::runtime_error("Can't open leveladditions.config file [empty string?]");

        point2DataProviderName_.open(getConfigFile(options().loading().dataproviderConfig).file_string());
        point2ValueParameter_.open(getConfigFile(options().loading().valueparameterConfig).file_string());
        point2LevelParameter_.open(getConfigFile(options().loading().levelparameterConfig).file_string());
        point2LevelAdditions_.open(getConfigFile(options().loading().leveladditionsConfig).file_string());
    }

    FileLoader::~FileLoader()
    {
        // NOOP
    }

    bool FileLoader::openDataCDM(const std::string& fileName)
    {
        std::string fType = options().input().type;
        std::string fimexConfig = options().loading().fimexConfig;

        if(fimexConfig.empty())
            throw std::runtime_error(" Can't open fimex reader configuration file!");

        if(!boost::filesystem::exists(fimexConfig))
            throw std::runtime_error(" Fimex configuration file: " + fimexConfig + " doesn't exist!");

        if(fType == "felt")
            cdmData_ = CDMFileReaderFactory::create(MIFI_FILETYPE_FELT, fileName, fimexConfig);
        else if(fType == "grib")
            cdmData_ = CDMFileReaderFactory::create(MIFI_FILETYPE_GRIB, fileName, fimexConfig);
        else
            throw std::runtime_error("Unknow file type!");

//        cdmReader_->getCDM().toXMLStream(std::cerr);
        return true;
    }

    bool FileLoader::interpolate()
    {
        std::string templateFile = options().loading().fimexTemplate;

        if(templateFile.empty())
            return false;
        if(not cdmTemplate().get())
            return false;
        if(not cdmData_.get())
            return false;

        boost::shared_ptr<CDMInterpolator> interpolator = boost::shared_ptr<CDMInterpolator>(new CDMInterpolator(cdmData_));

        interpolator->changeProjection(controller_.interpolatemethod(), templateFile);

        cdmData_ = interpolator;

        return true;
    }

    bool FileLoader::time2string()
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
    }

    void FileLoader::load(const string& fileName)
    {
        openDataCDM(fileName);

        time2string();

        interpolate();

        loadInterpolated(fileName);
    }

} } } // end namespaces
