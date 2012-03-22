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
#include "CmdLine.hpp"
#include "CfgFileReader.hpp"
#include "WdbConnection.hpp"

// wdb
#include <wdbLogHandler.h>
#include <wdb/WdbLevel.h>
#include <WdbConfigFile.h>
#include <wdb/LoaderConfiguration.h>
#include <wdb/LoaderDatabaseConnection.h>

// libfelt
#include <felt/FeltFile.h>
#include <felt/FeltField.h>
#include <felt/FeltConstants.h>

// libfimex
#include <fimex/CDMInterpolator.h>

// boost
#include <boost/date_time/posix_time/posix_time_types.hpp>

// std
#include <vector>
#include <tr1/unordered_map>

namespace MetNoFimex {
    class CDMReader;
}

namespace wdb { namespace load { namespace point {

    class Loader;

    class FeltLoader
    {
    public:
        FeltLoader(Loader& controller);
        ~FeltLoader();

        void load(const felt::FeltFile& feltFile);

    private:

        bool openDataCDM(const std::string& file, const std::string& fimexCfg);
        bool interpolate(const std::string& templateFile);
        void loadInterpolated(const felt::FeltFile& feltFile);

        const CmdLine& options() { return controller_.options(); }
        WdbConnection& wdbConnection() {  return controller_.wdbConnection();  }
        boost::shared_ptr<MetNoFimex::CDMReader> cdmTemplate() { return controller_.cdmTemplate(); }
        const vector<string>& placenames() { return controller_.placenames(); }
        const set<string>& stations2load() { return controller_.stations2load(); }

        std::string dataProviderName(const felt::FeltField& field);
        boost::posix_time::ptime referenceTime(const felt::FeltField & field);
        boost::posix_time::ptime validTimeFrom(const felt::FeltField & field);
        boost::posix_time::ptime validTimeTo(const felt::FeltField & field);
        std::string valueParameterName(const felt::FeltField & field);
        std::string valueParameterUnit(const felt::FeltField & field);
        void levelValues( std::vector<wdb::load::Level>& levels, const felt::FeltField& field);
        int dataVersion(const felt::FeltField & field);

        Loader& controller_;

        // CDMReader for data that will be interpolated
        boost::shared_ptr<MetNoFimex::CDMReader> cdmData_;

        // format time to string
        std::vector<std::string> times_;
        bool time2string();
        const std::vector<std::string>& times() { return times_;}

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
    };

} } }  // end namepsaces
#endif /*POINTFELTLOADER_H_*/
