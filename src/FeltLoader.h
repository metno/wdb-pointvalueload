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

#ifndef POINTFELTLOADER_H_
#define POINTFELTLOADER_H_

// project
//
#include "CmdLine.h"
#include "CfgFileReader.h"
#include "DBConnection.hpp"

// wdb
//
#include <wdbLogHandler.h>
#include <wdb/WdbLevel.h>
#include <WdbConfigFile.h>
#include <wdb/LoaderConfiguration.h>
#include <wdb/LoaderDatabaseConnection.h>

// libfelt
//
#include <felt/FeltFile.h>
#include <felt/FeltField.h>
#include <felt/FeltConstants.h>

// libfimex
//
#include <fimex/CDMInterpolator.h>

// boost
//
#include <boost/date_time/posix_time/posix_time_types.hpp>

// std
//
#include <vector>
#include <tr1/unordered_map>

namespace MetNoFimex {
    class CDMReader;
}

namespace wdb { namespace load { namespace point {

    class FeltLoader
    {
    public:
        FeltLoader(DBConnection& connection, const CmdLine& cmdLine);
        ~FeltLoader();

        void load(const felt::FeltFile& feltFile);

    private:

        bool openTemplateCDM(const std::string& fileName);
        bool openDataCDM(const std::string& fileName, const std::string& fimexCfgFileName);
        bool interpolate(const std::string& templateFileName);
        bool extractBounds();
        bool extractPointIds();
        bool extractData();
        void loadInterpolated(const felt::FeltFile& feltFile);

        std::string dataProviderName(const felt::FeltField& field);
        boost::posix_time::ptime referenceTime(const felt::FeltField & field);
        boost::posix_time::ptime validTimeFrom(const felt::FeltField & field);
        boost::posix_time::ptime validTimeTo(const felt::FeltField & field);
        std::string valueParameterName(const felt::FeltField & field);
        std::string valueParameterUnit(const felt::FeltField & field);
        void levelValues( std::vector<wdb::load::Level>& levels, const felt::FeltField& field);
        int dataVersion(const felt::FeltField & field);
        int confidenceCode(const felt::FeltField & field);

	/// The Database Connection
        DBConnection& connection_;

        /// The Loader cmd line options
        const CmdLine& options_;

        /// Conversion Hash Map - Config Path Name
        CfgFileReader mainCfg_;
	/// Conversion Hash Map - Dataprovider Name
        CfgFileReader point2DataProviderName_;
	/// Conversion Hash Map - Value Parameter
        CfgFileReader point2ValidTime_;
	/// Conversion Hash Map - Value Parameter
        CfgFileReader point2ValueParameter_;
	/// Conversion Hash Map - Level Parameter
        CfgFileReader point2LevelParameter_;
	/// Conversion Hash Map - Level Additions
        CfgFileReader point2LevelAdditions_;
        /// Conversion Hash Map - ID -> (lon lat)
        CfgFileReader point2StationAdditions_;

        /// CDMReader for template used in interpolation
        boost::shared_ptr<MetNoFimex::CDMReader> cdmTemplate_;
        /// CDMReader for data that will be interpolated
        boost::shared_ptr<MetNoFimex::CDMReader> cdmData_;

        /// bounds used in to limit interpolation
        double northBound_;
        double southBound_;
        double westBound_;
        double eastBound_;

        /// point ids found in cdm template
        boost::shared_array<unsigned int> pointids_;
        std::vector<std::string>  placenames_;

        /// precalculate to string time axis
        std::vector<std::string> times_;
    };

} } }  // end namepsaces
#endif /*POINTFELTLOADER_H_*/
