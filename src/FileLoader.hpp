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

#ifndef FILELOADER_HPP
#define FILELOADER_HPP

// project
#include "Loader.hpp"
#include "FileLoader.hpp"
#include "CmdLine.hpp"
#include "CfgFileReader.hpp"
#include "WdbConnection.hpp"

using namespace std;

namespace MetNoFimex {
    class CDMReader;
}

namespace wdb { namespace load { namespace point {

    class Loader;
    class WdbLogHandler;
    class GribFile;
    class GribField;

    class FileLoader
    {
    public:
        FileLoader(Loader& controller);
        virtual ~FileLoader( );

        void load(const string& fileName);

    protected:
        virtual void loadInterpolated(const string& fileName) = 0;
        virtual bool openDataCDM(const std::string& file);
        virtual bool interpolate();

        const CmdLine& options() { return controller_.options(); }
        WdbConnection& wdbConnection() {  return controller_.wdbConnection();  }
        boost::shared_ptr<MetNoFimex::CDMReader> cdmTemplate() { return controller_.cdmTemplate(); }
        const vector<string>& placenames() { return controller_.placenames(); }
        const set<string>& stations2load() { return controller_.stations2load(); }

        int editionNumber(const GribField & field) const;
        std::string dataProviderName(const GribField& field) const;
        std::string valueParameterName( const GribField & field ) const;
        std::string valueParameterUnit( const GribField & field ) const;
        void levelValues( std::vector<wdb::load::Level> & levels, const GribField & field );
        int dataVersion(const GribField & field) const;
        int confidenceCode(const GribField & field) const;

        // GRIB Edition Number
        int editionNumber_;

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
        CfgFileReader point2ValueParameter_;
        /// Conversion Hash Map - Level Parameter
        CfgFileReader point2LevelParameter_;
        /// Conversion Hash Map - Level Additions
        CfgFileReader point2LevelAdditions_;

    };

    struct EntryToLoad {
        std::string name_;
        std::string unit_;
        std::string provider_;
        std::string levelname_;
        std::set<double> levels_;
    };
} } } // end namespaces

#endif // FILELOADER_HPP
