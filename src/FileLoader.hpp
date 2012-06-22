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

using namespace std;

namespace MetNoFimex {
    class CDMReader;
}

namespace wdb { namespace load { namespace point {

    class Loader;

    struct EntryToLoad {
        string name_;
        string unit_;
        string provider_;
        string levelname_;
        set<double> levels_;
        size_t fimexXDimLength;
        size_t fimexYDimLength;
        string fimexName;
        string fimexLevelName;
        vector<string> fimexShape;
        boost::shared_array<double> data_;
    };

    class FileLoader
    {
    public:
        FileLoader(Loader& controller);
        virtual ~FileLoader( );

        void load(const string& fileName);

    protected:
        virtual void loadInterpolated(const string& fileName) = 0;
        virtual void setup();
        virtual void loadEntries();
        virtual void loadWindEntries();
        virtual bool openCDM(const std::string& file);
        virtual bool timeFromCDM();
        virtual bool processCDM();
        virtual bool interpolateCDM();

        void readUnit(const string& unitname, float& coeff, float& term);
        const CmdLine& options() { return controller_.options(); }
        boost::shared_ptr<MetNoFimex::CDMReader> cdmTemplate() { return controller_.cdmTemplate(); }
        std::map<std::string, EntryToLoad>& entries2load()  {return entries2Load_; }

        vector<string>& uwinds() { return uWinds_; }
        vector<string>& vwinds() { return vWinds_; }

        Loader& controller_;

        // CDMReader for data that will be interpolated
        boost::shared_ptr<MetNoFimex::CDMReader> cdmData_;

        // format time to string
        vector<string> times_;
        const vector<string>& times() { return times_;}

        /// Conversion Hash Map - Dataprovider Name
        CfgFileReader point2DataProviderName_;
        /// Conversion Hash Map - Value Parameter
        CfgFileReader point2ValueParameter_;
        /// Conversion Hash Map - Level Parameter
        CfgFileReader point2LevelParameter_;
        /// Conversion Hash Map - Level Additions
        CfgFileReader point2LevelAdditions_;
        CfgFileReader point2Units_;

        vector<string> uWinds_;
        vector<string> vWinds_;

        map<string, EntryToLoad> entries2Load_;
    };

} } } // end namespaces

#endif // FILELOADER_HPP
