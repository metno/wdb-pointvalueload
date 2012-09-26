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
#include "FileLoader.hpp"

using namespace std;

namespace wdb { namespace load { namespace point {

    class Loader;
    class GribFile;
    class GribField;

    class GribLoader : public FileLoader
    {
    public:
        GribLoader(Loader& controller);
        ~GribLoader();

    private:

        // read config files with metadata about
        void setup();

        // create CDMreader for GRIB data file
        // fimex needs xml confiog file for this
        bool openCDM(const string& fileName);

        // iterate input file and gather metadata about entries to be loaded
        void loadInterpolated(const string& fileName);

        // read config files to see grib -> wdb mapping
        int editionNumber(const GribField & field) const;
        string dataProviderName(const GribField& field) const;
        string valueParameterName(const GribField & field) const;
        string valueParameterUnit(const GribField & field) const;
        void levelValues(vector<wdb::load::Level>& levels, const GribField& field);
        ///////////////////////////////////////////////////////////////////////////

        // GRIB Edition Number
        // used to decide which config files to read
        int editionNumber_;

        // these will hole metadata for GRIB 2 files
        /// Conversion Hash Map - Value Parameter GRIB2
        CfgFileReader point2ValueParameter2_;
        /// Conversion Hash Map - Level Parameter GRIB2
        CfgFileReader point2LevelParameter2_;
        /// Conversion Hash Map - Level Additions GRIB2
        CfgFileReader point2LevelAdditions2_;
};


} } } // end namespaces

#endif /*POINTGRIBLOADER_H_*/
