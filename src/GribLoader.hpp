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

#ifndef POINTGRIBLOADER_H_
#define POINTGRIBLOADER_H_

// project
#include "CmdLine.hpp"
#include "CfgFileReader.hpp"
#include "WdbConnection.hpp"

// wdb
#include <wdb/WdbLevel.h>
#include <WdbConfigFile.h>
#include <wdbLogHandler.h>
#include <wdb/LoaderDatabaseConnection.h>

// libfimex
//
#include <fimex/CDMReader.h>

// boost
#include <boost/shared_array.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

// std
#include <vector>

namespace wdb { namespace load { namespace point {

    class WdbLogHandler;
    class GribFile;
    class GribField;

    class GribLoader
    {
    public:
        GribLoader(WdbConnection& wdbConnection, const CmdLine& cmdLine); //, WdbLogHandler& logHandler);
        ~GribLoader( );

        void load(GribFile& gribFile);
//        void load(const GribField& field, int fieldNumber = 0);
//        int loadFromTemplate(GribFile& file, boost::shared_ptr<MetNoFimex::CDMReader> cdmreader, const std::string& tmplFileName);

    private:

        bool openTemplateCDM(const std::string& fileName);
        bool openDataCDM(const std::string& fileName, const std::string& fimexCfgFileName);
        bool interpolate(const std::string& templateFileName);
        bool extractBounds();
        bool extractPointIds();
        bool extractData();

        void loadInterpolated(GribFile& gribFile);

        std::string dataProviderName(const GribField& field) const;
        std::string valueParameterName( const GribField & field ) const;
        std::string valueParameterUnit( const GribField & field ) const;
        void levelValues( std::vector<wdb::load::Level> & levels, const GribField & field ) const;
        int dataVersion(const GribField & field) const;
        int confidenceCode(const GribField & field) const;

        /// GRIB Edition Number
        int editionNumber_;
        /// The Database Connection
        WdbConnection& connection_;
        /// The GribLoad Configuration
        const CmdLine& options_;
        /// The GribLoad Logger
//        WdbLogHandler& logHandler_;

        /// Conversion Hash Map - Dataprovider Name
        CfgFileReader point2DataProviderName_;
        /// Conversion Hash Map - Value Parameter
        CfgFileReader point2ValueParameter1_;
        /// Conversion Hash Map - Value Parameter
        CfgFileReader point2ValueParameter2_;
        /// Conversion Hash Map - Level Parameter
        CfgFileReader point2LevelParameter1_;
        /// Conversion Hash Map - Level Additions
        CfgFileReader point2LevelAdditions1_;
        /// Conversion Hash Map - Level Parameter
        CfgFileReader point2LevelParameter2_;
        /// Conversion Hash Map - Level Additions
        CfgFileReader point2LevelAdditions2_;

        /** Retrieve the edition number of the GRIB field
          * @param	field	The GRIB field to be loaded
          * @returns			The version number
          */
        int editionNumber(const GribField & field) const;
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

} } } // end namespaces

#endif /*POINTGRIBLOADER_H_*/
